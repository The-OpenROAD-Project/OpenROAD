/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#include "RepairAntennas.h"

#include <algorithm>
#include <limits>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "Net.h"
#include "Pin.h"
#include "grt/GlobalRouter.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

RepairAntennas::RepairAntennas(GlobalRouter* grouter,
                               ant::AntennaChecker* arc,
                               dpl::Opendp* opendp,
                               odb::dbDatabase* db,
                               utl::Logger* logger)
    : grouter_(grouter),
      arc_(arc),
      opendp_(opendp),
      db_(db),
      logger_(logger),
      unique_diode_index_(1),
      illegal_diode_placement_count_(0)
{
  block_ = db_->getChip()->getBlock();
}

bool RepairAntennas::checkAntennaViolations(NetRouteMap& routing,
                                            int max_routing_layer,
                                            odb::dbMTerm* diode_mterm,
                                            float ratio_margin)
{
  makeNetWires(routing, max_routing_layer);
  arc_->initAntennaRules();
  for (auto& [db_net, route] : routing) {
    if (db_net->getWire()) {
      std::vector<ant::Violation> net_violations
          = arc_->getAntennaViolations(db_net, diode_mterm, ratio_margin);
      if (!net_violations.empty()) {
        antenna_violations_[db_net] = std::move(net_violations);
        debugPrint(logger_,
                   GRT,
                   "repair_antennas",
                   1,
                   "antenna violations {}",
                   db_net->getConstName());
      }
    }
  }
  destroyNetWires();

  logger_->info(
      GRT, 12, "Found {} antenna violations.", antenna_violations_.size());
  return !antenna_violations_.empty();
}

void RepairAntennas::makeNetWires(NetRouteMap& routing, int max_routing_layer)
{
  std::map<int, odb::dbTechVia*> default_vias
      = grouter_->getDefaultVias(max_routing_layer);

  for (auto& [db_net, route] : routing) {
    if (!grouter_->getNet(db_net)->isLocal()) {
      makeNetWire(db_net, route, default_vias);
    }
  }
}

odb::dbWire* RepairAntennas::makeNetWire(
    odb::dbNet* db_net,
    GRoute& route,
    std::map<int, odb::dbTechVia*>& default_vias)
{
  odb::dbWire* wire = odb::dbWire::create(db_net);
  if (wire) {
    Net* net = grouter_->getNet(db_net);
    odb::dbTech* tech = db_->getTech();
    odb::dbWireEncoder wire_encoder;
    wire_encoder.begin(wire);
    RoutePtPins route_pt_pins = findRoutePtPins(net);
    std::unordered_set<GSegment, GSegmentHash> wire_segments;
    int prev_conn_layer = -1;
    for (GSegment& seg : route) {
      int l1 = seg.init_layer;
      int l2 = seg.final_layer;
      auto [bottom_layer, top_layer] = std::minmax(l1, l2);

      odb::dbTechLayer* bottom_tech_layer
          = tech->findRoutingLayer(bottom_layer);
      odb::dbTechLayer* top_tech_layer = tech->findRoutingLayer(top_layer);

      if (std::abs(seg.init_layer - seg.final_layer) > 1) {
        debugPrint(logger_,
                   GRT,
                   "check_antennas",
                   1,
                   "invalid seg: ({}, {})um to ({}, {})um",
                   grouter_->dbuToMicrons(seg.init_x),
                   grouter_->dbuToMicrons(seg.init_y),
                   grouter_->dbuToMicrons(seg.final_x),
                   grouter_->dbuToMicrons(seg.final_y));

        logger_->error(GRT,
                       68,
                       "Global route segment for net {} not "
                       "valid. The layers {} and {} "
                       "are not adjacent.",
                       net->getName(),
                       bottom_tech_layer->getName(),
                       top_tech_layer->getName());
      }
      if (wire_segments.find(seg) == wire_segments.end()) {
        int x1 = seg.init_x;
        int y1 = seg.init_y;

        if (seg.isVia()) {
          if (bottom_layer >= grouter_->getMinRoutingLayer()) {
            if (bottom_layer == prev_conn_layer) {
              wire_encoder.newPath(bottom_tech_layer, odb::dbWireType::ROUTED);
              wire_encoder.addPoint(x1, y1);
              wire_encoder.addTechVia(default_vias[bottom_layer]);
              prev_conn_layer = std::max(l1, l2);
            } else if (top_layer == prev_conn_layer) {
              wire_encoder.newPath(top_tech_layer, odb::dbWireType::ROUTED);
              wire_encoder.addPoint(x1, y1);
              wire_encoder.addTechVia(default_vias[bottom_layer]);
              prev_conn_layer = std::min(l1, l2);
            }
            addWireTerms(net,
                         route,
                         x1,
                         y1,
                         bottom_layer,
                         bottom_tech_layer,
                         route_pt_pins,
                         wire_encoder,
                         default_vias,
                         false);
            wire_segments.insert(seg);
          }
        } else {
          // Add wire
          int x2 = seg.final_x;
          int y2 = seg.final_y;
          if (x1 != x2 || y1 != y2) {
            odb::dbTechLayer* tech_layer = tech->findRoutingLayer(l1);
            wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
            wire_encoder.addPoint(x1, y1);
            wire_encoder.addPoint(x2, y2);
            addWireTerms(net,
                         route,
                         x1,
                         y1,
                         l1,
                         tech_layer,
                         route_pt_pins,
                         wire_encoder,
                         default_vias,
                         true);
            addWireTerms(net,
                         route,
                         x2,
                         y2,
                         l1,
                         tech_layer,
                         route_pt_pins,
                         wire_encoder,
                         default_vias,
                         true);
            wire_segments.insert(seg);
            prev_conn_layer = l1;
          }
        }
      }
    }
    wire_encoder.end();

    odb::orderWires(logger_, db_net);
    return wire;
  } else {
    logger_->error(
        GRT, 221, "Cannot create wire for net {}.", db_net->getConstName());
    // suppress gcc warning
    return nullptr;
  }
}

RoutePtPins RepairAntennas::findRoutePtPins(Net* net)
{
  RoutePtPins route_pt_pins;
  for (Pin& pin : net->getPins()) {
    int conn_layer = pin.getConnectionLayer();
    odb::Point grid_pt = pin.getOnGridPosition();
    route_pt_pins[RoutePt(grid_pt.x(), grid_pt.y(), conn_layer)].push_back(
        &pin);
  }
  return route_pt_pins;
}

void RepairAntennas::addWireTerms(Net* net,
                                  GRoute& route,
                                  int grid_x,
                                  int grid_y,
                                  int layer,
                                  odb::dbTechLayer* tech_layer,
                                  RoutePtPins& route_pt_pins,
                                  odb::dbWireEncoder& wire_encoder,
                                  std::map<int, odb::dbTechVia*>& default_vias,
                                  bool connect_to_segment)
{
  if (layer == grouter_->getMinRoutingLayer()) {
    layer--;
  }
  auto itr = route_pt_pins.find(RoutePt(grid_x, grid_y, layer));
  if (itr != route_pt_pins.end()) {
    for (const Pin* pin : itr->second) {
      int conn_layer = pin->getConnectionLayer();
      std::vector<odb::Rect> pin_boxes = pin->getBoxes().at(conn_layer);
      odb::Point grid_pt = pin->getOnGridPosition();
      odb::Point pin_pt = grid_pt;
      // create the local connection with the pin center only when the global
      // segment doesn't overlap the pin
      if (!pinOverlapsGSegment(grid_pt, conn_layer, pin_boxes, route)) {
        int min_dist = std::numeric_limits<int>::max();
        for (const odb::Rect& pin_box : pin_boxes) {
          odb::Point pos = grouter_->getRectMiddle(pin_box);
          int dist = odb::Point::manhattanDistance(pos, pin_pt);
          if (dist < min_dist) {
            min_dist = dist;
            pin_pt = pos;
          }
        }
      }

      if (conn_layer >= grouter_->getMinRoutingLayer()) {
        wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
        wire_encoder.addPoint(grid_pt.x(), grid_pt.y());
        wire_encoder.addPoint(pin_pt.x(), grid_pt.y());
        wire_encoder.addPoint(pin_pt.x(), pin_pt.y());
      } else {
        odb::dbTech* tech = db_->getTech();
        odb::dbTechLayer* layer1
            = tech->findRoutingLayer(grouter_->getMinRoutingLayer());
        odb::dbTechLayer* layer2
            = tech->findRoutingLayer(grouter_->getMinRoutingLayer() + 1);
        odb::dbTechLayer* h_layer
            = layer1->getDirection() == odb::dbTechLayerDir::HORIZONTAL
                  ? layer1
                  : layer2;
        odb::dbTechLayer* v_layer
            = layer1->getDirection() == odb::dbTechLayerDir::VERTICAL ? layer1
                                                                      : layer2;
        // create horizontal wire to connect to the pin
        if (connect_to_segment && tech_layer != h_layer) {
          // if wire to pin connects to a segment in a different layer, create a
          // via to connect both wires
          wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
          wire_encoder.addPoint(grid_pt.x(), grid_pt.y());
          wire_encoder.addTechVia(default_vias[grouter_->getMinRoutingLayer()]);
        }
        wire_encoder.newPath(h_layer, odb::dbWireType::ROUTED);
        wire_encoder.addPoint(grid_pt.x(), grid_pt.y());
        wire_encoder.addPoint(pin_pt.x(), grid_pt.y());
        wire_encoder.addTechVia(default_vias[grouter_->getMinRoutingLayer()]);

        // create vertical wire to connect to the pin
        wire_encoder.newPath(v_layer, odb::dbWireType::ROUTED);
        wire_encoder.addPoint(pin_pt.x(), grid_pt.y());
        wire_encoder.addPoint(pin_pt.x(), pin_pt.y());

        // create vias to reach the pin
        for (int i = v_layer->getRoutingLevel() - 1; i >= conn_layer; i--) {
          wire_encoder.addTechVia(default_vias[i]);
        }
      }
    }
  }
}

bool RepairAntennas::pinOverlapsGSegment(
    const odb::Point& pin_position,
    const int pin_layer,
    const std::vector<odb::Rect>& pin_boxes,
    const GRoute& route)
{
  // check if pin position on grid overlaps with the pin shape
  for (const odb::Rect& box : pin_boxes) {
    if (box.overlaps(pin_position)) {
      return true;
    }
  }

  // check if pin position on grid overlaps with at least one GSegment
  for (const odb::Rect& box : pin_boxes) {
    for (const GSegment& seg : route) {
      if (seg.init_layer == seg.final_layer &&  // ignore vias
          seg.init_layer == pin_layer) {
        int x0 = std::min(seg.init_x, seg.final_x);
        int y0 = std::min(seg.init_y, seg.final_y);
        int x1 = std::max(seg.init_x, seg.final_x);
        int y1 = std::max(seg.init_y, seg.final_y);
        odb::Rect seg_rect(x0, y0, x1, y1);

        if (box.intersects(seg_rect)) {
          return true;
        }
      }
    }
  }

  return false;
}

void RepairAntennas::destroyNetWires()
{
  for (odb::dbNet* db_net : block_->getNets()) {
    odb::dbWire* wire = db_net->getWire();
    if (wire)
      odb::dbWire::destroy(wire);
  }
}

void RepairAntennas::repairAntennas(odb::dbMTerm* diode_mterm)
{
  int site_width = -1;
  r_tree fixed_insts;

  illegal_diode_placement_count_ = 0;
  diode_insts_.clear();

  auto rows = block_->getRows();
  for (odb::dbRow* db_row : rows) {
    odb::dbSite* site = db_row->getSite();
    if (site->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    int width = site->getWidth();
    if (site_width == -1) {
      site_width = width;
    }

    if (site_width != width) {
      logger_->warn(GRT, 27, "Design has rows with different site widths.");
    }
  }

  setInstsPlacementStatus(odb::dbPlacementStatus::FIRM);
  getFixedInstances(fixed_insts);

  bool repair_failures = false;
  for (auto const& net_violations : antenna_violations_) {
    odb::dbNet* db_net = net_violations.first;
    auto violations = net_violations.second;

    bool inserted_diodes = false;
    for (ant::Violation& violation : violations) {
      debugPrint(logger_,
                 GRT,
                 "repair_antennas",
                 2,
                 "antenna {} insert {} diodes",
                 db_net->getConstName(),
                 violation.diode_count_per_gate * violation.gates.size());
      if (violation.diode_count_per_gate > 0) {
        for (odb::dbITerm* gate : violation.gates) {
          odb::dbInst* sink_inst = gate->getInst();
          for (int j = 0; j < violation.diode_count_per_gate; j++) {
            insertDiode(
                db_net, diode_mterm, sink_inst, gate, site_width, fixed_insts);
            inserted_diodes = true;
          }
        }
      } else
        repair_failures = true;
    }
    if (inserted_diodes)
      grouter_->addDirtyNet(db_net);
  }
  if (repair_failures)
    logger_->warn(GRT, 243, "Unable to repair antennas on net with diodes.");
}

void RepairAntennas::legalizePlacedCells()
{
  opendp_->detailedPlacement(0, 0, "");
  // After legalize placement, diodes and violated insts don't need to be FIRM
  setInstsPlacementStatus(odb::dbPlacementStatus::PLACED);
}

void RepairAntennas::insertDiode(odb::dbNet* net,
                                 odb::dbMTerm* diode_mterm,
                                 odb::dbInst* sink_inst,
                                 odb::dbITerm* gate,
                                 int site_width,
                                 r_tree& fixed_insts)
{
  const int max_legalize_itr = 50;
  bool legally_placed = false;
  bool place_at_left = true;
  int left_offset = 0;
  int right_offset = 0;
  int offset;

  odb::dbMaster* diode_master = diode_mterm->getMaster();

  int inst_loc_x, inst_loc_y, inst_width;
  odb::Rect sink_bbox = getInstRect(sink_inst, gate);
  inst_loc_x = sink_bbox.xMin();
  inst_loc_y = sink_bbox.yMin();
  inst_width = sink_bbox.xMax() - sink_bbox.xMin();
  odb::dbOrientType inst_orient = sink_inst->getOrient();

  std::string diode_inst_name
      = "ANTENNA_" + std::to_string(unique_diode_index_++);
  odb::dbInst* diode_inst
      = odb::dbInst::create(block_, diode_master, diode_inst_name.c_str());
  odb::dbITerm* diode_iterm
      = diode_inst->findITerm(diode_mterm->getConstName());
  odb::dbBox* diode_bbox = diode_inst->getBBox();
  int diode_width = diode_bbox->xMax() - diode_bbox->xMin();

  odb::Rect core_area = block_->getCoreArea();

  // Use R-tree to check if diode will not overlap or cause 1-site spacing with
  // other cells
  std::vector<value> overlap_insts;
  int legalize_itr = 0;
  while (!legally_placed && legalize_itr < max_legalize_itr) {
    if (place_at_left) {
      offset = -(diode_width + left_offset * site_width);
      left_offset++;
      place_at_left = false;
    } else {
      offset = inst_width + right_offset * site_width;
      right_offset++;
      place_at_left = true;
    }

    const int left_pad = opendp_->padLeft(diode_inst);
    const int right_pad = opendp_->padRight(diode_inst);
    diode_inst->setOrient(inst_orient);
    if (sink_inst->isBlock() || sink_inst->isPad()) {
      odb::dbOrientType orient
          = getRowOrient(odb::Point(inst_loc_x + offset, inst_loc_y));
      diode_inst->setOrient(orient);
    }
    diode_inst->setLocation(inst_loc_x + offset, inst_loc_y);

    odb::dbBox* instBox = diode_inst->getBBox();
    box box(point(instBox->xMin() - ((left_pad + right_pad) * site_width) + 1,
                  instBox->yMin() + 1),
            point(instBox->xMax() + ((left_pad + right_pad) * site_width) - 1,
                  instBox->yMax() - 1));
    fixed_insts.query(bgi::intersects(box), std::back_inserter(overlap_insts));

    if (overlap_insts.empty() && instBox->xMin() >= core_area.xMin()
        && instBox->xMax() <= core_area.xMax()) {
      legally_placed = true;
    }
    overlap_insts.clear();
    legalize_itr++;
  }

  odb::Rect inst_rect = diode_inst->getBBox()->getBox();

  legally_placed = legally_placed && diodeInRow(inst_rect);

  if (!legally_placed)
    illegal_diode_placement_count_++;

  // allow detailed placement to move diodes with geometry out of the core area,
  // or near macro pins (can be placed out of row), or illegal placed diodes
  if (core_area.contains(inst_rect) && !sink_inst->getMaster()->isBlock()
      && legally_placed) {
    diode_inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  } else {
    diode_inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  diode_iterm->connect(net);
  diode_insts_.push_back(diode_inst);

  // Add diode to the R-tree of fixed instances
  int fixed_inst_id = fixed_insts.size();
  box b(point(inst_rect.xMin(), inst_rect.yMin()),
        point(inst_rect.xMax(), inst_rect.yMax()));
  value v(b, fixed_inst_id);
  fixed_insts.insert(v);
  fixed_inst_id++;
}

void RepairAntennas::getFixedInstances(r_tree& fixed_insts)
{
  int fixed_inst_id = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    odb::dbPlacementStatus status = inst->getPlacementStatus();
    if (status == odb::dbPlacementStatus::FIRM
        || status == odb::dbPlacementStatus::LOCKED) {
      odb::dbBox* instBox = inst->getBBox();
      box b(point(instBox->xMin(), instBox->yMin()),
            point(instBox->xMax(), instBox->yMax()));
      value v(b, fixed_inst_id);
      fixed_insts.insert(v);
      fixed_inst_id++;
    }
  }
}

void RepairAntennas::setInstsPlacementStatus(
    odb::dbPlacementStatus placement_status)
{
  for (auto const& violation : antenna_violations_) {
    for (int i = 0; i < violation.second.size(); i++) {
      for (odb::dbITerm* gate : violation.second[i].gates) {
        if (!gate->getMTerm()->getMaster()->isBlock()) {
          gate->getInst()->setPlacementStatus(placement_status);
        }
      }
    }
  }

  for (odb::dbInst* diode_inst : diode_insts_) {
    diode_inst->setPlacementStatus(placement_status);
  }
}

odb::Rect RepairAntennas::getInstRect(odb::dbInst* inst, odb::dbITerm* iterm)
{
  const odb::dbTransform transform = inst->getTransform();

  odb::Rect inst_rect;

  if (inst->getMaster()->isBlock()) {
    inst_rect.mergeInit();
    odb::dbMTerm* mterm = iterm->getMTerm();
    if (mterm != nullptr) {
      for (odb::dbMPin* mterm_pin : mterm->getMPins()) {
        for (odb::dbBox* mterm_box : mterm_pin->getGeometry()) {
          odb::Rect rect = mterm_box->getBox();
          transform.apply(rect);

          inst_rect = rect;
        }
      }
    }
  } else {
    inst_rect = inst->getBBox()->getBox();
  }

  return inst_rect;
}

bool RepairAntennas::diodeInRow(odb::Rect diode_rect)
{
  for (odb::dbRow* row : block_->getRows()) {
    odb::Rect row_rect = row->getBBox();

    if (row_rect.contains(diode_rect)) {
      return true;
    }
  }

  return false;
}

odb::dbOrientType RepairAntennas::getRowOrient(const odb::Point& point)
{
  odb::dbOrientType orient;
  for (odb::dbRow* row : block_->getRows()) {
    odb::Rect row_rect = row->getBBox();

    if (row_rect.overlaps(point)) {
      orient = row->getOrient();
    }
  }

  return orient;
}

odb::dbMTerm* RepairAntennas::findDiodeMTerm()
{
  for (odb::dbLib* lib : db_->getLibs()) {
    for (auto master : lib->getMasters()) {
      if (master->getType() == odb::dbMasterType::CORE_ANTENNACELL) {
        for (odb::dbMTerm* mterm : master->getMTerms()) {
          if (diffArea(mterm) > 0.0)
            return mterm;
        }
      }
    }
  }
  return nullptr;
}

// copied from AntennaChecker
double RepairAntennas::diffArea(odb::dbMTerm* mterm)
{
  double max_diff_area = 0.0;
  std::vector<std::pair<double, odb::dbTechLayer*>> diff_areas;
  mterm->getDiffArea(diff_areas);
  for (auto [diff_area, layer] : diff_areas) {
    max_diff_area = std::max(max_diff_area, diff_area);
  }
  return max_diff_area;
}

}  // namespace grt
