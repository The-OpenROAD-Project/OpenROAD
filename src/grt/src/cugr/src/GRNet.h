#pragma once
#include "CUGR.h"
#include "Design.h"
#include "GridGraph.h"

namespace grt {

class GRNet
{
 public:
  GRNet(const CUGRNet& baseNet, Design* design, GridGraph* gridGraph);

  int getIndex() const { return index_; }
  std::string getName() const { return name_; }
  int getNumPins() const { return pin_access_points_.size(); }
  const std::vector<std::vector<GRPoint>>& getPinAccessPoints() const
  {
    return pin_access_points_;
  }
  const BoxT<int>& getBoundingBox() const { return bounding_box_; }
  const std::shared_ptr<GRTreeNode>& getRoutingTree() const
  {
    return routing_tree_;
  }
  // void getGuides(std::vector<std::pair<int, BoxT<int>>>& guides)
  // const;

  void setRoutingTree(std::shared_ptr<GRTreeNode> tree) { routing_tree_ = tree; }
  void clearRoutingTree() { routing_tree_ = nullptr; }

 private:
  int index_;
  std::string name_;
  std::vector<std::vector<GRPoint>> pin_access_points_;
  BoxT<int> bounding_box_;
  std::shared_ptr<GRTreeNode> routing_tree_;
};

}  // name_space grt
