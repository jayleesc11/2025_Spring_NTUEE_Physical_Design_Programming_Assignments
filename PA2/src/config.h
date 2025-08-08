#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    double kInitProb  = 0.98;
    double kAlphaBase = 0.78;
    int kAdaptiveNum  = 2736;
    int kSeed         = 933;
    int kPerturbNum   = 51;
    int kTempK        = 17;
    int kTempC        = 812;

    Config() = default;
    Config(double initProb, double alphaBase, int adaptiveNum, int seed, int perturbNum, int tempK, int tempC)
        : kInitProb(initProb), kAlphaBase(alphaBase), kAdaptiveNum(adaptiveNum), kSeed(seed), kPerturbNum(perturbNum), kTempK(tempK), kTempC(tempC) {}
};

extern Config config;

void setConfig(const std::string &case_name, const std::string &alpha);

#endif // CONFIG_H