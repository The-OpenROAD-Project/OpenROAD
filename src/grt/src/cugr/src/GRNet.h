#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "CUGR.h"
#include "GRTree.h"
#include "GridGraph.h"
#include "Netlist.h"
#include "geo.h"
#include "odb/db.h"

namespace grt {

class GRNet
{
 public:
  GRNet(const CUGRNet& baseNet, const GridGraph* gridGraph);

  int getIndex() const { return index_; }
  odb::dbNet* getDbNet() const { return db_net_; }
  std::string getName() const { return db_net_->getName(); }
  int getNumPins() const { return pin_access_points_.size(); }
  // vector foreach pin -> list of APs
  const std::vector<std::vector<GRPoint>>& getPinAccessPoints() const
  {
    return pin_access_points_;
  }

  int getITermIndex(odb::dbITerm* iterm) const
  {
    auto it = iterm_to_pin_index_.find(iterm);
    if (it != iterm_to_pin_index_.end()) {
      return it->second;
    }
    return -1;
  }

  int getBTermIndex(odb::dbBTerm* bterm) const
  {
    auto it = bterm_to_pin_index_.find(bterm);
    if (it != bterm_to_pin_index_.end()) {
      return it->second;
    }
    return -1;
  }

  const BoxT& getBoundingBox() const { return bounding_box_; }
  const std::shared_ptr<GRTreeNode>& getRoutingTree() const
  {
    return routing_tree_;
  }

  void setRoutingTree(std::shared_ptr<GRTreeNode> tree)
  {
    routing_tree_ = std::move(tree);
  }
  void clearRoutingTree() { routing_tree_ = nullptr; }
  bool isInsideLayerRange(int layer_index) const;

 private:
  int index_;
  odb::dbNet* db_net_;
  std::vector<std::vector<GRPoint>> pin_access_points_;
  std::map<odb::dbITerm*, int> iterm_to_pin_index_;
  std::map<odb::dbBTerm*, int> bterm_to_pin_index_;
  BoxT bounding_box_;
  std::shared_ptr<GRTreeNode> routing_tree_;
  LayerRange layer_range_;
};

}  // namespace grt
