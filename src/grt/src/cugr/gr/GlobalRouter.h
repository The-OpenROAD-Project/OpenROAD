#pragma once
#include "global.h"
#include "obj/Design.h"
#include "GridGraph.h"
#include "GRNet.h"

class GlobalRouter {
public:
    GlobalRouter(const Design& design, const Parameters& params);
    void route();
    void write(std::string guide_file = "");
    
private:
    const Parameters& parameters;
    GridGraph gridGraph;
    vector<GRNet> nets;
    
    int areaOfPinPatches;
    int areaOfWirePatches;
    
    void sortNetIndices(vector<int>& netIndices) const;
    void getGuides(const GRNet& net, vector<std::pair<int, utils::BoxT<int>>>& guides) ;
    void printStatistics() const;
};