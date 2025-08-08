#ifndef MODULE_H
#define MODULE_H

#include <string>
#include <vector>
using namespace std;

enum Dir { kNone, kLeft, kRight };

class Terminal {
  public:
    // constructor and destructor
    Terminal(const string& name, double x, double y) : name_(name), xc_(x), yc_(y) {}
    ~Terminal() {}

    // basic access methods
    const string& getName() const { return name_; }
    double getXc() const { return xc_; }
    double getYc() const { return yc_; }

    // set functions
    void setPosC(double x, double y) {
        xc_ = x;
        yc_ = y;
    }

  protected:
    string name_;  // module name
    double xc_;    // x coordinate of the terminal
    double yc_;    // y coordinate of the terminal
};

class Block : public Terminal {
  public:
    // constructor and destructor
    Block(const string& name, int w, int h)
        : Terminal(name, double(w) / 2, double(h) / 2),
          w_(w),
          h_(h),
          xl_(0),
          yl_(0),
          rotated_(false),
          best_xl_(0),
          best_yl_(0),
          best_rotated_(false),
          left_(nullptr),
          right_(nullptr),
          parent_(nullptr),
          prev_(nullptr),
          next_(nullptr) {}
    ~Block() {}

    // basic access methods
    int getWidth(bool best = false) const { return best ? best_rotated_ ? h_ : w_ : rotated_ ? h_ : w_; }
    int getHeight(bool best = false) const { return best ? best_rotated_ ? w_ : h_ : rotated_ ? w_ : h_; }
    int getXl(bool best = false) const { return best ? best_xl_ : xl_; }
    int getYl(bool best = false) const { return best ? best_yl_ : yl_; }
    static int getMaxX() { return max_x_; }
    static int getMaxY() { return max_y_; }

    // set functions
    void setXl(int x) {
        xl_    = x;
        xc_    = x + double(getWidth()) / 2;
        max_x_ = max(max_x_, x + getWidth());
    }
    void setYl(int y) {
        yl_    = y;
        yc_    = y + double(getHeight()) / 2;
        max_y_ = max(max_y_, y + getHeight());
    }
    void setLast() {
        last_xl_ = xl_;
        last_yl_ = yl_;
    }
    void backToLast() {
        xl_ = last_xl_;
        yl_ = last_yl_;
        xc_ = xl_ + double(getWidth()) / 2;
        yc_ = yl_ + double(getHeight()) / 2;
    }
    void setBest() {
        best_xl_      = xl_;
        best_yl_      = yl_;
        best_rotated_ = rotated_;
    }
    static void setMaxX(int x) { max_x_ = x; }
    static void setMaxY(int y) { max_y_ = y; }

    // perturbation functions
    void rotate() {
        rotated_ = !rotated_;
        swap(xc_, yc_);
    }

    // doubly linked list functions
    Block* deleteNodeNForward() {
        Block* next  = next_;
        prev_->next_ = next_;
        next_->prev_ = prev_;
        prev_        = nullptr;
        next_        = nullptr;
        return next;
    }
    void insertNode(Block* node) {
        node->prev_  = prev_;
        node->next_  = this;
        prev_->next_ = node;
        prev_        = node;
    }

    // puclic data members
    Block* left_;    // left child block in B*-tree
    Block* right_;   // right child block in B*-tree
    Block* parent_;  // parent block in B*-tree
    Block* prev_;    // previous block in the contour doubly linked list
    Block* next_;    // next block in the contour doubly linked list

  private:
    int w_;              // width of the block
    int h_;              // height of the block
    int xl_;             // x coordinate of the left bottom corner
    int yl_;             // y coordinate of the left bottom corner
    int last_xl_;        // last x coordinate of the left bottom corner
    int last_yl_;        // last y coordinate of the left bottom corner
    bool rotated_;       // whether the block is rotated
    int best_xl_;        // best x coordinate of the left bottom corner
    int best_yl_;        // best y coordinate of the left bottom corner
    bool best_rotated_;  // whether the block is rotated in the best solution
    static int max_x_;   // maximum x coordinate for all blocks
    static int max_y_;   // maximum y coordinate for all blocks
};

class Net {
  public:
    // constructor and destructor
    Net() {}
    ~Net() {}

    // modify methods
    void addTerm(Terminal* term) { term_list_.push_back(term); }

    // other member functions
    double calcHPWL() {
        double minX = term_list_[0]->getXc();
        double maxX = term_list_[0]->getXc();
        double minY = term_list_[0]->getYc();
        double maxY = term_list_[0]->getYc();
        for (int i = 1; i < term_list_.size(); i++) {
            if (term_list_[i]->getXc() < minX) {
                minX = term_list_[i]->getXc();
            } else if (term_list_[i]->getXc() > maxX) {
                maxX = term_list_[i]->getXc();
            }
            if (term_list_[i]->getYc() < minY) {
                minY = term_list_[i]->getYc();
            } else if (term_list_[i]->getYc() > maxY) {
                maxY = term_list_[i]->getYc();
            }
        }
        return (maxX - minX) + (maxY - minY);
    }

  private:
    vector<Terminal*> term_list_;  // list of terminals the net is connected to
};

#endif  // MODULE_H
