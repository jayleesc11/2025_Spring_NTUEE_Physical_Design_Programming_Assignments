#ifndef CONFIG_H
#define CONFIG_H

struct Config {
    unsigned kThreads;
    // Global placement algorithm
    double kOverflowAcceptRatio;
    double kCostImprovementRatio;
    double kAdjustGammaOverflow;
    double kMulLambda;
    double kMulGamma;
    unsigned kEarlyStopSteps;
    unsigned kMaxSteps;
    // Optimizer
    double kStepSize;
    // Density function
    double kNumBinSideRatio;
    double kObjectDensity;
};

extern Config config;

void setConfig(int case_id);

#endif  // CONFIG_H