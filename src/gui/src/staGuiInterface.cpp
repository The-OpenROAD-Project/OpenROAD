// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "staGuiInterface.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/ClkNetwork.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathGroup.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/StringUtil.hh"
#include "sta/VisitPathEnds.hh"
#include "utl/Logger.h"
#include "web/clockTree.h"

namespace gui {

std::string TimingPathNode::getNodeName(bool include_master) const
{
  if (isPinITerm()) {
    odb::dbITerm* iterm = getPinAsITerm();

    std::string name = iterm->getName();
    if (include_master) {
      odb::dbInst* inst = iterm->getInst();
      name += " (" + inst->getMaster()->getName() + ")";
    }

    return name;
  }
  return getNetName();
}

std::string TimingPathNode::getNetName() const
{
  return getNet()->getName();
}

odb::dbNet* TimingPathNode::getNet() const
{
  if (isPinITerm()) {
    return getPinAsITerm()->getNet();
  }
  return getPinAsBTerm()->getNet();
}

odb::dbInst* TimingPathNode::getInstance() const
{
  if (isPinITerm()) {
    return getPinAsITerm()->getInst();
  }

  return nullptr;
}

odb::dbITerm* TimingPathNode::getPinAsITerm() const
{
  if (isPinITerm()) {
    return static_cast<odb::dbITerm*>(pin_);
  }

  return nullptr;
}

odb::dbBTerm* TimingPathNode::getPinAsBTerm() const
{
  if (isPinBTerm()) {
    return static_cast<odb::dbBTerm*>(pin_);
  }

  return nullptr;
}

void TimingPathNode::copyData(TimingPathNode* other) const
{
  other->pin_ = pin_;
  other->stapin_ = stapin_;
  other->is_clock_ = is_clock_;
  other->is_rising_ = is_rising_;
  other->is_sink_ = is_sink_;
  other->has_values_ = has_values_;
  other->arrival_ = arrival_;
  other->delay_ = delay_;
  other->slew_ = slew_;
  other->load_ = load_;
  other->path_slack_ = path_slack_;
  other->fanout_ = fanout_;
}

odb::Rect TimingPathNode::getPinBBox() const
{
  if (isPinITerm()) {
    return getPinAsITerm()->getBBox();
  }
  return getPinAsBTerm()->getBBox();
}

odb::Rect TimingPathNode::getPinLargestBox() const
{
  if (isPinITerm()) {
    auto* iterm = getPinAsITerm();
    const odb::dbTransform transform = iterm->getInst()->getTransform();

    odb::Rect pin_rect;
    auto* mterm = iterm->getMTerm();
    for (auto* pin : mterm->getMPins()) {
      for (auto* box : pin->getGeometry()) {
        odb::Rect box_rect = box->getBox();
        transform.apply(box_rect);
        if (pin_rect.dx() < box_rect.dx()) {
          pin_rect = box_rect;
        }
      }
    }

    return pin_rect;
  }
  auto* bterm = getPinAsBTerm();

  odb::Rect pin_rect;
  for (auto* pin : bterm->getBPins()) {
    for (auto* box : pin->getBoxes()) {
      odb::Rect box_rect = box->getBox();
      if (pin_rect.dx() < box_rect.dx()) {
        pin_rect = box_rect;
      }
    }
  }
  return pin_rect;
}

/////////

TimingPath::TimingPath()
    : slack_(0),
      skew_(0),
      path_delay_(0),
      arr_time_(0),
      req_time_(0),
      logic_delay_(0),
      logic_depth_(0),
      fanout_(0),
      clk_path_end_index_(0),
      clk_capture_end_index_(0)
{
}

void TimingPath::populateNodeList(sta::Path* path,
                                  sta::dbSta* sta,
                                  sta::Scene* scene,
                                  const float offset,
                                  const bool is_capture_path,
                                  const bool clock_expanded,
                                  TimingNodeList& list)
{
  float arrival_prev_stage = 0.0f;
  float arrival_cur_stage = 0.0f;

  sta::PathExpanded expand(path, sta);
  auto* graph = sta->graph();
  auto* network = sta->network();
  const sta::Sdc* sdc = scene->sdc();

  // Used to compute logic metrics.
  std::set<sta::Instance*> logic_insts;
  sta::Instance* inst_of_prev_pin = nullptr;
  sta::Instance* prev_inst = nullptr;
  float curr_inst_delay = 0.0f;
  float prev_inst_delay = 0.0f;
  bool pin_belongs_to_inverter_pair_instance = false;

  for (size_t i = 0; i < expand.size(); i++) {
    const auto* ref = expand.path(i);
    sta::Vertex* vertex = ref->vertex(sta);
    const auto pin = vertex->pin();
    const bool pin_is_clock = sta->isClock(pin, scene->mode());
    const bool is_driver = network->isDriver(pin);
    const bool is_rising = ref->transition(sta) == sta::RiseFall::rise();
    const auto arrival = ref->arrival();

    // based on:
    // https://github.com/The-OpenROAD-Project/OpenSTA/blob/a48199d52df23732164c378b6c5dcea5b1b301a1/search/ReportPath.cc#L2756
    int node_fanout = 0;
    sta::VertexOutEdgeIterator iter(vertex, graph);
    while (iter.hasNext()) {
      sta::Edge* edge = iter.next();
      if (edge->isWire()) {
        const sta::Pin* pin = edge->to(graph)->pin();
        if (network->isTopLevelPort(pin)) {
          // Output port counts as a fanout.
          sta::Port* port = network->port(pin);
          node_fanout += sdc->portExtFanout(port, sta::MinMax::max()) + 1;
        } else {
          node_fanout++;
        }
      }
    }

    float cap = 0.0;
    if (is_driver && (clock_expanded || (!network->isCheckClk(pin) && i))) {
      sta::GraphDelayCalc* graph_delay_calc = sta->graphDelayCalc();
      cap = graph_delay_calc->loadCap(
          pin, ref->transition(sta), ref->scene(sta), ref->minMax(sta));
    }

    odb::dbITerm* term;
    odb::dbBTerm* port;
    odb::dbModITerm* moditerm;
    sta->getDbNetwork()->staToDb(pin, term, port, moditerm);
    odb::dbObject* pin_object = term;
    if (term == nullptr) {
      pin_object = port;
    }

    float slew = 0.0f;

    if (!sta->isIdealClock(pin, scene->mode())) {
      arrival_cur_stage = arrival;
      slew = ref->slew(sta);
    }

    const float pin_delay = arrival_cur_stage - arrival_prev_stage;

    if (!is_capture_path) {
      sta::Instance* inst_of_curr_pin = network->instance(pin);

      if (!sta->isClock(pin, scene->mode())) {
        updateLogicMetrics(network,
                           inst_of_curr_pin,
                           inst_of_prev_pin,
                           prev_inst,
                           pin_delay,
                           logic_insts,
                           curr_inst_delay,
                           prev_inst_delay,
                           pin_belongs_to_inverter_pair_instance);
      }

      logic_depth_ = static_cast<int>(logic_insts.size());
      inst_of_prev_pin = inst_of_curr_pin;
    }

    if (!sta->isClock(pin, scene->mode())) {
      fanout_ += node_fanout;
    }

    sta::dbNetwork* network = sta->getDbNetwork();
    if (network->flatNet(pin) == nullptr) {
      sta->getLogger()->error(
          utl::GUI, 111, "Timing pin {} has a null net.", network->name(pin));
    }
    list.push_back(std::make_unique<TimingPathNode>(pin_object,
                                                    pin,
                                                    pin_is_clock,
                                                    is_rising,
                                                    !is_driver,
                                                    true,
                                                    arrival_cur_stage + offset,
                                                    pin_delay,
                                                    slew,
                                                    cap,
                                                    node_fanout));
    arrival_prev_stage = arrival_cur_stage;
  }

  // populate list with source/sink nodes
  for (int i = 0; i < list.size(); i++) {
    auto* node = list[i].get();
    if (node->isSource()) {
      if (i < (list.size() - 1)) {
        // get the next node
        node->addPairedNode(list[i + 1].get());
      }
    } else {
      // node is sink
      node->addPairedNode(node);
    }
  }

  // first find the first instance node
  TimingPathNode* instance_node = nullptr;
  for (auto& node : list) {
    if (node->hasInstance()) {
      instance_node = node.get();
      break;
    }
  }
  // populate with instance nodes
  for (auto& node : list) {
    if (node->hasInstance()) {
      instance_node = node.get();
    }
    node->setInstanceNode(instance_node);
  }
}

void TimingPath::updateLogicMetrics(sta::Network* network,
                                    sta::Instance* inst_of_curr_pin,
                                    sta::Instance* inst_of_prev_pin,
                                    sta::Instance* prev_inst,
                                    const float pin_delay,
                                    std::set<sta::Instance*>& logic_insts,
                                    float& curr_inst_delay,
                                    float& prev_inst_delay,
                                    bool& pin_belongs_to_inverter_pair_instance)
{
  if (pin_belongs_to_inverter_pair_instance) {
    pin_belongs_to_inverter_pair_instance = false;
    return;
  }

  if (!instanceIsLogic(inst_of_curr_pin, network)) {
    return;
  }

  curr_inst_delay += pin_delay;
  logic_insts.insert(inst_of_curr_pin);

  if (inst_of_prev_pin == inst_of_curr_pin) {
    logic_delay_ += curr_inst_delay;
    curr_inst_delay = 0;

    prev_inst = inst_of_curr_pin;
    prev_inst_delay = curr_inst_delay;
  } else if (instancesAreInverterPair(inst_of_curr_pin, prev_inst, network)) {
    logic_delay_ -= prev_inst_delay;
    curr_inst_delay = 0;

    logic_insts.erase(prev_inst);
    logic_insts.erase(inst_of_curr_pin);

    // We need this to skip the next pin of inverter pair's second instance.
    pin_belongs_to_inverter_pair_instance = true;

    // We need this in case there's another inverter pair ahead.
    prev_inst = nullptr;
  }
}

bool TimingPath::instanceIsLogic(sta::Instance* inst, sta::Network* network)
{
  sta::LibertyCell* lib_cell = network->libertyCell(inst);

  if (!lib_cell) {
    return false;
  }

  if (lib_cell->isBuffer()) {
    return false;
  }

  return true;
}

bool TimingPath::instancesAreInverterPair(sta::Instance* curr_inst,
                                          sta::Instance* prev_inst,
                                          sta::Network* network)
{
  if (!prev_inst || !curr_inst) {
    return false;
  }

  sta::LibertyCell* prev = network->libertyCell(prev_inst);
  sta::LibertyCell* curr = network->libertyCell(curr_inst);

  if (!prev || !curr) {
    return false;
  }

  if (!curr->isInverter() || !prev->isInverter()) {
    return false;
  }

  return true;
}

void TimingPath::populatePath(sta::Path* path,
                              sta::dbSta* sta,
                              bool clock_expanded)
{
  populateNodeList(
      path, sta, path->scene(sta), 0, false, clock_expanded, path_nodes_);
}

void TimingPath::populateCapturePath(sta::Path* path,
                                     sta::dbSta* sta,
                                     float offset,
                                     bool clock_expanded)
{
  populateNodeList(path,
                   sta,
                   path->scene(sta),
                   offset,
                   true,
                   clock_expanded,
                   capture_nodes_);
}

std::vector<odb::dbNet*> TimingPath::getNets(
    const PathSection& path_section) const
{
  std::vector<odb::dbNet*> section_nets;
  switch (path_section) {
    case kAll: {
      getNets(section_nets, path_nodes_, false, false);
      getNets(section_nets, capture_nodes_, false, false);
      break;
    }
    case kLaunch: {
      getNets(section_nets, path_nodes_, true, false);
      break;
    }
    case kData: {
      getNets(section_nets, path_nodes_, false, true);
      break;
    }
    case kCapture: {
      getNets(section_nets, capture_nodes_, false, false);
      break;
    }
  }

  return section_nets;
}

void TimingPath::getNets(std::vector<odb::dbNet*>& nets,
                         const TimingNodeList& nodes,
                         const bool only_clock,
                         const bool only_data) const
{
  for (const auto& node : nodes) {
    if (only_clock && !node->isClock()) {
      continue;
    }
    if (only_data && node->isClock()) {
      continue;
    }

    if (node->isSource()) {
      for (auto* sink_node : node->getPairedNodes()) {
        if (sink_node != nullptr) {
          nets.push_back(sink_node->getNet());
        }
      }
    }
  }
}

std::string TimingPath::getStartStageName() const
{
  const int start_idx = getClkPathEndIndex() + 1;
  if (start_idx >= path_nodes_.size()) {
    return path_nodes_[0]->getNodeName();
  }
  return path_nodes_[start_idx]->getNodeName();
}

std::string TimingPath::getEndStageName() const
{
  return path_nodes_.back()->getNodeName();
}

const std::unique_ptr<TimingPathNode>& TimingPath::getStartStageNode() const
{
  const int start_idx = getClkPathEndIndex() + 1;
  if (start_idx >= path_nodes_.size()) {
    return path_nodes_.front();
  }

  return path_nodes_[start_idx];
}

const std::unique_ptr<TimingPathNode>& TimingPath::getEndStageNode() const
{
  return path_nodes_.back();
}

void TimingPath::computeClkEndIndex(TimingNodeList& nodes, int& index)
{
  for (int i = 0; i < nodes.size(); i++) {
    if (!nodes[i]->isClock()) {
      index = i - 1;
      return;
    }
  }

  index = nodes.size() - 1;  // assume last index is the end of the clock path
}

void TimingPath::computeClkEndIndex()
{
  computeClkEndIndex(path_nodes_, clk_path_end_index_);
  computeClkEndIndex(capture_nodes_, clk_capture_end_index_);
}

void TimingPath::setSlackOnPathNodes()
{
  for (const auto& node : path_nodes_) {
    node->setPathSlack(slack_);
  }
}

/////////////

class PathGroupSlackEndVisitor : public sta::PathEndVisitor
{
 public:
  PathGroupSlackEndVisitor(const sta::PathGroup* path_group, sta::Sta* sta);
  PathGroupSlackEndVisitor(const sta::PathGroup* path_group,
                           const sta::Clock* clk,
                           sta::Sta* sta);
  PathGroupSlackEndVisitor(const PathGroupSlackEndVisitor&) = default;
  PathEndVisitor* copy() const override;
  void visit(sta::PathEnd* path_end) override;
  float worstSlack() const { return worst_slack_; }
  bool hasSlack() const { return has_slack_; }
  void resetWorstSlack();

 private:
  const sta::PathGroup* path_group_;
  sta::Sta* sta_;
  const sta::Clock* clk_;
  bool has_slack_{false};
  float worst_slack_{std::numeric_limits<float>::max()};
};

PathGroupSlackEndVisitor::PathGroupSlackEndVisitor(
    const sta::PathGroup* path_group,
    sta::Sta* sta)
    : path_group_(path_group), sta_(sta), clk_(nullptr)
{
}

PathGroupSlackEndVisitor::PathGroupSlackEndVisitor(
    const sta::PathGroup* path_group,
    const sta::Clock* clk,
    sta::Sta* sta)
    : path_group_(path_group), sta_(sta), clk_(clk)
{
}

sta::PathEndVisitor* PathGroupSlackEndVisitor::copy() const
{
  return new PathGroupSlackEndVisitor(*this);
}

void PathGroupSlackEndVisitor::visit(sta::PathEnd* path_end)
{
  const sta::PathGroupSeq path_groups
      = sta_->cmdScene()->mode()->pathGroups(path_end);
  const auto iter
      = std::find(path_groups.begin(), path_groups.end(), path_group_);
  if (iter != path_groups.end()) {
    if (clk_ != nullptr) {
      sta::Path* path = path_end->path();
      if (path->clock(sta_) != clk_) {
        return;
      }
    }
    worst_slack_
        = std::min(worst_slack_, sta::delayAsFloat(path_end->slack(sta_)));
    if (!has_slack_) {
      has_slack_ = true;
    }
  }
}

void PathGroupSlackEndVisitor::resetWorstSlack()
{
  worst_slack_ = std::numeric_limits<float>::max();
  has_slack_ = false;
}

/////////////

STAGuiInterface::STAGuiInterface(sta::dbSta* sta)
    : sta_(sta),
      scene_(sta ? sta->cmdScene() : nullptr),
      use_max_(true),
      one_path_per_endpoint_(true),
      max_path_count_(50),
      include_unconstrained_(false),
      include_capture_path_(false),
      include_clk_gating_checks_(true)
{
}

StaPins STAGuiInterface::getEndPoints() const
{
  StaPins pins;
  for (auto end : sta_->endpoints()) {
    pins.insert(end->pin());
  }
  return pins;
}

float STAGuiInterface::getPinSlack(const sta::Pin* pin) const
{
  return sta_->slack(
      pin, sta::RiseFallBoth::riseFall(), sta_->scenes(), minMax());
}

std::set<std::string> STAGuiInterface::getGroupPathsNames() const
{
  std::set<std::string> group_paths_names;
  sta::Sdc* sdc = scene_->sdc();
  sta::GroupPathMap group_paths_map = sdc->groupPaths();
  for (const auto& [name, group_paths] : group_paths_map) {
    group_paths_names.insert(name);
  }
  return group_paths_names;
}

// Makes STA incorporate the Sdc GroupPaths information
// into Search PathGroups. This is equivalent to what happens
// when running "report_checks".
void STAGuiInterface::updatePathGroups()
{
  sta::StringSeq empty_group_names;
  for (sta::Mode* mode : sta_->modes()) {
    mode->makePathGroups(1,                 /* group count */
                         1,                 /* endpoint count*/
                         false,             /* unique pins */
                         false,             /* unique edges */
                         -sta::INF,         /* min slack */
                         sta::INF,          /* max slack*/
                         empty_group_names, /* group names */
                         true,              /* setup */
                         true,              /* hold */
                         true,              /* recovery */
                         true,              /* removal */
                         true,              /* clk gating setup */
                         true,              /* clk gating hold */
                         false /* unconstrained paths */);
  }
}

EndPointSlackMap STAGuiInterface::getEndPointToSlackMap(
    const std::string& path_group_name,
    const sta::Clock* clk)
{
  updatePathGroups();

  EndPointSlackMap end_point_to_slack;
  sta::VisitPathEnds visit_ends(sta_);
  sta::PathGroup* path_group = sta_->cmdMode()->pathGroups()->findPathGroup(
      path_group_name.c_str(), minMax());
  PathGroupSlackEndVisitor path_group_visitor(path_group, clk, sta_);
  for (sta::Vertex* vertex : sta_->endpoints()) {
    visit_ends.visitPathEnds(vertex, &path_group_visitor);
    if (path_group_visitor.hasSlack()) {
      end_point_to_slack[vertex->pin()] = path_group_visitor.worstSlack();
      path_group_visitor.resetWorstSlack();
    }
  }

  return end_point_to_slack;
}

EndPointSlackMap STAGuiInterface::getEndPointToSlackMap(const sta::Clock* clk)
{
  updatePathGroups();

  EndPointSlackMap end_point_to_slack;
  sta::VisitPathEnds visit_ends(sta_);
  sta::PathGroup* path_group
      = sta_->cmdMode()->pathGroups()->findPathGroup(clk, minMax());
  PathGroupSlackEndVisitor path_group_visitor(path_group, sta_);
  for (sta::Vertex* vertex : sta_->endpoints()) {
    visit_ends.visitPathEnds(vertex, &path_group_visitor);
    if (path_group_visitor.hasSlack()) {
      end_point_to_slack[vertex->pin()] = path_group_visitor.worstSlack();
      path_group_visitor.resetWorstSlack();
    }
  }
  return end_point_to_slack;
}

int STAGuiInterface::getEndPointCount() const
{
  return sta_->endpoints().size();
}

std::unique_ptr<TimingPathNode> STAGuiInterface::getTimingNode(
    const sta::Pin* pin) const
{
  for (const auto& path : getTimingPaths(pin)) {
    for (const auto& node : path->getPathNodes()) {
      if (node->getPinAsSTA() == pin) {
        std::unique_ptr<TimingPathNode> return_node
            = std::make_unique<TimingPathNode>(nullptr, nullptr);
        node->copyData(return_node.get());
        return return_node;
      }
    }
  }

  return nullptr;
}

TimingPathList STAGuiInterface::getTimingPaths(const sta::Pin* thru) const
{
  return getTimingPaths(
      {}, {{thru}}, {}, "" /* path group name */, nullptr /* clockset */);
}

TimingPathList STAGuiInterface::getTimingPaths(
    const StaPins& from,
    const std::vector<StaPins>& thrus,
    const StaPins& to,
    const std::string& path_group_name,
    const sta::ClockSet* clks) const
{
  TimingPathList paths;

  initSTA();

  sta::ClockSet* clks_from = nullptr;
  sta::ClockSet* clks_to = nullptr;
  if (clks) {
    clks_from = new sta::ClockSet;
    clks_to = new sta::ClockSet;
    for (auto clk : *clks) {
      clks_from->insert(clk);
      clks_to->insert(clk);
    }
  }
  sta::ExceptionFrom* e_from = nullptr;
  if (!from.empty()) {
    sta::PinSet* pins = new sta::PinSet(getNetwork());
    pins->insert(from.begin(), from.end());
    e_from = sta_->makeExceptionFrom(
        pins, clks_from, nullptr, sta::RiseFallBoth::riseFall(), scene_->sdc());
  } else if (clks_from) {
    e_from = sta_->makeExceptionFrom(nullptr,
                                     clks_from,
                                     nullptr,
                                     sta::RiseFallBoth::riseFall(),
                                     scene_->sdc());
  }

  sta::ExceptionThruSeq* e_thrus = nullptr;
  if (!thrus.empty()) {
    for (const auto& thru_set : thrus) {
      if (thru_set.empty()) {
        continue;
      }
      if (e_thrus == nullptr) {
        e_thrus = new sta::ExceptionThruSeq;
      }
      sta::PinSet* pins = new sta::PinSet(getNetwork());
      pins->insert(thru_set.begin(), thru_set.end());
      e_thrus->push_back(sta_->makeExceptionThru(pins,
                                                 nullptr,
                                                 nullptr,
                                                 sta::RiseFallBoth::riseFall(),
                                                 scene_->sdc()));
    }
  }

  sta::ExceptionTo* e_to = nullptr;
  if (!to.empty()) {
    sta::PinSet* pins = new sta::PinSet(getNetwork());
    pins->insert(to.begin(), to.end());
    e_to = sta_->makeExceptionTo(pins,
                                 clks_to,
                                 nullptr,
                                 sta::RiseFallBoth::riseFall(),
                                 sta::RiseFallBoth::riseFall(),
                                 scene_->sdc());
  } else if (clks_to) {
    e_to = sta_->makeExceptionTo(nullptr,
                                 clks_to,
                                 nullptr,
                                 sta::RiseFallBoth::riseFall(),
                                 sta::RiseFallBoth::riseFall(),
                                 scene_->sdc());
  }

  sta::StringSeq group_names;
  if (!path_group_name.empty()) {
    group_names = {path_group_name};
  }

  sta::Search* search = sta_->search();
  sta::SceneSeq search_scenes
      = scene_ != nullptr ? sta::SceneSeq{scene_} : sta_->scenes();
  sta::PathEndSeq path_ends
      = search->findPathEnds(  // from, thrus, to, unconstrained
          e_from,
          e_thrus,
          e_to,
          include_unconstrained_,
          // scene, min_max,
          search_scenes,
          minMaxAll(),
          // group_count, endpoint_count, unique_pins
          max_path_count_,
          one_path_per_endpoint_ ? 1 : max_path_count_,
          true,  // unique pins
          true,  // unique edges
          -sta::INF,
          sta::INF,  // slack_min, slack_max,
          true,      // sort_by_slack
          group_names,
          // setup, hold, recovery, removal,
          use_max_,
          !use_max_,
          false,
          false,
          // clk_gating_setup, clk_gating_hold
          include_clk_gating_checks_ && use_max_,
          include_clk_gating_checks_ && !use_max_);

  for (auto& path_end : path_ends) {
    TimingPath* timing_path = new TimingPath();
    sta::Path* path = path_end->path();

    auto* start_clock_edge = path_end->sourceClkEdge(sta_);
    if (start_clock_edge != nullptr) {
      timing_path->setStartClock(start_clock_edge->clock()->name());
    } else {
      timing_path->setStartClock("<No clock>");
    }
    auto* end_clock = path_end->targetClk(sta_);
    if (end_clock != nullptr) {
      timing_path->setEndClock(end_clock->name());
    } else {
      timing_path->setEndClock("<No clock>");
    }

    auto* path_delay = path_end->pathDelay();
    if (path_delay != nullptr) {
      timing_path->setPathDelay(path_delay->delay());
    } else {
      timing_path->setPathDelay(0.0);
    }
    timing_path->setSlack(path_end->slack(sta_));
    timing_path->setPathArrivalTime(path_end->dataArrivalTime(sta_));
    timing_path->setPathRequiredTime(path_end->requiredTime(sta_));
    timing_path->setSkew(path_end->clkSkew(sta_));

    bool clock_propagated = false;
    if (start_clock_edge != nullptr) {
      clock_propagated = start_clock_edge->clock()->isPropagated();
    }

    const bool clock_expaneded = clock_propagated;

    timing_path->populatePath(path, sta_, clock_expaneded);
    if (include_capture_path_) {
      if (path_end->targetClkPath()) {
        timing_path->populateCapturePath(path_end->targetClkPath(),
                                         sta_,
                                         path_end->targetClkOffset(sta_),
                                         clock_expaneded);
      }
    }

    timing_path->computeClkEndIndex();
    timing_path->setSlackOnPathNodes();

    paths.emplace_back(timing_path);
  }

  return paths;
}

ConeDepthMapPinSet STAGuiInterface::getFaninCone(const sta::Pin* pin) const
{
  sta::PinSeq pins_to;
  pins_to.push_back(pin);

  auto pins = sta_->findFaninPins(&pins_to,
                                  true,   // flat
                                  false,  // startpoints_only
                                  0,
                                  0,
                                  true,  // thru_disabled
                                  true,  // thru_constants
                                  scene_->mode());

  ConeDepthMapPinSet depth_map;
  for (auto& [level, pin_list] : getCone(pin, std::move(pins), true)) {
    depth_map[-level].insert(pin_list.begin(), pin_list.end());
  }

  return depth_map;
}

ConeDepthMapPinSet STAGuiInterface::getFanoutCone(const sta::Pin* pin) const
{
  sta::PinSeq pins_from;
  pins_from.push_back(pin);

  sta::PinSet pins = sta_->findFanoutPins(&pins_from,
                                          true,   // flat
                                          false,  // startpoints_only
                                          0,
                                          0,
                                          true,  // thru_disabled
                                          true,  // thru_constants
                                          scene_->mode());
  return getCone(pin, std::move(pins), false);
}

ConeDepthMapPinSet STAGuiInterface::getCone(const sta::Pin* source_pin,
                                            sta::PinSet pins,
                                            bool is_fanin) const
{
  initSTA();
  auto* network = sta_->getDbNetwork();
  auto* graph = sta_->graph();

  auto filter_pins
      = [network](const sta::Pin* pin) { return network->isRegClkPin(pin); };

  pins.erase(source_pin);

  int level = 0;
  std::map<int, std::set<const sta::Pin*>> mapped_pins;
  mapped_pins[level].insert(source_pin);
  int pins_size = -1;
  while (!pins.empty() && pins_size != pins.size()) {
    pins_size = pins.size();
    int next_level = level + 1;

    auto& source_pins = mapped_pins[level];
    auto& next_pins = mapped_pins[next_level];

    for (auto* pin : source_pins) {
      auto* pin_vertex = graph->pinDrvrVertex(pin);

      sta::VertexEdgeIterator* itr = nullptr;
      if (is_fanin) {
        itr = new sta::VertexInEdgeIterator(pin_vertex, graph);
      } else {
        itr = new sta::VertexOutEdgeIterator(pin_vertex, graph);
      }

      while (itr->hasNext()) {
        auto* next_edge = itr->next();
        sta::Vertex* next_vertex = next_edge->to(graph);
        if (next_vertex == pin_vertex) {
          next_vertex = next_edge->from(graph);
        }
        auto* next_pin = next_vertex->pin();
        auto pin_find = pins.find(next_pin);
        if (pin_find != pins.end()) {
          if (!filter_pins(next_pin)) {
            next_pins.insert(next_pin);
          }

          pins.erase(pin_find);
        }
      }

      delete itr;
    }

    level = next_level;
  }

  ConeDepthMapPinSet depth_map;

  for (const auto& [level, pin_list] : mapped_pins) {
    auto& dmap = depth_map[level];
    for (auto* pin : pin_list) {
      dmap.insert(pin);
    }
  }

  return depth_map;
}

ConeDepthMap STAGuiInterface::buildConeConnectivity(
    const sta::Pin* stapin,
    ConeDepthMapPinSet& depth_map) const
{
  auto* network = sta_->getDbNetwork();
  auto* graph = sta_->graph();

  ConeDepthMap map;
  for (const auto& [level, pins] : depth_map) {
    auto& map_level = map[level];
    for (auto* pin : pins) {
      odb::dbObject* dbpin = nullptr;

      odb::dbBTerm* bterm;
      odb::dbITerm* iterm;
      odb::dbModITerm* moditerm;
      network->staToDb(pin, iterm, bterm, moditerm);
      if (bterm != nullptr) {
        dbpin = bterm;
      } else {
        dbpin = iterm;
      }

      map_level.push_back(std::make_unique<TimingPathNode>(dbpin, pin));
    }
  }

  // clear map pairs
  for (const auto& [level, pin_list] : map) {
    for (const auto& pin : pin_list) {
      pin->clearPairedNodes();
    }
  }

  for (const auto& [level, pin_list] : map) {
    int next_level = level + 1;
    if (!map.contains(next_level)) {
      break;
    }

    std::map<sta::Vertex*, TimingPathNode*> next_pins;
    for (const auto& pin : map[next_level]) {
      const sta::Pin* sta_pin = pin->getPinAsSTA();
      next_pins[graph->pinDrvrVertex(sta_pin)] = pin.get();
      next_pins[graph->pinLoadVertex(sta_pin)] = pin.get();
    }

    for (const auto& source_pin : pin_list) {
      const sta::Pin* sta_pin = source_pin->getPinAsSTA();
      sta::VertexOutEdgeIterator fanout_iter(graph->pinDrvrVertex(sta_pin),
                                             graph);
      while (fanout_iter.hasNext()) {
        sta::Edge* edge = fanout_iter.next();
        sta::Vertex* fanout = edge->to(graph);
        auto next_pins_find = next_pins.find(fanout);
        if (next_pins_find != next_pins.end()) {
          next_pins_find->second->addPairedNode(source_pin.get());
        }
      }
    }
  }

  annotateConeTiming(stapin, map);
  return map;
}

void STAGuiInterface::annotateConeTiming(const sta::Pin* source_pin,
                                         ConeDepthMap& map) const
{
  int min_map_index = std::numeric_limits<int>::max();
  int max_map_index = std::numeric_limits<int>::min();
  for (const auto& [level, pins_list] : map) {
    min_map_index = std::min(min_map_index, level);
    max_map_index = std::max(max_map_index, level);
  }

  // annotate critical path and work forwards/backwards until all pins have
  // timing
  std::vector<TimingPathNode*> pin_order;
  for (int i = 0; i <= std::max(-min_map_index, max_map_index); i++) {
    if (i <= -min_map_index) {
      for (const auto& pin : map[-i]) {
        pin_order.push_back(pin.get());
      }
    }
    if (i <= max_map_index) {
      for (const auto& pin : map[i]) {
        pin_order.push_back(pin.get());
      }
    }
  }

  // populate with max number of unique paths, all other paths will appear as 0
  const int path_count = 1000;

  STAGuiInterface stagui(sta_);
  stagui.setUseMax(true);
  stagui.setIncludeUnconstrainedPaths(true);
  stagui.setMaxPathCount(path_count);
  stagui.setIncludeCapturePaths(false);

  TimingPathList paths = stagui.getTimingPaths(source_pin);

  for (const auto& path : paths) {
    for (const auto& node : path->getPathNodes()) {
      auto pin_find
          = std::ranges::find_if(pin_order,

                                 [&node](const TimingPathNode* other) {
                                   return node->getPin() == other->getPin();
                                 });

      if (pin_find != pin_order.end()) {
        TimingPathNode* pin_node = *pin_find;
        if (pin_node->hasValues()) {
          continue;
        }

        node->copyData(pin_node);
      }
    }
  }

  for (auto& [level, pin_list] : map) {
    std::ranges::sort(pin_list, [](const auto& l, const auto& r) {
      return l->getPathSlack() > r->getPathSlack();
    });
  }
}

const sta::ClockSeq* STAGuiInterface::getClocks() const
{
  return &scene_->sdc()->clocks();
}

std::vector<std::unique_ptr<ClockTree>> STAGuiInterface::getClockTrees() const
{
  return web::buildClockTrees(sta_, scene_);
}

void STAGuiInterface::initSTA() const
{
  sta_->ensureGraph();
  sta_->searchPreamble();
}

}  // namespace gui
