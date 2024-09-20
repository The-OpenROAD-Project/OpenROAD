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

#include <omp.h>

#include <algorithm>
#include <boost/pending/disjoint_sets.hpp>
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
      illegal_diode_placement_count_(0),
      routing_source_(RoutingSource::None)
{
  block_ = db_->getChip()->getBlock();
  while (block_->findInst(
      fmt::format("ANTENNA_{}", unique_diode_index_).c_str())) {
    unique_diode_index_++;
  }
}

bool RepairAntennas::checkAntennaViolations(
    NetRouteMap& routing,
    const std::vector<odb::dbNet*>& nets_to_repair,
    int max_routing_layer,
    odb::dbMTerm* diode_mterm,
    float ratio_margin,
    const int num_threads)
{
  antenna_violations_.clear();
  for (odb::dbNet* db_net : nets_to_repair) {
    antenna_violations_[db_net];
  }

  routing_source_ = grouter_->haveDetailedRoutes(nets_to_repair)
                        ? RoutingSource::DetailedRouting
                        : RoutingSource::GlobalRouting;
  bool destroy_wires = routing_source_ == RoutingSource::GlobalRouting;

  makeNetWires(routing, nets_to_repair, max_routing_layer);
  arc_->initAntennaRules();
  omp_set_num_threads(num_threads);
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < nets_to_repair.size(); i++) {
    odb::dbNet* db_net = nets_to_repair[i];
    checkNetViolations(db_net, diode_mterm, ratio_margin);
  }

  if (destroy_wires) {
    destroyNetWires(nets_to_repair);
  }

  // remove nets with zero violations
  for (auto it = antenna_violations_.begin();
       it != antenna_violations_.end();) {
    if (it->second.empty()) {
      it = antenna_violations_.erase(it);
    } else {
      ++it;
    }
  }

  logger_->info(
      GRT, 12, "Found {} antenna violations.", antenna_violations_.size());
  return !antenna_violations_.empty();
}

void RepairAntennas::checkNetViolations(odb::dbNet* db_net,
                                        odb::dbMTerm* diode_mterm,
                                        float ratio_margin)
{
  if (!db_net->isSpecial() && db_net->getWire()) {
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

void RepairAntennas::makeNetWires(
    NetRouteMap& routing,
    const std::vector<odb::dbNet*>& nets_to_repair,
    int max_routing_layer)
{
  std::map<int, odb::dbTechVia*> default_vias
      = grouter_->getDefaultVias(max_routing_layer);

  for (odb::dbNet* db_net : nets_to_repair) {
    if (!db_net->isSpecial() && !db_net->isConnectedByAbutment()
        && !grouter_->getNet(db_net)->isLocal()
        && !grouter_->isDetailedRouted(db_net)) {
      makeNetWire(db_net, routing[db_net], default_vias);
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
    RoutePtPinsMap route_pt_pins = findRoutePtPins(net);
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
                   block_->dbuToMicrons(seg.init_x),
                   block_->dbuToMicrons(seg.init_y),
                   block_->dbuToMicrons(seg.final_x),
                   block_->dbuToMicrons(seg.final_y));

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
              prev_conn_layer = std::max(l1, l2);
            } else if (top_layer == prev_conn_layer) {
              wire_encoder.newPath(top_tech_layer, odb::dbWireType::ROUTED);
              prev_conn_layer = std::min(l1, l2);
            } else {
              // if a via is the first object added to the wire_encoder, or the
              // via starts a new path and is not connected to previous wires
              // create a new path using the bottom layer and do not update the
              // prev_conn_layer. this way, this process is repeated until the
              // first wire is added and properly update the prev_conn_layer
              wire_encoder.newPath(bottom_tech_layer, odb::dbWireType::ROUTED);
            }
            wire_encoder.addPoint(x1, y1);
            wire_encoder.addTechVia(default_vias[bottom_layer]);
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
            wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
            wire_encoder.addPoint(x1, y1);
            wire_encoder.addPoint(x2, y2);
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

    return wire;
  } else {
    logger_->error(
        GRT, 221, "Cannot create wire for net {}.", db_net->getConstName());
    // suppress gcc warning
    return nullptr;
  }
}

RoutePtPinsMap RepairAntennas::findRoutePtPins(Net* net)
{
  RoutePtPinsMap route_pt_pins;
  for (Pin& pin : net->getPins()) {
    int conn_layer = pin.getConnectionLayer();
    odb::Point grid_pt = pin.getOnGridPosition();
    RoutePt route_pt(grid_pt.x(), grid_pt.y(), conn_layer);
    route_pt_pins[route_pt].pins.push_back(&pin);
    route_pt_pins[route_pt].connected = false;
  }
  return route_pt_pins;
}

void RepairAntennas::addWireTerms(Net* net,
                                  GRoute& route,
                                  int grid_x,
                                  int grid_y,
                                  int layer,
                                  odb::dbTechLayer* tech_layer,
                                  RoutePtPinsMap& route_pt_pins,
                                  odb::dbWireEncoder& wire_encoder,
                                  std::map<int, odb::dbTechVia*>& default_vias,
                                  bool connect_to_segment)
{
  std::vector<int> layers;
  layers.push_back(layer);
  if (layer == grouter_->getMinRoutingLayer()) {
    layer--;
    layers.push_back(layer);
  }

  for (int l : layers) {
    auto itr = route_pt_pins.find(RoutePt(grid_x, grid_y, l));
    if (itr != route_pt_pins.end() && !itr->second.connected) {
      for (const Pin* pin : itr->second.pins) {
        itr->second.connected = true;
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
          odb::dbTechLayer* min_layer
              = tech->findRoutingLayer(grouter_->getMinRoutingLayer());

          if (connect_to_segment && tech_layer != min_layer) {
            // create vias to connect the guide segment to the min routing
            // layer. the min routing layer will be used to connect to the pin.
            wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
            wire_encoder.addPoint(grid_pt.x(), grid_pt.y());
            for (int l = min_layer->getRoutingLevel();
                 l < tech_layer->getRoutingLevel();
                 l++) {
              wire_encoder.addTechVia(default_vias[l]);
            }
          }

          if (min_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
            makeWire(wire_encoder,
                     min_layer,
                     grid_pt,
                     odb::Point(grid_pt.x(), pin_pt.y()));
            wire_encoder.addTechVia(
                default_vias[grouter_->getMinRoutingLayer()]);
            makeWire(wire_encoder,
                     min_layer,
                     odb::Point(grid_pt.x(), pin_pt.y()),
                     pin_pt);
          } else {
            makeWire(wire_encoder,
                     min_layer,
                     grid_pt,
                     odb::Point(pin_pt.x(), grid_pt.y()));
            wire_encoder.addTechVia(
                default_vias[grouter_->getMinRoutingLayer()]);
            makeWire(wire_encoder,
                     min_layer,
                     odb::Point(pin_pt.x(), grid_pt.y()),
                     pin_pt);
          }

          // create vias to reach the pin
          for (int i = min_layer->getRoutingLevel() - 1; i >= conn_layer; i--) {
            wire_encoder.addTechVia(default_vias[i]);
          }
        }
      }
    }
  }
}

void RepairAntennas::makeWire(odb::dbWireEncoder& wire_encoder,
                              odb::dbTechLayer* layer,
                              const odb::Point& start,
                              const odb::Point& end)
{
  wire_encoder.newPath(layer, odb::dbWireType::ROUTED);
  wire_encoder.addPoint(start.x(), start.y());
  wire_encoder.addPoint(end.x(), end.y());
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

void RepairAntennas::destroyNetWires(
    const std::vector<odb::dbNet*>& nets_to_repair)
{
  for (odb::dbNet* db_net : nets_to_repair) {
    odb::dbWire* wire = db_net->getWire();
    if (wire) {
      odb::dbWire::destroy(wire);
    }
  }
}

void RepairAntennas::repairAntennas(odb::dbMTerm* diode_mterm)
{
  int site_width = -1;
  r_tree fixed_insts;
  odb::dbTech* tech = db_->getTech();

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

  std::vector<odb::dbInst*> insts_to_restore;
  if (routing_source_ == RoutingSource::DetailedRouting) {
    // set all other instances as fixed to prevent repair_antennas changing
    // other nets, prevent DRT rerouting nets unnecessarily
    setInstsPlacementStatus(insts_to_restore);
  }
  setDiodesAndGatesPlacementStatus(odb::dbPlacementStatus::FIRM);
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
          for (int j = 0; j < violation.diode_count_per_gate; j++) {
            odb::dbTechLayer* violation_layer
                = tech->findRoutingLayer(violation.routing_level);
            insertDiode(db_net,
                        diode_mterm,
                        gate,
                        site_width,
                        fixed_insts,
                        violation_layer);
            inserted_diodes = true;
          }
        }
      } else
        repair_failures = true;
    }
    if (inserted_diodes) {
      // if (db_net->hasJumpers()) {
      // db_net->setJumpers(false);
      //}
      grouter_->addDirtyNet(db_net);
    }
  }
  if (repair_failures) {
    logger_->warn(GRT, 243, "Unable to repair antennas on net with diodes.");
  }

  if (illegal_diode_placement_count_ > 0) {
    debugPrint(logger_,
               GRT,
               "repair_antennas",
               2,
               "using detailed placer to place {} diodes.",
               illegal_diode_placement_count_);
  }

  legalizePlacedCells();
  if (routing_source_ == RoutingSource::DetailedRouting) {
    // restore placement status of changed insts
    setInstsPlacementStatus(insts_to_restore);
  }
}

void RepairAntennas::legalizePlacedCells()
{
  opendp_->detailedPlacement(0, 0, "");
  // After legalize placement, diodes and violated insts don't need to be FIRM
  setDiodesAndGatesPlacementStatus(odb::dbPlacementStatus::PLACED);
}

void RepairAntennas::insertDiode(odb::dbNet* net,
                                 odb::dbMTerm* diode_mterm,
                                 odb::dbITerm* gate,
                                 int site_width,
                                 r_tree& fixed_insts,
                                 odb::dbTechLayer* violation_layer)
{
  odb::dbMaster* diode_master = diode_mterm->getMaster();
  std::string diode_inst_name
      = "ANTENNA_" + std::to_string(unique_diode_index_++);
  odb::dbInst* diode_inst
      = odb::dbInst::create(block_, diode_master, diode_inst_name.c_str());

  bool place_vertically
      = violation_layer->getDirection() == odb::dbTechLayerDir::VERTICAL;
  bool legally_placed = setDiodeLoc(
      diode_inst, gate, site_width, place_vertically, fixed_insts);

  odb::Rect inst_rect = diode_inst->getBBox()->getBox();

  legally_placed = legally_placed && diodeInRow(inst_rect);

  if (!legally_placed)
    illegal_diode_placement_count_++;

  // allow detailed placement to move diodes with geometry out of the core area,
  // or near macro pins (can be placed out of row), or illegal placed diodes
  const odb::Rect& core_area = block_->getCoreArea();
  odb::dbInst* sink_inst = gate->getInst();
  if (core_area.contains(inst_rect) && !sink_inst->getMaster()->isBlock()
      && legally_placed) {
    diode_inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  } else {
    diode_inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  odb::dbITerm* diode_iterm
      = diode_inst->findITerm(diode_mterm->getConstName());
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

void RepairAntennas::setDiodesAndGatesPlacementStatus(
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

void RepairAntennas::setInstsPlacementStatus(
    std::vector<odb::dbInst*>& insts_to_restore)
{
  if (insts_to_restore.empty()) {
    for (odb::dbInst* inst : block_->getInsts()) {
      if (inst->getPlacementStatus() == odb::dbPlacementStatus::PLACED) {
        inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
        insts_to_restore.push_back(inst);
      }
    }
  } else {
    for (odb::dbInst* inst : insts_to_restore) {
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
  }
}

bool RepairAntennas::setDiodeLoc(odb::dbInst* diode_inst,
                                 odb::dbITerm* gate,
                                 int site_width,
                                 const bool place_vertically,
                                 r_tree& fixed_insts)
{
  const int max_legalize_itr = 50;
  bool place_at_left = true;
  bool place_at_top = false;
  int left_offset = 0, right_offset = 0;
  int top_offset = 0, bottom_offset = 0;
  int horizontal_offset = 0, vertical_offset = 0;
  bool legally_placed = false;

  int inst_loc_x, inst_loc_y, inst_width, inst_height;
  odb::dbOrientType inst_orient;
  getInstancePlacementData(
      gate, inst_loc_x, inst_loc_y, inst_width, inst_height, inst_orient);

  odb::dbBox* diode_bbox = diode_inst->getBBox();
  int diode_width = diode_bbox->xMax() - diode_bbox->xMin();
  int diode_height = diode_bbox->yMax() - diode_bbox->yMin();
  odb::dbInst* sink_inst = gate->getInst();

  // Use R-tree to check if diode will not overlap or cause 1-site spacing with
  // other fixed cells
  int legalize_itr = 0;
  while (!legally_placed && legalize_itr < max_legalize_itr) {
    if (place_vertically) {
      computeVerticalOffset(inst_height,
                            top_offset,
                            bottom_offset,
                            place_at_top,
                            vertical_offset);
    } else {
      computeHorizontalOffset(diode_width,
                              inst_width,
                              site_width,
                              left_offset,
                              right_offset,
                              place_at_left,
                              horizontal_offset);
    }
    diode_inst->setOrient(inst_orient);
    if (sink_inst->isBlock() || sink_inst->isPad() || place_vertically) {
      int x_center = inst_loc_x + horizontal_offset + diode_width / 2;
      int y_center = inst_loc_y + vertical_offset + diode_height / 2;
      odb::Point diode_center(x_center, y_center);
      odb::dbOrientType orient = getRowOrient(diode_center);
      diode_inst->setOrient(orient);
    }
    diode_inst->setLocation(inst_loc_x + horizontal_offset,
                            inst_loc_y + vertical_offset);

    legally_placed = checkDiodeLoc(diode_inst, site_width, fixed_insts);
    legalize_itr++;
  }

  return legally_placed;
}

void RepairAntennas::getInstancePlacementData(odb::dbITerm* gate,
                                              int& inst_loc_x,
                                              int& inst_loc_y,
                                              int& inst_width,
                                              int& inst_height,
                                              odb::dbOrientType& inst_orient)
{
  odb::dbInst* sink_inst = gate->getInst();
  odb::Rect sink_bbox = getInstRect(sink_inst, gate);
  inst_loc_x = sink_bbox.xMin();
  inst_loc_y = sink_bbox.yMin();
  inst_width = sink_bbox.xMax() - sink_bbox.xMin();
  inst_height = sink_bbox.yMax() - sink_bbox.yMin();
  inst_orient = sink_inst->getOrient();
}

bool RepairAntennas::checkDiodeLoc(odb::dbInst* diode_inst,
                                   const int site_width,
                                   r_tree& fixed_insts)
{
  const odb::Rect& core_area = block_->getCoreArea();
  const int left_pad = opendp_->padLeft(diode_inst);
  const int right_pad = opendp_->padRight(diode_inst);
  odb::dbBox* instBox = diode_inst->getBBox();
  box box(point(instBox->xMin() - ((left_pad + right_pad) * site_width) + 1,
                instBox->yMin() + 1),
          point(instBox->xMax() + ((left_pad + right_pad) * site_width) - 1,
                instBox->yMax() - 1));

  std::vector<value> overlap_insts;
  fixed_insts.query(bgi::intersects(box), std::back_inserter(overlap_insts));

  return overlap_insts.empty() && core_area.contains(instBox->getBox());
}

void RepairAntennas::computeHorizontalOffset(const int diode_width,
                                             const int inst_width,
                                             const int site_width,
                                             int& left_offset,
                                             int& right_offset,
                                             bool& place_at_left,
                                             int& offset)
{
  if (place_at_left) {
    offset = -(diode_width + left_offset * site_width);
    left_offset++;
    place_at_left = false;
  } else {
    offset = inst_width + right_offset * site_width;
    right_offset++;
    place_at_left = true;
  }
}

void RepairAntennas::computeVerticalOffset(const int inst_height,
                                           int& top_offset,
                                           int& bottom_offset,
                                           bool& place_at_top,
                                           int& offset)
{
  if (place_at_top) {
    offset = top_offset * inst_height;
    top_offset++;
    place_at_top = false;
  } else {
    offset = -(bottom_offset * inst_height);
    bottom_offset++;
    place_at_top = true;
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
  int diode_height = diode_rect.dy();
  for (odb::dbRow* row : block_->getRows()) {
    odb::Rect row_rect = row->getBBox();
    int row_height = row_rect.dy();

    if (row_rect.contains(diode_rect) && diode_height == row_height) {
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

//////////////////////////////////

bool RepairAntennas::verifyCapacityForJumper(bool is_horizontal,
                                             const int& tile_size,
                                             const int& init_x,
                                             const int& init_y,
                                             const int& final_x,
                                             const int& final_y,
                                             const int& layer_level)
{
  int pos_x, pos_y;
  pos_x = init_x;
  pos_y = init_y;
  bool has_cap = true;
  while (pos_x <= final_x && pos_y <= final_y) {
    has_cap &= grouter_->hasCapacity(is_horizontal, pos_x, pos_y, layer_level);
    if (is_horizontal) {
      pos_x += tile_size;
    } else {
      pos_y += tile_size;
    }
  }
  return has_cap;
}

void addSegments(GRoute& route,
                 const int& init_x,
                 const int& init_y,
                 const int& final_x,
                 const int& final_y,
                 const int& layer_level)
{
  // create vias (2 by end)
  route.push_back(
      GSegment(init_x, init_y, layer_level, init_x, init_y, layer_level + 1));
  route.push_back(GSegment(
      init_x, init_y, layer_level + 1, init_x, init_y, layer_level + 2));
  route.push_back(GSegment(
      final_x, final_y, layer_level, final_x, final_y, layer_level + 1));
  route.push_back(GSegment(
      final_x, final_y, layer_level + 1, final_x, final_y, layer_level + 2));
  // Create segment in upper layer (jumper)
  route.push_back(GSegment(
      init_x, init_y, layer_level + 2, final_x, final_y, layer_level + 2));
}

bool validatePos(bool reverse,
                 const int& pos_x,
                 const int& pos_y,
                 const int& final_x,
                 const int& final_y)
{
  if (reverse) {
    return pos_x >= final_x && pos_y >= final_y;
  }
  return pos_x <= final_x && pos_y <= final_y;
}

int searchViaAware(bool reverse,
                   bool is_horizontal,
                   const GSegment& seg,
                   const std::set<std::pair<int, int>>& vias_pos,
                   const int& bridge_size,
                   const int& tile_size)
{
  int space = -1;
  int init_x = seg.init_x;
  int init_y = seg.init_y;
  int final_x = seg.final_x;
  int final_y = seg.final_y;
  int step = tile_size;
  if (reverse) {
    std::swap(init_x, final_x);
    std::swap(init_y, final_y);
    step *= -1;
  }
  int pos_x = init_x;
  int pos_y = init_y;
  int last_x = pos_x;
  int last_y = pos_y;
  while (validatePos(reverse, pos_x, pos_y, final_x, final_y)) {
    if (vias_pos.find(std::make_pair(pos_x, pos_y)) != vias_pos.end()) {
      const int len = std::abs(pos_x - last_x) + std::abs(pos_y - last_y);
      if (len > 0 && len >= bridge_size + 2 * tile_size) {
        space
            = std::abs(last_x - init_x) + std::abs(last_y - init_y) + tile_size;
        last_x = pos_x;
        last_y = pos_y;
      }
    }
    if (is_horizontal) {
      pos_x += step;
    } else {
      pos_y += step;
    }
  }
  if (last_x != final_x || last_y != final_y) {
    const int len = std::abs(final_x - last_x) + std::abs(final_y - last_y);
    if (len > 0 && len >= bridge_size + 2 * tile_size) {
      space = std::abs(last_x - init_x) + std::abs(last_y - init_y) + tile_size;
    }
  }
  return space;
}

int RepairAntennas::getSegmentPos(std::vector<int>& segment_ids,
                                  const GRoute& route,
                                  int& req_size,
                                  const int& bridge_size,
                                  const int& tile_size,
                                  bool isHorizontal,
                                  bool inStart)
{
  if (isHorizontal) {
    sort(segment_ids.begin(),
         segment_ids.end(),
         [&route](const int& seg_id1, const int& seg_id2) {
           return route[seg_id1].init_x < route[seg_id2].init_x;
         });
  } else {
    sort(segment_ids.begin(),
         segment_ids.end(),
         [&route](const int& seg_id1, const int& seg_id2) {
           return route[seg_id1].init_y < route[seg_id2].init_y;
         });
  }
  const int layer_level = route[segment_ids[0]].init_layer;
  const auto& vias_pos = vias_pos_[layer_level];
  int size_acum = 0;
  int cand = -1;
  int req_size_cand = -1;
  if (inStart) {
    for (int pos = 0; pos < segment_ids.size(); pos++) {
      const auto& seg = route[segment_ids[pos]];
      // if seg could have jumper, get via position
      if (seg.length() >= bridge_size + 2 * tile_size) {
        int len_found = searchViaAware(
            false, isHorizontal, seg, vias_pos, bridge_size, tile_size);
        if (len_found != -1 && len_found + size_acum <= req_size) {
          req_size_cand = len_found;
          cand = segment_ids[pos];
        }
      }
      size_acum += seg.length();
      if (size_acum > req_size && size_acum >= req_size + bridge_size) {
        req_size = req_size_cand;
        break;
      }
    }
  } else {
    for (int pos = segment_ids.size() - 1; pos >= 0; pos--) {
      const auto& seg = route[segment_ids[pos]];
      if (seg.length() >= bridge_size + 2 * tile_size) {
        int len_found = searchViaAware(
            true, isHorizontal, seg, vias_pos, bridge_size, tile_size);
        if (len_found != -1 && len_found + size_acum <= req_size) {
          req_size_cand = len_found;
          cand = segment_ids[pos];
        }
      }
      size_acum += seg.length();
      if (size_acum > req_size && size_acum >= req_size + bridge_size) {
        req_size = req_size_cand;
        break;
      }
    }
  }
  return cand;
}

void addSegmentHorizontal(const int& seg_id,
                          GRoute& route,
                          const int& init_x,
                          const int& final_x,
                          const int& layer_level)
{
  const int seg_init_x = route[seg_id].init_x;
  const int seg_init_y = route[seg_id].init_y;
  const int seg_final_y = route[seg_id].final_y;
  // Add vias and jumper in the position
  addSegments(route,
              init_x,
              seg_init_y,
              final_x,
              seg_final_y,
              layer_level);
  // Divide segment (new segment is added before jumper insertion)
  route.push_back(GSegment(seg_init_x,
                           seg_init_y,
                           layer_level,
                           init_x,
                           seg_init_y,
                           layer_level));
  // old segment is reduced (after jumper)
  route[seg_id].init_x = final_x;
}

void addSegmentVertical(const int& seg_id,
                        GRoute& route,
                        const int& init_y,
                        const int& final_y,
                        const int& layer_level)
{
  const int seg_init_x = route[seg_id].init_x;
  const int seg_final_x = route[seg_id].final_x;
  const int seg_init_y = route[seg_id].init_y;
  // Add vias and jumper in the position
  addSegments(route,
              seg_init_x,
              init_y,
              seg_final_x,
              final_y,
              layer_level);
  // Divide segment (new segment is added before jumper insertion)
  route.push_back(GSegment(seg_init_x,
                           seg_init_y,
                           layer_level,
                           seg_init_x,
                           init_y,
                           layer_level));
  // old segment is reduced (after jumper)
  route[seg_id].init_y = final_y;
}

int RepairAntennas::divideSegment(std::vector<int>& segment_ids,
                                  GRoute& route,
                                  odb::dbTechLayer* violation_layer,
                                  const int& tile_size,
                                  const double& ratio,
                                  const ViolationInfo& info)
{
  int length = 0;
  bool is_horizontal
      = violation_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
  // Get total length of segments
  for (const auto& seg_id : segment_ids) {
    length += route[seg_id].length();
  }
  // Get the required wirelength to avoid violation
  const int n_tiles = length / tile_size;
  int req_tiles = int(n_tiles / ratio) * 0.8;
  if (info.pin_num_near_to_start_ != 0 && info.pin_num_near_to_end_ != 0) {
    req_tiles
        = std::max(2, int(req_tiles * 0.15));  // Reduce when has more pins
  }
  int req_size = req_tiles * tile_size;
  const int bridge_size = 2 * tile_size;
  const int layer_level = violation_layer->getRoutingLevel();

  int jumper_count = 0;
  // place bridge in segment begin
  if (info.pin_num_near_to_end_ == 0
      || (info.pin_num_near_to_start_ != 0 && info.pin_num_near_to_end_ != 0)) {
    int pos = getSegmentPos(segment_ids,
                            route,
                            req_size,
                            bridge_size,
                            tile_size,
                            is_horizontal,
                            true);
    if (pos != -1) {
      auto& seg = route[pos];
      jumper_count++;
      if (is_horizontal) {
        // get jumper position
        const int bridge_init_x = seg.init_x + req_size;
        const int bridge_final_x = bridge_init_x + bridge_size;
        if (verifyCapacityForJumper(is_horizontal,
                                    tile_size,
                                    bridge_init_x,
                                    seg.init_y,
                                    bridge_final_x,
                                    seg.final_y,
                                    layer_level)) {
          addSegmentHorizontal(
              pos, route, bridge_init_x, bridge_final_x, layer_level);
        }
      } else {
        // Get jumper position
        const int bridge_init_y = seg.init_y + req_size;
        const int bridge_final_y = bridge_init_y + bridge_size;
        if (verifyCapacityForJumper(is_horizontal,
                                    tile_size,
                                    seg.init_x,
                                    bridge_init_y,
                                    seg.final_x,
                                    bridge_final_y,
                                    layer_level)) {
          addSegmentVertical(
              pos, route, bridge_init_y, bridge_final_y, layer_level);
        }
      }
    }
  }
  // if need place other in segment end
  if (info.pin_num_near_to_start_ == 0
      || (info.pin_num_near_to_start_ != 0 && info.pin_num_near_to_end_ != 0)) {
    int pos = getSegmentPos(segment_ids,
                            route,
                            req_size,
                            bridge_size,
                            tile_size,
                            is_horizontal,
                            false);
    if (pos != -1) {
      auto& seg = route[pos];
      jumper_count++;
      if (is_horizontal) {
        // Get jumper position
        const int bridge_init_x = seg.final_x - req_size - bridge_size;
        const int bridge_final_x = bridge_init_x + bridge_size;
        if (verifyCapacityForJumper(is_horizontal,
                                    tile_size,
                                    bridge_init_x,
                                    seg.init_y,
                                    bridge_final_x,
                                    seg.final_y,
                                    layer_level)) {
          addSegmentHorizontal(
              pos, route, bridge_init_x, bridge_final_x, layer_level);
        }
      } else {
        // Get jumper position
        const int bridge_init_y = seg.final_y - req_size - bridge_size;
        const int bridge_final_y = bridge_init_y + bridge_size;
        if (verifyCapacityForJumper(is_horizontal,
                                    tile_size,
                                    seg.init_x,
                                    bridge_init_y,
                                    seg.final_x,
                                    bridge_final_y,
                                    layer_level)) {
          addSegmentVertical(
              pos, route, bridge_init_y, bridge_final_y, layer_level);
        }
      }
    }
  }
  return jumper_count;
}

int manhattanDistance(const int& x1,
                      const int& y1,
                      const int& x2,
                      const int& y2)
{
  return std::abs(x1 - x2) + std::abs(y1 - y2);
}

double gateArea(odb::dbMTerm* mterm)
{
  double max_gate_area = 0;
  if (mterm->hasDefaultAntennaModel()) {
    odb::dbTechAntennaPinModel* pin_model = mterm->getDefaultAntennaModel();
    std::vector<std::pair<double, odb::dbTechLayer*>> gate_areas;
    pin_model->getGateArea(gate_areas);

    for (const auto& [gate_area, layer] : gate_areas) {
      max_gate_area = std::max(max_gate_area, gate_area);
    }
  }
  return max_gate_area;
}

void RepairAntennas::getPinNumberNearEndPoint(
    const std::vector<int>& segment_ids,
    const std::vector<odb::dbITerm*>& gates,
    const GRoute& route,
    ViolationInfo& info)
{
  int seg_init_x, seg_init_y, seg_final_x, seg_final_y;
  seg_init_x = seg_init_y = std::numeric_limits<int>::max();

  // Get x_min y_min x_max y_max of all segments
  seg_final_x = seg_final_y = 0;
  for (const auto& seg_id : segment_ids) {
    seg_init_x = std::min(seg_init_x, route[seg_id].init_x);
    seg_init_y = std::min(seg_init_y, route[seg_id].init_y);
    seg_final_x = std::max(seg_final_x, route[seg_id].final_x);
    seg_final_y = std::max(seg_final_y, route[seg_id].final_y);
  }
  // iterate all gates to count pins near of min/max position
  for (const auto& iterm : gates) {
    odb::dbInst* sink_inst = iterm->getInst();
    odb::Rect sink_bbox = getInstRect(sink_inst, iterm);
    const int x_min = sink_bbox.xMin();
    const int y_min = sink_bbox.yMin();
    const int x_max = sink_bbox.xMax();
    const int y_max = sink_bbox.yMax();
    const int dist1 = std::min(
        std::min(manhattanDistance(x_min, y_min, seg_init_x, seg_init_y),
                 manhattanDistance(x_min, y_max, seg_init_x, seg_init_y)),
        std::min(manhattanDistance(x_max, y_min, seg_init_x, seg_init_y),
                 manhattanDistance(x_max, y_max, seg_init_x, seg_init_y)));
    const int dist2 = std::min(
        std::min(manhattanDistance(x_min, y_min, seg_final_x, seg_final_y),
                 manhattanDistance(x_min, y_max, seg_final_x, seg_final_y)),
        std::min(manhattanDistance(x_max, y_min, seg_final_x, seg_final_y),
                 manhattanDistance(x_max, y_max, seg_final_x, seg_final_y)));
    if (dist1 < dist2) {
      info.pin_num_near_to_start_++;
      info.pin_area_near_to_start_ += gateArea(iterm->getMTerm());
    } else {
      info.pin_num_near_to_end_++;
      info.pin_area_near_to_end_ += gateArea(iterm->getMTerm());
    }
  }
}

struct SegInfo
{
  int id = -1;
  int seg_id;
  odb::Rect rect;
  std::vector<std::pair<odb::dbTechLayer*, int>> adj;
  std::unordered_set<std::string> gates;
  SegInfo(int id_, int seg_id_, odb::Rect rect_)
  {
    id = id_;
    seg_id = seg_id_;
    rect = rect_;
  }
};

void getSegmentsWithOverlap(SegInfo& seg_info,
                            const std::vector<SegInfo>& low_segs,
                            odb::dbTechLayer* layer)
{
  int index = 0;
  for (const SegInfo& seg_it : low_segs) {
    if (seg_info.rect.overlaps(seg_it.rect)) {
      seg_info.adj.push_back({layer, index});
    }
    index++;
  }
}

SegmentIdByViolation RepairAntennas::getSegmentsWithViolation(
    odb::dbNet* db_net,
    const GRoute& route,
    const int& max_layer,
    std::map<int, int>& layer_with_violation)
{
  odb::dbTech* tech = db_->getTech();
  int min_layer = 1;
  // get segments information
  std::unordered_map<odb::dbTechLayer*, std::vector<SegInfo>> segment_by_layer;
  int seg_count = 0;
  vias_pos_.clear();
  int pos = 0;
  for (const GSegment& seg : route) {
    // add only segments in lower layer of violation layer
    if (std::min(seg.final_layer, seg.init_layer) <= max_layer) {
      // get min layer of segment
      odb::dbTechLayer* tech_layer
          = tech->findRoutingLayer(std::min(seg.init_layer, seg.final_layer));
      if (seg.isVia()) {
        segment_by_layer[tech_layer->getUpperLayer()].push_back(
            SegInfo(seg_count, pos, grouter_->globalRoutingToBox(seg)));
        // add one segment in upper and lower layer (to connect stacked vias)
        segment_by_layer[tech->findRoutingLayer(seg.init_layer)].push_back(
            SegInfo(seg_count++, -1, grouter_->globalRoutingToBox(seg)));
        segment_by_layer[tech->findRoutingLayer(seg.final_layer)].push_back(
            SegInfo(seg_count++, -1, grouter_->globalRoutingToBox(seg)));
        vias_pos_[seg.init_layer].insert(
            std::make_pair(seg.init_x, seg.init_y));
        vias_pos_[seg.final_layer].insert(
            std::make_pair(seg.final_x, seg.final_y));
      } else {
        segment_by_layer[tech_layer].push_back(
            SegInfo(seg_count, pos, grouter_->globalRoutingToBox(seg)));
      }
      seg_count++;
    }
    pos++;
  }

  // Set adjacents segments (neighbor)
  for (auto& layer_it : segment_by_layer) {
    odb::dbTechLayer* tech_layer = layer_it.first;
    // find segments in same layer that touch segment
    for (SegInfo& seg_info : layer_it.second) {
      getSegmentsWithOverlap(
          seg_info, segment_by_layer[tech_layer], tech_layer);
    }
    // find segments in lower layer (upper layer connect with lower)
    odb::dbTechLayer* lower_layer = tech_layer->getLowerLayer();
    if (lower_layer
        && segment_by_layer.find(lower_layer) != segment_by_layer.end()) {
      for (SegInfo& seg_info : layer_it.second) {
        getSegmentsWithOverlap(
            seg_info, segment_by_layer[lower_layer], lower_layer);
      }
    }
  }

  // iterate all instance pins to get segments with overlap
  std::unordered_map<std::string, std::unordered_set<int>> seg_connected;
  for (odb::dbITerm* iterm : db_net->getITerms()) {
    odb::dbMTerm* mterm = iterm->getMTerm();
    std::string pin_name = fmt::format("  {}/{} ({})",
                                       iterm->getInst()->getConstName(),
                                       mterm->getConstName(),
                                       mterm->getMaster()->getConstName());
    odb::dbInst* inst = iterm->getInst();
    const odb::dbTransform transform = inst->getTransform();
    for (odb::dbMPin* mterm : mterm->getMPins()) {
      for (odb::dbBox* box : mterm->getGeometry()) {
        odb::dbTechLayer* tech_layer = box->getTechLayer();
        if (tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
          continue;
        }

        odb::Rect pin_rect = box->getBox();
        transform.apply(pin_rect);
        // get segment in same layer overlap the pin_rect
        for (const SegInfo& it : segment_by_layer[tech_layer]) {
          if (it.rect.overlaps(pin_rect)) {
            seg_connected[pin_name].insert(it.id);
          }
        }
      }
    }
  }

  // get violation info
  auto violations = antenna_violations_[db_net];
  SegmentIdByViolation segment_with_violations;

  // Init DSU
  std::vector<int> dsu_parent(seg_count);
  std::vector<int> dsu_size(seg_count);
  for (int i = 0; i < seg_count; i++) {
    dsu_size[i] = 1;
    dsu_parent[i] = i;
  }

  boost::disjoint_sets<int*, int*> dsu(&dsu_size[0], &dsu_parent[0]);

  // Run DSU
  odb::dbTechLayer* layer_iter = tech->findRoutingLayer(min_layer);
  for (; layer_iter; layer_iter = layer_iter->getUpperLayer()) {
    // iterate each node of this layer to union set
    for (auto& seg_it : segment_by_layer[layer_iter]) {
      int id_u = seg_it.id;
      // get neighbors and union
      for (const auto& lower_it : seg_it.adj) {
        int id_v = segment_by_layer[lower_it.first][lower_it.second].id;
        // if they are on different sets then union
        if (dsu.find_set(id_u) != dsu.find_set(id_v)) {
          dsu.union_set(id_u, id_v);
        }
      }
    }
    int iter = layer_iter->getRoutingLevel();
    // verify if layer has violation and save segments with same set than the
    // violation pins
    if (layer_with_violation.find(iter) != layer_with_violation.end()) {
      for (auto& seg_it : segment_by_layer[layer_iter]) {
        int id_u = seg_it.id;
        for (const auto& iterm : violations[layer_with_violation[iter]].gates) {
          odb::dbMTerm* mterm = iterm->getMTerm();
          std::string pin_name
              = fmt::format("  {}/{} ({})",
                            iterm->getInst()->getConstName(),
                            mterm->getConstName(),
                            mterm->getMaster()->getConstName());
          bool is_conected = false;
          // iterate all segment with overlap with pin
          for (const int& nbr_id : seg_connected[pin_name]) {
            if (dsu.find_set(id_u) == dsu.find_set(nbr_id)) {
              is_conected = true;
              break;
            }
          }
          // if there is a segment in same set
          if (is_conected) {
            // it is not via
            if (seg_it.seg_id != -1) {
              segment_with_violations[layer_with_violation[iter]].push_back(
                  seg_it.seg_id);
            }
            break;
          }
        }
      }
    }
  }
  return segment_with_violations;
}

void RepairAntennas::jumperInsertion(NetRouteMap& routing,
                                     const int tile_size,
                                     const int& max_routing_layer)
{
  odb::dbTech* tech = db_->getTech();
  int total_jumpers = 0;
  int jumpers_by_net;
  int jumpers_by_layer;
  int net_with_jumpers = 0;
  for (auto const& net_violations : antenna_violations_) {
    odb::dbNet* db_net = net_violations.first;
    const auto violations = net_violations.second;
    std::map<int, int> routing_layer_with_violations;

    jumpers_by_net = 0;
    int max_layer = 1;
    // Iterate found layers with violations
    int violation_id = 0;
    for (const ant::Violation& violation : violations) {
      odb::dbTechLayer* violation_layer
          = tech->findRoutingLayer(violation.routing_level);
      odb::dbTechLayer* upper_layer
          = tech->findRoutingLayer(violation.routing_level + 2);
      if (upper_layer && upper_layer->getRoutingLevel() <= max_routing_layer
          && violation_layer->getType()
                 == odb::dbTechLayerType::ROUTING) {  // avoid other layers
        routing_layer_with_violations[violation.routing_level] = violation_id;
        max_layer = std::max(max_layer, violation.routing_level);
      }
      violation_id++;
    }

    GRoute& route = routing[db_net];

    // Get segments with violation in each layer
    SegmentIdByViolation segment_with_violations_id;
    if (routing_layer_with_violations.size()) {
      segment_with_violations_id = getSegmentsWithViolation(
          db_net, route, max_layer, routing_layer_with_violations);
    }
    // add jumpers in segments for each layer
    for (auto it : routing_layer_with_violations) {
      ViolationInfo info;
      if (segment_with_violations_id[it.second].size() == 0) {
        continue;
      }
      getPinNumberNearEndPoint(segment_with_violations_id[it.second],
                               violations[it.second].gates,
                               route,
                               info);
      jumpers_by_layer = divideSegment(segment_with_violations_id[it.second],
                                       route,
                                       tech->findRoutingLayer(it.first),
                                       tile_size,
                                       violations[it.second].ratio,
                                       info);
      jumpers_by_net += jumpers_by_layer;
    }
    if (jumpers_by_net) {
      net_with_jumpers++;
      total_jumpers += jumpers_by_net;
      // db_net->setJumpers(true);
    }
  }
  logger_->info(GRT,
                302,
                "Inserted {} jumpers for {} nets.",
                total_jumpers,
                net_with_jumpers);
}

}  // namespace grt
