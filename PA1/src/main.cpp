#include <fstream>
#include <iostream>

#include "partitioner.h"
using namespace std;
int main(int argc, char** argv) {
    cin.tie(0);
    ios::sync_with_stdio(false);
    
    fstream input, output;

    if (argc == 3) {
        input.open(argv[1], ios::in);
        output.open(argv[2], ios::out);
        if (!input) {
            cerr << "Cannot open the input file \"" << argv[1] << "\". The program will be terminated..." << endl;
            exit(1);
        }
        if (!output) {
            cerr << "Cannot open the output file \"" << argv[2] << "\". The program will be terminated..." << endl;
            exit(1);
        }
    } else {
        cerr << "Usage: ./fm <input file> <output file>" << endl;
        exit(1);
    }

    Partitioner* partitioner = new Partitioner(input);
    partitioner->partition();
    partitioner->printSummary();
    partitioner->writeResult(output);
    
    delete partitioner;
    input.close();
    output.close();

    return 0;
}
