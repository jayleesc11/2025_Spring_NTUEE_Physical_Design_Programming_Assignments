#define _GLIBCXX_USE_CXX11_ABI 0  // Align the ABI version to avoid compatibility issues with `Placement.h`
#ifndef OBJECTIVEFUNCTION_H
#define OBJECTIVEFUNCTION_H

#include <array>
#include <vector>

#include "Placement.h"
#include "Point.h"

/**
 * @brief Base class for objective functions
 */
class BaseFunction {
  public:
    /////////////////////////////////
    // Constructors
    /////////////////////////////////

    BaseFunction(const vector<Point2<double>> &input)
        : input_(input), grad_(vector<Point2<double>>(input.size(), Point2<double>(0., 0.))), num_inputs_(input.size()), value_(0.) {}

    /////////////////////////////////
    // Accessors
    /////////////////////////////////

    const vector<Point2<double>> &grad() const { return grad_; }
    const double &value() const { return value_; }

    /////////////////////////////////
    // Methods
    /////////////////////////////////

    // Forward pass, compute the value of the function
    virtual const double &operator()() = 0;

    // Backward pass, compute the gradient of the function
    virtual const vector<Point2<double>> &Backward() = 0;

  protected:
    /////////////////////////////////
    // Data members
    /////////////////////////////////

    const vector<Point2<double>> &input_;  // Input of the function
    vector<Point2<double>> grad_;          // Gradient of the function
    const unsigned num_inputs_;            // Number of modules
    double value_;                         // Value of the function
};

/**
 * @brief Wirelength function
 */
class Wirelength : public BaseFunction {
    friend class ObjectiveFunction;

  public:
    /////////////////////////////////
    // Constructors
    /////////////////////////////////

    Wirelength(Placement &placement, const vector<Point2<double>> &input);

    /////////////////////////////////
    // Methods
    /////////////////////////////////

    const double &operator()() override;
    const vector<Point2<double>> &Backward() override;

  private:
    /////////////////////////////////
    // Data members
    /////////////////////////////////

    Placement &placement_;
    // Cache the input for backward pass
    Point2<double> maxCoord_, minCoord_;
    vector<array<Point2<double>, 4>> all_exp_term_;  // sum_xe_max, sum_weight_max, sum_xe_min, sum_weight_min of net i
    double gamma_;
};

/**
 * @brief Density function
 */
class Density : public BaseFunction {
    friend class ObjectiveFunction;

  public:
    /////////////////////////////////
    // Constructors
    /////////////////////////////////

    Density(Placement &placement, const vector<Point2<double>> &input);

    /////////////////////////////////
    // Methods
    /////////////////////////////////

    const double &operator()() override;
    const vector<Point2<double>> &Backward() override;

  private:
    /////////////////////////////////
    // Methods
    /////////////////////////////////

    double overLapping(double center_dist, double bin_size, double module_size);
    double diff_overLapping(double center_dist, double bin_size, double module_size);

    /////////////////////////////////
    // Data members
    /////////////////////////////////

    // pseudo const variables
    Placement &placement_;
    unsigned num_bins_side_;
    double bin_width_;
    double bin_height_;
    double bin_area_;
    double object_area_;

    // variables
    vector<vector<double>> density_map_;
    vector<double> density_coeff_;
    double overflow_ratio_;
};

/**
 * @brief Objective function for global placement
 */
class ObjectiveFunction : public BaseFunction {
  public:
    /////////////////////////////////
    // Constructors
    /////////////////////////////////

    ObjectiveFunction(Placement &placement, const vector<Point2<double>> &input);

    /////////////////////////////////////////////
    // get
    /////////////////////////////////////////////

    double lambda() const { return lambda_; }
    double bin_width() const { return density_.bin_width_; }
    double bin_height() const { return density_.bin_height_; }
    double overflow_ratio() const { return density_.overflow_ratio_; }
    double wirelength_cost() const { return wirelength_.value_; }
    double density_cost() const { return density_.value_; }

    /////////////////////////////////
    // Methods
    /////////////////////////////////

    void multiply_gamma(const double times) { wirelength_.gamma_ *= times; }
    void mutiply_lambda(const double times) { lambda_ *= times; }
    void initialize_lambda();
    const double &operator()() override;
    const vector<Point2<double>> &Backward() override;

  private:
    /////////////////////////////////
    // Data members
    /////////////////////////////////

    Wirelength wirelength_;
    Density density_;
    double lambda_;
};

#endif  // OBJECTIVEFUNCTION_H
