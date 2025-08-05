#pragma once
#include "GRNet.h"
#include "geo.h"
#include "global.h"

namespace grt {

class SteinerTreeNode : public PointT<int>
{
 public:
  std::vector<std::shared_ptr<SteinerTreeNode>> children;
  IntervalT<int> fixedLayers;

  SteinerTreeNode(PointT<int> point) : PointT<int>(point) {}
  SteinerTreeNode(PointT<int> point, IntervalT<int> _fixedLayers)
      : PointT<int>(point), fixedLayers(_fixedLayers)
  {
  }

  static void preorder(
      std::shared_ptr<SteinerTreeNode> node,
      std::function<void(std::shared_ptr<SteinerTreeNode>)> visit);
  static std::string getPythonString(std::shared_ptr<SteinerTreeNode> node);
};

class PatternRoutingNode : public PointT<int>
{
 public:
  const int index;
  // int x
  // int y
  std::vector<std::shared_ptr<PatternRoutingNode>> children;
  std::vector<std::vector<std::shared_ptr<PatternRoutingNode>>> paths;
  // childIndex -> pathIndex -> path
  IntervalT<int> fixedLayers;
  // layers that must be visited in order to connect all the pins
  std::vector<CostT> costs;  // layerIndex -> cost
  std::vector<std::vector<std::pair<int, int>>> bestPaths;
  // best path for each child; layerIndex -> childIndex -> (pathIndex,
  // layerIndex)
  bool optional;

  PatternRoutingNode(PointT<int> point, int _index, bool _optional = false)
      : PointT<int>(point), index(_index), optional(_optional)
  {
  }
  PatternRoutingNode(PointT<int> point,
                     IntervalT<int> _fixedLayers,
                     int _index = 0)
      : PointT<int>(point),
        fixedLayers(_fixedLayers),
        index(_index),
        optional(false)
  {
  }
  static std::string getPythonString(
      std::shared_ptr<PatternRoutingNode> routingDag);
};

class PatternRoute
{
 public:
  PatternRoute(GRNet& _net, const GridGraph& graph, const Parameters& param)
      : net(_net), gridGraph(graph), parameters(param), numDagNodes(0)
  {
  }
  void constructSteinerTree();
  void constructRoutingDAG();
  void constructDetours(GridGraphView<bool>& congestionView);
  void run();
  void setSteinerTree(std::shared_ptr<SteinerTreeNode> tree)
  {
    steinerTree = tree;
  }

 private:
  const Parameters& parameters;
  const GridGraph& gridGraph;
  GRNet& net;
  int numDagNodes;
  std::shared_ptr<SteinerTreeNode> steinerTree;
  std::shared_ptr<PatternRoutingNode> routingDag;

  void constructPaths(std::shared_ptr<PatternRoutingNode>& start,
                      std::shared_ptr<PatternRoutingNode>& end,
                      int childIndex = -1);
  void calculateRoutingCosts(std::shared_ptr<PatternRoutingNode>& node);
  std::shared_ptr<GRTreeNode> getRoutingTree(
      std::shared_ptr<PatternRoutingNode>& node,
      int parentLayerIndex = -1);
};

}  // namespace grt
