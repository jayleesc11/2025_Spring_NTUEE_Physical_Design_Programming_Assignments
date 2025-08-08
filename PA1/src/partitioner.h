#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <fstream>
#include <unordered_map>

#include "cell.h"
#include "net.h"
using namespace std;

class Partitioner {
  public:
    // constructor and destructor
    Partitioner(fstream& in_file) : cut_size_(0), net_num_(0), all_net_num_(0), cell_num_(0), b_factor_(0), part_size_{0, 0} {
        parseInput(in_file);
        initPartition();
    }
    ~Partitioner();

    // modify method
    void parseInput(fstream& in_file);
    void partition();

    // member functions about reporting
    void printSummary() const;
    void reportNet() const;
    void reportCell() const;
    void writeResult(fstream& out_file);

  private:
    // Input data
    int net_num_;                                // number of non-single-pin nets
    int all_net_num_;                            // number of all nets
    int cell_num_;                               // number of cells
    double b_factor_;                            // the balance factor to be met
    vector<Net*> net_array_;                     // net array of the circuit
    vector<Cell*> cell_array_;                   // cell array of the circuit
    unordered_map<string, int> cell_name_2_id_;  // mapping from cell name to id

    // Partition solution
    int cut_size_;      // cut size
    int part_size_[2];  // size (cell number) of partition A(0) and B(1)

    // Bucket list data structure
    int blist_offset_;             // offset of bucket list
    Node* max_clip_gain_cell_[2];  // pointer to max CLIP gain cell node
    vector<Node*> blist_[2];       // bucket list of partition A(0) and B(1)

    // Algorithm data
    int acc_gain_;            // accumulative gain
    int max_acc_gain_;        // maximum accumulative gain
    int move_num_;            // number of cell movements
    int best_move_num_;       // store move_num_ when max_acc_gain_ occurs
    vector<int> move_stack_;  // history of cell movement

    // Partitioner methods
    void initPartition();
    void initPass();
    void moveCell(int cell_id);
    void updateGain(int cell_id, bool from, bool to);
    void updateBucketList(Cell* cell, int clip_gain);
    void insertBucketList(Cell* cell, int clip_gain);
    void removeBucketList(Cell* cell);

    // Index conversion methods for bucket list
    int getBlistId(int clip_gain) { return clip_gain - blist_offset_; }
};

#endif  // PARTITIONER_H