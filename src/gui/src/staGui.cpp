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

const char* TimingPathDetailModel::up_down_arrows = u8"\u21C5";
const char* TimingPathDetailModel::up_arrow = u8"\u2191";
const char* TimingPathDetailModel::down_arrow = u8"\u2193";

// These two definitions need to stay in sync
const std::vector<std::string> TimingPathsModel::_path_columns
    = {"Capture Clock", "Required", "Arrival", "Slack", "Start", "End"};
enum TimingPathsModel::Column : int
{
  Clock,
  Required,
  Arrival,
  Slack,
  Start,
  End
};

// These two definitions need to stay in sync
const std::vector<std::string> TimingPathDetailModel::_path_details_columns
    = {"Pin", up_down_arrows, "Time", "Delay", "Slew", "Load"};
enum TimingPathDetailModel::Column : int
{
  Pin,
  RiseFall,
  Time,
  Delay,
  Slew,
  Load
};

gui::Painter::Color TimingPathRenderer::inst_highlight_color_
    = gui::Painter::highlight;
gui::Painter::Color TimingPathRenderer::path_inst_color_
    = gui::Painter::magenta;
gui::Painter::Color TimingPathRenderer::term_color_ = gui::Painter::blue;
gui::Painter::Color TimingPathRenderer::signal_color_ = gui::Painter::red;
gui::Painter::Color TimingPathRenderer::clock_color_ = gui::Painter::cyan;

/////////

TimingPathsModel::TimingPathsModel(sta::dbSta* sta, bool get_max, int path_count)
    : sta_(sta)
{
}

TimingPathsModel::~TimingPathsModel()
{
  // TBD
}

int TimingPathsModel::rowCount(const QModelIndex& parent) const
{
  return timing_paths_.size();
}

int TimingPathsModel::columnCount(const QModelIndex& parent) const
{
  return TimingPathsModel::_path_columns.size();
}

QVariant TimingPathsModel::data(const QModelIndex& index, int role) const
{
  const Column col_index = static_cast<Column>(index.column());
  if (index.isValid() && role == Qt::TextAlignmentRole) {
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
  }

  if (!index.isValid() || role != Qt::DisplayRole) {
    return QVariant();
  }

  auto time_units = sta_->search()->units()->timeUnit();

  unsigned int row_index = index.row();
  if (row_index > timing_paths_.size())
    return QVariant();
  auto* timing_path = getPathAt(row_index);
  switch (col_index) {
    case Clock:
      return QString::fromStdString(timing_path->getStartClock());
    case Required:
      return QString(time_units->asString(timing_path->getPathRequiredTime()));
    case Arrival:
      return QString(time_units->asString(timing_path->getPathArrivalTime()));
    case Slack:
      return QString(time_units->asString(timing_path->getSlack()));
    case Start:
      return QString(timing_path->getStartStageName().c_str());
    case End:
      return QString::fromStdString(timing_path->getEndStageName());
  }
  return QVariant();
}

QVariant TimingPathsModel::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    return QString::fromStdString(TimingPathsModel::_path_columns[section]);
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
  // columns {"Capture Clock", "Required", "Arrival", "Slack", "Start", "End"};

  std::function<bool(const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2)> sort_func;
  if (col_index == 0) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getStartClock() < path2->getStartClock();
    };
  } else if (col_index == 1) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getPathRequiredTime() < path2->getPathRequiredTime();
    };
  } else if (col_index == 2) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getPathArrivalTime() < path2->getPathArrivalTime();
    };
  } else if (col_index == 3) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getSlack() < path2->getSlack();
    };
  } else if (col_index == 4) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1, const std::unique_ptr<TimingPath>& path2) {
      return path1->getStartStageName() < path2->getStartStageName();
    };
  } else if (col_index == 5) {
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
    float arrival_prev_stage = 0;
    float arrival_cur_stage = 0;
    sta::PathExpanded expanded(path_end->path(), sta_);
    for (size_t i = 0; i < expanded.size(); i++) {
      auto ref = expanded.path(i);
      auto pin = ref->vertex(sta_)->pin();
      auto slew = ref->slew(sta_);
      float cap = 0.0;
      if (sta_->network()->isDriver(pin)
          && !(!clockExpanded && (sta_->network()->isCheckClk(pin) || !i))) {
        sta::Parasitic* parasitic = nullptr;
        sta::ArcDelayCalc* arc_delay_calc = sta_->arcDelayCalc();
        if (arc_delay_calc)
          parasitic = arc_delay_calc->findParasitic(
              pin, ref->transition(sta_), dcalc_ap);
        sta::GraphDelayCalc* graph_delay_calc = sta_->graphDelayCalc();
        cap = graph_delay_calc->loadCap(
            pin, parasitic, ref->transition(sta_), dcalc_ap);
      }

      auto is_rising = ref->transition(sta_) == sta::RiseFall::rise();
      auto arrival = ref->arrival(sta_);
      auto path_ap = ref->pathAnalysisPt(sta_);
      auto path_required = !first_path ? 0 : ref->required(sta_);
      if (!path_required || sta::delayInf(path_required)) {
        auto* vert = sta_->getDbNetwork()->graph()->pinLoadVertex(pin);
        auto req = sta_->vertexRequired(
            vert, is_rising ? sta::RiseFall::rise() : sta::RiseFall::fall(), path_ap);
        if (sta::delayInf(req)) {
          path_required = 0;
        }
        path_required = req;
      }
      auto slack = !first_path ? path_required - arrival : ref->slack(sta_);
      odb::dbITerm* term;
      odb::dbBTerm* port;
      sta_->getDbNetwork()->staToDb(pin, term, port);
      odb::dbObject* pin_object = term;
      if (term == nullptr)
        pin_object = port;
      arrival_cur_stage = arrival;

      if (ref == expanded.startPath()) {
        path->setPathStartIndex(path->getNodeCount());
      }

      if (i == 0)
        path->appendNode(new TimingPathNode(pin_object,
                                            is_rising,
                                            arrival,
                                            path_required,
                                            0,
                                            slack,
                                            slew,
                                            cap));
      else {
        path->appendNode(new TimingPathNode(pin_object,
                                            is_rising,
                                            arrival,
                                            path_required,
                                            arrival_cur_stage - arrival_prev_stage,
                                            slack,
                                            slew,
                                            cap));
        arrival_prev_stage = arrival_cur_stage;
      }
      first_path = false;
    }

    path->populateCapturePath(path_end->targetClkPath(), sta_, dcalc_ap, clockPropagated, first_path);

    timing_paths_.push_back(std::unique_ptr<TimingPath>(path));
  }
  QApplication::restoreOverrideCursor();
  delete path_ends;
  return true;
}

/////////

void TimingPath::populateCapturePath(sta::Path* path, sta::dbSta* sta, sta::DcalcAnalysisPt* dcalc_ap, bool clock_expanded, bool first_path)
{
  float arrival_prev_stage = 0;
  float arrival_cur_stage = 0;

  sta::PathExpanded expand(path, sta);
  for (size_t i = 0; i < expand.size(); i++) {
    auto ref = expand.path(i);
    auto pin = ref->vertex(sta)->pin();
    auto slew = ref->slew(sta);
    float cap = 0.0;
    if (sta->network()->isDriver(pin)
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

    if (i == 0) {
      capture_nodes_.push_back(std::make_unique<TimingPathNode>(pin_object,
          is_rising,
          arrival,
          path_required,
          0,
          slack,
          slew,
          cap));
    } else {
      capture_nodes_.push_back(std::make_unique<TimingPathNode>(pin_object,
          is_rising,
          arrival,
          path_required,
          arrival_cur_stage - arrival_prev_stage,
          slack,
          slew,
          cap));
      arrival_prev_stage = arrival_cur_stage;
    }
    first_path = false;
  }
}

std::string TimingPath::getStartStageName() const
{
  return path_nodes_[path_start_index_]->getNodeName();
}

std::string TimingPath::getEndStageName() const
{
  return path_nodes_.back()->getNodeName();
}

std::string TimingPathNode::getNodeName(bool include_master) const
{
  if (pin_->getObjectType() == odb::dbObjectType::dbITermObj) {
    odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(pin_);
    return db_iterm->getInst()->getName() + "/"
           + db_iterm->getMTerm()->getName()
           + (include_master
                  ? " (" + db_iterm->getInst()->getMaster()->getName() + ")"
                  : "");
  }
  odb::dbBTerm* db_bterm = static_cast<odb::dbBTerm*>(pin_);
  return db_bterm->getName();
}

std::string TimingPathNode::getNetName() const
{
  if (pin_->getObjectType() == odb::dbObjectType::dbITermObj) {
    odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(pin_);
    return db_iterm->getNet()->getName();
  }
  odb::dbBTerm* db_bterm = static_cast<odb::dbBTerm*>(pin_);
  return db_bterm->getNet()->getName();
}

/////////

TimingPathDetailModel::TimingPathDetailModel(sta::dbSta* sta)
  : QAbstractTableModel(),
    sta_(sta),
    path_(nullptr),
    nodes_(nullptr)
{
}

int TimingPathDetailModel::rowCount(const QModelIndex& parent) const
{
  if (path_ == nullptr)
    return 0;
  return nodes_->size();
}

int TimingPathDetailModel::columnCount(const QModelIndex& parent) const
{
  return TimingPathDetailModel::_path_details_columns.size();
}

QVariant TimingPathDetailModel::data(const QModelIndex& index, int role) const
{
  const Column col_index = static_cast<Column>(index.column());
  if (index.isValid() && role == Qt::TextAlignmentRole) {
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
  }

  if (!index.isValid() || role != Qt::DisplayRole || path_ == nullptr) {
    return QVariant();
  }

  const auto time_units = sta_->search()->units()->timeUnit();

  const int row_index = index.row();
  if (row_index > nodes_->size()) {
    return QVariant();
  }
  auto& node = nodes_->at(row_index);
  switch (col_index) {
    case Pin:
      return QString(node->getNodeName(/* include_master */ true).c_str());
    case RiseFall:
      return node->is_rising_ ? QString(up_arrow) : QString(down_arrow);
    case Time:
      return time_units->asString(node->arrival_);
    case Delay:
      return time_units->asString(node->delay_);
    case Slew:
      return time_units->asString(node->slew_);
    case Load: {
      if (node->load_ == 0)
        return "";
      const auto cap_units = sta_->search()->units()->capacitanceUnit();
      return cap_units->asString(node->load_);
    }
  }
  return QVariant();
}

QVariant TimingPathDetailModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    return QString::fromStdString(
        TimingPathDetailModel::_path_details_columns.at(section));
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

TimingPathRenderer::TimingPathRenderer(sta::dbSta* sta) :
    sta_(sta),
    path_(nullptr),
    highlight_stage_(nullptr)
{
  TimingPathRenderer::path_inst_color_.a = 100;
  TimingPathRenderer::inst_highlight_color_.a = 100;
  TimingPathRenderer::clock_color_.a = 100;
  TimingPathRenderer::signal_color_.a = 100;
  TimingPathRenderer::term_color_.a = 100;
}

TimingPathRenderer::~TimingPathRenderer()
{
}

void TimingPathRenderer::highlight(TimingPath* path)
{
  path_ = path;
  highlight_stage_ = nullptr;
  redraw();
}

void TimingPathRenderer::highlightNode(const TimingPathNode* node, TimingPath::TimingNodeList* nodes)
{
  if (path_ != nullptr) {
    odb::dbObject* sink_node = nullptr;
    odb::dbNet* net = nullptr;
    odb::dbInst* inst = nullptr;

    auto getInstance = [&](const TimingPathNode* node) {
      if (inst != nullptr) {
        return;
      }

      if (node->pin_->getObjectType() == odb::dbObjectType::dbITermObj) {
        odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(node->pin_);
        inst = db_iterm->getInst();
      }
    };

    auto getNet = [&](const TimingPathNode* node) {
      if (net != nullptr) {
        return;
      }

      if (node->pin_->getObjectType() == odb::dbObjectType::dbITermObj) {
        odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(node->pin_);
        net = db_iterm->getNet();
      } else {
        odb::dbBTerm* bterm = static_cast<odb::dbBTerm*>(node->pin_);
        net = bterm->getNet();
      }
    };

    auto getSink = [&](const TimingPathNode* node) {
      if (sink_node != nullptr) {
        return;
      }

      if (node->pin_->getObjectType() == odb::dbObjectType::dbITermObj) {
        odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(node->pin_);
        auto io_dir = db_iterm->getIoType();
        if (io_dir == odb::dbIoType::INPUT || io_dir == odb::dbIoType::INOUT) {
          sink_node = db_iterm;
        }
      } else {
        odb::dbBTerm* bterm = static_cast<odb::dbBTerm*>(node->pin_);
        auto io_dir = bterm->getIoType();
        if (io_dir == odb::dbIoType::OUTPUT || io_dir == odb::dbIoType::INOUT) {
          sink_node = bterm;
        }
      }
    };

    getNet(node);

    getSink(node);
    if (sink_node == nullptr) {
      auto node_itr = std::find_if(nodes->begin(), nodes->end(), [node](const auto& other) -> bool {
        return other.get() == node;
      });

      if (node_itr != nodes->end()) {
        node_itr++;
      } else if (node_itr != nodes->begin()) {
        node_itr--;
      }
      getSink(node_itr->get());
    }

    getInstance(node);
    if (inst == nullptr) {
      auto node_itr = std::find_if(nodes->begin(), nodes->end(), [node](const auto& other) -> bool {
        return other.get() == node;
      });

      if (node_itr != nodes->end()) {
        node_itr++;
      } else if (node_itr != nodes->begin()) {
        node_itr--;
      }
      getInstance(node_itr->get());
    }

    if (net != nullptr || inst != nullptr) {
      highlight_stage_ = std::make_unique<HighlightStage>(HighlightStage{net, inst, sink_node});
    } else {
      highlight_stage_ = nullptr;
    }

    redraw();
  }
}

void TimingPathRenderer::drawNodesList(TimingPath::TimingNodeList* nodes,
                                       gui::Painter& painter,
                                       const gui::Descriptor* net_descriptor)
{
  odb::dbObject* sink_node = nullptr;
  odb::dbNet* net = nullptr;
  int node_count = nodes->size();
  for (int i = node_count - 1; i >= 0; --i) {
    auto& node = nodes->at(i);
    if (node->pin_->getObjectType() == odb::dbObjectType::dbITermObj) {
      odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(node->pin_);
      odb::dbInst* db_inst = db_iterm->getInst();
      highlightInst(db_inst, painter, TimingPathRenderer::path_inst_color_);
      auto io_dir = db_iterm->getIoType();
      if (!sink_node
          && (io_dir == odb::dbIoType::INPUT
              || io_dir == odb::dbIoType::INOUT)) {
        sink_node = db_iterm;
        net = db_iterm->getNet();
        continue;
      } else if (sink_node) {
        highlightNet(net, db_iterm /*source*/, sink_node, painter, net_descriptor);
        sink_node = nullptr;
        net = nullptr;
      }
    } else {
      odb::dbBTerm* bterm = static_cast<odb::dbBTerm*>(node->pin_);
      highlightTerm(bterm, painter);
      if (!sink_node
          && (bterm->getIoType() == odb::dbIoType::OUTPUT
              || bterm->getIoType() == odb::dbIoType::INOUT)) {
        sink_node = bterm;
        net = bterm->getNet();
        continue;
      } else if (sink_node) {
        highlightNet(net, bterm, sink_node, painter, net_descriptor);
        sink_node = nullptr;
        net = nullptr;
      }
    }
  }
}

void TimingPathRenderer::drawObjects(gui::Painter& painter)
{
  if (path_ == nullptr)
    return;

  auto* net_descriptor = Gui::get()->getDescriptor<odb::dbNet*>();

  drawNodesList(path_->getPathNodes(), painter, net_descriptor);
  drawNodesList(path_->getCaptureNodes(), painter, net_descriptor);

  highlightStage(painter, net_descriptor);
}

void TimingPathRenderer::highlightStage(gui::Painter& painter, const gui::Descriptor* net_descriptor)
{
  if (highlight_stage_ == nullptr) {
    return;
  }

  if (highlight_stage_->inst != nullptr) {
    highlightInst(highlight_stage_->inst, painter, TimingPathRenderer::inst_highlight_color_);
  }

  if (highlight_stage_->net != nullptr) {
    painter.setPen(TimingPathRenderer::inst_highlight_color_, true);
    painter.setBrush(TimingPathRenderer::inst_highlight_color_);
    net_descriptor->highlight(highlight_stage_->net, painter, highlight_stage_->sink);
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
                                      odb::dbObject* source_node,
                                      odb::dbObject* sink_node,
                                      gui::Painter& painter,
                                      const gui::Descriptor* net_descriptor)
{
  if (path_ == nullptr) {
    return;
  }

  auto isClockTerm = [this](odb::dbObject* node) -> bool {
    if (node->getObjectType() == odb::dbObjectType::dbBTermObj) {
      odb::dbBTerm* bterm = static_cast<odb::dbBTerm*>(node);
      auto* sta_term = sta_->getDbNetwork()->dbToSta(bterm);
      return sta_->isClock(sta_term);
    } else if (node->getObjectType() == odb::dbObjectType::dbITermObj) {
      odb::dbITerm* iterm = static_cast<odb::dbITerm*>(node);
      auto* sta_term = sta_->getDbNetwork()->dbToSta(iterm);
      return sta_->isClock(sta_term);
    }
    return false;
  };

  gui::Painter::Color wire_color = isClockTerm(source_node) || isClockTerm(sink_node)
                                       ? TimingPathRenderer::clock_color_
                                       : TimingPathRenderer::signal_color_;
  painter.setPen(wire_color, true);
  painter.setBrush(wire_color);
  net_descriptor->highlight(net, painter, sink_node);
}

}  // namespace gui
