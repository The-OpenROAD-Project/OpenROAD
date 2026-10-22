// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2026, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "sta/NetworkClass.hh"
#include "sta/SdcClass.hh"
#include "web/core.h"

namespace sta {
class dbNetwork;
class dbSta;
class PathExpanded;
class Scene;
class StaState;
class Vertex;
}  // namespace sta

namespace web {

// ClockWidget's title and object name, and ClockTreeRenderer's control.
inline constexpr const char* kClockTreeViewerTitle = "Clock Tree Viewer";
inline constexpr const char* kClockTreeViewerName = "clock_viewer";
inline constexpr const char* kClockTreesControl = "Clock trees";

class ClockTree
{
 public:
  using PinDelays = std::map<const sta::Pin*, sta::Delay>;

  ClockTree(ClockTree* parent, sta::Net* net);
  ClockTree(sta::Clock* clock, sta::dbNetwork* network);

  ClockTree* getParent() const { return parent_; }
  sta::dbNetwork* getNetwork() const { return network_; }

  sta::Net* getNet() const { return net_; }

  int getLevel() const { return level_; }
  bool isRoot() const { return level_ == 0; }

  std::set<const sta::Pin*> getDrivers(bool visibility) const;

  const std::vector<std::unique_ptr<ClockTree>>& getFanout() const
  {
    return fanout_;
  }

  sta::Clock* getClock() const { return clock_; }

  const PinDelays& getDriverDelays() const { return drivers_; }
  const PinDelays& getLeavesDelays() const { return leaves_; }

  std::pair<const sta::Pin*, sta::Delay> getPairedSink(
      const sta::Pin* paired_pin) const;

  int getTotalFanout(bool visibility) const;
  int getTotalLeaves(bool visibility) const;
  int getMaxLeaves(bool visibility) const;
  sta::Delay getMinimumArrival(bool visibility) const;
  sta::Delay getMaximumArrival(bool visibility) const;
  sta::Delay getMinimumDriverDelay(bool visibility) const;
  int getSinkCount() const;

  odb::PtrSet<odb::dbNet> getNets(bool visibility) const;

  void addPath(sta::PathExpanded& path, const sta::StaState* sta);

  std::vector<std::pair<const sta::Pin*, const sta::Pin*>> findPathTo(
      const sta::Pin* pin) const;
  ClockTree* findTree(odb::dbNet* net, bool include_children = true);
  ClockTree* findTree(sta::Net* net, bool include_children = true);

  void setSubtreeVisibility(bool visibility);
  bool getSubtreeVisibility() const { return subtree_visibility_; };
  bool isVisible() const
  {
    return (parent_) ? parent_->getSubtreeVisibility() : true;
  };

 private:
  ClockTree* parent_;

  sta::Clock* clock_;
  sta::dbNetwork* network_;
  sta::Net* net_;

  int level_;

  PinDelays drivers_;
  PinDelays child_sinks_;
  PinDelays leaves_;

  std::vector<std::unique_ptr<ClockTree>> fanout_;

  bool subtree_visibility_;

  void addPath(sta::PathExpanded& path, int idx, const sta::StaState* sta);
  ClockTree* getTree(sta::Net* net);
  bool isLeaf(const sta::Pin* pin) const;
  bool addVertex(sta::Vertex* vertex, sta::Delay delay);
  sta::Net* getNet(const sta::Pin* pin) const;

  std::map<const sta::Pin*, std::set<const sta::Pin*>> getPinMapping() const;
};

// The trees of `scene`'s clocks, or of `only`, from the arrivals at the
// register clock pins, as the Qt gui's clock tree viewer builds them.
std::vector<std::unique_ptr<ClockTree>> buildClockTrees(sta::dbSta* sta,
                                                        sta::Scene* scene,
                                                        const sta::Clock* only
                                                        = nullptr);

// For the nets above the tree's first split: white over a dark background,
// black over a light one.
Painter::Color clockTreeTrunkColor(const Painter::Color& background);

// Draws `tree`'s nets as the clock tree viewer's renderer does: the trunk in
// `trunk`, then a new color per branch, up to `max_depth` splits down.
void paintClockTree(const ClockTree& tree,
                    int max_depth,
                    const Painter::Color& trunk,
                    Painter& painter);

}  // namespace web
