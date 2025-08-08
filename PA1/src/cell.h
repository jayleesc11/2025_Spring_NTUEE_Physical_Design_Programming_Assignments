#ifndef CELL_H
#define CELL_H

#include <vector>
using namespace std;

class Node {
    friend class Cell;

  public:
    // Constructor and destructor
    Node(const int id) : id_(id), prev_(nullptr), next_(nullptr) {}
    ~Node() {}

    // Basic access methods
    int getId() const { return id_; }
    Node* getPrev() const { return prev_; }
    Node* getNext() const { return next_; }

    // Set functions
    void setId(const int id) { id_ = id; }
    void setPrev(Node* prev) { prev_ = prev; }
    void setNext(Node* next) { next_ = next; }

  private:
    int id_;      // id of the node (indicating the cell)
    Node* prev_;  // pointer to the previous node
    Node* next_;  // pointer to the next node
};

class Cell {
  public:
    // Constructor and destructor
    Cell(const string& name, bool part, int id) : gain_(0), init_gain_(0), part_(part), lock_(false), name_(name), net_list_() {
        node_ = new Node(id);
    }
    ~Cell() {}

    // Basic access methods
    int getGain() const { return gain_; }
    int getCLIPGain() const { return gain_ - init_gain_; }
    int getPinNum() const { return net_list_.size(); }
    bool getPart() const { return part_; }
    bool getLock() const { return lock_; }
    Node* const getNode() const { return node_; }
    const string& getName() const { return name_; }
    const vector<int>& getNetList() const { return net_list_; }

    // Set functions
    void setGain(int gain) { gain_ = gain; }
    void setPart(bool part) { part_ = part; }

    // Modify methods
    void incGain() { ++gain_; }
    void decGain() { --gain_; }
    void setInitGain() { init_gain_ = gain_; }
    void move() { part_ = !part_; }
    void lock() { lock_ = true; }
    void unlock() { lock_ = false; }
    void addNet(int net_id) { net_list_.push_back(net_id); }
    void cancelNet() { net_list_.pop_back(); }

  private:
    int gain_;              // real gain of the cell
    int init_gain_;         // initial gain in a pass, for CLIP
    bool part_;             // partition the cell belongs to (A(0) or B(1))
    bool lock_;             // whether the cell is locked
    Node* node_;            // node used to link the cells together
    string name_;           // name of the cell
    vector<int> net_list_;  // list of nets the cell is connected to
};

#endif  // CELL_H
