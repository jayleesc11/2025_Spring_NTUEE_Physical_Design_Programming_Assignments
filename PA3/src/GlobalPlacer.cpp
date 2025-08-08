#include "GlobalPlacer.h"

#include "ObjectiveFunction.h"
#include "Optimizer.h"
#include "Point.h"
#include "config.h"

GlobalPlacer::GlobalPlacer(Placement &placement) : _placement(placement) {}

void GlobalPlacer::place() {
    const unsigned num_modules = _placement.numModules();
    vector<Point2<double>> module_positions(num_modules, Point2<double>(0, 0));  // Optimization variables

// Initialize module positions in parallel
#pragma omp parallel for schedule(static)
    for (unsigned i = 0; i < num_modules; ++i) {
        const Module &module = _placement.module(i);
        if (module.isFixed()) {
            module_positions[i].x = module.x();
            module_positions[i].y = module.y();
        } else {
            module_positions[i].x = (_placement.boundaryLeft() + _placement.boundaryRight()) * 0.5;
            module_positions[i].y = (_placement.boundaryBottom() + _placement.boundaryTop()) * 0.5;
        }
    }
    ObjectiveFunction obj(_placement, module_positions);                   // Objective function
    SimpleConjugateGradient optimizer(obj, module_positions, _placement);  // Optimizer

    ////////////////////////////////////////////////////////////////////
    // Global placement algorithm
    ////////////////////////////////////////////////////////////////////

    // Initialize the optimizer
    optimizer.Initialize();

    // Global placement algorithm
    // step 0
    optimizer.Step();
    double best_overflow       = obj.overflow_ratio();
    double last_cost           = obj.value();
    unsigned steps             = 1;
    unsigned halt_spread_steps = 0;
    bool adjust_gamma          = false;

    // Following steps
    while (true) {
        optimizer.Step();
        double overflow = obj.overflow_ratio();

        // Dynamic lambda_ control
        if ((last_cost - obj.value()) / last_cost > config.kCostImprovementRatio) {
            last_cost = obj.value();
        } else {
            obj.mutiply_lambda(config.kMulLambda);
            last_cost = numeric_limits<double>::max();
        }

        // Check overflow
        if (overflow < best_overflow) {
            // Spread enough
            if (overflow < config.kOverflowAcceptRatio) break;

            // Update best overflow
            best_overflow     = overflow;
            halt_spread_steps = 0;

            // Dynamic gamma_ control
            if (!adjust_gamma && overflow < config.kAdjustGammaOverflow) {
                obj.multiply_gamma(config.kMulGamma);
                adjust_gamma = true;
            }
        } else {
            halt_spread_steps++;
            if (overflow < config.kOverflowAcceptRatio && halt_spread_steps > config.kEarlyStopSteps) break;
        }
        steps++;
        if (steps > config.kMaxSteps) break;
    }

////////////////////////////////////////////////////////////////////
// Write the placement result into the database.
////////////////////////////////////////////////////////////////////
#pragma omp parallel for schedule(static)
    for (unsigned i = 0; i < num_modules; ++i) {
        if (_placement.module(i).isFixed()) { continue; }
        _placement.module(i).setPosition(module_positions[i].x, module_positions[i].y);
    }

    // Report how many modules were fixed
    unsigned fixed_cnt = 0;
    for (unsigned i = 0; i < num_modules; ++i) {
        if (_placement.module(i).isFixed()) { ++fixed_cnt; }
    }
    printf("INFO: %u / %u modules are fixed.\n", fixed_cnt, num_modules);
}

void GlobalPlacer::plotPlacementResult(const string outfilename, bool isPrompt) {
    ofstream outfile(outfilename.c_str(), ios::out);
    outfile << " " << endl;
    outfile << "set title \"wirelength = " << _placement.computeHpwl() << "\"" << endl;
    outfile << "set size ratio 1" << endl;
    outfile << "set nokey" << endl << endl;
    outfile << "plot[:][:] '-' w l lt 3 lw 2, '-' w l lt 1" << endl << endl;
    outfile << "# bounding box" << endl;
    plotBoxPLT(outfile, _placement.boundaryLeft(), _placement.boundaryBottom(), _placement.boundaryRight(), _placement.boundaryTop());
    outfile << "EOF" << endl;
    outfile << "# modules" << endl << "0.00, 0.00" << endl << endl;
    for (unsigned i = 0; i < _placement.numModules(); ++i) {
        Module &module = _placement.module(i);
        plotBoxPLT(outfile, module.x(), module.y(), module.x() + module.width(), module.y() + module.height());
    }
    outfile << "EOF" << endl;
    outfile << "pause -1 'Press any key to close.'" << endl;
    outfile.close();

    if (isPrompt) {
        char cmd[200];
        sprintf(cmd, "gnuplot %s", outfilename.c_str());
        if (!system(cmd)) { cout << "Fail to execute: \"" << cmd << "\"." << endl; }
    }
}

void GlobalPlacer::plotBoxPLT(ofstream &stream, double x1, double y1, double x2, double y2) {
    stream << x1 << ", " << y1 << endl
           << x2 << ", " << y1 << endl
           << x2 << ", " << y2 << endl
           << x1 << ", " << y2 << endl
           << x1 << ", " << y1 << endl
           << endl;
}
