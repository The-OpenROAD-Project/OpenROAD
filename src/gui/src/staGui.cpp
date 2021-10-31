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
#include <string>

#include "db.h"
#include "dbShape.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/ArcDelayCalc.hh"
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
#include "staGui.h"

namespace gui {

static gui::Painter::Color makeColor(const gui::Painter::Color& color)
{
  return gui::Painter::Color(color.r, color.g, color.b, 100);
}

const gui::Painter::Color TimingPathRenderer::inst_highlight_color_
    = makeColor(gui::Painter::highlight);
const gui::Painter::Color TimingPathRenderer::path_inst_color_
    = makeColor(gui::Painter::magenta);
const gui::Painter::Color TimingPathRenderer::term_color_ = makeColor(gui::Painter::blue);
const gui::Painter::Color TimingPathRenderer::signal_color_ = makeColor(gui::Painter::red);
const gui::Painter::Color TimingPathRenderer::clock_color_ = makeColor(gui::Painter::cyan);
const gui::Painter::Color TimingPathRenderer::capture_clock_color_ = makeColor(gui::Painter::green);

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
                                     int path_count,
                                     bool clockExpanded)
{
  // On lines of DataBaseHandler
  QApplication::setOverrideCursor(Qt::WaitCursor);
  sta_->ensureGraph();
  sta_->searchPreamble();

  auto sta_state = sta_->search();

  sta::PathEndSeq* path_ends
      = sta_state->findPathEnds(  // from, thrus, to, unconstrained
          nullptr,
          nullptr,
          nullptr,
          false,
          // corner, min_max,
          sta_->findCorner("default"),
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

  bool first_path = true;
  for (auto& path_end : *path_ends) {
    TimingPath* path = new TimingPath();
    sta::DcalcAnalysisPt* dcalc_ap
        = path_end->path()->pathAnalysisPt(sta_)->dcalcAnalysisPt();

    auto* start_clock_edge = path_end->sourceClkEdge(sta_);
    if (start_clock_edge != nullptr) {
      path->setStartClock(start_clock_edge->clock()->name());
    } else {
      path->setStartClock("<No clock>");
    }
    auto* end_clock = path_end->targetClk(sta_);
    if (end_clock != nullptr) {
      path->setEndClock(end_clock->name());
    } else {
      path->setEndClock("<No clock>");
    }
    path->setPathDelay(path_end->pathDelay() ? path_end->pathDelay()->delay()
                                             : 0);
    path->setSlack(path_end->slack(sta_));
    path->setPathArrivalTime(path_end->dataArrivalTime(sta_));
    path->setPathRequiredTime(path_end->requiredTime(sta_));
    bool clockPropagated = false;
    if (start_clock_edge != nullptr) {
      clockPropagated = start_clock_edge->clock()->isPropagated();
    }
    if (!clockPropagated)
      clockExpanded = false;
    else
      clockExpanded = true;

    path->populatePath(path_end->path(), sta_, dcalc_ap, clockExpanded, first_path);
    path->populateCapturePath(path_end->targetClkPath(), sta_, dcalc_ap, path_end->targetClkOffset(sta_), clockExpanded, first_path);

    first_path = false;

    path->computeClkEndIndex();

    timing_paths_.push_back(std::unique_ptr<TimingPath>(path));
  }
  QApplication::restoreOverrideCursor();
  delete path_ends;
  return true;
}

/////////

void TimingPath::populateNodeList(sta::Path* path, sta::dbSta* sta, sta::DcalcAnalysisPt* dcalc_ap, float offset, bool clock_expanded, bool first_path, TimingNodeList& list)
{
  float arrival_prev_stage = 0;
  float arrival_cur_stage = 0;

  sta::PathExpanded expand(path, sta);
  for (size_t i = 0; i < expand.size(); i++) {
    auto ref = expand.path(i);
    auto pin = ref->vertex(sta)->pin();
    auto slew = ref->slew(sta);
    float cap = 0.0;
    const bool is_driver = sta->network()->isDriver(pin);
    if (is_driver
        && !(!clock_expanded && (sta->network()->isCheckClk(pin) || !i))) {
      sta::Parasitic* parasitic = nullptr;
      sta::ArcDelayCalc* arc_delay_calc = sta->arcDelayCalc();
      if (arc_delay_calc)
        parasitic = arc_delay_calc->findParasitic(
            pin, ref->transition(sta), dcalc_ap);
      sta::GraphDelayCalc* graph_delay_calc = sta->graphDelayCalc();
      cap = graph_delay_calc->loadCap(
          pin, parasitic, ref->transition(sta), dcalc_ap);
    }

    auto is_rising = ref->transition(sta) == sta::RiseFall::rise();
    auto arrival = ref->arrival(sta);
    auto path_ap = ref->pathAnalysisPt(sta);
    auto path_required = !first_path ? 0 : ref->required(sta);
    if (!path_required || sta::delayInf(path_required)) {
      auto* vert = sta->getDbNetwork()->graph()->pinLoadVertex(pin);
      auto req = sta->vertexRequired(
          vert, is_rising ? sta::RiseFall::rise() : sta::RiseFall::fall(), path_ap);
      if (sta::delayInf(req)) {
        path_required = 0;
      }
      path_required = req;
    }
    auto slack = !first_path ? path_required - arrival : ref->slack(sta);
    odb::dbITerm* term;
    odb::dbBTerm* port;
    sta->getDbNetwork()->staToDb(pin, term, port);
    odb::dbObject* pin_object = term;
    if (term == nullptr)
      pin_object = port;
    arrival_cur_stage = arrival;

    bool pin_is_clock = sta->isClock(pin);

    if (i == 0) {
      list.push_back(std::make_unique<TimingPathNode>(pin_object,
          pin_is_clock,
          is_rising,
          !is_driver,
          arrival + offset,
          path_required + offset,
          0,
          slack,
          slew,
          cap));
    } else {
      list.push_back(std::make_unique<TimingPathNode>(pin_object,
          pin_is_clock,
          is_rising,
          !is_driver,
          arrival + offset,
          path_required + offset,
          arrival_cur_stage - arrival_prev_stage,
          slack,
          slew,
          cap));
      arrival_prev_stage = arrival_cur_stage;
    }
    first_path = false;
  }

  // populate list with source/sink nodes
  for (int i = 0; i < list.size(); i++) {
    auto* node = list[i].get();
    if (node->isSource()) {
      if (i < (list.size() - 1)) {
        // get the next node
        node->setSinkNode(list[i + 1].get());
      }
    } else {
      // node is sink
      node->setSinkNode(node);
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

void TimingPath::populatePath(sta::Path* path, sta::dbSta* sta, sta::DcalcAnalysisPt* dcalc_ap, bool clock_expanded, bool first_path)
{
  populateNodeList(path, sta, dcalc_ap, 0, clock_expanded, first_path, path_nodes_);
}

void TimingPath::populateCapturePath(sta::Path* path, sta::dbSta* sta, sta::DcalcAnalysisPt* dcalc_ap, float offset, bool clock_expanded, bool first_path)
{
  populateNodeList(path, sta, dcalc_ap, offset, clock_expanded, first_path, capture_nodes_);
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

/////////

TimingPathDetailModel::TimingPathDetailModel(bool is_capture, sta::dbSta* sta, QObject* parent)
  : QAbstractTableModel(parent),
    sta_(sta),
    is_capture_(is_capture),
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

bool TimingPathDetailModel::shouldHide(const QModelIndex& index,
                                       bool expand_clock) const
{
  const int row = index.row();
  const int last_clock = getClockEndIndex() + 1; // accounting for clock_summary would +1

  if (row == 0) {
    return false;
  }

  if (row == clock_summary_row_) {
    return expand_clock;
  }

  if (row >= last_clock) {
    return false;
  } else {
    return !expand_clock;
  }

  return false;
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

void TimingPathRenderer::highlightNode(const TimingPathNode* node, TimingPath::TimingNodeList* nodes)
{
  if (path_ != nullptr) {
    odb::dbNet* net = node->getNet();
    odb::dbObject* sink = nullptr;
    odb::dbInst* inst = nullptr;

    auto* instance_node = node->getInstanceNode();
    if (instance_node != nullptr) {
      inst = instance_node->getInstance();
    }

    auto* sink_node = node->getSinkNode();
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
                                       const Painter::Color& clock_color)
{
  int node_count = nodes->size();
  for (int i = node_count - 1; i >= 0; --i) {
    auto& node = nodes->at(i);

    odb::dbInst* db_inst = node->getInstance();
    if (db_inst != nullptr) {
      highlightInst(db_inst, painter, TimingPathRenderer::path_inst_color_);
    }

    if (node->isPinBTerm()) {
      highlightTerm(node->getPinAsBTerm(), painter);
    }

    if (node->isSource()) {
      auto* sink_node = node->getSinkNode();
      if (sink_node != nullptr) {
        highlightNet(node->getNet(),
                     node->isClock(),
                     node->getPin() /*source*/,
                     sink_node->getPin(),
                     painter,
                     net_descriptor,
                     clock_color);
      }
    }
  }
}

void TimingPathRenderer::drawObjects(gui::Painter& painter)
{
  if (path_ == nullptr)
    return;

  auto* net_descriptor = Gui::get()->getDescriptor<odb::dbNet*>();

  drawNodesList(path_->getCaptureNodes(), painter, net_descriptor, capture_clock_color_);
  drawNodesList(path_->getPathNodes(), painter, net_descriptor, clock_color_);

  highlightStage(painter, net_descriptor);
}

void TimingPathRenderer::highlightStage(gui::Painter& painter, const gui::Descriptor* net_descriptor)
{
  if (highlight_stage_.empty()) {
    return;
  }

  for (const auto& highlight : highlight_stage_) {
    if (highlight->inst != nullptr) {
      highlightInst(highlight->inst, painter, TimingPathRenderer::inst_highlight_color_);
    }
  }

  painter.setPen(TimingPathRenderer::inst_highlight_color_, true);
  painter.setBrush(TimingPathRenderer::inst_highlight_color_);
  for (const auto& highlight : highlight_stage_) {
    if (highlight->net != nullptr) {
      net_descriptor->highlight(highlight->net, painter, highlight->sink);
    }
  }
}

// Color in the instances to make them more visible.
void TimingPathRenderer::highlightInst(odb::dbInst* db_inst,
                                       gui::Painter& painter,
                                       const gui::Painter::Color& inst_color)
{
  if (path_ == nullptr) {
    return;
  }
  if (!db_inst->isPlaced()) {
    return;
  }

  odb::dbBox* bbox = db_inst->getBBox();
  odb::Rect rect;
  bbox->getBox(rect);
  painter.setBrush(inst_color);
  painter.drawRect(rect);
}

void TimingPathRenderer::highlightTerm(odb::dbBTerm* term,
                                       gui::Painter& painter)
{
  if (path_ == nullptr)
    return;
  odb::dbShape port_shape;
  if (term->getFirstPin(port_shape)) {
    odb::Rect rect;
    port_shape.getBox(rect);
    painter.setBrush(TimingPathRenderer::term_color_);
    painter.drawRect(rect);
  }
}

void TimingPathRenderer::highlightNet(odb::dbNet* net,
                                      bool is_clock,
                                      odb::dbObject* source_node,
                                      odb::dbObject* sink_node,
                                      gui::Painter& painter,
                                      const gui::Descriptor* net_descriptor,
                                      const Painter::Color& clock_color)
{
  if (path_ == nullptr) {
    return;
  }

  gui::Painter::Color wire_color = is_clock ? clock_color : TimingPathRenderer::signal_color_;

  painter.setPen(wire_color, true);
  painter.setBrush(wire_color);
  net_descriptor->highlight(net, painter, sink_node);
}

}  // namespace gui
