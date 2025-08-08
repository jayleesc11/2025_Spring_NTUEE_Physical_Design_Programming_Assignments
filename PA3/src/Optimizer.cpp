#include "Optimizer.h"

#include <omp.h>

#include <cmath>

SimpleConjugateGradient::SimpleConjugateGradient(ObjectiveFunction &obj, vector<Point2<double>> &var, Placement &placement)
    : BaseOptimizer(obj, var), placement_(placement), grad_prev_(var.size()), dir_prev_(var.size()) {}

void SimpleConjugateGradient::Initialize() {
    // Initialize lambda_
    obj_.initialize_lambda();

    const int num_modules = var_.size();
    vector<Point2<double>> dir(num_modules, {0., 0.});

#pragma omp parallel for schedule(static)
    for (int i = 0; i < num_modules; ++i) {
        Module &module = placement_.module(i);
        if (!module.isFixed()) {
            dir[i]           = -obj_.grad()[i];
            double dir_norm  = Norm2(dir[i]);
            var_[i].x       += alpha_s * obj_.bin_width() * dir[i].x / dir_norm;
            var_[i].y       += alpha_s * obj_.bin_height() * dir[i].y / dir_norm;
        }

        // Move the module back to the boundary if it is out of the boundary
        double x      = var_[i].x;
        double y      = var_[i].y;
        double left   = placement_.boundaryLeft();
        double right  = placement_.boundaryRight() - module.width();
        double bottom = placement_.boundaryBottom();
        double top    = placement_.boundaryTop() - module.height();

        if (x < left)
            x = left;
        else if (x > right)
            x = right;
        if (y < bottom)
            y = bottom;
        else if (y > top)
            y = top;

        var_[i].x = x;
        var_[i].y = y;
    }
    grad_prev_ = obj_.grad();
    dir_prev_  = dir;
}

/**
 * @details Update the solution once using the conjugate gradient method.
 */
void SimpleConjugateGradient::Step() {
    const int num_modules = var_.size();
    obj_();
    obj_.Backward();

    // 1. Determine number of blocks (B) and block size
    const int B   = omp_get_max_threads();  // one block per thread
    int blockSize = (num_modules + B - 1) / B;

    // 2. Allocate per-block accumulators
    vector<double> t1_block(B, 0.0), t2_block(B, 0.0);

// 3. Parallel compute each block's partial sums
#pragma omp parallel
    {
        int tid       = omp_get_thread_num();
        int start     = tid * blockSize;
        int end       = std::min(num_modules, start + blockSize);
        double t1_loc = 0.0, t2_loc = 0.0;

        for (int i = start; i < end; ++i) {
            if (!placement_.module(i).isFixed()) {
                auto &g   = obj_.grad()[i];
                auto &gp  = grad_prev_[i];
                t1_loc   += g.x * (g.x - gp.x) + g.y * (g.y - gp.y);
                t2_loc   += std::abs(g.x) + std::abs(g.y);
            }
        }

        t1_block[tid] = t1_loc;
        t2_block[tid] = t2_loc;
    }

    // 4. Serially merge blocks in fixed order
    double t1 = 0.0, t2 = 0.0;
    for (int b = 0; b < B; ++b) {
        t1 += t1_block[b];
        t2 += t2_block[b];
    }
    double beta = (t2 != 0.0 ? t1 / (t2 * t2) : 0.0);

    vector<Point2<double>> dir(num_modules);

#pragma omp parallel for schedule(static)
    for (int i = 0; i < num_modules; ++i) {
        if (!placement_.module(i).isFixed()) dir[i] = -obj_.grad()[i] + beta * dir_prev_[i];
    }

    // Update the solution
    double bin_width  = obj_.bin_width();
    double bin_height = obj_.bin_height();

#pragma omp parallel for schedule(static)
    for (int i = 0; i < num_modules; ++i) {
        Module &module = placement_.module(i);
        if (!module.isFixed()) {
            // normalize direction
            double dir_norm  = Norm2(dir[i]);
            var_[i].x       += alpha_s * bin_width * dir[i].x / dir_norm;
            var_[i].y       += alpha_s * bin_height * dir[i].y / dir_norm;
        }
        // clamp x within [left, right–width]
        double left   = placement_.boundaryLeft();
        double right  = placement_.boundaryRight() - module.width();
        double bottom = placement_.boundaryBottom();
        double top    = placement_.boundaryTop() - module.height();

        if (var_[i].x < left)
            var_[i].x = left;
        else if (var_[i].x > right)
            var_[i].x = right;
        if (var_[i].y < bottom)
            var_[i].y = bottom;
        else if (var_[i].y > top)
            var_[i].y = top;
    }

    // Update the cache data members
    grad_prev_ = obj_.grad();
    dir_prev_  = dir;
}
