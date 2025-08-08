#include "config.h"

#include <thread>

Config config;

void setConfig(int case_id) {
    config.kThreads = std::thread::hardware_concurrency() / 2;

    switch (case_id) {
        case 1:
            config.kOverflowAcceptRatio  = 0.05;
            config.kCostImprovementRatio = 0.0012;
            config.kAdjustGammaOverflow  = 5 * config.kOverflowAcceptRatio;
            config.kMulLambda            = 1.3;
            config.kMulGamma             = 0.3;
            config.kEarlyStopSteps       = 10;
            config.kMaxSteps             = 600;
            config.kStepSize             = 0.15;
            config.kNumBinSideRatio      = 0.5;
            config.kObjectDensity        = 0.9;
            break;
        case 5:
            config.kOverflowAcceptRatio  = 0.05;
            config.kCostImprovementRatio = 0.0012;
            config.kAdjustGammaOverflow  = 5 * config.kOverflowAcceptRatio;
            config.kMulLambda            = 1.3;
            config.kMulGamma             = 0.3;
            config.kEarlyStopSteps       = 10;
            config.kMaxSteps             = 600;
            config.kStepSize             = 0.07;
            config.kNumBinSideRatio      = 0.21;
            config.kObjectDensity        = 0.8;
            break;
        default:
            config.kOverflowAcceptRatio  = 0.05;
            config.kCostImprovementRatio = 0.0012;
            config.kAdjustGammaOverflow  = 5 * config.kOverflowAcceptRatio;
            config.kMulLambda            = 1.3;
            config.kMulGamma             = 0.3;
            config.kEarlyStopSteps       = 10;
            config.kMaxSteps             = 600;
            config.kStepSize             = 0.1;
            config.kNumBinSideRatio      = 0.25;
            config.kObjectDensity        = 0.9;
    }
}