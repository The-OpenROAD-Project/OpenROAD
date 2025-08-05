#pragma once
#include "global.h"
#include "GridGraph.h"
#include "GRNet.h"
#include "PatternRoute.h"

struct SparseGrid {
    utils::PointT<int> interval;
    utils::PointT<int> offset;
    SparseGrid(int xInterval, int yInterval, int xOffset, int yOffset):
        interval(xInterval, yInterval), offset(xOffset, yOffset) {}
    void step() {
        offset.x = (offset.x + 1) % interval.x;
        offset.y = (offset.y + 1) % interval.y;
    }
    void reset(int xInterval, int yInterval) {
        interval.x = xInterval;
        interval.y = yInterval;
        step();
    }
};

class SparseGraph {
public:
    SparseGraph(GRNet& _net, const GridGraph& graph3d, const Parameters& param):
        net(_net), gridGraph(graph3d), parameters(param) {}
    void init(GridGraphView<CostT>& wireCostView, SparseGrid& grid);
    int getNumVertices() const { return vertices.size(); }
    int getNumPseudoPins() const { return pseudoPins.size(); }
    std::pair<utils::PointT<int>, utils::IntervalT<int>> getPseudoPin(int pinIndex) const { return pseudoPins[pinIndex]; }
    int getPinVertex(const int pinIndex) const { return pinVertex[pinIndex]; }
    int getVertexPin(const int vertex) const {
        auto it = vertexPin.find(vertex);
        return it == vertexPin.end() ? -1 : it->second;
    }
    int getNextVertex(const int vertex, const int edgeIndex) const { return edges[vertex][edgeIndex]; }
    CostT getEdgeCost(const int vertex, const int edgeIndex) const { return costs[vertex][edgeIndex]; }
    GRPoint getPoint(const int vertex) const { return vertices[vertex]; }
    
private:
    const Parameters& parameters;
    const GridGraph& gridGraph;   
    GRNet& net;
    
    vector<std::pair<utils::PointT<int>, utils::IntervalT<int>>> pseudoPins;
    
    vector<int> xs;
    vector<int> ys;
    
    vector<GRPoint> vertices;
    vector<std::array<int, 3>> edges;
    vector<std::array<CostT, 3>> costs;
    // robin_hood::unordered_map<int, vector<int>> vertexPins;
    // vector<vector<int>> pinVertices;
    robin_hood::unordered_map<int, int> vertexPin;
    vector<int> pinVertex;
    
    inline int getVertexIndex(int direction, int xi, int yi) const {
        return direction * xs.size() * ys.size() + yi * xs.size() + xi;
    }
    
};

struct Solution {
    CostT cost;
    int vertex;
    std::shared_ptr<Solution> prev;
    Solution(CostT c, int v, const std::shared_ptr<Solution> &p): cost(c), vertex(v), prev(p) {}
};

class MazeRoute {
public:
    MazeRoute(GRNet& _net, const GridGraph& graph3d, const Parameters& param):
        net(_net), gridGraph(graph3d), parameters(param), graph(_net, graph3d, param) {}
    
    void run();
    void constructSparsifiedGraph(GridGraphView<CostT>& wireCostView, SparseGrid& grid) {
        graph.init(wireCostView, grid);
    }
    std::shared_ptr<SteinerTreeNode> getSteinerTree() const;
    
private: 
    const Parameters& parameters;
    const GridGraph& gridGraph;   
    GRNet& net;
    SparseGraph graph;
    
    vector<std::shared_ptr<Solution>> solutions;
};