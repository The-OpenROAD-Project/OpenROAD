#pragma once
#include "global.h"
#include "GeoTypes.h"
#include "Layers.h"
#include "Net.h"
#include "Instance.h"

using CostT = double;

class Design {
public:
    Design(const Parameters& params): parameters(params) {
        read(parameters.lef_file, parameters.def_file);
        setUnitCosts();
    }
    DBU getLibDBU() const { return libDBU; }

    CostT getUnitLengthWireCost() const { return unit_length_wire_cost; }
    CostT getUnitViaCost() const { return unit_via_cost; }
    CostT getUnitLengthShortCost(const int layerIndex) const { return unit_length_short_costs[layerIndex]; }
    
    int getNumLayers() const { return layers.size(); }
    const MetalLayer& getLayer(int layerIndex) const { return layers[layerIndex]; }
    void getPinShapes(const PinReference& pinRef, vector<BoxOnLayer>& pinShapes) const;
    
    // For global routing 
    const vector<vector<DBU>>& getGridlines() const { return gridlines; }
    const vector<Net>& getAllNets() const { return nets; }
    void getAllObstacles(vector<vector<utils::BoxT<DBU>>>& allObstacles, bool skipM1 = true) const;
    
private:
    const Parameters& parameters;
    
    DBU libDBU;
    utils::BoxT<DBU> dieRegion;
    vector<MetalLayer> layers;
    vector<Macro> macros; // macros with a SPECIFIC orientation
    vector<Instance> instances;
    vector<Net> nets;
    vector<BoxOnLayer> obstacles;
    
    // For detailed routing
    CostT unit_length_wire_cost;
    CostT unit_via_cost;
    vector<CostT> unit_length_short_costs;
    
    // For global routing
    const static int defaultGridlineSpacing = 3000;
    vector<vector<DBU>> gridlines;
    
    void read(std::string lef_file, std::string def_file);
    void setUnitCosts();
};