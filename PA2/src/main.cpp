#include <fstream>
#include <iostream>
#include <vector>
#include "config.h"

#include "floorplanner.h"
#include "tm_usage.h"
using namespace std;

int main(int argc, char** argv) {
    CommonNs::TmUsage tmusg;
    CommonNs::TmStat stat;
    tmusg.totalStart();

    fstream input_blk, input_net, output;
    double alpha;

    if (argc == 5) {
        alpha = stod(argv[1]);
        input_blk.open(argv[2], ios::in);
        input_net.open(argv[3], ios::in);
        output.open(argv[4], ios::out);
        if (!input_blk) {
            cerr << "Cannot open the input file \"" << argv[2] << "\". The program will be terminated..." << endl;
            exit(1);
        }
        if (!input_net) {
            cerr << "Cannot open the input file \"" << argv[3] << "\". The program will be terminated..." << endl;
            exit(1);
        }
        if (!output) {
            cerr << "Cannot open the output file \"" << argv[4] << "\". The program will be terminated..." << endl;
            exit(1);
        }
    } else {
        cerr << "Usage: ./Floorplanner <alpha> <input block file> "
             << "<input net file> <output file>" << endl;
        exit(1);
    }

    setConfig(argv[2], argv[1]);
    Floorplanner* fp = new Floorplanner(input_blk, input_net, alpha);

    tmusg.periodStart();
    fp->floorplan();
    tmusg.getPeriodUsage(stat);
    double run_time = double((stat.u_time + stat.s_time) / 1000000.0);
    fp->writeOutput(output, run_time);

    delete fp;
    input_blk.close();
    input_net.close();
    output.close();
    
    tmusg.getTotalUsage(stat);

    return 0;
}
