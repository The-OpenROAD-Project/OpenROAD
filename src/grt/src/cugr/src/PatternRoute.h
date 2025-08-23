#pragma once
#include "CUGR.h"
#include "GRNet.h"
#include "geo.h"

namespace grt {

class SteinerTreeNode : public PointT<int>
{
 public:
  SteinerTreeNode(PointT<int> point) : PointT<int>(point) {}
  SteinerTreeNode(PointT<int> point, IntervalT<int> fixed_layers)
      : PointT<int>(point), fixed_layers_(fixed_layers)
  {
  }

  static void preorder(
      std::shared_ptr<SteinerTreeNode> node,
      std::function<void(std::shared_ptr<SteinerTreeNode>)> visit);
  static std::string getPythonString(std::shared_ptr<SteinerTreeNode> node);

  IntervalT<int>& getFixedLayers() { return fixed_layers_; }

  void setFixedLayers(const IntervalT<int>& fixed_layers)
  {
    fixed_layers_ = fixed_layers;
  }

  void addChild(std::shared_ptr<SteinerTreeNode> child)
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
  IntervalT<int> fixed_layers_;
  std::vector<std::shared_ptr<SteinerTreeNode>> children_;
};

class PatternRoutingNode : public PointT<int>
{
 public:
  PatternRoutingNode(PointT<int> point, int index, bool optional = false)
      : PointT<int>(point), index_(index), optional_(optional)
  {
  }
  PatternRoutingNode(PointT<int> point,
                     IntervalT<int> fixed_layers,
                     int index = 0)
      : PointT<int>(point),
        index_(index),
        fixed_layers_(fixed_layers),
        optional_(false)
  {
  }

  const int& getIndex() const { return index_; }

  const IntervalT<int>& getFixedLayers() const { return fixed_layers_; }

  bool isOptional() const { return optional_; }

  std::vector<std::shared_ptr<PatternRoutingNode>>& getChildren()
  {
    return children_;
  }

  void addChild(std::shared_ptr<PatternRoutingNode> child)
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
  IntervalT<int> fixed_layers_;
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
               stt::SteinerTreeBuilder* stt_builder)
      : net_(net),
        grid_graph_(graph),
        stt_builder_(stt_builder),
        num_dag_nodes_(0)
  {
  }
  void constructSteinerTree();
  void constructRoutingDAG();
  void constructDetours(GridGraphView<bool>& congestion_view);
  void run();
  void setSteinerTree(std::shared_ptr<SteinerTreeNode> tree)
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
  int num_dag_nodes_;
  std::shared_ptr<SteinerTreeNode> steiner_tree_;
  std::shared_ptr<PatternRoutingNode> routing_dag_;

  int default_gridline_spacing_ = 3000;
  std::vector<std::vector<int>> gridlines_;

  const int flute_accuracy_ = 3;
  const double weight_wire_length_ = 0.5;
  const double weight_via_number_ = 4.0;
  const double weight_short_area_ = 500.0;
  //
  const int min_routing_layer_ = 1;
  const double cost_logistic_slope_ = 1.0;
  const double max_detour_ratio_
      = 0.25;  // allowed stem length increase to trunk length ratio
  const int target_detour_count_ = 20;
  const double via_multiplier_ = 2.0;
  //
  const double maze_logistic_slope_ = 0.5;
  //
  const double pin_patch_threshold_ = 20.0;
  const int pin_patch_padding_ = 1;
  const double wire_patch_threshold_ = 2.0;
  const double wire_patch_inflation_rate_ = 1.2;
  //
  const bool write_heatmap_ = false;
};

}  // namespace grt
