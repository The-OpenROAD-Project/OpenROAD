///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, OpenROAD
// All rights reserved.
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

#include <iomanip>
#include <sstream>

#include "db.h"
#include "dbDescriptors.h"
#include "dbShape.h"

namespace gui {

std::string DbInstDescriptor::getName(std::any object) const
{
  return std::any_cast<odb::dbInst*>(object)->getName();
}

std::string DbInstDescriptor::getTypeName(std::any object) const
{
  return "Inst";
}

std::string DbInstDescriptor::getLocation(std::any object) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  auto block = inst->getDb()->getChip()->getBlock();
  double to_microns = block->getDbUnitsPerMicron();
  auto inst_bbox = inst->getBBox();
  auto placement_status = inst->getPlacementStatus();
  auto inst_orient = inst->getOrient().getString();
  std::stringstream ss;
  if (placement_status.isPlaced()) {
    ss << std::fixed << std::setprecision(5) << "[("
       << inst_bbox->xMin() / to_microns << ","
       << inst_bbox->yMin() / to_microns << "), ("
       << inst_bbox->xMax() / to_microns << ","
       << inst_bbox->yMax() / to_microns << ")], " << inst_orient << ": "
       << placement_status.getString();
  } else {
    ss << placement_status.getString();
  }
  return ss.str();
}

bool DbInstDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  inst->getBBox()->getBox(bbox);
  return true;
}

void DbInstDescriptor::highlight(std::any object,
                                 Painter& painter,
                                 bool select_flag,
                                 int highlight_group,
                                 void* additional_data) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  if (!inst->getPlacementStatus().isPlaced()) {
    return;
  }

  auto highlight_color = Painter::persistHighlight;
  if (select_flag == true) {
    painter.setPen(Painter::highlight, true);
    painter.setBrush(Painter::transparent);
  } else {
    if (highlight_group >= 0 && highlight_group < 7) {
      highlight_color = Painter::highlightColors[highlight_group];
      highlight_color.a = 100;
      painter.setPen(highlight_color, true);
      painter.setBrush(highlight_color);
    } else
      painter.setPen(Painter::persistHighlight);
  }
  odb::dbBox* bbox = inst->getBBox();
  odb::Rect rect;
  bbox->getBox(rect);
  painter.drawRect(rect);
}

bool DbInstDescriptor::isInst(std::any object) const
{
  return true;
}

bool DbInstDescriptor::isNet(std::any object) const
{
  return false;
}

Descriptor::Properties DbInstDescriptor::getProperties(std::any object) const
{
  auto inst = std::any_cast<odb::dbInst*>(object);
  auto placed = inst->getPlacementStatus();
  Properties props({{"Master", inst->getMaster()->getConstName()},
                    {"Placement status", placed.getString()},
                    {"Source type", inst->getSourceType().getString()}});
  if (placed.isPlaced()) {
    int x, y;
    inst->getOrigin(x, y);
    double dbuPerUU = inst->getBlock()->getDbUnitsPerMicron();
    double user_x = x / dbuPerUU;
    double user_y = y / dbuPerUU;
    props.insert(props.end(),
                 {{"Orientation", inst->getOrient().getString()},
                  {"X", std::to_string(user_x)},
                  {"Y", std::to_string(user_y)}});
  }
  SelectionSet iterms;
  auto gui = Gui::get();
  for (auto iterm : inst->getITerms()) {
    iterms.insert(gui->makeSelected(iterm));
  }
  props.push_back({"ITerms", iterms});
  return props;
}

Selected DbInstDescriptor::makeSelected(std::any object,
                                        void* additional_data) const
{
  if (auto inst = std::any_cast<odb::dbInst*>(&object)) {
    return Selected(*inst, this, additional_data);
  }
  return Selected();
}

bool DbInstDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_inst = std::any_cast<odb::dbInst*>(l);
  auto r_inst = std::any_cast<odb::dbInst*>(r);
  return l_inst->getId() < r_inst->getId();
}

//////////////////////////////////////////////////
std::string DbNetDescriptor::getName(std::any object) const
{
  return std::any_cast<odb::dbNet*>(object)->getName();
}

std::string DbNetDescriptor::getTypeName(std::any object) const
{
  return "Net";
}

std::string DbNetDescriptor::getLocation(std::any object) const
{
  auto net = std::any_cast<odb::dbNet*>(object);
  auto block = net->getDb()->getChip()->getBlock();
  double to_microns = block->getDbUnitsPerMicron();
  auto wire = net->getWire();
  odb::Rect wire_bbox;
  if (wire && wire->getBBox(wire_bbox)) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(5) << "[("
       << wire_bbox.xMin() / to_microns << "," << wire_bbox.yMin() / to_microns
       << "), (" << wire_bbox.xMax() / to_microns << ","
       << wire_bbox.yMax() / to_microns << ")]";
    return ss.str();
  }
  return std::string("NA");
}

bool DbNetDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto net = std::any_cast<odb::dbNet*>(object);
  auto wire = net->getWire();
  if (wire && wire->getBBox(bbox)) {
    return true;
  }
  return false;
}

void DbNetDescriptor::highlight(std::any object,
                                Painter& painter,
                                bool select_flag,
                                int highlight_group,
                                void* additional_data) const
{
  auto highlight_color = Painter::persistHighlight;
  if (select_flag == true) {
    painter.setPen(Painter::highlight, true);
    painter.setBrush(Painter::transparent);
  } else {
    if (highlight_group >= 0 && highlight_group < 7) {
      highlight_color = Painter::highlightColors[highlight_group];
      highlight_color.a = 100;
      painter.setPen(highlight_color, true);
      painter.setBrush(highlight_color);
    } else
      painter.setPen(Painter::persistHighlight);
  }
  odb::dbObject* sink_object = nullptr;
  if (additional_data != nullptr)
    sink_object = static_cast<odb::dbObject*>(additional_data);
  auto net = std::any_cast<odb::dbNet*>(object);

  // Draw regular routing
  odb::Rect rect;
  odb::dbWire* wire = net->getWire();
  if (wire) {
    odb::dbWireShapeItr it;
    it.begin(wire);
    odb::dbShape shape;
    while (it.next(shape)) {
      shape.getBox(rect);
      painter.drawRect(rect);
    }
  } else if (!net->getSigType().isSupply()) {
    std::set<odb::Point> driver_locs;
    std::set<odb::Point> sink_locs;
    for (auto inst_term : net->getITerms()) {
      odb::Point rect_center;
      int x, y;
      if (!inst_term->getAvgXY(&x, &y)) {
        auto inst_term_inst = inst_term->getInst();
        odb::dbBox* bbox = inst_term_inst->getBBox();
        odb::Rect rect;
        bbox->getBox(rect);
        rect_center = odb::Point((rect.xMax() + rect.xMin()) / 2.0,
                                 (rect.yMax() + rect.yMin()) / 2.0);
      } else
        rect_center = odb::Point(x, y);
      auto iotype = inst_term->getIoType();
      if (iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT) {
        if (sink_object != nullptr && sink_object != inst_term)
          continue;
        sink_locs.insert(rect_center);
      }
      if (iotype == odb::dbIoType::INOUT || iotype == odb::dbIoType::OUTPUT)
        driver_locs.insert(rect_center);
    }
    for (auto blk_term : net->getBTerms()) {
      auto blk_term_pins = blk_term->getBPins();
      auto iotype = blk_term->getIoType();
      bool driver_term = iotype == odb::dbIoType::INPUT
                         || iotype == odb::dbIoType::INOUT
                         || iotype == odb::dbIoType::FEEDTHRU;
      bool sink_term = iotype == odb::dbIoType::INOUT
                       || iotype == odb::dbIoType::OUTPUT
                       || iotype == odb::dbIoType::FEEDTHRU;
      for (auto pin : blk_term_pins) {
        auto pin_rect = pin->getBBox();
        odb::Point rect_center((pin_rect.xMax() + pin_rect.xMin()) / 2.0,
                               (pin_rect.yMax() + pin_rect.yMin()) / 2.0);
        if (driver_term == true)
          driver_locs.insert(rect_center);
        if (sink_term)
          sink_locs.insert(rect_center);
      }
    }

    if (driver_locs.empty() || sink_locs.empty())
      return;
    highlight_color.a = 255;
    painter.setPen(highlight_color, true);
    painter.setBrush(highlight_color);
    for (auto& driver : driver_locs) {
      for (auto& sink : sink_locs) {
        painter.drawLine(driver, sink);
      }
    }
  }

  // Draw special (i.e. geometric) routing
  for (auto swire : net->getSWires()) {
    for (auto sbox : swire->getWires()) {
      sbox->getBox(rect);
      painter.drawGeomShape(sbox->getGeomShape());
    }
  }
}

bool DbNetDescriptor::isInst(std::any object) const
{
  return false;
}

bool DbNetDescriptor::isNet(std::any object) const
{
  return true;
}

Descriptor::Properties DbNetDescriptor::getProperties(std::any object) const
{
  auto net = std::any_cast<odb::dbNet*>(object);
  Properties props({{"Signal type", net->getSigType().getString()},
                    {"Source type", net->getSourceType().getString()},
                    {"Wire type", net->getWireType().getString()}});
  auto gui = Gui::get();
  SelectionSet iterms;
  for (auto iterm : net->getITerms()) {
    iterms.insert(gui->makeSelected(iterm));
  }
  props.push_back({"ITerms", iterms});
  SelectionSet bterms;
  for (auto bterm : net->getBTerms()) {
    bterms.insert(gui->makeSelected(bterm));
  }
  props.push_back({"BTerms", bterms});
  return props;
}

Selected DbNetDescriptor::makeSelected(std::any object,
                                       void* additional_data) const
{
  if (auto net = std::any_cast<odb::dbNet*>(&object)) {
    return Selected(*net, this, additional_data);
  }
  return Selected();
}

bool DbNetDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_net = std::any_cast<odb::dbNet*>(l);
  auto r_net = std::any_cast<odb::dbNet*>(r);
  return l_net->getId() < r_net->getId();
}

//////////////////////////////////////////////////

std::string DbITermDescriptor::getName(std::any object) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  return iterm->getInst()->getName() + '/' + iterm->getMTerm()->getName();
}

std::string DbITermDescriptor::getTypeName(std::any object) const
{
  return "ITerm";
}

std::string DbITermDescriptor::getLocation(std::any object) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  auto block = iterm->getDb()->getChip()->getBlock();
  if (iterm->getInst()->getPlacementStatus().isPlaced()) {
    double to_microns = block->getDbUnitsPerMicron();
    int x, y;
    iterm->getAvgXY(&x, &y);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(5);
    ss << "(" << (x / to_microns) << ", " << (y / to_microns) << ")";
    return ss.str();
  } else {
    return "<none>";
  }
}

bool DbITermDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  if (iterm->getInst()->getPlacementStatus().isPlaced()) {
    bbox = iterm->getBBox();
    return true;
  }
  return false;
}

void DbITermDescriptor::highlight(std::any object,
                                  Painter& painter,
                                  bool select_flag,
                                  int highlight_group,
                                  void* additional_data) const
{
  auto highlight_color = Painter::persistHighlight;
  if (select_flag == true) {
    painter.setPen(Painter::highlight, true);
    painter.setBrush(Painter::transparent);
  } else {
    if (highlight_group >= 0 && highlight_group < 7) {
      highlight_color = Painter::highlightColors[highlight_group];
      highlight_color.a = 100;
      painter.setPen(highlight_color, true);
      painter.setBrush(highlight_color);
    } else
      painter.setPen(Painter::persistHighlight);
  }
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  odb::dbTransform inst_xfm;
  iterm->getInst()->getTransform(inst_xfm);

  auto mterm = iterm->getMTerm();
  for (auto mpin : mterm->getMPins()) {
    for (auto box : mpin->getGeometry()) {
      odb::Rect rect;
      box->getBox(rect);
      inst_xfm.apply(rect);
      painter.drawRect(rect);
    }
  }
}

bool DbITermDescriptor::isInst(std::any object) const
{
  return false;
}

bool DbITermDescriptor::isNet(std::any object) const
{
  return false;
}

Descriptor::Properties DbITermDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto iterm = std::any_cast<odb::dbITerm*>(object);
  return Properties({{"Instance", gui->makeSelected(iterm->getInst())},
                     {"Net", gui->makeSelected(iterm->getNet())},
                     {"MTerm", iterm->getMTerm()->getConstName()}});
}

Selected DbITermDescriptor::makeSelected(std::any object,
                                         void* additional_data) const
{
  if (auto iterm = std::any_cast<odb::dbITerm*>(&object)) {
    return Selected(*iterm, this, additional_data);
  }
  return Selected();
}

bool DbITermDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_iterm = std::any_cast<odb::dbITerm*>(l);
  auto r_iterm = std::any_cast<odb::dbITerm*>(r);
  return l_iterm->getId() < r_iterm->getId();
}

//////////////////////////////////////////////////

std::string DbBTermDescriptor::getName(std::any object) const
{
  return std::any_cast<odb::dbBTerm*>(object)->getName();
}

std::string DbBTermDescriptor::getTypeName(std::any object) const
{
  return "BTerm";
}

std::string DbBTermDescriptor::getLocation(std::any object) const
{
  auto* bterm = std::any_cast<odb::dbBTerm*>(object);
  int x, y;
  if (bterm->getFirstPinLocation(x, y)) {
    auto block = bterm->getDb()->getChip()->getBlock();
    double to_microns = block->getDbUnitsPerMicron();
    std::stringstream ss;
    ss << std::fixed << std::setprecision(5);
    ss << "(" << (x / to_microns) << ", " << (y / to_microns) << ")";
    return ss.str();
  } else {
    return "<none>";
  }
}

bool DbBTermDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto* bterm = std::any_cast<odb::dbBTerm*>(object);
  bbox = bterm->getBBox();
  return true;
}

void DbBTermDescriptor::highlight(std::any object,
                                  Painter& painter,
                                  bool select_flag,
                                  int highlight_group,
                                  void* additional_data) const
{
  auto highlight_color = Painter::persistHighlight;
  if (select_flag == true) {
    painter.setPen(Painter::highlight, true);
    painter.setBrush(Painter::transparent);
  } else {
    if (highlight_group >= 0 && highlight_group < 7) {
      highlight_color = Painter::highlightColors[highlight_group];
      highlight_color.a = 100;
      painter.setPen(highlight_color, true);
      painter.setBrush(highlight_color);
    } else
      painter.setPen(Painter::persistHighlight);
  }
  auto* bterm = std::any_cast<odb::dbBTerm*>(object);
  for (auto bpin : bterm->getBPins()) {
    for (auto box : bpin->getBoxes()) {
      odb::Rect rect;
      box->getBox(rect);
      painter.drawRect(rect);
    }
  }
}

bool DbBTermDescriptor::isInst(std::any object) const
{
  return false;
}

bool DbBTermDescriptor::isNet(std::any object) const
{
  return false;
}

Descriptor::Properties DbBTermDescriptor::getProperties(std::any object) const
{
  auto gui = Gui::get();
  auto bterm = std::any_cast<odb::dbBTerm*>(object);
  return Properties({{"Net", gui->makeSelected(bterm->getNet())},
                     {"Signal type", bterm->getSigType().getString()},
                     {"IO type", bterm->getIoType().getString()}});
}

Selected DbBTermDescriptor::makeSelected(std::any object,
                                         void* additional_data) const
{
  if (auto bterm = std::any_cast<odb::dbBTerm*>(&object)) {
    return Selected(*bterm, this, additional_data);
  }
  return Selected();
}

bool DbBTermDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_bterm = std::any_cast<odb::dbBTerm*>(l);
  auto r_bterm = std::any_cast<odb::dbBTerm*>(r);
  return l_bterm->getId() < r_bterm->getId();
}

}  // namespace gui
