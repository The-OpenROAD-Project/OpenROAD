/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QObject>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

namespace sta {
class Corner;
class PathExpanded;
}  // namespace sta

namespace gui {
class TimingPath;
class TimingPathNode;
class STAGuiInterface;

using TimingPathList = std::vector<std::unique_ptr<TimingPath>>;
using TimingNodeList = std::vector<std::unique_ptr<TimingPathNode>>;
using StaPins = std::set<const sta::Pin*>;
using EndPointSlackMap = std::map<const sta::Pin*, float>;
using ConeDepthMapPinSet = std::map<int, StaPins>;
using ConeDepthMap = std::map<int, TimingNodeList>;

class TimingPathNode
{
 public:
  TimingPathNode(odb::dbObject* pin,
                 const sta::Pin* stapin,
                 bool is_clock = false,
                 bool is_rising = false,
                 bool is_sink = false,
                 bool has_values = false,
                 float arrival = 0.0,
                 float delay = 0.0,
                 float slew = 0.0,
                 float load = 0.0,
                 int fanout = 0)
      : pin_(pin),
        stapin_(stapin),
        is_clock_(is_clock),
        is_rising_(is_rising),
        is_sink_(is_sink),
        has_values_(has_values),
        arrival_(arrival),
        delay_(delay),
        slew_(slew),
        load_(load),
        path_slack_(0.0),
        fanout_(fanout),
        paired_nodes_({}),
        instance_node_(nullptr)
  {
  }

  std::string getNodeName(bool include_master = false) const;
  std::string getNetName() const;

  odb::dbNet* getNet() const;
  odb::dbInst* getInstance() const;
  bool hasInstance() const { return getInstance() != nullptr; }

  bool isPinITerm() const
  {
    return pin_->getObjectType() == odb::dbObjectType::dbITermObj;
  }
  bool isPinBTerm() const
  {
    return pin_->getObjectType() == odb::dbObjectType::dbBTermObj;
  }

  odb::dbObject* getPin() const { return pin_; }
  const sta::Pin* getPinAsSTA() const { return stapin_; }
  odb::dbITerm* getPinAsITerm() const;
  odb::dbBTerm* getPinAsBTerm() const;
  const odb::Rect getPinBBox() const;
  const odb::Rect getPinLargestBox() const;

  bool isClock() const { return is_clock_; }
  bool isRisingEdge() const { return is_rising_; }
  bool isSink() const { return is_sink_; }
  bool isSource() const { return !is_sink_; }

  float getArrival() const { return arrival_; }
  float getDelay() const { return delay_; }
  float getSlew() const { return slew_; }
  float getLoad() const { return load_; }

  void setPathSlack(float value) { path_slack_ = value; }
  float getPathSlack() const { return path_slack_; }

  void setFanout(int fanout) { fanout_ = fanout; }
  int getFanout() const { return fanout_; }

  bool hasValues() const { return has_values_; }

  void addPairedNode(const TimingPathNode* node) { paired_nodes_.insert(node); }
  void clearPairedNodes() { paired_nodes_.clear(); }
  const std::set<const TimingPathNode*>& getPairedNodes() const
  {
    return paired_nodes_;
  }
  void setInstanceNode(const TimingPathNode* node) { instance_node_ = node; }
  const TimingPathNode* getInstanceNode() const { return instance_node_; }

  void copyData(TimingPathNode* other) const;

 private:
  odb::dbObject* pin_;
  const sta::Pin* stapin_;
  bool is_clock_;
  bool is_rising_;
  bool is_sink_;
  bool has_values_;
  float arrival_;
  float delay_;
  float slew_;
  float load_;
  float path_slack_;
  int fanout_;

  std::set<const TimingPathNode*> paired_nodes_;
  const TimingPathNode* instance_node_;
};

class TimingPath
{
 public:
  TimingPath();

  void setStartClock(const char* name) { start_clk_ = name; }
  const std::string& getStartClock() const { return start_clk_; }
  void setEndClock(const char* name) { end_clk_ = name; }
  const std::string& getEndClock() const { return end_clk_; }

  float getPathArrivalTime() const { return arr_time_; }
  void setPathArrivalTime(float arr) { arr_time_ = arr; }
  float getPathRequiredTime() const { return req_time_; }
  void setPathRequiredTime(float req) { req_time_ = req; }
  float getSlack() const { return slack_; }
  void setSlack(float slack) { slack_ = slack; }
  float getPathDelay() const { return path_delay_; }
  void setPathDelay(float del) { path_delay_ = del; }
  float getSkew() const { return skew_; }
  void setSkew(float skew) { skew_ = skew; }
  float getLogicDelay() const { return logic_delay_; }
  int getLogicDepth() const { return logic_depth_; }
  int getFanout() const { return fanout_; }

  void computeClkEndIndex();
  void setSlackOnPathNodes();

  int getClkPathEndIndex() const { return clk_path_end_index_; }
  int getClkCaptureEndIndex() const { return clk_capture_end_index_; }

  TimingNodeList& getPathNodes() { return path_nodes_; }
  TimingNodeList& getCaptureNodes() { return capture_nodes_; }

  std::string getStartStageName() const;
  std::string getEndStageName() const;

  const std::unique_ptr<TimingPathNode>& getStartStageNode() const;
  const std::unique_ptr<TimingPathNode>& getEndStageNode() const;

  void populatePath(sta::Path* path,
                    sta::dbSta* sta,
                    sta::DcalcAnalysisPt* dcalc_ap,
                    bool clock_expanded);
  void populateCapturePath(sta::Path* path,
                           sta::dbSta* sta,
                           sta::DcalcAnalysisPt* dcalc_ap,
                           float offset,
                           bool clock_expanded);

 private:
  TimingNodeList path_nodes_;
  TimingNodeList capture_nodes_;
  std::string start_clk_;
  std::string end_clk_;
  float slack_;
  float skew_;
  float path_delay_;
  float arr_time_;
  float req_time_;
  float logic_delay_;
  int logic_depth_;
  int fanout_;
  int clk_path_end_index_;
  int clk_capture_end_index_;

  void populateNodeList(sta::Path* path,
                        sta::dbSta* sta,
                        sta::DcalcAnalysisPt* dcalc_ap,
                        float offset,
                        bool clock_expanded,
                        bool is_capture_path,
                        TimingNodeList& list);
  bool instanceIsLogic(sta::Instance* inst, sta::Network* network);
  bool instancesAreInverterPair(sta::Instance* curr_inst,
                                sta::Instance* prev_inst,
                                sta::Network* network);
  void updateLogicMetrics(sta::Network* network,
                          sta::Instance* inst_of_curr_pin,
                          sta::Instance* inst_of_prev_pin,
                          sta::Instance* prev_inst,
                          float pin_delay,
                          std::set<sta::Instance*>& logic_insts,
                          float& curr_inst_delay,
                          float& prev_inst_delay,
                          bool& pin_belongs_to_inverter_pair_instance);
  void computeClkEndIndex(TimingNodeList& nodes, int& index);
};

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

  std::set<const sta::Pin*> getDrivers() const;
  std::set<const sta::Pin*> getLeaves() const;

  const std::vector<std::unique_ptr<ClockTree>>& getFanout() const
  {
    return fanout_;
  }

  sta::Clock* getClock() const { return clock_; }

  const PinDelays& getDriverDelays() const { return drivers_; }
  const PinDelays& getChildSinkDelays() const { return child_sinks_; }
  const PinDelays& getLeavesDelays() const { return leaves_; }

  std::pair<const sta::Pin*, sta::Delay> getPairedSink(
      const sta::Pin* paired_pin) const;

  int getTotalFanout() const;
  int getTotalLeaves() const;
  int getMaxLeaves() const;
  sta::Delay getMinimumArrival() const;
  sta::Delay getMaximumArrival() const;
  sta::Delay getMinimumDriverDelay() const;
  int getSinkCount() const;

  std::set<odb::dbNet*> getNets() const;

  void addPath(sta::PathExpanded& path, const sta::StaState* sta);

  std::vector<std::pair<const sta::Pin*, const sta::Pin*>> findPathTo(
      const sta::Pin* pin) const;
  ClockTree* findTree(odb::dbNet* net, bool include_children = true);
  ClockTree* findTree(sta::Net* net, bool include_children = true);

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

  void addPath(sta::PathExpanded& path, int idx, const sta::StaState* sta);
  ClockTree* getTree(sta::Net* net);
  bool isLeaf(const sta::Pin* pin) const;
  bool addVertex(sta::Vertex* vertex, sta::Delay delay);
  sta::Net* getNet(const sta::Pin* pin) const;

  std::map<const sta::Pin*, std::set<const sta::Pin*>> getPinMapping() const;
};

class STAGuiInterface
{
 public:
  STAGuiInterface(sta::dbSta* sta = nullptr);

  void setSTA(sta::dbSta* sta) { sta_ = sta; }
  sta::dbSta* getSTA() const { return sta_; }

  sta::dbNetwork* getNetwork() const { return sta_->getDbNetwork(); }

  sta::Corner* getCorner() const { return corner_; }
  void setCorner(sta::Corner* corner) { corner_ = corner; }

  bool isUseMax() const { return use_max_; }
  void setUseMax(bool use_max) { use_max_ = use_max; }

  int getMaxPathCount() const { return max_path_count_; }
  void setMaxPathCount(int max_paths) { max_path_count_ = max_paths; }

  bool isIncludeUnconstrainedPaths() const { return include_unconstrained_; }
  void setIncludeUnconstrainedPaths(bool value)
  {
    include_unconstrained_ = value;
  }

  bool isOnePathPerEndpoint() const { return one_path_per_endpoint_; }
  void setOnePathPerEndpoint(bool value) { one_path_per_endpoint_ = value; }

  bool isIncludeCapturePaths() const { return include_capture_path_; }
  void setIncludeCapturePaths(bool value) { include_capture_path_ = value; }

  TimingPathList getTimingPaths(const StaPins& from,
                                const std::vector<StaPins>& thrus,
                                const StaPins& to,
                                const std::string& path_group_name) const;
  TimingPathList getTimingPaths(const sta::Pin* thru) const;

  std::unique_ptr<TimingPathNode> getTimingNode(const sta::Pin* pin) const;

  ConeDepthMapPinSet getFaninCone(const sta::Pin* pin) const;
  ConeDepthMapPinSet getFanoutCone(const sta::Pin* pin) const;
  ConeDepthMap buildConeConnectivity(const sta::Pin* pin,
                                     ConeDepthMapPinSet& depth_map) const;

  sta::ClockSeq* getClocks() const;
  std::vector<std::unique_ptr<ClockTree>> getClockTrees() const;

  int getEndPointCount() const;
  StaPins getEndPoints() const;
  StaPins getStartPoints() const;

  float getPinSlack(const sta::Pin* pin) const;
  EndPointSlackMap getEndPointToSlackMap(const std::string& path_group_name);

  std::set<std::string> getGroupPathsNames() const;
  void updatePathGroups();

 private:
  sta::dbSta* sta_;

  sta::Corner* corner_;
  bool use_max_;
  bool one_path_per_endpoint_;
  int max_path_count_;

  bool include_unconstrained_;
  bool include_capture_path_;

  ConeDepthMapPinSet getCone(const sta::Pin* pin,
                             sta::PinSet pin_set,
                             bool is_fanin) const;
  void annotateConeTiming(const sta::Pin* pin, ConeDepthMap& map) const;

  void initSTA() const;
};

}  // namespace gui
