#include "floorplanner.h"
#include "config.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <queue>
#include <stack>
#include <string>

#include "module.h"
using namespace std;

int Block::max_x_ = 0;
int Block::max_y_ = 0;

Floorplanner::Floorplanner(fstream& blk_file, fstream& net_file, double alpha) : kAlpha(alpha) {
    string str;
    blk_file >> str >> outline_width_ >> outline_height_;
    outline_ratio_ = double(outline_height_) / outline_width_;
    blk_file >> str >> num_blks_;
    blk_file >> str >> num_terms_;

    srand(config.kSeed);
    perturb_num_ = config.kPerturbNum * num_blks_;
    temp_k_      = max(2, num_blks_ / config.kTempK);
    temp_c_      = max(config.kTempC - num_blks_, 10);

    string blk_name;
    for (int i = 0; i < num_blks_; i++) {
        int w, h;
        blk_file >> blk_name >> w >> h;
        Block* blk = new Block(blk_name, w, h);
        blk_array_.push_back(blk);
        terms_[blk_name] = blk;
    }

    string term_name;
    for (int i = 0; i < num_terms_; i++) {
        int x, y;
        blk_file >> term_name >> str >> x >> y;
        Terminal* term    = new Terminal(term_name, x, y);
        terms_[term_name] = term;
    }

    net_file >> str >> num_nets_;
    for (int i = 0; i < num_nets_; i++) {
        Net* net = new Net();
        int degree;
        net_file >> str >> degree;
        while (degree-- > 0) {
            net_file >> term_name;
            net->addTerm(terms_[term_name]);
        }
        net_array_.push_back(net);
    }
}

void Floorplanner::floorplan() {
    num_sa_iter_  = 0;
    num_recent_   = 0;
    num_feasible_ = 0;
    mod_blks_[0]  = nullptr;
    mod_blks_[1]  = nullptr;
    best_cost_    = numeric_limits<double>::max();
    best_box_x_   = 0;
    best_box_y    = 0;
    int uphill    = 0;
    int reject    = 0;
    bool found    = false;
    Cost cost     = beginningIter();
    double temp   = temperature();
    queue<bool> feas_queue;
    while (1) {
        int iter   = 0;
        delta_avg_ = 0;
        uphill     = 0;
        reject     = 0;
        Cost new_cost;
        while (iter < perturb_num_ && uphill < perturb_num_ / 2) {
            for (Block* blk : blk_array_) blk->setLast();
            last_box_x_ = getBoxX();
            last_box_y_ = getBoxY();
            // perturb the solution
            PerturbType type = perturb();
            calPosition();
            new_cost = calCost();

            // record num_feasible_ for adaptive adapt_alpha
            bool feas = getBoxX() <= outline_width_ && getBoxY() <= outline_height_;
            feas_queue.push(feas);
            if (feas) ++num_feasible_;
            if (num_recent_ == config.kAdaptiveNum) {
                if (feas_queue.front()) --num_feasible_;
                feas_queue.pop();
            } else {
                ++num_recent_;
            }

            // accept or reject the new solution
            double delta = new_cost.total - cost.total;
            if (delta <= 0 || rand() / double(RAND_MAX) <= exp(-delta / temp)) {
                if (delta > 0) ++uphill;
                cost = new_cost;
                // update best solution
                if (feas && cost.real < best_cost_) {
                    found      = true;
                    best_cost_ = cost.real;
                    for (Block* blk : blk_array_) { blk->setBest(); }
                    best_box_x_ = getBoxX();
                    best_box_y  = getBoxY();
                }
            } else {
                backToPrev(type);
                for (Block* blk : blk_array_) blk->backToLast();
                Block::setMaxX(last_box_x_);
                Block::setMaxY(last_box_y_);
                ++reject;
            }

            delta_avg_ = (delta_avg_ * iter + delta) / (iter + 1);
            ++iter;
        }

        // prepare for next iteration
        if (temp < 1e-10) {
            if (found) break;
            num_sa_iter_ = 0;
            temp         = temperature();
        } else if (reject >= perturb_num_) {
            if (found) break;
            num_sa_iter_ = 0;
            temp         = temperature();
        } else {
            ++num_sa_iter_;
            temp = temperature();
        }
    };
}

Cost Floorplanner::beginningIter() {
    // construct head and tail of the contour doubly linked list
    string dummy_root = "dummy_root", tail = "tail";
    dummy_root_ = new Block(dummy_root, 0, 0);
    dummy_root_->setXl(0);
    dummy_root_->setYl(0);
    tail_ = new Block(tail, 0, 0);
    tail_->setXl(numeric_limits<int>::max());
    tail_->setYl(0);

    // construct initial solution: a complete binary tree
    dummy_root_->left_     = blk_array_[0];
    blk_array_[0]->parent_ = dummy_root_;
    queue<int> blockQueue;
    blockQueue.push(0);
    for (int i = 1; i < num_blks_;) {
        int cur = blockQueue.front();
        blockQueue.pop();
        blockQueue.push(i);
        blk_array_[cur]->left_ = blk_array_[i];
        blk_array_[i]->parent_ = blk_array_[cur];
        ++i;
        if (i < num_blks_) {
            blockQueue.push(i);
            blk_array_[cur]->right_ = blk_array_[i];
            blk_array_[i]->parent_  = blk_array_[cur];
            ++i;
        }
    }

    // start beginning iterations
    delta_begin_avg_ = 0;
    area_norm_       = 0;
    wire_norm_       = 0;
    ratio_diff_norm_ = 0;
    int uphill_count = 0;
    beginning_iter_sol_.resize(num_blks_);
    for (int i = 0; i < num_blks_; i++) {
        perturb();
        calPosition();
        double wirelength = 0;
        for (Net* net : net_array_) { wirelength += net->calcHPWL(); }
        beginning_iter_sol_[i] = {getBoxX(), getBoxY(), wirelength};
        area_norm_             = (getBoxX() * getBoxY() + area_norm_ * i) / (i + 1);
        wire_norm_             = (wirelength + wire_norm_ * i) / (i + 1);
        ratio_diff_norm_       = (abs(double(getBoxY()) / getBoxX() - outline_ratio_) + ratio_diff_norm_ * i) / (i + 1);
    };

    // calculate delta_begin_avg_
    Cost cost = calCost(0), new_cost;
    for (int i = 1; i < num_blks_; i++) {
        new_cost     = calCost(i);
        double delta = new_cost.total - cost.total;
        if (delta > 0) {
            delta_begin_avg_ = (delta_begin_avg_ * uphill_count + delta) / (uphill_count + 1);
            ++uphill_count;
        }
        cost = new_cost;
    }
    return cost;
}

Cost Floorplanner::calCost(int iter) {
    int boxX, boxY;
    double wirelength = 0;
    if (iter == -1) {
        boxX = getBoxX();
        boxY = getBoxY();
        for (Net* net : net_array_) { wirelength += net->calcHPWL(); }
    } else {
        boxX       = get<0>(beginning_iter_sol_[iter]);
        boxY       = get<1>(beginning_iter_sol_[iter]);
        wirelength = get<2>(beginning_iter_sol_[iter]);
    }
    double real_cost     = kAlpha * boxX * boxY / area_norm_ + (1 - kAlpha) * wirelength / wire_norm_;
    double adapt_alpha   = num_recent_ ? config.kAlphaBase + (1 - config.kAlphaBase) * num_feasible_ / num_recent_ : config.kAlphaBase;
    double outline_cost_ = pow((double(boxY) / boxX - outline_ratio_) / ratio_diff_norm_, 2);
    double total_cost    = adapt_alpha * real_cost + (1 - adapt_alpha) * outline_cost_;
    return {real_cost, total_cost};
}

double Floorplanner::temperature() {
    if (num_sa_iter_ <= 0) {
        init_temp_ = -delta_begin_avg_ / log(config.kInitProb);
        return init_temp_;
    } else if (num_sa_iter_ <= temp_k_ - 1) {
        return init_temp_ * delta_avg_ / (temp_c_ * num_sa_iter_);
    } else {
        return init_temp_ * delta_avg_ / num_sa_iter_;
    }
}

void Floorplanner::calPosition() {
    Block::setMaxX(0);
    Block::setMaxY(0);
    Block* root = dummy_root_->left_;
    root->setXl(0);
    // doubly linked list with tail for y-coordinate contour
    dummy_root_->next_ = tail_;
    tail_->prev_       = dummy_root_;

    // traverse the tree in preorder
    stack<Block*> block_stack;
    block_stack.push(root);
    while (!block_stack.empty()) {
        Block* par = block_stack.top();
        block_stack.pop();
        // x-coordinate
        if (par->right_) {
            par->right_->setXl(par->getXl());
            block_stack.push(par->right_);
        }
        if (par->left_) {
            par->left_->setXl(par->getXl() + par->getWidth());
            block_stack.push(par->left_);
        }
        // y-coordinate (inorder part)
        Block* to_insert = par;  // only change name
        Block* cur       = to_insert->parent_;
        if (cur->left_ == to_insert) cur = cur->next_;
        int yl = 0;
        // contour update
        while (to_insert->getXl() + to_insert->getWidth() >= cur->getXl() + cur->getWidth()) {
            yl  = max(yl, cur->getYl() + cur->getHeight());
            cur = cur->deleteNodeNForward();
        }
        yl = max(yl, cur->getYl() + cur->getHeight());
        to_insert->setYl(yl);
        cur->insertNode(to_insert);
    }

    // clear the contour doubly linked list
    Block* cur = dummy_root_->next_;
    while (cur != tail_) cur = cur->deleteNodeNForward();
}

PerturbType Floorplanner::perturb() {
    PerturbType type = static_cast<PerturbType>(rand() % 3);
    switch (type) {
        int id1;
        int id2;
        case kRotate:
            id1 = rand() % num_blks_;
            blk_array_[id1]->rotate();
            mod_blks_[0] = blk_array_[id1];
            break;

        case kMove:
            swap_count_ = 0;
            id1         = rand() % num_blks_;
            do { id2 = rand() % num_blks_; } while (id1 == id2);
            mod_blks_[0] = blk_array_[id1];
            mod_blks_[1] = blk_array_[id2];
            moveBlock(blk_array_[id1], blk_array_[id2]);
            break;

        case kSwap:
            id1 = rand() % num_blks_;
            do { id2 = rand() % num_blks_; } while (id1 == id2);
            mod_blks_[0] = blk_array_[id1];
            mod_blks_[1] = blk_array_[id2];
            swapBlocks(blk_array_[id1], blk_array_[id2]);
            break;
    }
    return type;
}

void Floorplanner::moveBlock(Block* to_move, Block* place_parent) {
    // Deletion
    deleteBlock(to_move);

    // Insertion
    if (rand() & 1) {
        Block* ori_left     = place_parent->left_;
        place_parent->left_ = to_move;
        to_move->parent_    = place_parent;
        // Shift needed
        if (ori_left) {
            ori_left->parent_ = to_move;
            if (rand() & 1)
                to_move->left_ = ori_left;
            else
                to_move->right_ = ori_left;
        }
    } else {
        Block* ori_right     = place_parent->right_;
        place_parent->right_ = to_move;
        to_move->parent_     = place_parent;
        // Shift needed
        if (ori_right) {
            ori_right->parent_ = to_move;
            if (rand() & 1)
                to_move->left_ = ori_right;
            else
                to_move->right_ = ori_right;
        }
    }
}

void Floorplanner::deleteBlock(Block* to_delete) {
    // to_delete has 0 child: leaf node
    if (!to_delete->left_ && !to_delete->right_) {
        recordMoveBlocks();
        Block* parent = to_delete->parent_;
        if (parent) {
            if (parent->left_ == to_delete)
                parent->left_ = nullptr;
            else
                parent->right_ = nullptr;
        }
        to_delete->parent_ = nullptr;
    }
    // to_delete has 2 children
    else if (to_delete->left_ && to_delete->right_) {
        // swap downward
        do {
            swapNear(to_delete, to_delete->left_);
            ++swap_count_;
        } while (to_delete->left_ && to_delete->right_);
        // now to_delete has 0 or 1 child
        deleteBlock(to_delete);
    }
    // to_delete has 1 child
    else {
        recordMoveBlocks();
        Block* par   = to_delete->parent_;
        Block* child = to_delete->left_ ? to_delete->left_ : to_delete->right_;
        if (par->left_ == to_delete)
            par->left_ = child;
        else
            par->right_ = child;
        
        child->parent_     = par;
        to_delete->parent_ = nullptr;
        to_delete->left_   = nullptr;
        to_delete->right_  = nullptr;
    }
}

void Floorplanner::swapBlocks(Block* block1, Block* block2) {
    Block* parent1 = block1->parent_;
    Block* parent2 = block2->parent_;
    if (parent2 == block1) {
        swapNear(block1, block2);
    } else if (parent1 == block2) {
        swapNear(block2, block1);
    } else {
        // parents
        if (parent1 == parent2) {
            swap(parent1->left_, parent1->right_);
        } else {
            if (parent1->left_ == block1)
                parent1->left_ = block2;
            else
                parent1->right_ = block2;
            if (parent2->left_ == block2)
                parent2->left_ = block1;
            else
                parent2->right_ = block1;
            swap(block1->parent_, block2->parent_);
        }
        // children
        if (block1->left_) block1->left_->parent_ = block2;
        if (block1->right_) block1->right_->parent_ = block2;
        if (block2->left_) block2->left_->parent_ = block1;
        if (block2->right_) block2->right_->parent_ = block1;
        swap(block1->left_, block2->left_);
        swap(block1->right_, block2->right_);
    }
}

void Floorplanner::swapNear(Block* parent, Block* child) {
    // swap with left child
    if (parent->left_ == child) {
        // parent
        if (parent->parent_->left_ == parent)
            parent->parent_->left_ = child;
        else
            parent->parent_->right_ = child;
        child->parent_  = parent->parent_;
        parent->parent_ = child;
        // right child
        if (child->right_) child->right_->parent_ = parent;
        if (parent->right_) parent->right_->parent_ = child;
        swap(parent->right_, child->right_);
        // left child
        if (child->left_) child->left_->parent_ = parent;
        parent->left_ = child->left_;
        child->left_  = parent;
    }
    // swap with right child
    else {
        // parent
        if (parent->parent_->left_ == parent)
            parent->parent_->left_ = child;
        else
            parent->parent_->right_ = child;
        child->parent_  = parent->parent_;
        parent->parent_ = child;
        // left child
        if (child->left_) child->left_->parent_ = parent;
        if (parent->left_) parent->left_->parent_ = child;
        swap(parent->left_, child->left_);
        // right child
        if (child->right_) child->right_->parent_ = parent;
        parent->right_ = child->right_;
        child->right_  = parent;
    }
}

void Floorplanner::recordMoveBlocks() {
    for (int i = 0; i < 2; i++) {
        Block* block = mod_blks_[i];
        if (block->parent_->left_ == block)
            record_[i] = {block->left_, block->right_, block->parent_, kLeft};
        else
            record_[i] = {block->left_, block->right_, block->parent_, kRight};
    }
}

void Floorplanner::backToPrev(PerturbType type) {
    switch (type) {
        case kRotate:
            mod_blks_[0]->rotate();
            break;

        case kMove: {
            // back to deleted place
            for (int i = 0; i < 2; i++) {
                Block* block   = mod_blks_[i];
                block->left_   = record_[i].last_left;
                block->right_  = record_[i].last_right;
                block->parent_ = record_[i].last_parent;
                if (block->left_) block->left_->parent_ = block;
                if (block->right_) block->right_->parent_ = block;
                if (record_[i].last_par_dir == kLeft) {
                    block->parent_->left_ = block;
                } else if (record_[i].last_par_dir == kRight) {
                    block->parent_->right_ = block;
                }
                record_[i] = {nullptr, nullptr, nullptr, kNone};
            }
            // keep swapping upwards
            Block* to_move = mod_blks_[0];
            while (swap_count_ > 0) {
                swapNear(to_move->parent_, to_move);
                --swap_count_;
            }
            break;
        }

        case kSwap:
            swapBlocks(mod_blks_[0], mod_blks_[1]);
            break;
    }
    mod_blks_[0] = nullptr;
    mod_blks_[1] = nullptr;
}

void Floorplanner::writeOutput(fstream& out_file, double run_time) {
    for (Block* blk : blk_array_) {
        blk->setPosC(blk->getXl(true) + double(blk->getWidth(true)) / 2, blk->getYl(true) + double(blk->getHeight(true)) / 2);
    }
    int area          = best_box_x_ * best_box_y;
    double wirelength = 0;
    for (Net* net : net_array_) { wirelength += net->calcHPWL(); }
    double cost = kAlpha * area + (1 - kAlpha) * wirelength;
    out_file << fixed << setprecision(6) << cost << endl;
    out_file << fixed << setprecision(1) << wirelength << endl;
    out_file << area << endl;
    out_file << best_box_x_ << " " << best_box_y << endl;
    out_file << fixed << setprecision(6) << run_time << endl;
    for (Block* blk : blk_array_) {
        out_file << blk->getName() << " " << blk->getXl(true) << " " << blk->getYl(true) << " " << blk->getXl(true) + blk->getWidth(true) << " "
                 << blk->getYl(true) + blk->getHeight(true) << endl;
    }
}

Floorplanner::~Floorplanner() {
    for (auto& term : terms_) delete term.second;
    for (Net* net : net_array_) delete net;
    delete dummy_root_;
    delete tail_;
}