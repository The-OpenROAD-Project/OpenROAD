#pragma once

#include <map>
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

  const BoxT& getBoundingBox() const { return bounding_box_; }
  const std::shared_ptr<GRTreeNode>& getRoutingTree() const
  {
    return routing_tree_;
  }

  void setRoutingTree(std::shared_ptr<GRTreeNode> tree)
  {
    routing_tree_ = std::move(tree);
  }
  void setSlack(float slack) { slack_ = slack; }
  float getSlack() const { return slack_; }
  void setCritical(bool is_critical) { is_critical_ = is_critical; }
  bool isCritical() const { return is_critical_; }
  void clearRoutingTree() { routing_tree_ = nullptr; }
  bool isInsideLayerRange(int layer_index) const;
  void addPreferredAccessPoint(int pin_index, const AccessPoint& ap);
  void addBTermAccessPoint(odb::dbBTerm* bterm, const AccessPoint& ap);
  void addITermAccessPoint(odb::dbITerm* iterm, const AccessPoint& ap);
  const std::map<odb::dbBTerm*, AccessPoint>& getBTermAccessPoints() const
  {
    return bterm_to_ap_;
  }
  const std::map<odb::dbITerm*, AccessPoint>& getITermAccessPoints() const
  {
    return iterm_to_ap_;
  }
  bool isLocal() const;

 private:
  int index_;
  odb::dbNet* db_net_;
  std::vector<std::vector<GRPoint>> pin_access_points_;
  std::map<int, odb::dbITerm*> pin_index_to_iterm_;
  std::map<int, odb::dbBTerm*> pin_index_to_bterm_;
  std::map<odb::dbBTerm*, AccessPoint> bterm_to_ap_;
  std::map<odb::dbITerm*, AccessPoint> iterm_to_ap_;
  BoxT bounding_box_;
  std::shared_ptr<GRTreeNode> routing_tree_;
  LayerRange layer_range_;
  float slack_;
  bool is_critical_;
};

}  // namespace grt
