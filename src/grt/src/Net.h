// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Pin.h"
#include "grt/GRoute.h"
#include "grt/RoutePt.h"
#include "odb/db.h"

namespace grt {

class Net
{
 public:
  Net(odb::dbNet* net, bool has_wires);
  odb::dbNet* getDbNet() const { return net_; }
  const std::string getName() const;
  const char* getConstName() const;
  odb::dbSigType getSignalType() const;
  void addPin(Pin& pin);
  void deleteSegment(int seg_id, GRoute& routes);
  std::vector<Pin>& getPins() { return pins_; }
  int getNumPins() const { return pins_.size(); }
  float getSlack() const { return slack_; }
  void setSlack(float slack) { slack_ = slack; }
  void setHasWires(bool in) { has_wires_ = in; }
  void setSegmentParent(std::vector<SegmentIndex> segment_parent)
  {
    parent_segment_indices_ = std::move(segment_parent);
  }
  std::vector<SegmentIndex> getSegmentParent() const
  {
    return parent_segment_indices_;
  }
  std::vector<std::vector<SegmentIndex>> buildSegmentsGraph();
  bool isLocal();
  void destroyPins();
  void destroyITermPin(odb::dbITerm* iterm);
  void destroyBTermPin(odb::dbBTerm* bterm);
  bool hasWires() const { return has_wires_; }
  bool hasStackedVias(odb::dbTechLayer* max_routing_layer);
  void saveLastPinPositions();
  void clearLastPinPositions() { last_pin_positions_.clear(); }
  const std::multiset<RoutePt>& getLastPinPositions()
  {
    return last_pin_positions_;
  }
  void setMergedNet(bool merged_net) { merged_net_ = merged_net; }
  bool isMergedNet() const { return merged_net_; }
  void setDirtyNet(bool is_dirty_net) { is_dirty_net_ = is_dirty_net; }
  bool isDirtyNet() const { return is_dirty_net_; }

 private:
  int getNumBTermsAboveMaxLayer(odb::dbTechLayer* max_routing_layer);

  odb::dbNet* net_;
  std::vector<Pin> pins_;
  float slack_;
  bool has_wires_;
  std::vector<SegmentIndex> parent_segment_indices_;
  std::multiset<RoutePt> last_pin_positions_;
  bool merged_net_;
  bool is_dirty_net_;
};

}  // namespace grt
