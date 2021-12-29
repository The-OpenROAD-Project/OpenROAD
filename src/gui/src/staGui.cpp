/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
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

#include <QApplication>
#include <QDebug>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#include "db.h"
#include "dbShape.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Bfs.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "sta/VertexVisitor.hh"
#include "staGui.h"

namespace gui {

const Painter::Color TimingPathRenderer::inst_highlight_color_
    = Painter::Color(gui::Painter::highlight, 100);
const Painter::Color TimingPathRenderer::path_inst_color_
    = Painter::Color(gui::Painter::magenta, 100);
const Painter::Color TimingPathRenderer::term_color_ = Painter::Color(gui::Painter::blue, 100);
const Painter::Color TimingPathRenderer::signal_color_ = Painter::Color(gui::Painter::red, 100);
const Painter::Color TimingPathRenderer::clock_color_ = Painter::Color(gui::Painter::cyan, 100);
const Painter::Color TimingPathRenderer::capture_clock_color_ = Painter::Color(gui::Painter::green, 100);

/////////

TimingPathsModel::TimingPathsModel(sta::dbSta* sta, QObject* parent)
    : QAbstractTableModel(parent), sta_(sta)
{
}

TimingPath* TimingPathsModel::getPathAt(const QModelIndex& index) const
{
  return timing_paths_[index.row()].get();
}

int TimingPathsModel::rowCount(const QModelIndex& parent) const
{
  return timing_paths_.size();
}

int TimingPathsModel::columnCount(const QModelIndex& parent) const
{
  return 6;
}

QVariant TimingPathsModel::data(const QModelIndex& index, int role) const
{
  const Column col_index = static_cast<Column>(index.column());

  if (role == Qt::TextAlignmentRole) {
    switch (col_index) {
      case Clock:
      case Start:
      case End:
        return Qt::AlignLeft;
      case Required:
      case Arrival:
      case Slack:
        return Qt::AlignRight;
    }
  } else if (role == Qt::DisplayRole) {
    auto time_units = sta_->search()->units()->timeUnit();
    auto* timing_path = getPathAt(index);
    switch (col_index) {
      case Clock:
        return QString::fromStdString(timing_path->getEndClock());
      case Required:
        return QString(time_units->asString(timing_path->getPathRequiredTime()));
      case Arrival:
        return QString(time_units->asString(timing_path->getPathArrivalTime()));
      case Slack:
        return QString(time_units->asString(timing_path->getSlack()));
      case Start:
        return QString::fromStdString(timing_path->getStartStageName());
      case End:
        return QString::fromStdString(timing_path->getEndStageName());
    }
  }
  return QVariant();
}

QVariant TimingPathsModel::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    switch (static_cast<Column>(section)) {
    case Clock:
      return "Capture Clock";
    case Required:
      return "Required";
    case Arrival:
      return "Arrival";
    case Slack:
      return "Slack";
    case Start:
      return "Start";
    case End:
      return "End";
    }
  }
  return QVariant();
}

void TimingPathsModel::resetModel()
{
  beginResetModel();
  timing_paths_.clear();
  endResetModel();
}

void TimingPathsModel::sort(int col_index, Qt::SortOrder sort_order)
{
  std::function<bool(const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2)> sort_func;
  if (col_index == Clock) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getEndClock() < path2->getEndClock();
    };
  } else if (col_index == Required) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getPathRequiredTime() < path2->getPathRequiredTime();
    };
  } else if (col_index == Arrival) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getPathArrivalTime() < path2->getPathArrivalTime();
    };
  } else if (col_index == Slack) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getSlack() < path2->getSlack();
    };
  } else if (col_index == Start) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getStartStageName() < path2->getStartStageName();
    };
  } else if (col_index == End) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getEndStageName() < path2->getEndStageName();
    };
  } else {
    return;
  }

  beginResetModel();

  if (sort_order == Qt::AscendingOrder) {
    std::stable_sort(timing_paths_.begin(), timing_paths_.end(), sort_func);
  } else {
    std::stable_sort(timing_paths_.rbegin(), timing_paths_.rend(), sort_func);
  }

  endResetModel();
}

void TimingPathsModel::populateModel(bool setup_hold, int path_count)
{
  beginResetModel();
  timing_paths_.clear();
  populatePaths(setup_hold, path_count);
  endResetModel();
}

bool TimingPathsModel::populatePaths(bool get_max,
                                     int path_count)
{
  // On lines of DataBaseHandler
  QApplication::setOverrideCursor(Qt::WaitCursor);

  TimingPath::buildPaths(sta_,
                         get_max,
                         false, // unconstrained
                         path_count,
                         {}, // to
                         {}, // through
                         {}, // from
                         true,
                         timing_paths_);

  QApplication::restoreOverrideCursor();
  return true;
}

/////////

void TimingPath::buildPaths(sta::dbSta* sta,
                            bool get_max,
                            bool include_unconstrained,
                            int path_count,
                            const std::set<sta::Pin*>& from,
                            const std::set<sta::Pin*>& thrus,
                            const std::set<sta::Pin*>& to,
                            bool include_capture,
                            std::vector<std::unique_ptr<TimingPath>>& paths)
{
  sta->ensureGraph();
  sta->searchPreamble();

  sta::ExceptionFrom* e_from = nullptr;
  if (!from.empty()) {
    sta::PinSet* pins = new sta::PinSet;
    pins->insert(from.begin(), from.end());
    e_from = sta->makeExceptionFrom(pins,
                                    nullptr,
                                    nullptr,
                                    sta::RiseFallBoth::riseFall());
  }
  sta::ExceptionThruSeq* e_thrus = nullptr;
  if (!thrus.empty()) {
    e_thrus = new sta::ExceptionThruSeq;
    for (sta::Pin* thru : thrus) {
      sta::PinSet* pins = new sta::PinSet;
      pins->insert(thru);
      e_thrus->push_back(sta->makeExceptionThru(pins,
                                                nullptr,
                                                nullptr,
                                                sta::RiseFallBoth::riseFall()));
    }
  }
  sta::ExceptionTo* e_to = nullptr;
  if (!to.empty()) {
    sta::PinSet* pins = new sta::PinSet;
    pins->insert(to.begin(), to.end());
    e_to = sta->makeExceptionTo(pins,
                                nullptr,
                                nullptr,
                                sta::RiseFallBoth::riseFall(),
                                sta::RiseFallBoth::riseFall());
  }

  sta::PathEndSeq* path_ends
      = sta->search()->findPathEnds(  // from, thrus, to, unconstrained
          e_from,
          e_thrus,
          e_to,
          include_unconstrained,
          // corner, min_max,
          sta->cmdCorner(),
          get_max ? sta::MinMaxAll::max() : sta::MinMaxAll::min(),
          // group_count, endpoint_count, unique_pins
          path_count,
          path_count,
          true,
          -sta::INF,
          sta::INF,  // slack_min, slack_max,
          true,      // sort_by_slack
          nullptr,   // group_names
          // setup, hold, recovery, removal,
          get_max,
          !get_max,
          false,
          false,
          // clk_gating_setup, clk_gating_hold
          false,
          false);

  for (auto& path_end : *path_ends) {
    TimingPath* timing_path = new TimingPath();
    auto* path = path_end->path();

    sta::DcalcAnalysisPt* dcalc_ap
        = path->pathAnalysisPt(sta)->dcalcAnalysisPt();

    auto* start_clock_edge = path_end->sourceClkEdge(sta);
    if (start_clock_edge != nullptr) {
      timing_path->setStartClock(start_clock_edge->clock()->name());
    } else {
      timing_path->setStartClock("<No clock>");
    }
    auto* end_clock = path_end->targetClk(sta);
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
    timing_path->setSlack(path_end->slack(sta));
    timing_path->setPathArrivalTime(path_end->dataArrivalTime(sta));
    timing_path->setPathRequiredTime(path_end->requiredTime(sta));

    bool clock_propagated = false;
    if (start_clock_edge != nullptr) {
      clock_propagated = start_clock_edge->clock()->isPropagated();
    }

    const bool clock_expaneded = clock_propagated;

    timing_path->populatePath(path,
                              sta,
                              dcalc_ap,
                              clock_expaneded);
    if (include_capture) {
      timing_path->populateCapturePath(path_end->targetClkPath(),
                                       sta,
                                       dcalc_ap,
                                       path_end->targetClkOffset(sta),
                                       clock_expaneded);
    }

    timing_path->computeClkEndIndex();
    timing_path->setSlackOnPathNodes();

    paths.push_back(std::unique_ptr<TimingPath>(timing_path));
  }

  delete path_ends;
}

void TimingPath::populateNodeList(sta::Path* path,
                                  sta::dbSta* sta,
                                  sta::DcalcAnalysisPt* dcalc_ap,
                                  float offset,
                                  bool clock_expanded,
                                  TimingNodeList& list)
{
  float arrival_prev_stage = 0;
  float arrival_cur_stage = 0;

  sta::PathExpanded expand(path, sta);
  for (size_t i = 0; i < expand.size(); i++) {
    const auto* ref = expand.path(i);
    const auto pin = ref->vertex(sta)->pin();
    const bool pin_is_clock = sta->isClock(pin);
    const auto slew = ref->slew(sta);
    const bool is_driver = sta->network()->isDriver(pin);
    const auto is_rising = ref->transition(sta) == sta::RiseFall::rise();
    const auto arrival = ref->arrival(sta);

    float cap = 0.0;
    if (is_driver
        && !(!clock_expanded && (sta->network()->isCheckClk(pin) || !i))) {
      sta::Parasitic* parasitic = nullptr;
      sta::ArcDelayCalc* arc_delay_calc = sta->arcDelayCalc();
      if (arc_delay_calc) {
        parasitic = arc_delay_calc->findParasitic(
            pin, ref->transition(sta), dcalc_ap);
      }
      sta::GraphDelayCalc* graph_delay_calc = sta->graphDelayCalc();
      cap = graph_delay_calc->loadCap(
          pin, parasitic, ref->transition(sta), dcalc_ap);
    }

    odb::dbITerm* term;
    odb::dbBTerm* port;
    sta->getDbNetwork()->staToDb(pin, term, port);
    odb::dbObject* pin_object = term;
    if (term == nullptr) {
      pin_object = port;
    }
    arrival_cur_stage = arrival;

    list.push_back(std::make_unique<TimingPathNode>(pin_object,
        pin_is_clock,
        is_rising,
        !is_driver,
        true,
        arrival + offset,
        arrival_cur_stage - arrival_prev_stage,
        slew,
        cap));
      arrival_prev_stage = arrival_cur_stage;
  }

  // populate list with source/sink nodes
  for (int i = 0; i < list.size(); i++) {
    auto* node = list[i].get();
    if (node->isSource()) {
      if (i < (list.size() - 1)) {
        // get the next node
        node->setPairedNode(list[i + 1].get());
      }
    } else {
      // node is sink
      node->setPairedNode(node);
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

void TimingPath::populatePath(sta::Path* path,
                              sta::dbSta* sta,
                              sta::DcalcAnalysisPt* dcalc_ap,
                              bool clock_expanded)
{
  populateNodeList(path, sta, dcalc_ap, 0, clock_expanded, path_nodes_);
}

void TimingPath::populateCapturePath(sta::Path* path,
                                     sta::dbSta* sta,
                                     sta::DcalcAnalysisPt* dcalc_ap,
                                     float offset,
                                     bool clock_expanded)
{
  populateNodeList(path, sta, dcalc_ap, offset, clock_expanded, capture_nodes_);
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

void TimingPath::computeClkEndIndex(TimingNodeList& nodes, int& index)
{
  for (int i = 0; i < nodes.size(); i++) {
    if (!nodes[i]->isClock()) {
      index = i - 1;
      return;
    }
  }

  index = nodes.size() - 1; // assume last index is the end of the clock path
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

std::string TimingPathNode::getNodeName(bool include_master) const
{
  if (isPinITerm()) {
    odb::dbITerm* db_iterm = getPinAsITerm();
    return db_iterm->getInst()->getName() + "/"
           + db_iterm->getMTerm()->getName()
           + (include_master
                  ? " (" + db_iterm->getInst()->getMaster()->getName() + ")"
                  : "");
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
  other->is_clock_ = is_clock_;
  other->is_rising_ = is_rising_;
  other->is_sink_ = is_sink_;
  other->has_values_ = has_values_;
  other->arrival_ = arrival_;
  other->delay_ = delay_;
  other->slew_ = slew_;
  other->load_ = load_;
  other->path_slack_ = path_slack_;
}

const odb::Rect TimingPathNode::getPinBBox() const
{
  if (isPinITerm()) {
    return getPinAsITerm()->getBBox();
  } else {
    return getPinAsBTerm()->getBBox();
  }
}

const odb::Rect TimingPathNode::getPinLargestBox() const
{
  if (isPinITerm()) {
    auto* iterm = getPinAsITerm();
    odb::dbTransform transform;
    iterm->getInst()->getTransform(transform);

    odb::Rect pin_rect;
    auto* mterm = iterm->getMTerm();
    for (auto* pin : mterm->getMPins()) {
      for (auto* box : pin->getGeometry()) {
        odb::Rect box_rect;
        box->getBox(box_rect);
        if (pin_rect.area() < box_rect.area()) {
          transform.apply(box_rect);
          pin_rect = box_rect;
        }
      }
    }

    return pin_rect;
  } else {
    auto* bterm = getPinAsBTerm();

    odb::Rect pin_rect;
    for (auto* pin : bterm->getBPins()) {
      for (auto* box : pin->getBoxes()) {
        odb::Rect box_rect;
        box->getBox(box_rect);
        if (pin_rect.area() < box_rect.area()) {
          pin_rect = box_rect;
        }
      }
    }
    return pin_rect;
  }
}

/////////

TimingPathDetailModel::TimingPathDetailModel(bool is_capture, sta::dbSta* sta, QObject* parent)
  : QAbstractTableModel(parent),
    sta_(sta),
    is_capture_(is_capture),
    expand_clock_(false),
    path_(nullptr),
    nodes_(nullptr)
{
}

int TimingPathDetailModel::rowCount(const QModelIndex& parent) const
{
  if (path_ == nullptr || !hasNodes()) {
    return 0;
  }

  return nodes_->size() + 1;
}

int TimingPathDetailModel::columnCount(const QModelIndex& parent) const
{
  return 6;
}

const TimingPathNode* TimingPathDetailModel::getNodeAt(const QModelIndex& index) const
{
  int node_idx = index.row();

  if (node_idx > clock_summary_row_) {
    // account for collapsed clock entry
    node_idx--;
  }

  return nodes_->at(node_idx).get();
}

QVariant TimingPathDetailModel::data(const QModelIndex& index, int role) const
{
  if (path_ == nullptr ||
      nodes_ == nullptr ||
      !hasNodes()) {
    return QVariant();
  }

  const Column col_index = static_cast<Column>(index.column());
  if (role == Qt::TextAlignmentRole) {
    switch (col_index) {
      case Pin:
        return Qt::AlignLeft;
      case Time:
      case Delay:
      case Slew:
      case Load:
        return Qt::AlignRight;
      case RiseFall:
        return Qt::AlignCenter;
    }
  } else if (role == Qt::DisplayRole) {
    const auto time_units = sta_->search()->units()->timeUnit();

    if (index.row() == clock_summary_row_) {
      int start_idx = getClockEndIndex();
      if (start_idx < 0) {
        start_idx = 0;
      }
      const auto& node = nodes_->at(start_idx);

      switch (col_index) {
      case Pin:
        return "clock network delay";
      case Time:
        return time_units->asString(node->getArrival());
      case Delay:
        return time_units->asString(node->getArrival() - nodes_->at(0)->getArrival());
      default:
        return QVariant();
      }
    } else {
      const auto* node = getNodeAt(index);
      switch (col_index) {
        case Pin:
          return QString::fromStdString(node->getNodeName(/* include_master */ true));
        case RiseFall:
          return node->isRisingEdge() ? up_arrow_ : down_arrow_;
        case Time:
          return time_units->asString(node->getArrival());
        case Delay:
          return time_units->asString(node->getDelay());
        case Slew:
          return time_units->asString(node->getSlew());
        case Load: {
          if (node->getLoad() == 0)
            return "";
          const auto cap_units = sta_->search()->units()->capacitanceUnit();
          return cap_units->asString(node->getLoad());
        }
      }
    }
  }
  return QVariant();
}

bool TimingPathDetailModel::shouldHide(const QModelIndex& index) const
{
  const int row = index.row();
  const int last_clock = getClockEndIndex() + 1; // accounting for clock_summary would +1

  if (row == 0) {
    return false;
  }

  if (row == clock_summary_row_) {
    return expand_clock_;
  }

  if (row >= last_clock) {
    return false;
  } else {
    return !expand_clock_;
  }

  return false;
}

Qt::ItemFlags TimingPathDetailModel::flags(const QModelIndex& index) const
{
  auto flags = QAbstractTableModel::flags(index);
  flags.setFlag(Qt::ItemIsEnabled, !shouldHide(index));

  return flags;
}

QVariant TimingPathDetailModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    switch (static_cast<Column>(section)) {
    case Pin:
      return "Pin";
    case RiseFall:
      return up_down_arrows_;
    case Time:
      return "Time";
    case Delay:
      return "Delay";
    case Slew:
      return "Slew";
    case Load:
      return "Load";
    }
  }
  return QVariant();
}

void TimingPathDetailModel::populateModel(TimingPath* path, TimingPath::TimingNodeList* nodes)
{
  beginResetModel();
  path_ = path;
  nodes_ = nodes;
  endResetModel();
}

/////////

TimingPathRenderer::TimingPathRenderer() :
    path_(nullptr),
    highlight_stage_()
{
}

void TimingPathRenderer::highlight(TimingPath* path)
{
  path_ = path;
  highlight_stage_.clear();
  redraw();
}

void TimingPathRenderer::highlightNode(const TimingPathNode* node)
{
  if (node != nullptr) {
    odb::dbNet* net = node->getNet();
    odb::dbObject* sink = nullptr;
    odb::dbInst* inst = nullptr;

    auto* instance_node = node->getInstanceNode();
    if (instance_node != nullptr) {
      inst = instance_node->getInstance();
    }

    auto* sink_node = node->getPairedNode();
    if (sink_node != nullptr) {
      sink = sink_node->getPin();
    }

    if (net != nullptr || inst != nullptr) {
      highlight_stage_.push_back(std::make_unique<HighlightStage>(HighlightStage{net, inst, sink}));
    }

    redraw();
  }
}

void TimingPathRenderer::drawNodesList(TimingPath::TimingNodeList* nodes,
                                       gui::Painter& painter,
                                       const gui::Descriptor* net_descriptor,
                                       const gui::Descriptor* inst_descriptor,
                                       const gui::Descriptor* bterm_descriptor,
                                       const Painter::Color& clock_color)
{
  for (auto node_itr = nodes->rbegin(); node_itr != nodes->rend(); node_itr++) {
    auto& node = *node_itr;

    odb::dbInst* db_inst = node->getInstance();
    if (db_inst != nullptr) {
      painter.setPenAndBrush(TimingPathRenderer::path_inst_color_, true);
      inst_descriptor->highlight(db_inst, painter);
    }

    if (node->isPinBTerm()) {
      painter.setPenAndBrush(TimingPathRenderer::term_color_, true);
      bterm_descriptor->highlight(node->getPinAsBTerm(), painter);
    }

    if (node->isSource()) {
      auto* sink_node = node->getPairedNode();
      if (sink_node != nullptr) {
        gui::Painter::Color wire_color = node->isClock() ? clock_color : TimingPathRenderer::signal_color_;
        painter.setPenAndBrush(wire_color, true);
        net_descriptor->highlight(node->getNet(), painter, sink_node->getPin());
      }
    }
  }
}

void TimingPathRenderer::drawObjects(gui::Painter& painter)
{
  if (path_ == nullptr) {
    return;
  }

  auto* net_descriptor = Gui::get()->getDescriptor<odb::dbNet*>();
  auto* inst_descriptor = Gui::get()->getDescriptor<odb::dbInst*>();
  auto* bterm_descriptor = Gui::get()->getDescriptor<odb::dbBTerm*>();

  drawNodesList(path_->getCaptureNodes(),
                painter,
                net_descriptor,
                inst_descriptor,
                bterm_descriptor,
                capture_clock_color_);
  drawNodesList(path_->getPathNodes(),
                painter,
                net_descriptor,
                inst_descriptor,
                bterm_descriptor,
                clock_color_);

  highlightStage(painter, net_descriptor, inst_descriptor);
}

void TimingPathRenderer::highlightStage(gui::Painter& painter,
                                        const gui::Descriptor* net_descriptor,
                                        const gui::Descriptor* inst_descriptor)
{
  if (highlight_stage_.empty()) {
    return;
  }

  painter.setPenAndBrush(TimingPathRenderer::inst_highlight_color_, true);
  for (const auto& highlight : highlight_stage_) {
    if (highlight->inst != nullptr) {
      inst_descriptor->highlight(highlight->inst, painter);
    }
  }

  for (const auto& highlight : highlight_stage_) {
    if (highlight->net != nullptr) {
      net_descriptor->highlight(highlight->net, painter, highlight->sink);
    }
  }
}

/////////

TimingConeRenderer::TimingConeRenderer() :
    sta_(nullptr),
    term_(nullptr),
    fanin_(false),
    fanout_(false),
    map_(),
    min_map_index_(0),
    max_map_index_(0),
    min_timing_(0.0),
    max_timing_(0.0),
    color_generator_(SpectrumGenerator(1.0))
{
}

void TimingConeRenderer::setITerm(odb::dbITerm* term, bool fanin, bool fanout)
{
  if (sta_ == nullptr) {
    return;
  }

  auto* network = sta_->getDbNetwork();
  setPin(network->dbToSta(term), fanin, fanout);
}

void TimingConeRenderer::setBTerm(odb::dbBTerm* term, bool fanin, bool fanout)
{
  if (sta_ == nullptr) {
    return;
  }

  auto* network = sta_->getDbNetwork();
  setPin(network->dbToSta(term), fanin, fanout);
}

void TimingConeRenderer::setPin(sta::Pin* pin, bool fanin, bool fanout)
{
  if (sta_ == nullptr) {
    return;
  }

  if (pin != term_) {
    term_ = pin;
    fanin_ = fanin;
    fanout_ = fanout;
  } else {
    // toggle options
    if (fanin) {
      fanin_ = !fanin_;
    }
    if (fanout) {
      fanout_ = !fanout_;
    }
  }

  if (pin == nullptr || (!fanin_ && !fanout_)) {
    Gui::get()->unregisterRenderer(this);
    return;
  } else {
    Gui::get()->registerRenderer(this);
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  sta_->ensureGraph();

  map_.clear();

  DepthMapSet depth_map;
  if (fanin_) {
    getFaninCone(pin, depth_map);
  }
  if (fanout_) {
    getFanoutCone(pin, depth_map);
  }

  for (const auto& [level, pins] : depth_map) {
    auto& map_level = map_[level];
    for (auto* pin : pins) {
      map_level.push_back(std::make_unique<TimingPathNode>(pin));
    }
  }

  min_map_index_ = std::numeric_limits<int>::max();
  max_map_index_ = std::numeric_limits<int>::min();
  for (const auto& [level, pins_list] : map_) {
    min_map_index_ = std::min(min_map_index_, level);
    max_map_index_ = std::max(max_map_index_, level);
  }

  buildConnectivity();
  annotateTiming(pin);

  QApplication::restoreOverrideCursor();

  redraw();
}

void TimingConeRenderer::drawObjects(gui::Painter& painter)
{
  if (map_.empty()) {
    return;
  }

  // draw timing connections
  const double timing_range = max_timing_ - min_timing_;

  auto timingToRatio = [this, timing_range](const TimingPathNode* node) {
    if (timing_range == 0.0) {
      return 1.0;
    }
    double value = 0.0;
    if (node->hasValues()) {
      value = 1.0 - (node->getPathSlack() - min_timing_) / timing_range;
    }
    return value;
  };

  // draw instances
  std::map<odb::dbInst*, TimingPathNode*> instances;
  for (const auto& [level, pins] : map_) {
    for (const auto& pin : pins) {
      if (pin->isPinITerm()) {
        odb::dbInst* inst = pin->getPinAsITerm()->getInst();

        if (inst != nullptr) {
          if (instances.count(inst) == 0) {
            instances[inst] = pin.get();
          } else {
            auto& worst_pin = instances[inst];
            if (!worst_pin->hasValues()) {
              worst_pin = pin.get();
            } else if (pin->hasValues()) {
              if (worst_pin->getPathSlack() > pin->getPathSlack()) {
                worst_pin = pin.get();
              }
            }
          }
        }
      }
    }
  }
  auto* inst_descriptor = Gui::get()->getDescriptor<odb::dbInst*>();
  for (const auto& [inst, slack_pin] : instances) {
    const auto color = color_generator_.getColor(timingToRatio(slack_pin), 150);
    painter.setPenAndBrush(color, true);
    inst_descriptor->highlight(inst, painter);
  }

  const int line_width = 2; // 2 pixels
  auto* iterm_descriptor = Gui::get()->getDescriptor<odb::dbITerm*>();
  auto* bterm_descriptor = Gui::get()->getDescriptor<odb::dbBTerm*>();
  for (const auto& [level, pins] : map_) {
    for (const auto& pin : pins) {
      const auto color = color_generator_.getColor(timingToRatio(pin.get()), 255);
      painter.setPenAndBrush(color, true);

      if (pin->isPinITerm()) {
        iterm_descriptor->highlight(pin->getPinAsITerm(), painter);
      } else {
        bterm_descriptor->highlight(pin->getPinAsBTerm(), painter);
      }

      auto* source_node = pin->getPairedNode();
      if (source_node == nullptr) {
        continue;
      }

      const odb::Rect source_rect = source_node->getPinLargestBox();
      const odb::Rect sink_rect = pin->getPinLargestBox();

      const odb::Point source_pt(0.5 * (source_rect.xMin() + source_rect.xMax()),
                                 0.5 * (source_rect.yMin() + source_rect.yMax()));
      const odb::Point sink_pt(0.5 * (sink_rect.xMin() + sink_rect.xMax()),
                               0.5 * (sink_rect.yMin() + sink_rect.yMax()));

      painter.setPen(color, true, line_width);
      painter.drawLine(source_pt.x(), source_pt.y(), sink_pt.x(), sink_pt.y());
    }
  }

  // annotate with depth
  const auto text_anchor = gui::Painter::Anchor::CENTER;
  const double text_margin = 2.0;
  painter.setPen(gui::Painter::white, true);
  for (const auto& [level, pins] : map_) {
    for (const auto& pin : pins) {
      const odb::Rect pin_rect = pin->getPinLargestBox();
      const odb::Point pin_pt(0.5 * (pin_rect.xMin() + pin_rect.xMax()),
                              0.5 * (pin_rect.yMin() + pin_rect.yMax()));

      const std::string text = std::to_string(level);
      const odb::Rect text_bound = painter.stringBoundaries(pin_pt.x(), pin_pt.y(), text_anchor, text);

      if (text_bound.dx() < text_margin * pin_rect.dx() && text_bound.dy() < text_margin * pin_rect.dy()) {
        painter.drawString(pin_pt.x(), pin_pt.y(), text_anchor, text);
      }
    }
  }

  // draw legend
  const int legend_keys = 5;
  const int color_count = color_generator_.getColorCount();
  auto* units = sta_->units()->timeUnit();
  const std::string text_units = std::string(units->scaleAbreviation()) + units->suffix();
  std::vector<std::pair<int, std::string>> legend;
  for (int i = 0; i < legend_keys; i++) {
    const double scale = static_cast<double>(i) / (legend_keys - 1);
    const int color_index = color_count * scale;
    const double slack = max_timing_ - timing_range * scale;
    const std::string text = units->asString(slack) + text_units;
    legend.push_back({color_index, text});
  }
  std::reverse(legend.begin(), legend.end());
  color_generator_.drawLegend(painter, legend);
}

void TimingConeRenderer::getFaninCone(sta::Pin* source_pin, DepthMapSet& depth_map)
{
  sta::PinSeq pins_to;
  pins_to.push_back(source_pin);

  auto* pins = sta_->findFaninPins(&pins_to,
                                   true, // flat
                                   false, // startpoints_only
                                   0,
                                   0,
                                   true, // thru_disabled
                                   true); // thru_constants

  sta::SearchPred2 srch_pred(sta_);
  sta::BfsBkwdIterator bfs(sta::BfsIndex::other, &srch_pred, sta_);

  DepthMapSet depth_map_set;
  getCone(source_pin, pins, bfs, depth_map_set);
  for (auto& [level, pin_list] : depth_map_set) {
    depth_map[-level].insert(pin_list.begin(), pin_list.end());
  }
}

void TimingConeRenderer::getFanoutCone(sta::Pin* source_pin, DepthMapSet& depth_map)
{
  sta::PinSeq pins_from;
  pins_from.push_back(source_pin);

  auto* pins = sta_->findFanoutPins(&pins_from,
                                    true, // flat
                                    false, // startpoints_only
                                    0,
                                    0,
                                    true, // thru_disabled
                                    true); // thru_constants
  sta::SearchPred2 srch_pred(sta_);

  sta::BfsFwdIterator bfs(sta::BfsIndex::other, &srch_pred, sta_);
  getCone(source_pin, pins, bfs, depth_map);
}

void TimingConeRenderer::getCone(sta::Pin* source_pin, sta::PinSet* pins, sta::BfsIterator& bfs, DepthMapSet& depth_map)
{
  auto* network = sta_->getDbNetwork();
  auto* graph = sta_->graph();

  auto filter_pins = [network](sta::Pin* pin) {
    return network->isRegClkPin(pin);
  };

  for (auto* pin : *pins) {
    if (filter_pins(pin)) {
      continue;
    }
    sta::Vertex* vertex = graph->pinDrvrVertex(pin);
    bfs.enqueueAdjacentVertices(vertex);
  }
  bfs.enqueueAdjacentVertices(graph->pinDrvrVertex(source_pin));
  bfs.enqueue(graph->pinDrvrVertex(source_pin));
  delete pins;

  int level = 0;
  int last_bfs_level = -1;
  while (bfs.hasNext()) {
    auto* vertex = bfs.next();
    if (last_bfs_level == -1) {
      last_bfs_level = vertex->level();
    }

    if (last_bfs_level != vertex->level()) {
      last_bfs_level = vertex->level();
      level++;
    }

    auto& pin_list = depth_map[level];

    odb::dbITerm* iterm = nullptr;
    odb::dbBTerm* bterm = nullptr;

    auto* sta_pin = vertex->pin();
    if (filter_pins(sta_pin)) {
      continue;
    }

    network->staToDb(sta_pin, iterm, bterm);
    odb::dbObject* pin_term = nullptr;
    if (iterm != nullptr) {
      pin_term = iterm;
    } else {
      pin_term = bterm;
    }
    pin_list.insert(pin_term);
  }
}

void TimingConeRenderer::buildConnectivity()
{
  auto* network = sta_->getDbNetwork();
  auto* graph = sta_->graph();

  // clear map pairs
  for (const auto& [level, pin_list] : map_) {
    for (const auto& pin : pin_list) {
      pin->setPairedNode(nullptr);
    }
  }

  for (const auto& [level, pin_list] : map_) {
    int next_level = level + 1;
    if (map_.count(next_level) == 0) {
      break;
    }

    std::map<sta::Vertex*, TimingPathNode*> next_pins;
    for (const auto& pin : map_[next_level]) {
      if (!pin->isSource()) {
        continue;
      }
      sta::Pin* sta_pin = nullptr;
      if (pin->isPinITerm()) {
        sta_pin = network->dbToSta(pin->getPinAsITerm());
      } else {
        sta_pin = network->dbToSta(pin->getPinAsBTerm());
      }
      next_pins[graph->pinDrvrVertex(sta_pin)] = pin.get();
      next_pins[graph->pinLoadVertex(sta_pin)] = pin.get();
    }

    for (const auto& source_pin : pin_list) {
      if (!source_pin->isSource()) {
        continue;
      }

      sta::Pin* sta_pin = nullptr;
      if (source_pin->isPinITerm()) {
        sta_pin = network->dbToSta(source_pin->getPinAsITerm());
      } else {
        sta_pin = network->dbToSta(source_pin->getPinAsBTerm());
      }
      sta::VertexOutEdgeIterator fanout_iter(graph->pinDrvrVertex(sta_pin), graph);
      while (fanout_iter.hasNext()) {
        sta::Edge* edge = fanout_iter.next();
        sta::Vertex* fanout = edge->to(graph);
        auto next_pins_find = next_pins.find(fanout);
        if (next_pins_find != next_pins.end()) {
          next_pins_find->second->setPairedNode(source_pin.get());
        }
      }
    }
  }
}

void TimingConeRenderer::annotateTiming(sta::Pin* source_pin)
{
  // annotate critical path and work forwards/backwards until all pins have timing
  std::vector<TimingPathNode*> pin_order;
  for (int i = 0; i <= std::max(-min_map_index_, max_map_index_); i++) {
    if (i <= -min_map_index_) {
      for (const auto& pin : map_[-i]) {
        pin_order.push_back(pin.get());
      }
    }
    if (i <= max_map_index_) {
      for (const auto& pin : map_[i]) {
        pin_order.push_back(pin.get());
      }
    }
  }

  // populate with max number of unique paths, all other paths will appear as 0
  const int path_count = 1000;

  std::vector<std::unique_ptr<TimingPath>> paths;
  TimingPath::buildPaths(sta_,
      true, // max
      true, // unconstrained
      path_count, // paths
      {}, // from
      {source_pin}, // thru
      {}, // to
      false,
      paths);
  for (const auto& path : paths) {
    for (const auto& node : *path->getPathNodes()) {
      auto pin_find = std::find_if(pin_order.begin(), pin_order.end(), [&node](const TimingPathNode* other) {
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

  min_timing_ = std::numeric_limits<float>::max();
  max_timing_ = std::numeric_limits<float>::min();
  for (const auto& [level, pin_list] : map_) {
    for (const auto& pin : pin_list) {
      if (sta::delayInf(pin->getPathSlack()) || !pin->hasValues()) {
        continue;
      }

      min_timing_ = std::min(min_timing_, pin->getPathSlack());
      max_timing_ = std::max(max_timing_, pin->getPathSlack());
    }
  }

  for (auto& [level, pin_list] : map_) {
    std::sort(pin_list.begin(), pin_list.end(), [](const auto& l, const auto& r) {
      return l->getPathSlack() > r->getPathSlack();
    });
  }
}

}  // namespace gui
