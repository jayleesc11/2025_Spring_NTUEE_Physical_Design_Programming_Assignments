//////////////////  WARNING /////////////////////////
// DO NOT MODIFY THIS FILE, THIS IS THE HEADER FILE
// FOR THE PRECOMPILED LIBRARY.
// IF YOU WANT TO MODIFY THIS FILE, PLEASE ENSURE
// YOU UNDERSTAND WHAT YOU ARE DOING.
/////////////////////////////////////////////////////

#ifndef PLACEMENT_H
#define PLACEMENT_H

#include <string>
#include <vector>
using namespace std;

#include "Module.h"
#include "Net.h"
#include "Pin.h"
#include "Rectangle.h"
#include "Row.h"

class Placement {
  public:
    Placement();

    /////////////////////////////////////////////
    // input/output
    /////////////////////////////////////////////
    void readBookshelfFormat(string filePathName, string plFileName);
    void outputBookshelfFormat(string filePathName);  // output file function

    /////////////////////////////////////////////
    // get
    /////////////////////////////////////////////
    const string& name() const { return _name; }
    const string& plname() const { return _loadplname; }
    double computeHpwl();
    double computeTotalNetLength(int cellid);
    const Rectangle& rectangleChip() const { return _rectangleChip; }  // Chip rectangle
    double boundaryTop() const { return _boundaryTop; }
    double boundaryLeft() const { return _boundaryLeft; }
    double boundaryBottom() const { return _boundaryBottom; }
    double boundaryRight() const { return _boundaryRight; }
    double chipWidth() const { return _boundaryRight - _boundaryLeft; }
    double chipHeight() const { return _boundaryTop - _boundaryBottom; }
    double chipArea() const { return chipWidth() * chipHeight(); }

    /////////////////////////////////////////////
    // operation
    /////////////////////////////////////////////
    void moveDesignCenter(double xOffset, double yOffset);

    /////////////////////////////////////////////
    // get design objects/properties
    /////////////////////////////////////////////
    Module& module(unsigned moduleId) { return _modules[moduleId]; }
    Net& net(unsigned netId) { return _nets[netId]; }
    Pin& pin(unsigned pinId) { return _pins[pinId]; }
    Row& row(unsigned rowId) { return _rows[rowId]; }

    double getRowHeight() const { return _rowHeight; }

    unsigned numModules() const { return _modules.size(); }
    unsigned numNets() const { return _nets.size(); }
    unsigned numPins() const { return _pins.size(); }
    unsigned numRows() const { return _rows.size(); }

    /////////////////////////////////////////////
    // methods for design (hypergraph) construction
    /////////////////////////////////////////////
    void addModule(const Module& module) { _modules.push_back(module); }
    void addPin(const Pin& pin) { _pins.push_back(pin), _pins.back().setPinId(_pins.size()); }
    void addRow(const Row& row) { _rows.push_back(row); }

    void setNumModules(unsigned size) { _modules.resize(size); }
    void setNumNets(unsigned size) { _nets.resize(size); }
    void setNumPins(unsigned size) { _pins.resize(size); }
    void setNumRows(unsigned size) { _rows.resize(size); }

    void clearModules() { _modules.clear(); }
    void clearNets() { _nets.clear(); }
    void clearPins() { _pins.clear(); }
    void clearRows() { _rows.clear(); }

    // initialize pins for modules and nets (construct hypergraph)
    void connectPinsWithModulesAndNets();

    ////////////////////
    vector<Row> m_sites;         // for Legalization and Detailplace
    vector<Module> modules_bak;  // for Detailplace

  private:
    /////////////////////////////////////////////
    // properties
    /////////////////////////////////////////////
    string _name;
    string _loadplname;

    /////////////////////////////////////////////
    // design data
    /////////////////////////////////////////////
    vector<Module> _modules;
    vector<Net> _nets;
    vector<Pin> _pins;
    vector<Row> _rows;

    /////////////////////////////////////////////
    // design statistics
    /////////////////////////////////////////////
    void updateDesignStatistics();
    Rectangle _rectangleChip;
    double _rowHeight;
    double _boundaryTop;
    double _boundaryLeft;
    double _boundaryBottom;
    double _boundaryRight;
};

#endif  // PLACEMENT_H
