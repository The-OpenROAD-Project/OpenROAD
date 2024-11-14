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

#include "staGui.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QStandardItemModel>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#include "dbDescriptors.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "sta/Corner.hh"
#include "sta/PatternMatch.hh"
#include "sta/Units.hh"

Q_DECLARE_METATYPE(sta::Corner*);

namespace gui {

const Painter::Color TimingPathRenderer::inst_highlight_color_
    = Painter::Color(gui::Painter::highlight, 100);
const Painter::Color TimingPathRenderer::path_inst_color_
    = Painter::Color(gui::Painter::magenta, 100);
const Painter::Color TimingPathRenderer::term_color_
    = Painter::Color(gui::Painter::blue, 100);
const Painter::Color TimingPathRenderer::signal_color_
    = Painter::Color(gui::Painter::red, 100);
const Painter::Color TimingPathRenderer::clock_color_
    = Painter::Color(gui::Painter::cyan, 100);
const Painter::Color TimingPathRenderer::capture_clock_color_
    = Painter::Color(gui::Painter::green, 100);

static QString convertDelay(float time, sta::Unit* convert)
{
  if (sta::delayInf(time)) {
    const QString infinity = "∞";

    if (time < 0) {
      return "-" + infinity;
    } else {
      return infinity;
    }
  } else {
    return convert->asString(time);
  }
}

/////////

TimingPathsModel::TimingPathsModel(bool is_setup,
                                   STAGuiInterface* sta,
                                   QObject* parent)
    : QAbstractTableModel(parent), sta_(sta), is_setup_(is_setup)
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
  return getColumnNames().size();
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
      case LogicDelay:
      case LogicDepth:
      case Fanout:
      case Slack:
      case Skew:
        return Qt::AlignRight;
    }
  } else if (role == Qt::DisplayRole) {
    auto time_units = sta_->getSTA()->units()->timeUnit();
    auto* timing_path = getPathAt(index);
    switch (col_index) {
      case Clock:
        return QString::fromStdString(timing_path->getEndClock());
      case Required:
        return convertDelay(timing_path->getPathRequiredTime(), time_units);
      case Arrival:
        return convertDelay(timing_path->getPathArrivalTime(), time_units);
      case Slack:
        return convertDelay(timing_path->getSlack(), time_units);
      case Skew:
        return convertDelay(timing_path->getSkew(), time_units);
      case LogicDelay:
        return convertDelay(timing_path->getLogicDelay(), time_units);
      case LogicDepth:
        return timing_path->getLogicDepth();
      case Fanout:
        return timing_path->getFanout();
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
  Column column = static_cast<Column>(section);

  if (role == Qt::ToolTipRole) {
    switch (column) {
      case Clock:
      case Required:
      case Arrival:
      case Slack:
      case Fanout:
      case Start:
      case End:
        // return empty string so that the tooltip goes away
        // when switching from a header item that has a tooltip
        // to a header item that doesn't.
        return "";
      case Skew:
        // A rather verbose tooltip, move some of this to a help/documentation
        // file when one is introduced into OpenROAD. Meanwhile, this is the
        // best that can be done.
        return "The difference in arrival times between\n"
               "source and destination clock pins of a macro/register,\n"
               "adjusted for CRPR and subtracting a clock period.\n"
               "Setup and hold times account for internal clock delays.";
      case LogicDelay:
        return "Path delay from instances (excluding buffers and consecutive "
               "inverter pairs)";
      case LogicDepth:
        return "Path instances (excluding buffers and consecutive inverter "
               "pairs)";
    }
  }

  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    return getColumnNames().at(column);
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
  std::function<bool(const std::unique_ptr<TimingPath>& path1,
                     const std::unique_ptr<TimingPath>& path2)>
      sort_func;
  if (col_index == Clock) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1,
                   const std::unique_ptr<TimingPath>& path2) {
      return path1->getEndClock() < path2->getEndClock();
    };
  } else if (col_index == Required) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1,
                   const std::unique_ptr<TimingPath>& path2) {
      return path1->getPathRequiredTime() < path2->getPathRequiredTime();
    };
  } else if (col_index == Arrival) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1,
                   const std::unique_ptr<TimingPath>& path2) {
      return path1->getPathArrivalTime() < path2->getPathArrivalTime();
    };
  } else if (col_index == Slack) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1,
                   const std::unique_ptr<TimingPath>& path2) {
      return path1->getSlack() < path2->getSlack();
    };
  } else if (col_index == Skew) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1,
                   const std::unique_ptr<TimingPath>& path2) {
      return path1->getSkew() < path2->getSkew();
    };
  } else if (col_index == LogicDelay) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1,
                   const std::unique_ptr<TimingPath>& path2) {
      return path1->getLogicDelay() < path2->getLogicDelay();
    };
  } else if (col_index == LogicDepth) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1,
                   const std::unique_ptr<TimingPath>& path2) {
      return path1->getLogicDepth() < path2->getLogicDepth();
    };
  } else if (col_index == Fanout) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1,
                   const std::unique_ptr<TimingPath>& path2) {
      return path1->getFanout() < path2->getFanout();
    };
  } else if (col_index == Start) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1,
                   const std::unique_ptr<TimingPath>& path2) {
      return path1->getStartStageName() < path2->getStartStageName();
    };
  } else if (col_index == End) {
    sort_func = [](const std::unique_ptr<TimingPath>& path1,
                   const std::unique_ptr<TimingPath>& path2) {
      return path1->getEndStageName() < path2->getEndStageName();
    };
  } else {
    return;
  }

  beginResetModel();

  if (sort_order == Qt::AscendingOrder) {
    std::stable_sort(
        timing_paths_.begin(), timing_paths_.end(), std::move(sort_func));
  } else {
    std::stable_sort(
        timing_paths_.rbegin(), timing_paths_.rend(), std::move(sort_func));
  }

  endResetModel();
}

void TimingPathsModel::populateModel(
    const std::set<const sta::Pin*>& from,
    const std::vector<std::set<const sta::Pin*>>& thru,
    const std::set<const sta::Pin*>& to,
    const std::string& path_group_name)
{
  beginResetModel();
  timing_paths_.clear();
  populatePaths(from, thru, to, path_group_name);
  endResetModel();
}

bool TimingPathsModel::populatePaths(
    const std::set<const sta::Pin*>& from,
    const std::vector<std::set<const sta::Pin*>>& thru,
    const std::set<const sta::Pin*>& to,
    const std::string& path_group_name)
{
  // On lines of DataBaseHandler
  QApplication::setOverrideCursor(Qt::WaitCursor);

  const bool sta_max = sta_->isUseMax();
  sta_->setUseMax(is_setup_);
  timing_paths_ = sta_->getTimingPaths(from, thru, to, path_group_name);
  sta_->setUseMax(sta_max);

  QApplication::restoreOverrideCursor();
  return true;
}

/////////

TimingPathDetailModel::TimingPathDetailModel(bool is_capture,
                                             sta::dbSta* sta,
                                             QObject* parent)
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
  return getColumnNames().size();
}

const TimingPathNode* TimingPathDetailModel::getNodeAt(
    const QModelIndex& index) const
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
  if (path_ == nullptr || nodes_ == nullptr || !hasNodes()) {
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
      case Fanout:
        return Qt::AlignRight;
      case RiseFall:
        return Qt::AlignCenter;
    }
  } else if (role == Qt::DisplayRole) {
    const auto time_units = sta_->units()->timeUnit();

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
          return convertDelay(node->getArrival(), time_units);
        case Delay:
          return convertDelay(node->getArrival() - nodes_->at(0)->getArrival(),
                              time_units);
        default:
          return QVariant();
      }
    } else {
      const auto* node = getNodeAt(index);
      switch (col_index) {
        case Pin:
          return QString::fromStdString(
              node->getNodeName(/* include_master */ true));
        case RiseFall:
          return node->isRisingEdge() ? up_arrow_ : down_arrow_;
        case Time:
          return convertDelay(node->getArrival(), time_units);
        case Delay:
          return convertDelay(node->getDelay(), time_units);
        case Slew:
          return convertDelay(node->getSlew(), time_units);
        case Load: {
          if (node->getLoad() == 0)
            return "";
          const auto cap_units = sta_->units()->capacitanceUnit();
          return cap_units->asString(node->getLoad());
        }
        case Fanout: {
          if (node->getFanout() == 0) {
            return "";
          }
          return node->getFanout();
        }
      }
    }
  }
  return QVariant();
}

bool TimingPathDetailModel::shouldHide(const QModelIndex& index) const
{
  const int row = index.row();
  const int last_clock
      = getClockEndIndex() + 1;  // accounting for clock_summary would +1

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
      case Fanout:
        return "Fanout";
    }
  }
  return QVariant();
}

void TimingPathDetailModel::populateModel(TimingPath* path,
                                          TimingNodeList* nodes)
{
  beginResetModel();
  path_ = path;
  nodes_ = nodes;
  endResetModel();
}

/////////

TimingPathRenderer::TimingPathRenderer() : path_(nullptr), highlight_stage_()
{
  addDisplayControl(data_path_label_, true);
  addDisplayControl(launch_clock_label_, true);
  addDisplayControl(capture_clock_label_, true);
}

void TimingPathRenderer::highlight(TimingPath* path)
{
  {
    std::lock_guard guard(rendering_);
    path_ = path;
    highlight_stage_.clear();
  }
  redraw();
}

void TimingPathRenderer::clearHighlightNodes()
{
  std::lock_guard guard(rendering_);
  highlight_stage_.clear();
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

    const TimingPathNode* sink_node = nullptr;
    for (auto* pair_node : node->getPairedNodes()) {
      sink_node = pair_node;
    }
    if (sink_node != nullptr) {
      sink = sink_node->getPin();
    }

    if (net != nullptr || inst != nullptr) {
      std::lock_guard guard(rendering_);
      highlight_stage_.push_back(
          std::make_unique<HighlightStage>(HighlightStage{net, inst, sink}));
    }

    redraw();
  }
}

void TimingPathRenderer::drawNodesList(TimingNodeList* nodes,
                                       gui::Painter& painter,
                                       const gui::Descriptor* net_descriptor,
                                       const gui::Descriptor* inst_descriptor,
                                       const gui::Descriptor* bterm_descriptor,
                                       const Painter::Color& clock_color,
                                       bool draw_clock,
                                       bool draw_signal)
{
  for (auto node_itr = nodes->rbegin(); node_itr != nodes->rend(); node_itr++) {
    const auto& node = *node_itr;
    if (node->isClock() && !draw_clock) {
      continue;
    }
    if (!node->isClock() && !draw_signal) {
      continue;
    }

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
      for (auto* sink_node : node->getPairedNodes()) {
        if (sink_node != nullptr) {
          gui::Painter::Color wire_color
              = node->isClock() ? clock_color
                                : TimingPathRenderer::signal_color_;
          painter.setPenAndBrush(wire_color, true);
          net_descriptor->highlight(
              DbNetDescriptor::NetWithSink{node->getNet(), sink_node->getPin()},
              painter);
        }
      }
    }
  }
}

void TimingPathRenderer::drawObjects(gui::Painter& painter)
{
  std::lock_guard guard(rendering_);
  if (path_ == nullptr) {
    return;
  }

  auto* net_descriptor = Gui::get()->getDescriptor<odb::dbNet*>();
  auto* inst_descriptor = Gui::get()->getDescriptor<odb::dbInst*>();
  auto* bterm_descriptor = Gui::get()->getDescriptor<odb::dbBTerm*>();

  const bool capture_path = checkDisplayControl(capture_clock_label_);
  drawNodesList(&path_->getCaptureNodes(),
                painter,
                net_descriptor,
                inst_descriptor,
                bterm_descriptor,
                capture_clock_color_,
                capture_path,
                capture_path);
  drawNodesList(&path_->getPathNodes(),
                painter,
                net_descriptor,
                inst_descriptor,
                bterm_descriptor,
                clock_color_,
                checkDisplayControl(launch_clock_label_),
                checkDisplayControl(data_path_label_));

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
      net_descriptor->highlight(
          DbNetDescriptor::NetWithSink{highlight->net, highlight->sink},
          painter);
    }
  }
}

/////////

TimingConeRenderer::TimingConeRenderer()
    : sta_(nullptr),
      term_(nullptr),
      fanin_(false),
      fanout_(false),
      map_(),
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

void TimingConeRenderer::setPin(const sta::Pin* pin, bool fanin, bool fanout)
{
  if (sta_ == nullptr) {
    return;
  }

  if (isSupplyPin(pin)) {
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

  // populate with max number of unique paths, all other paths will appear as 0
  const int path_count = 1000;

  STAGuiInterface stagui(sta_);
  stagui.setCorner(sta_->cmdCorner());
  stagui.setUseMax(true);
  stagui.setIncludeUnconstrainedPaths(true);
  stagui.setMaxPathCount(path_count);
  stagui.setIncludeCapturePaths(false);

  ConeDepthMapPinSet pin_map;
  if (fanin_) {
    const auto fanin = stagui.getFaninCone(pin);
    pin_map.insert(fanin.begin(), fanin.end());
  }
  if (fanout_) {
    const auto fanout = stagui.getFanoutCone(pin);
    pin_map.insert(fanout.begin(), fanout.end());
  }

  map_ = stagui.buildConeConnectivity(pin, pin_map);

  min_timing_ = std::numeric_limits<float>::max();
  max_timing_ = std::numeric_limits<float>::lowest();
  for (const auto& [level, pin_list] : map_) {
    for (const auto& pin : pin_list) {
      if (sta::delayInf(pin->getPathSlack()) || !pin->hasValues()) {
        continue;
      }

      min_timing_ = std::min(min_timing_, pin->getPathSlack());
      max_timing_ = std::max(max_timing_, pin->getPathSlack());
    }
  }

  QApplication::restoreOverrideCursor();

  redraw();
}

bool TimingConeRenderer::isSupplyPin(const sta::Pin* pin) const
{
  auto* network = sta_->getDbNetwork();
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  odb::dbModBTerm* modbterm;
  network->staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm != nullptr) {
    if (iterm->getSigType().isSupply()) {
      return true;
    }
  } else if (bterm != nullptr) {
    if (bterm->getSigType().isSupply()) {
      return true;
    }
  }

  return false;
}

void TimingConeRenderer::drawObjects(gui::Painter& painter)
{
  if (map_.empty()) {
    return;
  }

  // draw timing connections
  const double timing_range = max_timing_ - min_timing_;
  const bool cone_unconstrained = max_timing_ < min_timing_;

  std::function<double(const TimingPathNode* node)> timingToRatio;
  if (timing_range == 0.0 || cone_unconstrained) {
    timingToRatio = [](const TimingPathNode* node) { return 0.5; };
  } else {
    timingToRatio = [this, timing_range](const TimingPathNode* node) {
      double value = 0.0;
      if (node->hasValues()) {
        value = 1.0 - (node->getPathSlack() - min_timing_) / timing_range;
      }
      return value;
    };
  }

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

  const int line_width = 2;  // 2 pixels
  auto* iterm_descriptor = Gui::get()->getDescriptor<odb::dbITerm*>();
  auto* bterm_descriptor = Gui::get()->getDescriptor<odb::dbBTerm*>();
  for (const auto& [level, pins] : map_) {
    for (const auto& pin : pins) {
      const auto color
          = color_generator_.getColor(timingToRatio(pin.get()), 255);
      painter.setPenAndBrush(color, true);

      if (pin->isPinITerm()) {
        iterm_descriptor->highlight(pin->getPinAsITerm(), painter);
      } else {
        bterm_descriptor->highlight(pin->getPinAsBTerm(), painter);
      }

      painter.setPen(color, true, line_width);
      const odb::Rect sink_rect = pin->getPinLargestBox();
      const odb::Point sink_pt(0.5 * (sink_rect.xMin() + sink_rect.xMax()),
                               0.5 * (sink_rect.yMin() + sink_rect.yMax()));

      for (auto* source_node : pin->getPairedNodes()) {
        const odb::Rect source_rect = source_node->getPinLargestBox();
        const odb::Point source_pt(
            0.5 * (source_rect.xMin() + source_rect.xMax()),
            0.5 * (source_rect.yMin() + source_rect.yMax()));
        const auto source_color
            = color_generator_.getColor(timingToRatio(source_node), 255);
        painter.setPen(source_color, true, line_width);
        painter.drawLine(
            source_pt.x(), source_pt.y(), sink_pt.x(), sink_pt.y());
      }
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
      const odb::Rect text_bound
          = painter.stringBoundaries(pin_pt.x(), pin_pt.y(), text_anchor, text);

      if (text_bound.dx() < text_margin * pin_rect.dx()
          && text_bound.dy() < text_margin * pin_rect.dy()) {
        painter.drawString(pin_pt.x(), pin_pt.y(), text_anchor, text);
      }
    }
  }

  if (!cone_unconstrained) {
    // draw legend, dont draw if cone is unonstrained
    const int legend_keys = 5;
    const int color_count = color_generator_.getColorCount();
    auto* units = sta_->units()->timeUnit();
    const std::string text_units
        = std::string(units->scaleAbbreviation()) + units->suffix();
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
}

/////////

PinSetWidget::PinSetWidget(bool add_remove_button, QWidget* parent)
    : QWidget(parent),
      sta_(nullptr),
      pins_({}),
      box_(new QListWidget(this)),
      find_pin_(new QLineEdit(this)),
      clear_(new QPushButton(this)),
      add_remove_(nullptr),
      add_mode_(true)
{
  const int min_characters = 25;
  const auto font_metrics = fontMetrics();
  const int min_width = min_characters * font_metrics.averageCharWidth();
  find_pin_->setMinimumWidth(min_width);

  box_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  const int max_rows = 5;
  const int row_height = font_metrics.height();
  box_->setFixedHeight(max_rows * row_height);

  clear_->setIcon(QIcon(":/delete.png"));
  clear_->setToolTip(tr("Clear pins"));
  clear_->setAutoDefault(false);
  clear_->setDefault(false);
  connect(clear_, &QPushButton::pressed, this, &PinSetWidget::clearPins);

  connect(find_pin_, &QLineEdit::returnPressed, this, &PinSetWidget::findPin);

  QVBoxLayout* layout = new QVBoxLayout;
  QHBoxLayout* row_layout = new QHBoxLayout;
  row_layout->addWidget(find_pin_);
  row_layout->addWidget(clear_);
  if (add_remove_button) {
    add_remove_ = new QPushButton(this);
    add_remove_->setToolTip(tr("Add/Remove rows"));
    row_layout->addWidget(add_remove_);

    add_remove_->setAutoDefault(false);
    add_remove_->setDefault(false);
    connect(add_remove_, &QPushButton::pressed, [this]() {
      emit addRemoveTriggered(this);
    });
    setAddMode();
  }

  layout->addLayout(row_layout);
  layout->addWidget(box_);

  setLayout(layout);

  box_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(box_,
          &QListWidget::customContextMenuRequested,
          this,
          &PinSetWidget::showMenu);
}

void PinSetWidget::setAddMode()
{
  if (add_remove_ == nullptr) {
    return;
  }

  add_mode_ = true;
  add_remove_->setIcon(QIcon(":/add.png"));
}

void PinSetWidget::setRemoveMode()
{
  if (add_remove_ == nullptr) {
    return;
  }

  add_mode_ = false;
  add_remove_->setIcon(QIcon(":/remove.png"));
}

void PinSetWidget::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Delete && box_->hasFocus()) {
    removeSelectedPins();
  } else {
    QWidget::keyPressEvent(event);
  }
}

void PinSetWidget::updatePins()
{
  box_->clear();
  if (!pins_.empty()) {
    auto* network = sta_->getDbNetwork();
    for (const auto* pin : pins_) {
      auto* item = new QListWidgetItem(network->name(pin));
      item->setData(Qt::UserRole, QVariant::fromValue((void*) pin));
      box_->addItem(item);
    }
  }
}

void PinSetWidget::setPins(const std::set<const sta::Pin*>& pins)
{
  pins_.clear();
  pins_.insert(pins_.end(), pins.begin(), pins.end());
  updatePins();
}

const std::set<const sta::Pin*> PinSetWidget::getPins() const
{
  std::set<const sta::Pin*> pins(pins_.begin(), pins_.end());
  return pins;
}

void PinSetWidget::addPin(const sta::Pin* pin)
{
  if (pin == nullptr) {
    return;
  }

  if (std::find(pins_.begin(), pins_.end(), pin) != pins_.end()) {
    return;
  }

  pins_.push_back(pin);
}

void PinSetWidget::removePin(const sta::Pin* pin)
{
  pins_.erase(std::find(pins_.begin(), pins_.end(), pin));
}

void PinSetWidget::removeSelectedPins()
{
  for (auto* selection : box_->selectedItems()) {
    void* pin_data = selection->data(Qt::UserRole).value<void*>();
    removePin((const sta::Pin*) pin_data);
  }
  updatePins();
}

void PinSetWidget::findPin()
{
  const QString text = find_pin_->text();
  if (text.isEmpty()) {
    return;
  }
  auto* network = sta_->getDbNetwork();
  auto* top_inst = network->topInstance();

  for (QString pin_name : text.split(" ")) {
    pin_name = pin_name.trimmed();

    if (pin_name.isEmpty()) {
      continue;
    }

    QByteArray name = pin_name.toLatin1();
    sta::PatternMatch matcher(name.constData());

    // search pins
    sta::PinSeq found_pins = network->findPinsHierMatching(top_inst, &matcher);

    for (const sta::Pin* pin : found_pins) {
      addPin(pin);
    }

    // search ports
    sta::PortSeq found_ports
        = network->findPortsMatching(network->cell(top_inst), &matcher);

    for (auto* port : found_ports) {
      if (network->isBus(port) || network->isBundle(port)) {
        sta::PortMemberIterator* member_iter = network->memberIterator(port);
        while (member_iter->hasNext()) {
          sta::Port* member = member_iter->next();
          addPin(network->findPin(top_inst, member));
        }
        delete member_iter;
      } else {
        addPin(network->findPin(top_inst, port));
      }
    }
  }

  updatePins();
}

void PinSetWidget::showMenu(const QPoint& point)
{
  // Handle global position
  const QPoint global = box_->mapToGlobal(point);

  auto* pin_item = box_->itemAt(box_->viewport()->mapFromGlobal(global));
  if (pin_item == nullptr) {
    return;
  }

  const sta::Pin* pin
      = (const sta::Pin*) pin_item->data(Qt::UserRole).value<void*>();

  // Create menu and insert some actions
  QMenu pin_menu;
  QAction* remove = pin_menu.addAction("Remove");
  connect(remove, &QAction::triggered, [this, pin]() {
    removePin(pin);
    updatePins();
  });
  QAction* remove_sel = pin_menu.addAction("Remove selected");
  connect(remove_sel, &QAction::triggered, [this]() { removeSelectedPins(); });
  QAction* clear_all = pin_menu.addAction("Clear all");
  connect(clear_all, &QAction::triggered, this, &PinSetWidget::clearPins);
  QAction* inspect_action = pin_menu.addAction("Inspect");
  connect(inspect_action, &QAction::triggered, [this, pin]() {
    auto* gui = Gui::get();
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModITerm* moditerm;
    odb::dbModBTerm* modbterm;
    sta_->getDbNetwork()->staToDb(pin, iterm, bterm, moditerm, modbterm);
    if (iterm != nullptr) {
      emit inspect(gui->makeSelected(iterm));
    } else {
      emit inspect(gui->makeSelected(bterm));
    }
  });

  // Show context menu at handling position
  pin_menu.exec(global);
}

/////////

TimingControlsDialog::TimingControlsDialog(QWidget* parent)
    : QDialog(parent),
      sta_(std::make_unique<STAGuiInterface>()),
      layout_(new QFormLayout),
      path_count_spin_box_(new QSpinBox(this)),
      corner_box_(new QComboBox(this)),
      unconstrained_(new QCheckBox(this)),
      one_path_per_endpoint_(new QCheckBox(this)),
      expand_clk_(new QCheckBox(this)),
      from_(new PinSetWidget(false, this)),
      thru_({}),
      to_(new PinSetWidget(false, this))
{
  setWindowTitle("Timing Controls");

  path_count_spin_box_->setRange(0, 10000);
  path_count_spin_box_->setValue(100);

  layout_->addRow("Paths:", path_count_spin_box_);
  layout_->addRow("Expand clock:", expand_clk_);
  layout_->addRow("Corner:", corner_box_);

  setupPinRow("From:", from_);
  setThruPin({});
  setupPinRow("To:", to_);

  setUnconstrained(false);
  layout_->addRow("Unconstrained:", unconstrained_);
  layout_->addRow("One path per endpoint:", one_path_per_endpoint_);

  setLayout(layout_);

  connect(path_count_spin_box_,
          QOverload<int>::of(&QSpinBox::valueChanged),
          [this](int value) { sta_->setMaxPathCount(value); });
  connect(unconstrained_, &QCheckBox::stateChanged, [this]() {
    sta_->setIncludeUnconstrainedPaths(unconstrained_->checkState()
                                       == Qt::Checked);
  });
  connect(one_path_per_endpoint_, &QCheckBox::stateChanged, [this]() {
    sta_->setOnePathPerEndpoint(one_path_per_endpoint_->checkState()
                                == Qt::Checked);
  });

  connect(corner_box_,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          [this](int index) {
            if (index < 0 || index >= corner_box_->count()) {
              return;
            }
            auto* corner = corner_box_->itemData(index).value<sta::Corner*>();
            sta_->setCorner(corner);
          });

  connect(expand_clk_,
          &QCheckBox::toggled,
          this,
          &TimingControlsDialog ::expandClock);

  sta_->setIncludeCapturePaths(true);
}

void TimingControlsDialog::setupPinRow(const QString& label,
                                       PinSetWidget* row,
                                       int row_index)
{
  if (row_index < 0) {
    layout_->addRow(label, row);
  } else {
    layout_->insertRow(row_index, label, row);
  }

  row->setSTA(sta_->getSTA());

  connect(row, &PinSetWidget::inspect, this, &TimingControlsDialog::inspect);
}

void TimingControlsDialog::setSTA(sta::dbSta* sta)
{
  sta_->setSTA(sta);
  from_->setSTA(sta_->getSTA());
  for (auto* row : thru_) {
    row->setSTA(sta_->getSTA());
  }
  to_->setSTA(sta_->getSTA());
}

void TimingControlsDialog::setOnePathPerEndpoint(bool value)
{
  sta_->setOnePathPerEndpoint(value);
  one_path_per_endpoint_->setCheckState(value ? Qt::Checked : Qt::Unchecked);
}

void TimingControlsDialog::setUnconstrained(bool unconstrained)
{
  sta_->setIncludeUnconstrainedPaths(unconstrained);
  unconstrained_->setCheckState(unconstrained ? Qt::Checked : Qt::Unchecked);
}

void TimingControlsDialog::setPathCount(int path_count)
{
  sta_->setMaxPathCount(path_count);
  path_count_spin_box_->setValue(path_count);
}

void TimingControlsDialog::setExpandClock(bool expand)
{
  expand_clk_->setCheckState(expand ? Qt::Checked : Qt::Unchecked);
}

bool TimingControlsDialog::getExpandClock() const
{
  return expand_clk_->checkState() == Qt::Checked;
}

void TimingControlsDialog::populate()
{
  setPinSelections();

  auto* current_corner = sta_->getCorner();
  corner_box_->clear();
  int selection = 0;
  for (auto* corner : sta_->getSTA()->corners()->corners()) {
    if (corner == current_corner) {
      selection = corner_box_->count();
    }
    corner_box_->addItem(corner->name(), QVariant::fromValue(corner));
  }

  if (corner_box_->count() > 1) {
    selection += 1;
    corner_box_->insertItem(
        0, "All", QVariant::fromValue(static_cast<sta::Corner*>(nullptr)));
    if (current_corner == nullptr) {
      selection = 0;
    }
  }

  corner_box_->setCurrentIndex(selection);
}

void TimingControlsDialog::setPinSelections()
{
  from_->updatePins();
  for (auto* row : thru_) {
    row->updatePins();
  }
  to_->updatePins();
}

const sta::Pin* TimingControlsDialog::convertTerm(Gui::odbTerm term) const
{
  sta::dbNetwork* network = sta_->getNetwork();

  if (std::holds_alternative<odb::dbITerm*>(term)) {
    return network->dbToSta(std::get<odb::dbITerm*>(term));
  } else {
    return network->dbToSta(std::get<odb::dbBTerm*>(term));
  }
}

void TimingControlsDialog::setThruPin(
    const std::vector<std::set<const sta::Pin*>>& pins)
{
  for (size_t i = 0; i < thru_.size(); i++) {
    layout_->removeRow(thru_start_row_);
  }
  thru_.clear();
  adjustSize();

  auto new_pins = pins;
  if (pins.empty()) {
    new_pins.push_back({});  // add one row
  }

  for (const auto& pin_set : new_pins) {
    addThruRow(pin_set);
  }
}

void TimingControlsDialog::addThruRow(const std::set<const sta::Pin*>& pins)
{
  auto* row = new PinSetWidget(true, this);

  setupPinRow("Through:", row, thru_start_row_ + thru_.size());
  row->setPins(pins);

  connect(row,
          &PinSetWidget::addRemoveTriggered,
          this,
          &TimingControlsDialog::addRemoveThru);

  for (const auto& lower_row : thru_) {
    lower_row->setRemoveMode();
  }
  thru_.push_back(row);
}

void TimingControlsDialog::addRemoveThru(PinSetWidget* row)
{
  if (row->isAddMode()) {
    addThruRow({});
  } else {
    auto find_row = std::find(thru_.begin(), thru_.end(), row);
    const int row_index = std::distance(thru_.begin(), find_row);

    layout_->removeRow(thru_start_row_ + row_index);
    thru_.erase(thru_.begin() + row_index);
    thru_.back()->setAddMode();
    adjustSize();
  }
}

const std::vector<std::set<const sta::Pin*>> TimingControlsDialog::getThruPins()
    const
{
  std::vector<std::set<const sta::Pin*>> pins;
  for (auto* row : thru_) {
    pins.push_back(row->getPins());
  }
  return pins;
}
}  // namespace gui
