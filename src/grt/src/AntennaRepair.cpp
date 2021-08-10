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

#include "AntennaRepair.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

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
    : grouter_(grouter), arc_(arc), opendp_(opendp), db_(db), logger_(logger)
{
  block_ = db_->getChip()->getBlock();
}

int AntennaRepair::checkAntennaViolations(NetRouteMap& routing,
                                          int max_routing_layer,
                                          odb::dbMTerm* diode_mterm)
{
  odb::dbTech* tech = db_->getTech();

  arc_->load_antenna_rules();

  std::map<int, odb::dbTechVia*> default_vias
      = grouter_->getDefaultVias(max_routing_layer);

  for (auto net_route : routing) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    odb::dbWire* wire = odb::dbWire::create(db_net);
    if (wire != nullptr) {
      odb::dbWireEncoder wire_encoder;
      wire_encoder.begin(wire);
      odb::dbWireType wire_type = odb::dbWireType::ROUTED;

      for (GSegment& seg : route) {
        if (std::abs(seg.init_layer - seg.final_layer) > 1) {
          logger_->error(GRT, 68, "Global route segment not valid.");
        }
        int x1 = seg.init_x;
        int y1 = seg.init_y;
        int x2 = seg.final_x;
        int y2 = seg.final_y;
        int l1 = seg.init_layer;
        int l2 = seg.final_layer;

        odb::dbTechLayer* layer = tech->findRoutingLayer(l1);

        if (l1 == l2) {  // Add wire
          if (x1 == x2 && y1 == y2)
            continue;
          wire_encoder.newPath(layer, wire_type);
          wire_encoder.addPoint(x1, y1);
          wire_encoder.addPoint(x2, y2);
        } else {  // Add via
          int bottom_layer = (l1 < l2) ? l1 : l2;
          wire_encoder.newPath(layer, wire_type);
          wire_encoder.addPoint(x1, y1);
          wire_encoder.addTechVia(default_vias[bottom_layer]);
        }
      }
      wire_encoder.end();

      odb::orderWires(db_net, false, logger_ ,false);

      std::vector<ant::VINFO> netViol = arc_->get_net_antenna_violations(
          db_net,
          diode_mterm->getMaster()->getConstName(),
          diode_mterm->getConstName());
      if (!netViol.empty()) {
        antenna_violations_[db_net] = netViol;
        grouter_->addDirtyNet(db_net);
      }

      odb::dbWire::destroy(wire);
    } else {
      logger_->error(
          GRT, 221, "Cannot create wire for net {}.", db_net->getConstName());
    }
  }

  logger_->info(GRT, 12, "Antenna violations: {}", antenna_violations_.size());
  return antenna_violations_.size();
}

void AntennaRepair::repairAntennas(odb::dbMTerm* diode_mterm)
{
  int site_width = -1;
  int cnt = diode_insts_.size();
  r_tree fixed_insts;

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

  deleteFillerCells();

  setInstsPlacementStatus(odb::dbPlacementStatus::FIRM);
  getFixedInstances(fixed_insts);

  for (auto const& violation : antenna_violations_) {
    odb::dbNet* net = violation.first;
    for (int i = 0; i < violation.second.size(); i++) {
      for (odb::dbITerm* sink_iterm : violation.second[i].iterms) {
        odb::dbInst* sink_inst = sink_iterm->getInst();
        for (int j = 0; j < violation.second[i].antenna_cell_nums; j++) {
          std::string antenna_inst_name = "ANTENNA_" + std::to_string(cnt);
          insertDiode(net,
                      diode_mterm,
                      sink_inst,
                      sink_iterm,
                      antenna_inst_name,
                      site_width,
                      fixed_insts);
          cnt++;
        }
      }
    }
  }
}

void AntennaRepair::legalizePlacedCells()
{
  AntennaCbk* cbk = new AntennaCbk(grouter_);
  cbk->addOwner(block_);

  opendp_->detailedPlacement(0, 0);
  opendp_->checkPlacement(false);

  cbk->removeOwner();

  // After legalize placement, diodes and violated insts don't need to be FIRM
  setInstsPlacementStatus(odb::dbPlacementStatus::PLACED);
}

void AntennaRepair::deleteFillerCells()
{
  int filler_cnt = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->getMaster()->getType() == odb::dbMasterType::CORE_SPACER) {
      filler_cnt++;
      odb::dbInst::destroy(inst);
    }
  }

  if (filler_cnt > 0) {
    logger_->info(GRT, 11, "Deleted {} filler cells.", filler_cnt);
  }
}

void AntennaRepair::insertDiode(odb::dbNet* net,
                                odb::dbMTerm* diode_mterm,
                                odb::dbInst* sink_inst,
                                odb::dbITerm* sink_iterm,
                                std::string antenna_inst_name,
                                int site_width,
                                r_tree& fixed_insts)
{
  const int max_legalize_itr = 50;
  bool legally_placed = false;
  bool place_at_left = true;
  int left_offset = 0;
  int right_offset = 0;
  int offset;

  odb::dbMaster* antenna_master = diode_mterm->getMaster();

  int inst_loc_x, inst_loc_y, inst_width;
  odb::Rect sink_bbox = getInstRect(sink_inst, sink_iterm);
  inst_loc_x = sink_bbox.xMin();
  inst_loc_y = sink_bbox.yMin();
  inst_width = sink_bbox.xMax() - sink_bbox.xMin();
  odb::dbOrientType inst_orient = sink_inst->getOrient();

  odb::dbInst* antenna_inst
      = odb::dbInst::create(block_, antenna_master, antenna_inst_name.c_str());
  odb::dbITerm* antenna_iterm
      = antenna_inst->findITerm(diode_mterm->getConstName());
  odb::dbBox* antenna_bbox = antenna_inst->getBBox();
  int antenna_width = antenna_bbox->xMax() - antenna_bbox->xMin();

  odb::Rect core_area;
  block_->getCoreArea(core_area);

  // Use R-tree to check if diode will not overlap or cause 1-site spacing with
  // other cells
  int left_pad = opendp_->padGlobalLeft();
  int right_pad = opendp_->padGlobalRight();
  std::vector<value> overlap_insts;
  int legalize_itr = 0;
  while (!legally_placed && legalize_itr < max_legalize_itr) {
    if (place_at_left) {
      offset = -(antenna_width + left_offset * site_width);
      left_offset++;
      place_at_left = false;
    } else {
      offset = inst_width + right_offset * site_width;
      right_offset++;
      place_at_left = true;
    }

    antenna_inst->setOrient(inst_orient);
    antenna_inst->setLocation(inst_loc_x + offset, inst_loc_y);

    odb::dbBox* instBox = antenna_inst->getBBox();
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

  odb::Rect inst_rect;
  antenna_inst->getBBox()->getBox(inst_rect);

  if (!legally_placed) {
    logger_->warn(
        GRT,
        54,
        "Placement of diode {} will be legalized by detailed placement.",
        antenna_inst_name);
  }

  // allow detailed placement to move diodes with geometry out of the core area,
  // or near macro pins (can be placed out of row), or illegal placed diodes
  if (core_area.contains(inst_rect) && !sink_inst->getMaster()->isBlock()
      && legally_placed) {
    antenna_inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  } else {
    antenna_inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  odb::dbITerm::connect(antenna_iterm, net);
  diode_insts_.push_back(antenna_inst);

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
          odb::Rect rect;
          mterm_box->getBox(rect);
          transform.apply(rect);

          inst_rect = rect;
        }
      }
    }
  } else {
    inst->getBBox()->getBox(inst_rect);
  }

  return inst_rect;
}

AntennaCbk::AntennaCbk(GlobalRouter* grouter) : grouter_(grouter)
{
}

void AntennaCbk::inDbPostMoveInst(odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    odb::dbNet* db_net = iterm->getNet();
    if (db_net != nullptr && !db_net->isSpecial())
      grouter_->addDirtyNet(iterm->getNet());
  }
}

}  // namespace grt
