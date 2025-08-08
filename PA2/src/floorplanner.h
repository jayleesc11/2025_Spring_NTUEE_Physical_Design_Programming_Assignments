#ifndef FLOROPLANNER_H
#define FLOROPLANNER_H

#include <fstream>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "module.h"
using namespace std;

enum PerturbType { kRotate, kMove, kSwap };

struct Cost {
    double real;
    double total;
};

struct Record {
    Block* last_left;
    Block* last_right;
    Block* last_parent;
    Dir last_par_dir;
};

class Floorplanner {
  public:
    // constructor and destructor
    Floorplanner(fstream& blk_file, fstream& net_file, double alpha);
    ~Floorplanner();

    // floorplanning functions
    void floorplan();
    void writeOutput(fstream& out_file, double run_time);

  private:
    Cost beginningIter();
    Cost calCost(int iter = -1);
    double temperature();
    void calPosition();

    // perturbation functions
    PerturbType perturb();
    void moveBlock(Block* to_move, Block* place_parent);
    void deleteBlock(Block* to_delete);
    void recordMoveBlocks();
    void swapBlocks(Block* block1, Block* block2);
    void swapNear(Block* parent, Block* child);
    void backToPrev(PerturbType type);

    // basic access methods
    int getBoxX() const { return Block::getMaxX(); }
    int getBoxY() const { return Block::getMaxY(); }

    // private data members
    // constants
    int temp_k_;
    int temp_c_;
    int perturb_num_;

    // simulated annealing temperature
    int num_sa_iter_;         // number of iterations for simulated annealing
    double delta_begin_avg_;  // average cost difference during beginning iterations
    double delta_avg_;        // average cost difference during SA iterations
    double init_temp_;        // initial temperature

    // cost function
    const double kAlpha;                                  // cost function parameter
    int num_feasible_;                                    // number of feasible solutions in recent solutions
    int num_recent_;                                      // number of recent solutions
    int outline_width_;                                   // width of the outline
    int outline_height_;                                  // height of the outline
    double outline_ratio_;                                // aspect ratio of the outline
    double area_norm_;                                    // normalization factor for area
    double wire_norm_;                                    // normalization factor for wirelength
    double ratio_diff_norm_;                              // normalization factor for aspect ratio difference
    vector<tuple<int, int, double>> beginning_iter_sol_;  // beginning iteration solutions

    // B*-tree and contour doubly linked list
    Block* dummy_root_;  // dummy root block: the real root block is dummy_root_->left_
    Block* tail_;        // tail block in the contour doubly linked list

    // perturbation
    Block* mod_blks_[2];  // modified blocks in this iteration
    Record record_[2];    // record of moved block
    int swap_count_;      // number of swap with child in moveBlock
    int last_box_x_;      // last box x
    int last_box_y_;      // last box y
    double best_cost_;    // best cost
    int best_box_x_;      // best box x
    int best_box_y;       // best box y

    int num_blks_;                            // number of blocks
    int num_terms_;                           // number of terminals
    int num_nets_;                            // number of nets
    vector<Block*> blk_array_;                // block array
    vector<Net*> net_array_;                  // net array
    unordered_map<string, Terminal*> terms_;  // map of terminals
};

#endif  // FLOORPLANNER_H