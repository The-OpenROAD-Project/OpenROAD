#pragma once
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "CUGR.h"
#include "Design.h"
#include "GRNet.h"
#include "GRTree.h"
#include "GridGraph.h"
#include "geo.h"

namespace grt {

class SteinerTreeNode : public PointT
{
 public:
  SteinerTreeNode(PointT point) : PointT(point) {}
  SteinerTreeNode(PointT point, IntervalT fixed_layers)
      : PointT(point), fixed_layers_(fixed_layers)
  {
  }

  static void preorder(
      const std::shared_ptr<SteinerTreeNode>& node,
      const std::function<void(std::shared_ptr<SteinerTreeNode>)>& visit);
  static std::string getPythonString(
      const std::shared_ptr<SteinerTreeNode>& node);

  IntervalT& getFixedLayers() { return fixed_layers_; }

  void setFixedLayers(const IntervalT& fixed_layers)
  {
    fixed_layers_ = fixed_layers;
  }

  void addChild(const std::shared_ptr<SteinerTreeNode>& child)
  {
    children_.emplace_back(child);
  }

  void removeChild(int index) { children_.erase(children_.begin() + index); }

  std::vector<std::shared_ptr<SteinerTreeNode>>& getChildren()
  {
    return children_;
  }

  int getNumChildren() const { return children_.size(); }

 private:
  IntervalT fixed_layers_;
  std::vector<std::shared_ptr<SteinerTreeNode>> children_;
};

class PatternRoutingNode : public PointT
{
 public:
  PatternRoutingNode(PointT point, int index, bool optional = false)
      : PointT(point), index_(index), optional_(optional)
  {
  }
  PatternRoutingNode(PointT point, IntervalT fixed_layers, int index = 0)
      : PointT(point),
        index_(index),
        fixed_layers_(fixed_layers),
        optional_(false)
  {
  }

  const int& getIndex() const { return index_; }

  const IntervalT& getFixedLayers() const { return fixed_layers_; }

  bool isOptional() const { return optional_; }

  std::vector<std::shared_ptr<PatternRoutingNode>>& getChildren()
  {
    return children_;
  }

  void addChild(const std::shared_ptr<PatternRoutingNode>& child)
  {
    children_.emplace_back(child);
  }

  void removeChild(int index) { children_.erase(children_.begin() + index); }

  int getNumChildren() const { return children_.size(); }

  std::vector<std::vector<std::shared_ptr<PatternRoutingNode>>>& getPaths()
  {
    return paths_;
  }

  std::vector<CostT>& getCosts() { return costs_; }

  std::vector<std::vector<std::pair<int, int>>>& getBestPaths()
  {
    return best_paths_;
  }

  static std::string getPythonString(
      std::shared_ptr<PatternRoutingNode> routing_dag);

 private:
  const int index_;
  // childIndex -> pathIndex -> path
  IntervalT fixed_layers_;
  bool optional_;

  std::vector<std::shared_ptr<PatternRoutingNode>> children_;
  std::vector<std::vector<std::shared_ptr<PatternRoutingNode>>> paths_;
  // layers that must be visited in order to connect all the pins
  std::vector<CostT> costs_;  // layerIndex -> cost
  std::vector<std::vector<std::pair<int, int>>> best_paths_;
  // best path for each child; layerIndex -> childIndex -> (pathIndex,
  // layerIndex)
};

class PatternRoute
{
 public:
  PatternRoute(GRNet* net,
               const GridGraph* graph,
               stt::SteinerTreeBuilder* stt_builder,
               const Constants& constants)
      : net_(net),
        grid_graph_(graph),
        stt_builder_(stt_builder),
        constants_(constants)
  {
  }
  void constructSteinerTree();
  void constructRoutingDAG();
  void constructDetours(GridGraphView<bool>& congestion_view);
  void run();
  void setSteinerTree(const std::shared_ptr<SteinerTreeNode>& tree)
  {
    steiner_tree_ = tree;
  }

  const GRNet* getNet() const { return net_; }

  const GridGraph* getGridGraph() const { return grid_graph_; }

  int getNumDAGNodes() const { return num_dag_nodes_; }

  std::shared_ptr<SteinerTreeNode> getSteinerTree() const
  {
    return steiner_tree_;
  }

  std::shared_ptr<PatternRoutingNode> getRoutingDAG() const
  {
    return routing_dag_;
  }

 private:
  void constructPaths(std::shared_ptr<PatternRoutingNode>& start,
                      std::shared_ptr<PatternRoutingNode>& end,
                      int child_index = -1);
  void calculateRoutingCosts(std::shared_ptr<PatternRoutingNode>& node);
  std::shared_ptr<GRTreeNode> getRoutingTree(
      std::shared_ptr<PatternRoutingNode>& node,
      int parentLayerIndex = -1);

  GRNet* net_;
  const GridGraph* grid_graph_;
  stt::SteinerTreeBuilder* stt_builder_;
  int num_dag_nodes_{0};
  std::shared_ptr<SteinerTreeNode> steiner_tree_;
  std::shared_ptr<PatternRoutingNode> routing_dag_;
  std::vector<std::vector<int>> gridlines_;

  Constants constants_;
  const int flute_accuracy_ = 3;
};

}  // namespace grt
