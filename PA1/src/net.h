#ifndef NET_H
#define NET_H

#include <vector>
using namespace std;

class Net {
  public:
    // constructor and destructor
    Net(const string& name) : name_(name), part_count_{0, 0}, cell_list_() {}
    ~Net() {}

    // basic access methods
    int getPartCount(bool part) const { return part_count_[part]; }
    const string& getName() const { return name_; }
    const vector<int>& getCellList() const { return cell_list_; }

    // modify methods
    void incPartCount(bool part) { ++part_count_[part]; }
    void decPartCount(bool part) { --part_count_[part]; }
    void moveNetCell(bool to_part) {
        ++part_count_[to_part];
        --part_count_[!to_part];
    }
    void addCell(int cell_id) { cell_list_.push_back(cell_id); }

  private:
    int part_count_[2];      // cell number in partition A(0) and B(1)
    string name_;            // name of the net
    vector<int> cell_list_;  // list of cells the net is connected to
};

#endif  // NET_H
