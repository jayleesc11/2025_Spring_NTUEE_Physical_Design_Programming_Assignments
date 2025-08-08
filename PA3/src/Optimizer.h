#define _GLIBCXX_USE_CXX11_ABI 0  // Align the ABI version to avoid compatibility issues with `Placement.h`
#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ObjectiveFunction.h"
#include "Point.h"
#include "config.h"

/**
 * @brief Base class for optimizers
 */
class BaseOptimizer {
  public:
    /////////////////////////////////
    // Constructors
    /////////////////////////////////

    BaseOptimizer(ObjectiveFunction &obj, vector<Point2<double>> &var) : obj_(obj), var_(var) {}

    /////////////////////////////////
    // Methods
    /////////////////////////////////

    // Initialize the optimizer, e.g., do some calculation for the first step.
    virtual void Initialize() = 0;

    // Perform one optimization step
    virtual void Step() = 0;

    const vector<Point2<double>> &getVar() const { return var_; }

  protected:
    /////////////////////////////////
    // Data members
    /////////////////////////////////

    ObjectiveFunction &obj_;       // Objective function to optimize
    vector<Point2<double>> &var_;  // Variables to optimize
};

/**
 * @brief Simple conjugate gradient optimizer with constant step size
 */
class SimpleConjugateGradient : public BaseOptimizer {
  public:
    /////////////////////////////////
    // Constructors
    /////////////////////////////////

    SimpleConjugateGradient(ObjectiveFunction &obj, vector<Point2<double>> &var, Placement &placement);

    /////////////////////////////////
    // Methods
    /////////////////////////////////

    // Initialize the optimizer
    void Initialize() override;

    // Perform one optimization step
    void Step() override;

  private:
    /////////////////////////////////
    // Data members
    /////////////////////////////////
    Placement &placement_;
    vector<Point2<double>> grad_prev_;  // Gradient of the objective function at the previous
                                        // step, i.e., g_{k-1} in the NTUPlace3 paper
    vector<Point2<double>> dir_prev_;   // Direction of the previous step,
                                        // i.e., d_{k-1} in the NTUPlace3 paper
    double alpha_s = config.kStepSize;  // Step size
};

#endif  // OPTIMIZER_H
