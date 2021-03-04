/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, OpenROAD
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

#include <QApplication>
#include <QDebug>
#include <fstream>
#include <iostream>
#include <string>

#include "db.h"
#include "dbShape.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "openroad/OpenRoad.hh"
#include "sta/Corner.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"

namespace gui {

gui::Painter::Color TimingPathRenderer::inst_highlight_color_
    = gui::Painter::cyan;
gui::Painter::Color TimingPathRenderer::path_inst_color_
    = gui::Painter::magenta;
gui::Painter::Color TimingPathRenderer::term_color_ = gui::Painter::blue;
gui::Painter::Color TimingPathRenderer::signal_color_ = gui::Painter::red;
gui::Painter::Color TimingPathRenderer::clock_color_ = gui::Painter::yellow;
// helper functions
float getRequiredTime(sta::dbSta* staRoot,
                      sta::Pin* term,
                      bool is_rise,
                      sta::PathAnalysisPt* path_ap)
{
  auto vert = staRoot->getDbNetwork()->graph()->pinLoadVertex(term);
  auto req = staRoot->vertexRequired(
      vert, is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(), path_ap);
  if (sta::delayInf(req)) {
    return 0;
  }
  return req;
}

TimingPathsModel::TimingPathsModel() : openroad_(ord::OpenRoad::openRoad())
{
  populateModel();
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
  if (!index.isValid() || role != Qt::DisplayRole) {
    return QVariant();
  }

  sta::dbSta* sta = openroad_->getSta();
  auto time_units = sta->search()->units()->timeUnit();

  int row_index = index.row();
  if (row_index > timing_paths_.size())
    return QVariant();
  int col_index = index.column();
  auto timing_path = timing_paths_[row_index];
  switch (col_index) {
    case 0:  // Path Id
      return QString::number(row_index + 1);
    case 1:  // Clock
      return QString(timing_path->getStartClock().c_str());
    case 2:  // Required Time
      return QString(time_units->asString(timing_path->getPathRequiredTime()));
    case 3:  // Arrival Time
      return QString(time_units->asString(timing_path->getPathArrivalTime()));
    case 4:  // Slack
      return QString(time_units->asString(timing_path->getSlack()));
    case 5:  // Path Start
      return QString(timing_path->getStartStageName().c_str());
    case 6:  // Path End
      return QString(timing_path->getEndStageName().c_str());
  }
  return QVariant();
}

QVariant TimingPathsModel::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    if (section > 1 && section < 5) {
      sta::dbSta* sta = openroad_->getSta();
      auto time_units = sta->search()->units()->timeUnit();
      return QString(TimingPathsModel::_path_columns[section].c_str())
             + QString(" (") + QString(time_units->suffix()) + QString(")");
    } else
      return QString(TimingPathsModel::_path_columns[section].c_str());
  }
  return QVariant();
}

// Get the network for commands.
void TimingPathsModel::findInstances(std::string pattern,
                                     std::vector<odb::dbInst*>& insts)
{
  if (!openroad_)
    return;
  std::vector<sta::Instance*> staInsts = findInstancesNetwork(pattern);
  for (auto staInst : staInsts) {
    insts.push_back(openroad_->getSta()->getDbNetwork()->staToDb(staInst));
  }
  return;
}

void TimingPathsModel::findNets(std::string pattern,
                                std::vector<odb::dbNet*>& nets)
{
  if (!openroad_)
    return;
  std::vector<sta::Net*> staNets = findNetsNetwork(pattern);
  for (auto staNet : staNets) {
    nets.push_back(openroad_->getSta()->getDbNetwork()->staToDb(staNet));
  }
  return;
}

void TimingPathsModel::findPins(std::string pattern,
                                std::vector<odb::dbObject*>& pins)
{
  if (!openroad_)
    return;
  std::vector<sta::Pin*> staPins = findPinsNetwork(pattern);
  for (auto staPin : staPins) {
    odb::dbITerm* term;
    odb::dbBTerm* port;
    openroad_->getSta()->getDbNetwork()->staToDb(staPin, term, port);
    odb::dbObject* pinObject
        = term ? (odb::dbObject*) term : (odb::dbObject*) port;
    pins.push_back(pinObject);
  }
  return;
}

std::vector<sta::Instance*> TimingPathsModel::findInstancesNetwork(
    std::string pattern)
{
  sta::InstanceSeq all_insts;
  sta::PatternMatch staPattern(pattern.c_str());
  if (openroad_)
    openroad_->getSta()->getDbNetwork()->findInstancesMatching(
        openroad_->getSta()->getDbNetwork()->topInstance(),
        &staPattern,
        &all_insts);
  return static_cast<std::vector<sta::Instance*>>(all_insts);
}

std::vector<sta::Net*> TimingPathsModel::findNetsNetwork(std::string pattern)
{
  sta::NetSeq all_nets;
  sta::PatternMatch staPattern(pattern.c_str());
  if (openroad_)
    openroad_->getSta()->getDbNetwork()->findNetsMatching(
        openroad_->getSta()->getDbNetwork()->topInstance(),
        &staPattern,
        &all_nets);
  return static_cast<std::vector<sta::Net*>>(all_nets);
}

std::vector<sta::Pin*> TimingPathsModel::findPinsNetwork(std::string pattern)
{
  sta::PinSeq all_pins;
  sta::PatternMatch staPattern(pattern.c_str());
  if (openroad_)
    openroad_->getSta()->getDbNetwork()->findPinsMatching(
        openroad_->getSta()->getDbNetwork()->topInstance(),
        &staPattern,
        &all_pins);
  return static_cast<std::vector<sta::Pin*>>(all_pins);
}

void TimingPathsModel::populateModel()
{
  beginResetModel();
  populatePaths();
  endResetModel();
  qDebug() << "Timing Path Model populated with : " << timing_paths_.size()
           << " Paths...";
  if (timing_paths_.size() > 0)
    timing_paths_[7]->printPath("pathReport.csv");
}

bool TimingPathsModel::populatePaths(bool get_max, int path_count)
{
  // On lines of DataBaseHandler
  QApplication::setOverrideCursor(Qt::WaitCursor);
  sta::dbSta* sta_ = openroad_->getSta();
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
    sta::PathExpanded* expanded = new sta::PathExpanded(path_end->path(), sta_);

    TimingPath* path = new TimingPath();
    path->setStartClock(path_end->sourceClkEdge(sta_)->clock()->name());
    path->setEndClock(path_end->targetClk(sta_)->name());
    path->setPathDelay(path_end->pathDelay() ? path_end->pathDelay()->delay()
                                             : 0);
    path->setSlack(path_end->slack(sta_));
    path->setArrTime(path_end->dataArrivalTime(sta_));
    path->setReqTime(path_end->requiredTime(sta_));

    // for (size_t i = expanded->size() - 1; i > 0; i--) {
    for (size_t i = 0; i < expanded->size(); i++) {
      auto ref = expanded->path(i);
      auto pin = ref->vertex(sta_)->pin();
      auto slew = ref->slew(sta_);
      float cap = 0.0;
      // if (sta_->getDbNetwork()->isDriver(pin)) {
      //  cap = loadCap(pin, ref->transition(sta_),
      //  ref->pathAnalysisPt(sta_)->dcalcAnalysisPt());
      //}
      auto is_rising = ref->transition(sta_) == sta::RiseFall::rise();
      auto arrival = ref->arrival(sta_);
      auto path_ap = ref->pathAnalysisPt(sta_);
      auto path_required = !first_path ? 0 : ref->required(sta_);
      if (!path_required || sta::delayInf(path_required)) {
        path_required = getRequiredTime(sta_, pin, is_rising, path_ap);
      }
      auto slack = !first_path ? path_required - arrival : ref->slack(sta_);
      odb::dbITerm* term;
      odb::dbBTerm* port;
      sta_->getDbNetwork()->staToDb(pin, term, port);
      odb::dbObject* pinObject
          = term ? (odb::dbObject*) term : (odb::dbObject*) port;
      path->appendNode(TimingPathNode(
          pinObject, is_rising, arrival, path_required, slack, slew, cap));
      first_path = false;
    }
    path->setPathExp(expanded);
    timing_paths_.push_back(path);
  }
  QApplication::restoreOverrideCursor();
  // delete path_ends;
  return true;
}

std::string TimingPath::getStartStageName() const
{
  auto node = getNodeAt(1);
  return node.getNodeName();
}

std::string TimingPath::getEndStageName() const
{
  auto node = getNodeAt(path_nodes_.size() - 1);
  return node.getNodeName();
}

void TimingPath::printPath(const std::string& file_name) const
{
  std::ofstream ofs(file_name.c_str());
  if (ofs.is_open()) {
    ofs << "Pin,Direction,Inst,Net,Transition,Arrival,Required,Slack"
        << std::endl;
    for (auto& node : path_nodes_) {
      std::string obj_name;
      std::string obj_dir;
      std::string inst_name;
      std::string net_name;
      if (node.pin_->getObjectType() == odb::dbObjectType::dbITermObj) {
        odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(node.pin_);
        obj_name = db_iterm->getInst()->getName() + "/"
                   + db_iterm->getMTerm()->getName();
        obj_dir = db_iterm->getIoType().getString();
        inst_name = db_iterm->getInst()->getName();
        if (db_iterm->getNet())
          net_name = db_iterm->getNet()->getName();
      } else {
        odb::dbBTerm* db_bterm = static_cast<odb::dbBTerm*>(node.pin_);
        obj_name = db_bterm->getName();
        obj_dir = db_bterm->getIoType().getString();
        if (db_bterm->getNet())
          net_name = db_bterm->getNet()->getName();
      }
      const char* trans = node.is_rising_ ? "R" : "F";
      ofs << obj_name << "," << obj_dir << "," << inst_name << "," << net_name
          << "," << trans << "," << node.arrival_ << "," << node.required_
          << "," << node.slack_ << std::endl;
    }
    ofs.close();
  }
}

std::string TimingPathNode::getNodeName() const
{
  if (pin_->getObjectType() == odb::dbObjectType::dbITermObj) {
    odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(pin_);
    return db_iterm->getInst()->getName() + "/"
           + db_iterm->getMTerm()->getName();
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

int TimingPathDetailModel::rowCount(const QModelIndex& parent) const
{
  return path_->levelsCount();
}

int TimingPathDetailModel::columnCount(const QModelIndex& parent) const
{
  return TimingPathDetailModel::_path_details_columns.size();
}

QVariant TimingPathDetailModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || role != Qt::DisplayRole) {
    return QVariant();
  }

  sta::dbSta* sta = ord::OpenRoad::openRoad()->getSta();
  auto time_units = sta->search()->units()->timeUnit();
  auto cap_units = sta->search()->units()->capacitanceUnit();

  int row_index = index.row();
  if (row_index > path_->levelsCount())
    return QVariant();
  int col_index = index.column();
  auto node = path_->getNodeAt(row_index);
  switch (col_index) {
    case 0:  // Node Name
      return QString(node.getNodeName().c_str());
    case 1:  // Rising
      return node.is_rising_ ? QString("R") : QString("F");
    case 2:  // Required Time
      return time_units->asString(node.required_);
    case 3:  // Arrival Time
      return time_units->asString(node.arrival_);
    case 4:  // Slack
      return time_units->asString(node.slack_);
    case 5:  // Slew
      return time_units->asString(node.slew_);
    case 6:  // Load
      return cap_units->asString(node.load_);
  }
  return QVariant();
}

QVariant TimingPathDetailModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    if (section <= 1)
      return QString(
          TimingPathDetailModel::_path_details_columns[section].c_str());
    sta::dbSta* sta = ord::OpenRoad::openRoad()->getSta();
    auto time_units = sta->search()->units()->timeUnit();
    auto cap_units = sta->search()->units()->capacitanceUnit();
    if (section > 1 && section < 6)
      return QString(
                 TimingPathDetailModel::_path_details_columns[section].c_str())
             + QString(" (") + QString(time_units->suffix()) + QString(")");
    return QString(
               TimingPathDetailModel::_path_details_columns[section].c_str())
           + QString(" (") + QString(cap_units->suffix()) + QString(")");
  }
  return QVariant();
}

void TimingPathDetailModel::populateModel(TimingPath* path)
{
  beginResetModel();
  path_ = path;
  endResetModel();
}

TimingPathRenderer::TimingPathRenderer() : path_(nullptr)
{
  static bool init_once = false;
  if (!init_once) {
    init_once = true;
    TimingPathRenderer::inst_highlight_color_.a = 100;
    TimingPathRenderer::path_inst_color_.a = 100;
  }
}

TimingPathRenderer::~TimingPathRenderer()
{
  // delete path_;
}

void TimingPathRenderer::highlight(TimingPath* path)
{
  path_ = path;
  highlight_inst_nodes_.clear();
}

void TimingPathRenderer::highlightInstNode(TimingPathNode node)
{
  if (node.pin_->getObjectType() == odb::dbObjectType::dbITermObj) {
    odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(node.pin_);
    odb::dbInst* db_inst = db_iterm->getInst();
    highlight_inst_nodes_.push_back(db_inst);
  }
}

void TimingPathRenderer::drawObjects(gui::Painter& painter)
{
  // Native Implementation of Path Tracing
  // drawObjectsNative(painter);
  // return;
  sta::dbSta* sta = ord::OpenRoad::openRoad()->getSta();
  if (path_) {
    sta::dbNetwork* network = sta->getDbNetwork();
    odb::Point prev_pt;
    auto path_exp = path_->getPathExp();
    for (int i = 0; i < path_exp->size(); i++) {
      sta::PathRef* path = path_exp->path(i);
      sta::TimingArc* prev_arc = path_exp->prevArc(i);
      // Draw lines for wires on the path.
      if (prev_arc && prev_arc->role()->isWire()) {
        sta::PathRef* prev_path = path_exp->path(i - 1);
        const sta::Pin* pin = path->pin(sta);
        const sta::Pin* prev_pin = prev_path->pin(sta);
        odb::Point pt1 = network->location(pin);
        odb::Point pt2 = network->location(prev_pin);
        gui::Painter::Color wire_color
            = sta->isClock(pin) ? TimingPathRenderer::clock_color_
                                : TimingPathRenderer::signal_color_;
        painter.setPen(wire_color, true);
        painter.drawLine(pt1, pt2);
        odb::dbITerm* term = nullptr;
        odb::dbBTerm* port = nullptr;
        sta->getDbNetwork()->staToDb(prev_pin, term, port);
        if (term) {
          highlightInst(
              term->getInst(), painter, TimingPathRenderer::path_inst_color_);
        }
        if (i == path_exp->size() - 1) {
          term = nullptr;
          port = nullptr;
          sta->getDbNetwork()->staToDb(pin, term, port);
          if (term)
            highlightInst(
                term->getInst(), painter, TimingPathRenderer::path_inst_color_);
        }
      }
    }
  }
}

void TimingPathRenderer::drawObjectsNative(gui::Painter& painter)
{
  if (path_) {
    sta::dbSta* sta = ord::OpenRoad::openRoad()->getSta();
    odb::dbObject* sink_node = nullptr;
    odb::dbNet* net = nullptr;
    int node_count = path_->levelsCount();
    for (int i = node_count - 1; i >= 0; --i) {
      auto node = path_->getNodeAt(i);
      if (node.pin_->getObjectType() == odb::dbObjectType::dbITermObj) {
        odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(node.pin_);
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
          highlightNet(net, db_iterm /*source*/, sink_node, painter);
          sink_node = nullptr;
          net = nullptr;
        } else {
          // Got an invalid path Seg
        }
      } else {
        odb::dbBTerm* bterm = static_cast<odb::dbBTerm*>(node.pin_);
        highlightTerm(bterm, painter);
        if (!sink_node
            && (bterm->getIoType() == odb::dbIoType::OUTPUT
                || bterm->getIoType() == odb::dbIoType::INOUT)) {
          sink_node = bterm;
          net = bterm->getNet();
          continue;
        } else if (sink_node) {
          highlightNet(net, bterm, sink_node, painter);
          sink_node = nullptr;
          net = nullptr;
        } else {
          // Got an invalid Path Seg
        }
      }
    }

    for (auto& inst : highlight_inst_nodes_)
      highlightInst(inst, painter, TimingPathRenderer::inst_highlight_color_);
  }
}

// Color in the instances to make them more visible.
void TimingPathRenderer::highlightInst(odb::dbInst* db_inst,
                                       gui::Painter& painter,
                                       const gui::Painter::Color& inst_color)
{
  odb::dbBox* bbox = db_inst->getBBox();
  odb::Rect rect;
  bbox->getBox(rect);
  painter.setBrush(inst_color);
  painter.drawRect(rect);
}

void TimingPathRenderer::highlightTerm(odb::dbBTerm* term,
                                       gui::Painter& painter)
{
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
                                      gui::Painter& painter)
{
  int src_x, src_y;
  int dst_x, dst_y;
  auto getSegmentEnd = [](odb::dbObject* node, int& x, int& y, bool& clk_node) {
    if (node->getObjectType() == odb::dbObjectType::dbITermObj) {
      odb::dbITerm* db_iterm = static_cast<odb::dbITerm*>(node);
      db_iterm->getAvgXY(&x, &y);
    } else {
      odb::dbBTerm* bterm = static_cast<odb::dbBTerm*>(node);
      bterm->getFirstPinLocation(x, y);
      clk_node = (bterm->getSigType() == odb::dbSigType::CLOCK);
    }
  };
  // SigType is not populated properly in OpenDB
  bool clk_node = false;

  getSegmentEnd(source_node, src_x, src_y, clk_node);
  getSegmentEnd(sink_node, dst_x, dst_y, clk_node);

  if (clk_node == false)
    clk_node = (net->getSigType() == odb::dbSigType::CLOCK);
  odb::Point pt1(src_x, src_y);
  odb::Point pt2(dst_x, dst_y);

  gui::Painter::Color wire_color = clk_node == true
                                       ? TimingPathRenderer::clock_color_
                                       : TimingPathRenderer::signal_color_;
  painter.setPen(wire_color, true);
  painter.drawLine(pt1, pt2);
}

}  // namespace gui
