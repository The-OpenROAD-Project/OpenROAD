#pragma once

#include <algorithm>
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
  GRNet(const CUGRNet& base_net, const GridGraph* grid_graph);

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
    // A newly installed tree is native (direction-legal) unless the
    // caller marks it adopted afterwards.
    adopted_ = false;
  }
  void setSlack(float slack) { slack_ = slack; }
  float getSlack() const { return slack_; }
  void setCritical(bool is_critical) { is_critical_ = is_critical; }
  bool isCritical() const { return is_critical_; }

  // Resistance-aware state for the PatternRoute cost term and getResAwareScore.
  void setResAware(bool res_aware) { is_res_aware_ = res_aware; }
  bool isResAware() const { return is_res_aware_; }
  void setResistance(float resistance) { resistance_ = resistance; }
  float getResistance() const { return resistance_; }
  void setNetLength(int net_length) { net_length_ = net_length; }
  int getNetLength() const { return net_length_; }
  void clearRoutingTree() { routing_tree_ = nullptr; }
  bool isInsideLayerRange(int layer_index) const;

  /**
   * @brief Returns the per-net 0-based routing layer range.
   *
   * Signal nets get `[signal_min, signal_max]`; clock nets get
   * `[clk_min, clk_max]` when the user set them via
   * `set_routing_layers -clock`.
   *
   * @returns Reference to the net's layer range.
   */
  const LayerRange& getLayerRange() const { return layer_range_; }

  double getNdrCost(int layer_index) const
  {
    if (layer_index < 0
        || std::cmp_greater_equal(layer_index, ndr_costs_.size())) {
      return 1.0;
    }
    return ndr_costs_[layer_index];
  }

  void setNdrCosts(std::vector<double> costs) { ndr_costs_ = std::move(costs); }

  const std::vector<double>& getNdrCosts() const { return ndr_costs_; }

  void setNdrWidths(std::vector<int> widths)
  {
    ndr_widths_ = std::move(widths);
  }

  // Effective wire width (DBU) on a layer: the NDR width if set, else 0 (use
  // the layer default).
  int getNdrWidth(int layer_index) const
  {
    if (layer_index < 0
        || std::cmp_greater_equal(layer_index, ndr_widths_.size())) {
      return 0;
    }
    return ndr_widths_[layer_index];
  }

  const std::vector<int>& getNdrWidths() const { return ndr_widths_; }

  /**
   * @brief Checks whether the net has an active demand-scaling NDR.
   *
   * A null `dbTechNonDefaultRule` or a rule that only restricts
   * layers (no width/spacing change) yields an all-1.0 cost vector,
   * which this method reports as `false`. Used by the maze stage to
   * decide whether to rebuild a per-net wire-cost view. Returns
   * `false` once the net has been soft-demoted (see `setSoftNdr`).
   *
   * @returns `true` iff any per-layer factor is strictly > 1.
   */
  bool hasNdr() const
  {
    for (double c : ndr_costs_) {
      if (c > 1.0) {
        return true;
      }
    }
    return false;
  }

  void setSoftNdr()
  {
    soft_ndr_ = true;
    std::ranges::fill(ndr_costs_, 1.0);
    // Drop the NDR width too, so the res-aware cost reverts to the default
    // wire width after demotion (keeps getNdrWidth in sync with hasNdr).
    std::ranges::fill(ndr_widths_, 0);
  }

  bool isSoftNdr() const { return soft_ndr_; }

  void addPreferredAccessPoint(int pin_index, const AccessPoint& ap)
  {
    preferred_aps_[pin_index] = ap;
  }
  // Selected AP per pin, keyed like getPinAccessPoints(); pins that have no
  // AP yet are absent.
  const std::map<int, AccessPoint>& getPreferredAccessPoints() const
  {
    return preferred_aps_;
  }
  // Terminals keyed by the pin index that also keys getPinAccessPoints().
  const std::map<int, odb::dbBTerm*>& getBTermsByPinIndex() const
  {
    return pin_index_to_bterm_;
  }
  const std::map<int, odb::dbITerm*>& getITermsByPinIndex() const
  {
    return pin_index_to_iterm_;
  }
  bool isLocal() const;
  // Set when the routing tree was adopted from detailed routes, which may
  // carry wrong-way spans; add/removeTreeUsage must allow them symmetrically.
  bool isAdopted() const { return adopted_; }
  void setAdopted(bool adopted) { adopted_ = adopted; }

 private:
  int index_;
  odb::dbNet* db_net_;
  std::vector<std::vector<GRPoint>> pin_access_points_;
  std::map<int, odb::dbITerm*> pin_index_to_iterm_;
  std::map<int, odb::dbBTerm*> pin_index_to_bterm_;
  std::map<int, AccessPoint> preferred_aps_;
  BoxT bounding_box_;
  std::shared_ptr<GRTreeNode> routing_tree_;
  LayerRange layer_range_;
  float slack_;
  bool is_critical_;
  bool is_res_aware_ = false;
  bool adopted_ = false;
  float resistance_ = 0.0f;
  int net_length_ = 0;
  std::vector<double> ndr_costs_;
  std::vector<int> ndr_widths_;
  bool soft_ndr_ = false;
};

}  // namespace grt
