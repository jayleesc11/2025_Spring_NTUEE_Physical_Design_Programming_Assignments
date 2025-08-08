#include "partitioner.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>

#include "cell.h"
#include "net.h"
using namespace std;

constexpr double init_factor  = 0.9;
constexpr double early_factor = 1.5;

void Partitioner::parseInput(fstream& in_file) {
    string str;
    // Set balance factor
    in_file >> str;
    b_factor_ = stod(str);

    // Set up whole circuit
    while (in_file >> str) {
        if (str == "NET") {
            string net_name, cell_name, tmp_cell_name = "";
            in_file >> net_name;
            Net* net = new Net(net_name);
            while (in_file >> cell_name) {
                if (cell_name == ";") {
                    tmp_cell_name = "";
                    break;
                } else {
                    int cell_id;
                    // a newly seen cell not in cell_array_
                    if (!cell_name_2_id_.count(cell_name)) {
                        cell_id = cell_num_;
                        cell_array_.push_back(new Cell(cell_name, 0, cell_id));
                        cell_name_2_id_[cell_name] = cell_id;
                        ++cell_num_;
                    }
                    // an seen cell in cell_array_, but already added to the net
                    else if (tmp_cell_name == cell_name) {
                        continue;
                        // an seen cell in cell_array_
                    } else {
                        cell_id = cell_name_2_id_[cell_name];
                    }
                    cell_array_[cell_id]->addNet(net_num_);
                    net->addCell(cell_id);
                    tmp_cell_name = cell_name;
                }
            }
            // Delete the net if it is a single-pin net
            if (net->getCellList().size() == 1) {
                cell_array_[net->getCellList()[0]]->cancelNet();
                delete net;
            } else {
                net_array_.push_back(net);
                ++net_num_;
            }
            ++all_net_num_;
        }
    }
}

void Partitioner::initPartition() {
    int max_pin_num = 0;
    int limit      = ceil((1 - init_factor * b_factor_) * cell_num_ / 2.0);
    for (int cell_id = 0; cell_id < cell_num_; ++cell_id) {
        Cell* cell  = cell_array_[cell_id];
        max_pin_num = max(max_pin_num, cell->getPinNum());
        // Set initial partition rule
        bool part = cell_id < limit;
        cell->setPart(part);
        ++part_size_[part];
        for (int net_id : cell->getNetList()) { net_array_[net_id]->incPartCount(part); }
    }
    // Calculate initial cutsize
    for (Net* net : net_array_) {
        if (net->getPartCount(0) != 0 && net->getPartCount(1) != 0) ++cut_size_;
    }

    // Initialize bucket list
    int bucket_size = 4 * max_pin_num + 1;
    blist_[0].assign(bucket_size, nullptr);
    blist_[1].assign(bucket_size, nullptr);
    blist_offset_ = -2 * max_pin_num;

    move_stack_.reserve(cell_num_);
}

void Partitioner::partition() {
    int lower_bound = ceil((1 - b_factor_) * cell_num_ / 2.0);
    while(1) {
        initPass();
        bool last_from = 0;
        while (1) {
            // Choose the cell to move
            int move_cell_id;
            bool can_move[2];
            can_move[0] = max_clip_gain_cell_[0] && part_size_[0] > lower_bound;
            can_move[1] = max_clip_gain_cell_[1] && part_size_[1] > lower_bound;
            if (!can_move[0] && !can_move[1])
                break;
            else if (!can_move[0] && can_move[1])
                move_cell_id = max_clip_gain_cell_[1]->getId();
            else if (can_move[0] && !can_move[1])
                move_cell_id = max_clip_gain_cell_[0]->getId();
            else {
                int max_clip_gain0 = cell_array_[max_clip_gain_cell_[0]->getId()]->getCLIPGain(),
                    max_clip_gain1 = cell_array_[max_clip_gain_cell_[1]->getId()]->getCLIPGain();
                if (max_clip_gain0 == max_clip_gain1)
                    move_cell_id = max_clip_gain_cell_[last_from]->getId();
                else
                    move_cell_id = max_clip_gain0 > max_clip_gain1 ? max_clip_gain_cell_[0]->getId() : max_clip_gain_cell_[1]->getId();
            }

            // Move the cell
            bool from = cell_array_[move_cell_id]->getPart();
            moveCell(move_cell_id);
            updateGain(move_cell_id, from, !from);
            last_from = from;
        }
        if (max_acc_gain_ <= 0) {
            break;
        } else {
            // Back to the best solution
            cut_size_ -= max_acc_gain_;
            for (auto it = move_stack_.begin() + best_move_num_; it != move_stack_.end(); ++it) {
                Cell* cell = cell_array_[*it];
                cell->move();
                bool real_part = cell->getPart();
                ++part_size_[real_part];
                --part_size_[!real_part];
                for (int net_id : cell->getNetList()) { net_array_[net_id]->moveNetCell(real_part); }
            }
        }
    }
}

void Partitioner::initPass() {
    acc_gain_              = 0;
    max_acc_gain_          = INT32_MIN;
    move_num_              = 0;
    best_move_num_         = 0;
    max_clip_gain_cell_[0] = nullptr;
    max_clip_gain_cell_[1] = nullptr;
    move_stack_.clear();
    for (auto& blist_part : blist_) { fill(blist_part.begin(), blist_part.end(), nullptr); }

    auto comp_cell = [](Cell* a, Cell* b) { return a->getGain() < b->getGain(); };
    priority_queue<Cell*, vector<Cell*>, decltype(comp_cell)> min_heap(comp_cell);

    for (Cell* cell : cell_array_) {
        Node* cell_node = cell->getNode();
        cell->unlock();
        int gain  = 0;
        bool part = cell->getPart();
        // Calculate initial gain
        for (int net_id : cell->getNetList()) {
            if (net_array_[net_id]->getPartCount(part) == 1)
                ++gain;
            else if (net_array_[net_id]->getPartCount(!part) == 0)
                --gain;
        }
        min_heap.push(cell);
        cell->setGain(gain);
        cell->setInitGain();
    }
    // CLIP: clear the gains to 0 while maintaining the orderings
    // Start inserting to bucket[part][0] from the min gain cell
    while (!min_heap.empty()) {
        insertBucketList(min_heap.top(), 0);
        min_heap.pop();
    }
}

void Partitioner::moveCell(int move_cell_id) {
    Cell* cell = cell_array_[move_cell_id];
    int part   = cell->getPart();
    --part_size_[part];
    ++part_size_[!part];
    removeBucketList(cell);
    cell->move();
    cell->lock();
    acc_gain_ += cell->getGain();
    ++move_num_;
    move_stack_.push_back(move_cell_id);
    if (acc_gain_ > max_acc_gain_) {
        max_acc_gain_  = acc_gain_;
        best_move_num_ = move_num_;
    }
}

void Partitioner::updateGain(int move_cell_id, bool from, bool to) {
    for (int net_id : cell_array_[move_cell_id]->getNetList()) {
        Net* net           = net_array_[net_id];
        auto& net_cell_list = net->getCellList();
        // Before move
        int to_part_cnt = net->getPartCount(to);
        if (to_part_cnt == 0) {
            for (int cell_id : net_cell_list) {
                Cell* cell = cell_array_[cell_id];
                if (!cell->getLock()) {
                    updateBucketList(cell, cell->getCLIPGain() + 1);
                    cell->incGain();
                }
            }
        } else if (to_part_cnt == 1) {
            for (int cell_id : net_cell_list) {
                Cell* cell = cell_array_[cell_id];
                if (!cell->getLock() && cell->getPart() == to) {
                    updateBucketList(cell, cell->getCLIPGain() - 1);
                    cell->decGain();
                }
            }
        }

        // Move base cell
        net->moveNetCell(to);

        // After move
        int from_part_cnt = net->getPartCount(from);
        if (from_part_cnt == 0) {
            for (int cell_id : net_cell_list) {
                Cell* cell = cell_array_[cell_id];
                if (!cell->getLock()) {
                    updateBucketList(cell, cell->getCLIPGain() - 1);
                    cell->decGain();
                }
            }
        } else if (from_part_cnt == 1) {
            for (int cell_id : net_cell_list) {
                Cell* cell = cell_array_[cell_id];
                if (!cell->getLock() && cell->getPart() == from) {
                    updateBucketList(cell, cell->getCLIPGain() + 1);
                    cell->incGain();
                }
            }
        }
    }
}

void Partitioner::updateBucketList(Cell* cell, int clip_gain) {
    removeBucketList(cell);
    insertBucketList(cell, clip_gain);
}

void Partitioner::insertBucketList(Cell* cell, int clip_gain) {
    Node* cell_node    = cell->getNode();
    bool part          = cell->getPart();
    Node*& bucket_node = blist_[part][getBlistId(clip_gain)];
    // Insert to the front of the bucket list
    // Check if the bucket is empty
    if (!bucket_node) {
        bucket_node = cell_node;
    } else {
        cell_node->setNext(bucket_node);
        bucket_node->setPrev(cell_node);
        bucket_node = cell_node;
    }

    // Update Max Gain pointer
    if (!max_clip_gain_cell_[part] || clip_gain >= cell_array_[max_clip_gain_cell_[part]->getId()]->getCLIPGain())
        max_clip_gain_cell_[part] = cell_node;
}

void Partitioner::removeBucketList(Cell* cell) {
    Node* cell_node = cell->getNode();
    // The cell to be removed is in the middle of the bucket list
    if (cell_node->getPrev()) {
        cell_node->getPrev()->setNext(cell_node->getNext());
        // The cell to be removed is at the front of the bucket list
    } else {
        bool part                         = cell->getPart();
        int clip_gain                     = cell->getCLIPGain();
        auto& blist_part                  = blist_[part];
        blist_part[getBlistId(clip_gain)] = cell_node->getNext();
        // Update Max Gain pointer
        if (max_clip_gain_cell_[part] == cell_node) {
            max_clip_gain_cell_[part] = nullptr;
            for (auto it = blist_part.rbegin() + (blist_part.size() - 1 - getBlistId(clip_gain)); it != blist_part.rend(); ++it) {
                if (*it) {
                    max_clip_gain_cell_[part] = *it;
                    break;
                }
            }
        }
    }
    if (cell_node->getNext()) cell_node->getNext()->setPrev(cell_node->getPrev());
    cell_node->setPrev(nullptr);
    cell_node->setNext(nullptr);
}

void Partitioner::printSummary() const {
    cout << "\n";
    cout << "==================== Summary ====================" << "\n";
    cout << " Cutsize: " << cut_size_ << "\n";
    cout << " Total cell number: " << cell_num_ << "\n";
    cout << " Total net number:  " << all_net_num_ << "\n";
    cout << " Cell Number of partition A: " << part_size_[0] << "\n";
    cout << " Cell Number of partition B: " << part_size_[1] << "\n";
    cout << "=================================================" << "\n";
    cout << "\n";
    return;
}

void Partitioner::reportNet() const {
    // This function will not report single-pin nets
    cout << "Number of nets: " << net_num_ << "\n";
    for (Net* net : net_array_) {
        cout << setw(8) << net->getName() << ": ";
        for (int cell_id : net->getCellList()) { cout << setw(8) << cell_array_[cell_id]->getName() << " "; }
        cout << "\n";
    }
    return;
}

void Partitioner::reportCell() const {
    cout << "Number of cells: " << cell_num_ << "\n";
    for (Cell* cell : cell_array_) {
        cout << setw(8) << cell->getName() << ": ";
        for (int net_id : cell->getNetList()) { cout << setw(8) << net_array_[net_id]->getName() << " "; }
        cout << "\n";
    }
    return;
}

void Partitioner::writeResult(fstream& outFile) {
    stringstream buff;
    buff << cut_size_;
    outFile << "Cutsize = " << buff.str() << '\n';
    buff.str("");
    buff << part_size_[0];
    outFile << "G1 " << buff.str() << '\n';
    for (Cell* cell : cell_array_) {
        if (cell->getPart() == 0) { outFile << cell->getName() << " "; }
    }
    outFile << ";\n";
    buff.str("");
    buff << part_size_[1];
    outFile << "G2 " << buff.str() << '\n';
    for (Cell* cell : cell_array_) {
        if (cell->getPart() == 1) { outFile << cell->getName() << " "; }
    }
    outFile << ";\n";
    return;
}

Partitioner::~Partitioner() {
    for (Cell* cell : cell_array_) { delete cell; }
    for (Net* net : net_array_) { delete net; }
    return;
}
