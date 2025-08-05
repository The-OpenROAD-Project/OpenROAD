#pragma once
#include "global.h"
#include "GeoTypes.h"

class Macro {
    // Macros of a SPECIFIC orientation (NOT the original macro)
public:
    Macro(int idx, const Rsyn::Cell& cell, const Rsyn::PhysicalDesign& physicalDesign);
    int getPinIndex(std::string pinName);
    const vector<BoxOnLayer>& getObstacles() const { return obstacles; }
    const vector<BoxOnLayer>& getPinShapes(int pinIndex) const { return pins[pinIndex]; }
    
private:
    std::string name;
    int orientation;
    int index;
    robin_hood::unordered_map<std::string, int> pinIndices;
    vector<vector<BoxOnLayer>> pins;
    vector<BoxOnLayer> obstacles;
};

class Instance {
public:
    const static int CELL = 0;
    const static int PORT = 1;
    
    Instance(int idx, std::string _name, int macroIndex, utils::PointT<DBU> pos): 
        index(idx), name(_name), macroIndex(macroIndex), position(pos), type(CELL) {}
    Instance(int idx, std::string _name, const Rsyn::PhysicalPort& phyPort);
    bool isCell() const { return type == CELL; }
    int getMacroIndex() const { return macroIndex; }
    utils::PointT<DBU> getPosition() const { return position; }
    const BoxOnLayer& getPort() const { return port; }
    
private:
    std::string name;
    int index;
    int type;
    // For cells
    utils::PointT<DBU> position;
    int macroIndex;
    // for ports
    BoxOnLayer port; 
};