// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <QObject>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/geom.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/SdcClass.hh"

namespace sta {
class Scene;
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
        fanout_(fanout),
        paired_nodes_({})
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
  odb::Rect getPinBBox() const;
  odb::Rect getPinLargestBox() const;

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
  float path_slack_{0.0};
  int fanout_;

  std::set<const TimingPathNode*> paired_nodes_;
  const TimingPathNode* instance_node_{nullptr};
};

class TimingPath
{
 public:
  enum PathSection
  {
    kAll,
    kLaunch,
    kData,
    kCapture
  };

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

  void populatePath(sta::Path* path, sta::dbSta* sta, bool clock_expanded);
  void populateCapturePath(sta::Path* path,
                           sta::dbSta* sta,
                           float offset,
                           bool clock_expanded);
  std::vector<odb::dbNet*> getNets(const PathSection& path_section) const;

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
                        sta::Scene* scene,
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
  void getNets(std::vector<odb::dbNet*>& nets,
               const TimingNodeList& nodes,
               bool only_clock,
               bool only_data) const;
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

  std::set<const sta::Pin*> getDrivers(bool visibility) const;
  std::set<const sta::Pin*> getLeaves(bool visibility) const;

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

  int getTotalFanout(bool visibility) const;
  int getTotalLeaves(bool visibility) const;
  int getMaxLeaves(bool visibility) const;
  sta::Delay getMinimumArrival(bool visibility) const;
  sta::Delay getMaximumArrival(bool visibility) const;
  sta::Delay getMinimumDriverDelay(bool visibility) const;
  int getSinkCount() const;

  std::set<odb::dbNet*> getNets(bool visibility) const;

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

class STAGuiInterface
{
 public:
  STAGuiInterface(sta::dbSta* sta = nullptr);

  void setSTA(sta::dbSta* sta)
  {
    sta_ = sta;
    scene_ = sta->cmdScene();
  }
  sta::dbSta* getSTA() const { return sta_; }

  sta::dbNetwork* getNetwork() const { return sta_->getDbNetwork(); }

  sta::Scene* getScene() const { return scene_; }
  void setScene(sta::Scene* scene) { scene_ = scene; }

  bool isUseMax() const { return use_max_; }
  void setUseMax(bool use_max) { use_max_ = use_max; }
  const sta::MinMaxAll* minMaxAll() const
  {
    return use_max_ ? sta::MinMaxAll::max() : sta::MinMaxAll::min();
  }
  const sta::MinMax* minMax() const
  {
    return use_max_ ? sta::MinMax::max() : sta::MinMax::min();
  }

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
                                const std::string& path_group_name,
                                const sta::ClockSet* clks) const;
  TimingPathList getTimingPaths(const sta::Pin* thru) const;

  std::unique_ptr<TimingPathNode> getTimingNode(const sta::Pin* pin) const;

  ConeDepthMapPinSet getFaninCone(const sta::Pin* pin) const;
  ConeDepthMapPinSet getFanoutCone(const sta::Pin* pin) const;
  ConeDepthMap buildConeConnectivity(const sta::Pin* pin,
                                     ConeDepthMapPinSet& depth_map) const;

  const sta::ClockSeq* getClocks() const;
  std::vector<std::unique_ptr<ClockTree>> getClockTrees() const;

  int getEndPointCount() const;
  StaPins getEndPoints() const;

  float getPinSlack(const sta::Pin* pin) const;
  EndPointSlackMap getEndPointToSlackMap(const std::string& path_group_name,
                                         const sta::Clock* clk = nullptr);
  EndPointSlackMap getEndPointToSlackMap(const sta::Clock* clk);

  std::set<std::string> getGroupPathsNames() const;
  void updatePathGroups();

 private:
  sta::dbSta* sta_;

  sta::Scene* scene_;
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
