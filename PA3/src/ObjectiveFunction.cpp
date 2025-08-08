#include "ObjectiveFunction.h"

#include <omp.h>

#include <algorithm>
#include <cmath>

#include "config.h"

Wirelength::Wirelength(Placement &placement, const vector<Point2<double>> &input)
    : BaseFunction(input),
      placement_(placement),
      all_exp_term_(placement.numNets(), {Point2<double>(0., 0.), Point2<double>(0., 0.), Point2<double>(0., 0.), Point2<double>(0., 0.)}) {
    // Initialize gamma
    gamma_ = 0.05 * min(placement_.chipWidth(), placement_.chipHeight());
}

const double &Wirelength::operator()() {
    // x_max, x_min, y_max, y_min
    maxCoord_ = input_[0];
    minCoord_ = input_[0];
    for (unsigned i = 1; i < num_inputs_; ++i) {
        maxCoord_ = Max(maxCoord_, input_[i]);
        minCoord_ = Min(minCoord_, input_[i]);
    }

    // Initialize wirelength value and zero out all_exp_term_
    value_ = 0.0;
#pragma omp parallel for schedule(static)
    for (auto &row : all_exp_term_) { fill(row.begin(), row.end(), Point2<double>(0.0, 0.0)); }

    const unsigned num_nets = placement_.numNets();

// Parallelize over nets with reduction on value_
#pragma omp parallel for schedule(dynamic) reduction(+ : value_)
    for (unsigned net_id = 0; net_id < num_nets; ++net_id) {
        Net &net                = placement_.net(net_id);
        const unsigned num_pins = net.numPins();

        // thread-local alias of the exp terms for this net
        auto &terms = all_exp_term_[net_id];

        // Pre-calc half modules sizes per module to avoid repeated .width()/2 calls
        // (you could also cache these in an array if modules are reused heavily)
        for (unsigned pin_id = 0; pin_id < num_pins; ++pin_id) {
            Pin &pin        = net.pin(pin_id);
            unsigned m      = pin.moduleId();
            const auto &mod = placement_.module(m);

            // Compute pin position
            double half_w = mod.width() * 0.5;
            double half_h = mod.height() * 0.5;
            Point2<double> pin_pos{input_[m].x + half_w + pin.xOffset(), input_[m].y + half_h + pin.yOffset()};

            // Compute exponentials
            Point2<double> posexp = Exp((pin_pos - maxCoord_) / gamma_);
            Point2<double> negexp = Exp((-pin_pos + minCoord_) / gamma_);

            // Accumulate sums
            terms[0] += pin_pos * posexp;  // sum_xe_max
            terms[1] += posexp;            // sum_weight_max
            terms[2] += pin_pos * negexp;  // sum_xe_min
            terms[3] += negexp;            // sum_weight_min
        }

        // Add this net's contribution to global wirelength
        // Summing X and Y
        for (unsigned coord = 0; coord < 2; ++coord) {
            double xe_max  = terms[0][coord];
            double w_max   = terms[1][coord];
            double xe_min  = terms[2][coord];
            double w_min   = terms[3][coord];
            value_        += (xe_max / w_max) - (xe_min / w_min);
        }
    }

    return value_;
}

const vector<Point2<double>> &Wirelength::Backward() {
    // Zero out the gradient
    fill(grad_.begin(), grad_.end(), Point2<double>(0.0, 0.0));

// Parallelize over modules
#pragma omp parallel for schedule(dynamic)
    for (unsigned i = 0; i < num_inputs_; ++i) {
        // If the module is fixed, skip
        if (placement_.module(i).isFixed()) continue;

        // Local accumulator for grad_[i]
        Point2<double> local_grad(0.0, 0.0);

        Module &module          = placement_.module(i);
        const unsigned num_pins = module.numPins();

        // Precompute half‐width/height
        double half_w = module.width() * 0.5;
        double half_h = module.height() * 0.5;

        for (unsigned mod_pin_id = 0; mod_pin_id < num_pins; ++mod_pin_id) {
            Pin &mod_pin    = module.pin(mod_pin_id);
            unsigned net_id = mod_pin.netId();

            // Compute pin position
            Point2<double> pin_pos{input_[i].x + half_w + mod_pin.xOffset(), input_[i].y + half_h + mod_pin.yOffset()};

            // Exponentials
            Point2<double> posexp = Exp((pin_pos - maxCoord_) / gamma_);
            Point2<double> negexp = Exp((-pin_pos + minCoord_) / gamma_);

            // Sum terms just for this one pin
            Point2<double> sum_xe_max = pin_pos * posexp;
            Point2<double> sum_w_max  = posexp;
            Point2<double> sum_xe_min = pin_pos * negexp;
            Point2<double> sum_w_min  = negexp;

            // All‐net precomputed totals for net_id:
            auto &terms = all_exp_term_[net_id];
            //   terms[0] = sum_xe_max,   terms[1] = sum_w_max
            //   terms[2] = sum_xe_min,   terms[3] = sum_w_min

            // dL/dx formula broken into two pieces
            Point2<double> max_term = (sum_xe_max / gamma_ + sum_w_max) / terms[1] - sum_w_max * terms[0] / (gamma_ * terms[1] * terms[1]);

            Point2<double> min_term = (sum_w_min - sum_xe_min / gamma_) / terms[3] + sum_w_min * terms[2] / (gamma_ * terms[3] * terms[3]);

            local_grad += (max_term - min_term);
        }

        // Store back into shared grad_
        grad_[i] = local_grad;
    }

    return grad_;
}

Density::Density(Placement &placement, const vector<Point2<double>> &input)
    : BaseFunction(input), placement_(placement), density_coeff_(num_inputs_, 0.), overflow_ratio_(1) {
    // Initialize density_map_
    num_bins_side_ = config.kNumBinSideRatio * sqrt(num_inputs_);
    bin_width_     = placement_.chipWidth() / num_bins_side_;
    bin_height_    = placement_.chipHeight() / num_bins_side_;
    bin_area_      = bin_width_ * bin_height_;
    density_map_   = vector<vector<double>>(num_bins_side_, vector<double>(num_bins_side_, 0.));

    double avail_area = 0.;
#pragma omp parallel for schedule(static) reduction(+ : avail_area)
    for (unsigned i = 0; i < num_inputs_; i++) {
        Module &module = placement_.module(i);
        if (!module.isFixed()) avail_area += module.area();
    }
    object_area_ = max(avail_area / placement_.chipArea(), config.kObjectDensity * bin_area_);
}

const double &Density::operator()() {
    // Zero out the global density map and coefficients
#pragma omp parallel for schedule(static)
    for (auto &row : density_map_) std::fill(row.begin(), row.end(), 0.0);
    fill(density_coeff_.begin(), density_coeff_.end(), 0.0);

    // Determine thread count and allocate per-thread maps
    int num_threads = 1;
#pragma omp parallel
    {
#pragma omp single
        num_threads = omp_get_num_threads();
    }
    // thread_maps[t][x][y] accumulates thread t’s contributions
    vector<vector<vector<double>>> thread_maps(num_threads, vector<vector<double>>(num_bins_side_, vector<double>(num_bins_side_, 0.0)));

// In parallel, compute each module’s temp_map and update its thread’s map
#pragma omp parallel
    {
        int tid = omp_get_thread_num();   // this thread’s ID
        vector<vector<double>> temp_map;  // reused by each thread for each module

#pragma omp for schedule(static)
        for (unsigned i = 0; i < num_inputs_; ++i) {
            Module &module = placement_.module(i);
            if (module.isFixed()) continue;  // skip fixed modules

            // Find bin‐range window around module center
            unsigned left_bin   = (input_[i].x - placement_.boundaryLeft()) / bin_width_;
            unsigned bottom_bin = (input_[i].y - placement_.boundaryBottom()) / bin_height_;
            unsigned right_bin  = (input_[i].x + module.width() - placement_.boundaryLeft()) / bin_width_ + 2;
            unsigned top_bin    = (input_[i].y + module.height() - placement_.boundaryBottom()) / bin_height_ + 2;

            if (left_bin >= 2) left_bin -= 2;
            if (bottom_bin >= 2) bottom_bin -= 2;
            right_bin = std::min(right_bin, num_bins_side_ - 1);
            top_bin   = std::min(top_bin, num_bins_side_ - 1);

            unsigned num_bins_x = right_bin - left_bin + 1;
            unsigned num_bins_y = top_bin - bottom_bin + 1;

            // Allocate and fill this module’s temporary map
            temp_map.assign(num_bins_x, vector<double>(num_bins_y, 0.0));
            double density_sum = 0.0;

            double center_x = input_[i].x + module.width() / 2.0 - placement_.boundaryLeft();
            double center_y = input_[i].y + module.height() / 2.0 - placement_.boundaryBottom();

            for (unsigned dx = 0; dx < num_bins_x; ++dx) {
                double bin_center_x = bin_width_ * (left_bin + dx + 0.5);
                double ov_x         = overLapping(center_x - bin_center_x, bin_width_, module.width());
                for (unsigned dy = 0; dy < num_bins_y; ++dy) {
                    double bin_center_y  = bin_height_ * (bottom_bin + dy + 0.5);
                    double ov_y          = overLapping(center_y - bin_center_y, bin_height_, module.height());
                    double val           = ov_x * ov_y;
                    temp_map[dx][dy]     = val;
                    density_sum         += val;
                }
            }

            // Compute and store this module’s coefficient
            density_coeff_[i] = module.area() / density_sum;

            // Scatter into this thread’s full density map
            double coeff = density_coeff_[i];
            for (unsigned dx = 0; dx < num_bins_x; ++dx) {
                unsigned x = left_bin + dx;
                for (unsigned dy = 0; dy < num_bins_y; ++dy) {
                    unsigned y              = bottom_bin + dy;
                    thread_maps[tid][x][y] += coeff * temp_map[dx][dy];
                }
            }
        }  // end omp for
    }  // end omp parallel

    // Serially merge thread_maps into density_map_ in tid order (deterministic!)
    for (int t = 0; t < num_threads; ++t) {
        for (unsigned x = 0; x < num_bins_side_; ++x) {
            for (unsigned y = 0; y < num_bins_side_; ++y) { density_map_[x][y] += thread_maps[t][x][y]; }
        }
    }

    // Compute the density cost and overflow ratio with parallel reduction
    value_               = 0.0;
    double overflow_area = 0.0;
#pragma omp parallel for reduction(+ : value_, overflow_area) collapse(2)
    for (unsigned x = 0; x < num_bins_side_; ++x) {
        for (unsigned y = 0; y < num_bins_side_; ++y) {
            double diff = density_map_[x][y] - object_area_;
            if (diff > 0.0) overflow_area += diff;
            value_ += diff * diff;
        }
    }
    overflow_ratio_ = overflow_area / placement_.chipArea();

    return value_;
}

const vector<Point2<double>> &Density::Backward() {
    // Initialize shared gradient vector (single-threaded)
    fill(grad_.begin(), grad_.end(), Point2<double>(0.0, 0.0));

// Parallel region over modules
#pragma omp parallel
    {
        // Thread-local temporary gradient for a single module
        double local_grad_x, local_grad_y;

#pragma omp for schedule(dynamic)
        for (unsigned i = 0; i < num_inputs_; ++i) {
            Module &module = placement_.module(i);
            // Skip fixed modules
            if (module.isFixed()) continue;

            // Find the bins that are within the radius (w_v/2 + 2w_b) from module center
            unsigned left_bin   = (input_[i].x - placement_.boundaryLeft()) / bin_width_;
            unsigned bottom_bin = (input_[i].y - placement_.boundaryBottom()) / bin_height_;
            unsigned right_bin  = (input_[i].x + module.width() - placement_.boundaryLeft()) / bin_width_ + 2;
            unsigned top_bin    = (input_[i].y + module.height() - placement_.boundaryBottom()) / bin_height_ + 2;
            if (left_bin >= 2) left_bin -= 2;
            if (bottom_bin >= 2) bottom_bin -= 2;
            if (right_bin >= num_bins_side_) right_bin = num_bins_side_ - 1;
            if (top_bin >= num_bins_side_) top_bin = num_bins_side_ - 1;

            // Precompute half-width/height and center offsets
            double center_x = input_[i].x + module.width() * 0.5 - placement_.boundaryLeft();
            double center_y = input_[i].y + module.height() * 0.5 - placement_.boundaryBottom();
            double coeff    = density_coeff_[i];

            // Zero local accumulation
            local_grad_x = 0.0;
            local_grad_y = 0.0;

            // Loop over affected bins and accumulate local gradient
            for (unsigned x = left_bin; x <= right_bin; ++x) {
                // Partial derivative of overlapping in x
                double dist_x = center_x - bin_width_ * (x + 0.5);
                double d_ov_x = diff_overLapping(dist_x, bin_width_, module.width());
                double ov_x   = overLapping(dist_x, bin_width_, module.width());

                for (unsigned y = bottom_bin; y <= top_bin; ++y) {
                    // Overlap and its derivative in y
                    double dist_y = center_y - bin_height_ * (y + 0.5);
                    double d_ov_y = diff_overLapping(dist_y, bin_height_, module.height());
                    double ov_y   = overLapping(dist_y, bin_height_, module.height());

                    // Compute density difference
                    double diff = density_map_[x][y] - object_area_;

                    // ∂Cost/∂x_i += 2 * diff * coeff * d_ov_x * ov_y
                    local_grad_x += 2.0 * diff * coeff * d_ov_x * ov_y;
                    // ∂Cost/∂y_i += 2 * diff * coeff * ov_x * d_ov_y
                    local_grad_y += 2.0 * diff * coeff * ov_x * d_ov_y;
                }
            }

// Atomically add this module's local gradient into the shared grad_
#pragma omp atomic
            grad_[i].x += local_grad_x;
#pragma omp atomic
            grad_[i].y += local_grad_y;
        }
    }  // end parallel region

    return grad_;
}

double Density::overLapping(double center_dist, double bin_size, double module_size) {
    double abs_center_dist = abs(center_dist);
    if (abs_center_dist >= 0.5 * module_size + 2 * bin_size) {
        return 0.;
    } else if (abs_center_dist <= 0.5 * module_size + bin_size) {
        double alpha = 4. / ((module_size + 2 * bin_size) * (module_size + 4 * bin_size));
        return 1 - alpha * abs_center_dist * abs_center_dist;
    } else {
        double beta = 2. / (bin_size * (module_size + 4 * bin_size));
        return beta * pow(abs_center_dist - 0.5 * module_size - 2 * bin_size, 2);
    }
}

double Density::diff_overLapping(double center_dist, double bin_size, double module_size) {
    double abs_center_dist = abs(center_dist);
    if (abs_center_dist >= 0.5 * module_size + 2 * bin_size) {
        return 0.;
    } else if (abs_center_dist <= 0.5 * module_size + bin_size) {
        double alpha = 4. / ((module_size + 2 * bin_size) * (module_size + 4 * bin_size));
        return -2 * alpha * center_dist;
    } else {
        double beta = 2. / (bin_size * (module_size + 4 * bin_size));
        if (center_dist > 0) {
            return 2 * beta * (center_dist - 0.5 * module_size - 2 * bin_size);
        } else {
            return 2 * beta * (center_dist + 0.5 * module_size + 2 * bin_size);
        }
    }
}

ObjectiveFunction::ObjectiveFunction(Placement &placement, const vector<Point2<double>> &input)
    : BaseFunction(input), wirelength_(placement, input), density_(placement, input) {}

void ObjectiveFunction::initialize_lambda() {
    double wirelength_cost                 = wirelength_();
    double density_cost                    = density_();
    vector<Point2<double>> wirelength_grad = wirelength_.Backward();
    vector<Point2<double>> density_grad    = density_.Backward();
    double wirelength_grad_sum             = 0;
    double density_grad_sum                = 0;
    for (unsigned i = 0; i < num_inputs_; ++i) {
        wirelength_grad_sum += Norm2(wirelength_grad[i]);
        density_grad_sum    += Norm2(density_grad[i]);
    }
    lambda_ = wirelength_grad_sum / density_grad_sum;
#pragma omp parallel for schedule(static)
    for (unsigned i = 0; i < num_inputs_; ++i) { grad_[i] = wirelength_grad[i] + lambda_ * density_grad[i]; }
    value_ = wirelength_cost + lambda_ * density_cost;
}

const double &ObjectiveFunction::operator()() {
    value_ = wirelength_() + lambda_ * density_();
    return value_;
}

const vector<Point2<double>> &ObjectiveFunction::Backward() {
    grad_                               = wirelength_.Backward();
    vector<Point2<double>> density_grad = density_.Backward();
#pragma omp parallel for schedule(static)
    for (unsigned i = 0; i < num_inputs_; ++i) { grad_[i] += lambda_ * density_grad[i]; }
    return grad_;
}