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

#include <algorithm>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <unordered_set>

#include "AntennaRepair.h"
#include "Net.h"
#include "Pin.h"
#include "grt/GlobalRouter.h"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

AntennaRepair::AntennaRepair(GlobalRouter* grouter,
                             ant::AntennaChecker* arc,
                             dpl::Opendp* opendp,
                             odb::dbDatabase* db,
                             utl::Logger* logger)
  : grouter_(grouter), arc_(arc), opendp_(opendp), db_(db), logger_(logger),
    unique_diode_index_(1)
{
  block_ = db_->getChip()->getBlock();
}

bool AntennaRepair::checkAntennaViolations(NetRouteMap& routing,
                                           int max_routing_layer,
                                           odb::dbMTerm* diode_mterm)
{
  makeNetWires(routing, max_routing_layer);
  arc_->initAntennaRules();
  for (auto net_route : routing) {
    odb::dbNet* db_net = net_route.first;
    if (db_net->getWire()) {
      std::vector<ant::ViolationInfo> netViol =
        arc_->getAntennaViolations(db_net, diode_mterm);
      if (!netViol.empty()) {
        antenna_violations_[db_net] = netViol;
        debugPrint(logger_, GRT, "repair_antennas", 1, "antenna violations {}",
                   db_net->getConstName());
      }
    }
  }
  destroyNetWires();

  logger_->info(GRT, 12, "Antenna violations: {}", antenna_violations_.size());
  return !antenna_violations_.empty();
}

void AntennaRepair::makeNetWires(NetRouteMap& routing,
                                 int max_routing_layer)
{
  std::map<int, odb::dbTechVia*> default_vias
    = grouter_->getDefaultVias(max_routing_layer);

  for (auto net_route : routing) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    makeNetWire(db_net, route, default_vias);
  }
}

odb::dbWire* AntennaRepair::makeNetWire(odb::dbNet* db_net,
                                        GRoute& route,
                                        std::map<int, odb::dbTechVia*> &default_vias)
{
  odb::dbWire* wire = odb::dbWire::create(db_net);
  if (wire) {
    odb::dbTech* tech = db_->getTech();
    odb::dbWireEncoder wire_encoder;
    wire_encoder.begin(wire);

    std::unordered_set<GSegment, GSegmentHash> wire_segments;
    for (GSegment& seg : route) {
      if (std::abs(seg.init_layer - seg.final_layer) > 1) {
        logger_->error(GRT, 68, "Global route segment not valid.");
      }

      if (wire_segments.find(seg) == wire_segments.end()) {
        int x1 = seg.init_x;
        int y1 = seg.init_y;
        int x2 = seg.final_x;
        int y2 = seg.final_y;
        int l1 = seg.init_layer;
        int l2 = seg.final_layer;

        odb::dbTechLayer* layer = tech->findRoutingLayer(l1);

        if (l1 == l2) {  // Add wire
          if (x1 != x2 || y1 != y2) {
            wire_encoder.newPath(layer, odb::dbWireType::ROUTED);
            wire_encoder.addPoint(x1, y1);
            wire_encoder.addPoint(x2, y2);
            wire_segments.insert(seg);
          }
        } else {  // Add via
          int bottom_layer = std::min(l1, l2);
          wire_encoder.newPath(layer, odb::dbWireType::ROUTED);
          wire_encoder.addPoint(x1, y1);
          wire_encoder.addTechVia(default_vias[bottom_layer]);
          wire_segments.insert(seg);
        }
      }
    }
    wire_encoder.end();

    odb::orderWires(db_net, false);
    return wire;
  }
  else {
    logger_->error(GRT, 221, "Cannot create wire for net {}.", db_net->getConstName());
    // suppress gcc warning
    return nullptr;
  }
}

void AntennaRepair::destroyNetWires()
{
  for (odb::dbNet* db_net : block_->getNets()) {
    odb::dbWire* wire = db_net->getWire();
    if (wire)
      odb::dbWire::destroy(wire);
  }
}

void AntennaRepair::repairAntennas(odb::dbMTerm* diode_mterm)
{
  int site_width = -1;
  r_tree fixed_insts;

  illegal_diode_placement_count_ = 0;
  diode_insts_.clear();

  auto rows = block_->getRows();
  for (odb::dbRow* db_row : rows) {
    odb::dbSite* site = db_row->getSite();
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
    for (ant::ViolationInfo &violation : violations) {
      debugPrint(logger_, GRT, "repair_antennas", 2, "antenna {} insert {} diodes",
                 db_net->getConstName(),
                 violation.required_diode_count * violation.iterms.size());
      if (violation.required_diode_count > 0) {
        for (odb::dbITerm* sink_iterm : violation.iterms) {
          odb::dbInst* sink_inst = sink_iterm->getInst();
          for (int j = 0; j < violation.required_diode_count; j++) {
            insertDiode(db_net,
                        diode_mterm,
                        sink_inst,
                        sink_iterm,
                        site_width,
                        fixed_insts);
            inserted_diodes = true;
          }
        }
      }
      else
        repair_failures = true;
    }
    if (inserted_diodes)
      grouter_->addDirtyNet(db_net);
  }
  if (repair_failures)
    logger_->warn(GRT, 243, "Unable to repair antennas on net with diodes.");
}

void AntennaRepair::legalizePlacedCells()
{
  opendp_->detailedPlacement(0, 0);
  opendp_->checkPlacement(false);

  // After legalize placement, diodes and violated insts don't need to be FIRM
  setInstsPlacementStatus(odb::dbPlacementStatus::PLACED);
}

void AntennaRepair::deleteFillerCells()
{
  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->getMaster()->getType() == odb::dbMasterType::CORE_SPACER) {
      odb::dbInst::destroy(inst);
    }
  }
}

void AntennaRepair::insertDiode(odb::dbNet* net,
                                odb::dbMTerm* diode_mterm,
                                odb::dbInst* sink_inst,
                                odb::dbITerm* sink_iterm,
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
  odb::Rect sink_bbox = getInstRect(sink_inst, sink_iterm);
  inst_loc_x = sink_bbox.xMin();
  inst_loc_y = sink_bbox.yMin();
  inst_width = sink_bbox.xMax() - sink_bbox.xMin();
  odb::dbOrientType inst_orient = sink_inst->getOrient();

  std::string diode_inst_name = "ANTENNA_"
    + std::to_string(unique_diode_index_++);
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

void AntennaRepair::getFixedInstances(r_tree& fixed_insts)
{
  int fixed_inst_id = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->getPlacementStatus() == odb::dbPlacementStatus::FIRM) {
      odb::dbBox* instBox = inst->getBBox();
      box b(point(instBox->xMin(), instBox->yMin()),
            point(instBox->xMax(), instBox->yMax()));
      value v(b, fixed_inst_id);
      fixed_insts.insert(v);
      fixed_inst_id++;
    }
  }
}

void AntennaRepair::setInstsPlacementStatus(
    odb::dbPlacementStatus placement_status)
{
  for (auto const& violation : antenna_violations_) {
    for (int i = 0; i < violation.second.size(); i++) {
      for (odb::dbITerm* sink_iterm : violation.second[i].iterms) {
        if (!sink_iterm->getMTerm()->getMaster()->isBlock()) {
          sink_iterm->getInst()->setPlacementStatus(placement_status);
        }
      }
    }
  }

  for (odb::dbInst* diode_inst : diode_insts_) {
    diode_inst->setPlacementStatus(placement_status);
  }
}

odb::Rect AntennaRepair::getInstRect(odb::dbInst* inst, odb::dbITerm* iterm)
{
  int min = std::numeric_limits<int>::min();
  int max = std::numeric_limits<int>::max();

  int x, y;
  inst->getOrigin(x, y);
  odb::Point origin = odb::Point(x, y);
  odb::dbTransform transform(inst->getOrient(), origin);

  odb::Rect inst_rect;

  if (inst->getMaster()->isBlock()) {
    inst_rect = odb::Rect(max, max, min, min);
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

bool AntennaRepair::diodeInRow(odb::Rect diode_rect)
{
  for (odb::dbRow* row : block_->getRows()) {
    odb::Rect row_rect = row->getBBox();

    if (row_rect.contains(diode_rect)) {
      return true;
    }
  }

  return false;
}

odb::dbMTerm* AntennaRepair::findDiodeMTerm()
{
  for (odb::dbLib *lib : db_->getLibs()) {
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
double AntennaRepair::diffArea(odb::dbMTerm *mterm)
{
  double max_diff_area = 0.0;
  std::vector<std::pair<double, odb::dbTechLayer*>> diff_areas;
  mterm->getDiffArea(diff_areas);
  for (auto area_layer : diff_areas) {
    double diff_area = area_layer.first;
    max_diff_area = std::max(max_diff_area, diff_area);
  }
  return max_diff_area;
}

}  // namespace grt
