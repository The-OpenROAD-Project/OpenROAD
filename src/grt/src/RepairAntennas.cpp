// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "RepairAntennas.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stack>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Net.h"
#include "Pin.h"
#include "ant/AntennaChecker.hh"
#include "boost/geometry/geometry.hpp"
#include "boost/pending/disjoint_sets.hpp"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "omp.h"
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
      has_new_violations_(false),
      routing_source_(RoutingSource::None),
      tile_size_(0),
      jumper_size_(0),
      smaller_seg_size_(0)
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
  // Save nets repaired in last iteration
  std::unordered_set<std::string> last_nets;
  for (const auto& violation : antenna_violations_) {
    last_nets.insert(violation.first->getConstName());
  }

  antenna_violations_.clear();
  for (odb::dbNet* db_net : nets_to_repair) {
    antenna_violations_[db_net];
  }

  routing_source_ = grouter_->haveDetailedRoutes(nets_to_repair)
                        ? RoutingSource::DetailedRouting
                        : RoutingSource::GlobalRouting;
  bool destroy_wires = routing_source_ == RoutingSource::GlobalRouting;

  arc_->makeNetWiresFromGuides(nets_to_repair);
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

  has_new_violations_ = false;
  // remove nets with zero violations
  for (auto it = antenna_violations_.begin();
       it != antenna_violations_.end();) {
    if (it->second.empty()) {
      it = antenna_violations_.erase(it);
    } else {
      // check if the net is new to repair
      if (last_nets.find(it->first->getConstName()) == last_nets.end()) {
        has_new_violations_ = true;
      }
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

void RepairAntennas::destroyNetWires(
    const std::vector<odb::dbNet*>& nets_to_repair)
{
  for (odb::dbNet* db_net : nets_to_repair) {
    odb::dbWire* wire = db_net->getWire();
    if (!db_net->isSpecial() && wire) {
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
  int fixed_inst_count = 0;
  getFixedInstances(fixed_insts, fixed_inst_count);
  getPlacementBlockages(fixed_insts, fixed_inst_count);

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
      } else {
        repair_failures = true;
      }
    }
    if (inserted_diodes) {
      // Diode insertion deletes the jumpers in guides
      db_net->setJumpers(false);
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

  if (!legally_placed) {
    illegal_diode_placement_count_++;
  }

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

void RepairAntennas::getFixedInstances(r_tree& fixed_insts,
                                       int& fixed_inst_count)
{
  for (odb::dbInst* inst : block_->getInsts()) {
    odb::dbPlacementStatus status = inst->getPlacementStatus();
    if (status == odb::dbPlacementStatus::FIRM
        || status == odb::dbPlacementStatus::LOCKED) {
      odb::dbBox* instBox = inst->getBBox();
      box b(point(instBox->xMin(), instBox->yMin()),
            point(instBox->xMax(), instBox->yMax()));
      value v(b, fixed_inst_count);
      fixed_insts.insert(v);
      fixed_inst_count++;
    }
  }
}

void RepairAntennas::getPlacementBlockages(r_tree& fixed_insts,
                                           int& fixed_inst_count)
{
  for (odb::dbBlockage* blockage : block_->getBlockages()) {
    if (blockage->isSoft()) {
      continue;
    }
    odb::Rect bbox = blockage->getBBox()->getBox();

    box blockage_box(point(bbox.xMin(), bbox.yMin()),
                     point(bbox.xMax(), bbox.yMax()));
    value blockage_value(blockage_box, fixed_inst_count);
    fixed_insts.insert(blockage_value);
    fixed_inst_count++;
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
          if (diffArea(mterm) > 0.0) {
            return mterm;
          }
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

/////////////////////////////////////////////////////////////////////////////////////////////////
// Jumper insertion functions

void RepairAntennas::addJumperAndVias(GRoute& route,
                                      const int& init_x,
                                      const int& init_y,
                                      const int& final_x,
                                      const int& final_y,
                                      const int& layer_level,
                                      odb::dbNet* db_net)
{
  // Create vias (at the start and end of the jumper)
  for (int layer = layer_level; layer < layer_level + 2; layer++) {
    route.push_back(GSegment(init_x, init_y, layer, init_x, init_y, layer + 1));
    route.push_back(
        GSegment(final_x, final_y, layer, final_x, final_y, layer + 1));
  }
  // Create segment in upper layer (jumper)
  route.push_back(GSegment(init_x,
                           init_y,
                           layer_level + 2,
                           final_x,
                           final_y,
                           layer_level + 2,
                           true));
  // Reducing usage in the layer level
  grouter_->updateResources(
      init_x, init_y, final_x, final_y, layer_level, -1, db_net);
  // Increasing usage in the layer level + 2
  grouter_->updateResources(
      init_x, init_y, final_x, final_y, layer_level + 2, 1, db_net);
  // Update FastRoute Tree Edges
  grouter_->updateFastRouteGridsLayer(
      init_x, init_y, final_x, final_y, layer_level, layer_level + 2, db_net);
}

void RepairAntennas::addJumperToRoute(GRoute& route,
                                      const int& seg_id,
                                      const int& jumper_init_pos,
                                      const int& jumper_final_pos,
                                      const int& layer_level,
                                      odb::dbNet* db_net)
{
  const int seg_init_x = route[seg_id].init_x;
  const int seg_init_y = route[seg_id].init_y;
  const bool is_horizontal = (seg_init_x != route[seg_id].final_x);

  int jumper_init_x;
  int jumper_init_y;
  int jumper_final_x;
  int jumper_final_y;

  if (is_horizontal) {
    jumper_init_x = jumper_init_pos;
    jumper_final_x = jumper_final_pos;
    jumper_init_y = seg_init_y;
    jumper_final_y = seg_init_y;
  } else {
    jumper_init_y = jumper_init_pos;
    jumper_final_y = jumper_final_pos;
    jumper_init_x = seg_init_x;
    jumper_final_x = seg_init_x;
  }
  // Add vias and jumper in the position
  addJumperAndVias(route,
                   jumper_init_x,
                   jumper_init_y,
                   jumper_final_x,
                   jumper_final_y,
                   layer_level,
                   db_net);
  // Divide segment (new segment is added before jumper insertion)
  route.push_back(GSegment(seg_init_x,
                           seg_init_y,
                           layer_level,
                           jumper_init_x,
                           jumper_init_y,
                           layer_level));
  // old segment is reduced (after jumper)
  route[seg_id].init_x = jumper_final_x;
  route[seg_id].init_y = jumper_final_y;
}

void RepairAntennas::addJumper(GRoute& route,
                               const int& segment_id,
                               const int& jumper_pos,
                               odb::dbNet* db_net)
{
  odb::dbTech* tech = db_->getTech();
  const int segment_layer_level = route[segment_id].init_layer;
  odb::dbTechLayer* segment_layer = tech->findRoutingLayer(segment_layer_level);
  const bool is_horizontal
      = segment_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;

  if (is_horizontal) {
    // Get start and final X position of jumper
    const int jumper_start_x = jumper_pos;
    const int jumper_final_x = jumper_start_x + jumper_size_;
    addJumperToRoute(route,
                     segment_id,
                     jumper_start_x,
                     jumper_final_x,
                     segment_layer_level,
                     db_net);
  } else {
    // Get start and final Y position jumper
    const int jumper_start_y = jumper_pos;
    const int jumper_final_y = jumper_start_y + jumper_size_;
    addJumperToRoute(route,
                     segment_id,
                     jumper_start_y,
                     jumper_final_y,
                     segment_layer_level,
                     db_net);
  }
}

int RepairAntennas::getSegmentsPerLayer(
    const GRoute& route,
    const int& max_layer,
    LayerToSegmentNodeVector& segment_by_layer)
{
  int added_seg_count = 0;
  odb::dbTech* tech = db_->getTech();
  int seg_id = 0;
  for (const GSegment& seg : route) {
    // Add only segments in lower layer of violation layer
    const int seg_min_layer = std::min(seg.final_layer, seg.init_layer);
    if (seg_min_layer <= max_layer) {
      // Get min layer of segment
      odb::dbTechLayer* tech_layer = tech->findRoutingLayer(seg_min_layer);
      if (seg.isVia()) {
        // Add segment node in via layer
        segment_by_layer[tech_layer->getUpperLayer()].push_back(SegmentNode(
            added_seg_count++, seg_id, grouter_->globalRoutingToBox(seg)));
        // Add one segment node in upper & lower layer (to connect stacked vias)
        segment_by_layer[tech->findRoutingLayer(seg.init_layer)].push_back(
            SegmentNode(
                added_seg_count++, -1, grouter_->globalRoutingToBox(seg)));
        segment_by_layer[tech->findRoutingLayer(seg.final_layer)].push_back(
            SegmentNode(
                added_seg_count++, -1, grouter_->globalRoutingToBox(seg)));
      } else {
        // Add segment node in route layer
        segment_by_layer[tech_layer].push_back(SegmentNode(
            added_seg_count++, seg_id, grouter_->globalRoutingToBox(seg)));
      }
    }
    seg_id++;
  }
  return added_seg_count;
}

static void getSegmentsWithOverlap(SegmentNode& seg_info,
                                   const std::vector<SegmentNode>& segs,
                                   odb::dbTechLayer* layer)
{
  int index = 0;
  // Iterate all segment vector to find overlap
  for (const SegmentNode& seg_it : segs) {
    if (seg_info.rect.overlaps(seg_it.rect)) {
      seg_info.adjs.push_back(std::make_pair(layer, index));
    }
    index++;
  }
}

void RepairAntennas::setAdjacentSegments(
    LayerToSegmentNodeVector& segment_by_layer)
{
  // Set adjacents segments (neighbor)
  for (auto& layer_it : segment_by_layer) {
    odb::dbTechLayer* tech_layer = layer_it.first;
    // Find segment nodess in lower layer
    odb::dbTechLayer* lower_layer = tech_layer->getLowerLayer();
    std::vector<SegmentNode> lower_segments;
    if (lower_layer
        && segment_by_layer.find(lower_layer) != segment_by_layer.end()) {
      lower_segments = segment_by_layer[lower_layer];
    }
    // Find the segment nodes in upper layer
    odb::dbTechLayer* upper_layer = tech_layer->getUpperLayer();
    std::vector<SegmentNode> upper_segments;
    if (upper_layer
        && segment_by_layer.find(upper_layer) != segment_by_layer.end()) {
      upper_segments = segment_by_layer[upper_layer];
    }
    // Get segments node of current layer
    auto& current_segments = layer_it.second;
    for (SegmentNode& seg_it : current_segments) {
      // Find segments in current layer that touch the segment node
      getSegmentsWithOverlap(seg_it, current_segments, tech_layer);

      // Find segment nodes in lower layer that touch the segment node
      if (!lower_segments.empty()) {
        getSegmentsWithOverlap(seg_it, lower_segments, lower_layer);
      }
      // Find segment nodes in upper layer that touch the segment node
      if (!upper_segments.empty()) {
        getSegmentsWithOverlap(seg_it, upper_segments, upper_layer);
      }
    }
  }
}

int RepairAntennas::buildSegmentGraph(const GRoute& route,
                                      const int& max_layer,
                                      LayerToSegmentNodeVector& graph)
{
  // Create nodes by each segment
  int seg_count = getSegmentsPerLayer(route, max_layer, graph);
  // Create conections between nodes on graph
  setAdjacentSegments(graph);

  return seg_count;
}

void RepairAntennas::getSegmentsConnectedToPin(
    const odb::dbITerm* iterm,
    LayerToSegmentNodeVector& segment_by_layer,
    SegmentNodeIds& seg_connected_to_pin)
{
  odb::dbMTerm* mterm = iterm->getMTerm();
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
      // Iterate segment nodes in the same layer
      int seg_id = 0;
      for (const SegmentNode& it : segment_by_layer[tech_layer]) {
        // Check if pin has overlap with segment
        if (it.rect.overlaps(pin_rect)) {
          seg_connected_to_pin[tech_layer].insert(seg_id);
        }
        seg_id++;
      }
    }
  }
}

// Find jumper position between a init and final position seeking to leave the
// segment with minimum size connected to the target
int RepairAntennas::getJumperPosition(const int& init_pos,
                                      const int& final_pos,
                                      const int& target_pos)
{
  int position;
  if (target_pos <= init_pos) {
    position = init_pos + tile_size_;
  } else if (target_pos >= final_pos) {
    position = final_pos - tile_size_ - jumper_size_;
  } else if ((target_pos - init_pos) > (final_pos - target_pos)) {
    position = target_pos - tile_size_ - jumper_size_;
  } else {
    position = target_pos + tile_size_;
  }
  return position;
}

void RepairAntennas::findJumperCandidatePositions(
    const int& init_x,
    const int& init_y,
    const int& final_x,
    const int& final_y,
    const odb::Point& parent_pos,
    const bool& is_horizontal,
    std::vector<int>& candidate_positions)
{
  // One tile size of distance from init or segment end
  const int jumper_dist = 1;
  // Get size from last vias to the position
  const int free_via_size
      = std::abs(final_x - init_x) + std::abs(final_y - init_y);
  // Check if the jumper can be added in the sub segment
  if (free_via_size > 0
      && free_via_size >= jumper_size_ + (2 * jumper_dist * tile_size_)) {
    // Calculate the final jumper position
    if (is_horizontal) {
      candidate_positions.push_back(
          getJumperPosition(init_x, final_x, parent_pos.x()));
    } else {
      candidate_positions.push_back(
          getJumperPosition(init_y, final_y, parent_pos.y()));
    }
  }
}

static void getViaPosition(LayerToSegmentNodeVector& segment_graph,
                           const GRoute& route,
                           const SegmentNode& seg_node,
                           std::unordered_set<int>& via_pos)
{
  const GSegment& seg = route[seg_node.seg_id];
  const int layer_level = seg.init_layer;
  const bool is_horizontal = (seg.init_x != seg.final_x);
  // Get via positions from adjacent segment nodes
  for (const auto& adj_it : seg_node.adjs) {
    int adj_layer_level = adj_it.first->getRoutingLevel();
    if (adj_layer_level != layer_level) {
      int adj_seg_id = segment_graph[adj_it.first][adj_it.second].seg_id;
      if (is_horizontal) {
        via_pos.insert(route[adj_seg_id].init_x);
      } else {
        via_pos.insert(route[adj_seg_id].init_y);
      }
    }
  }
}

int RepairAntennas::getBestPosition(const std::vector<int>& candidate_positions,
                                    const bool& is_horizontal,
                                    const odb::Point& parent_pos)
{
  int jumper_position = -1;
  // Find best position of the position candidates
  int min_dist = -1, dist;
  for (const int& pos : candidate_positions) {
    // Get distance
    if (is_horizontal) {
      dist = std::abs((pos + tile_size_) - parent_pos.x());
    } else {
      dist = std::abs((pos + tile_size_) - parent_pos.y());
    }
    if (min_dist == -1 || dist < min_dist) {
      min_dist = dist;
      jumper_position = pos;
    }
  }
  return jumper_position;
}

bool RepairAntennas::findPosToJumper(const GRoute& route,
                                     LayerToSegmentNodeVector& segment_graph,
                                     const SegmentNode& seg_node,
                                     const odb::Point& parent_pos,
                                     int& jumper_position,
                                     odb::dbNet* db_net)
{
  jumper_position = -1;
  const GSegment& seg = route[seg_node.seg_id];
  // Ignore small segments
  if (seg.length() < smaller_seg_size_) {
    return false;
  }
  // Get init and final position of segment
  const int seg_init_x = seg.init_x;
  const int seg_init_y = seg.init_y;
  const int seg_final_x = seg.final_x;
  const int seg_final_y = seg.final_y;
  const int step = tile_size_;

  int pos_x = seg_init_x;
  int pos_y = seg_init_y;
  int last_block_x = pos_x;
  int last_block_y = pos_y;
  bool has_available_resources;
  bool is_via;

  const int layer_level = seg.init_layer;
  const bool is_horizontal = (seg.init_x != seg.final_x);
  // Get map of pair with vias pos, horizontal save X and vertical save Y
  std::unordered_set<int> via_pos;
  getViaPosition(segment_graph, route, seg_node, via_pos);

  // Save best positions to add jumper
  std::vector<int> candidate_positions;
  // Iterate all position of segment
  while (pos_x <= seg_final_x && pos_y <= seg_final_y) {
    // Check if the position has resources available
    has_available_resources = grouter_->hasAvailableResources(
        is_horizontal, pos_x, pos_y, layer_level + 2, db_net);
    is_via = (is_horizontal && via_pos.find(pos_x) != via_pos.end())
             || (!is_horizontal && via_pos.find(pos_y) != via_pos.end());
    // If the position has vias or does not have resources
    if (is_via || !has_available_resources) {
      findJumperCandidatePositions(last_block_x,
                                   last_block_y,
                                   pos_x,
                                   pos_y,
                                   parent_pos,
                                   is_horizontal,
                                   candidate_positions);
      last_block_x = pos_x;
      last_block_y = pos_y;
    }
    if (is_horizontal) {
      pos_x += step;
    } else {
      pos_y += step;
    }
  }
  // If the segment has no vias in the end, verify last sub segment
  if (last_block_x != seg_final_x || last_block_y != seg_final_y) {
    findJumperCandidatePositions(last_block_x,
                                 last_block_y,
                                 seg_final_x,
                                 seg_final_y,
                                 parent_pos,
                                 is_horizontal,
                                 candidate_positions);
  }
  jumper_position
      = getBestPosition(candidate_positions, is_horizontal, parent_pos);
  return (jumper_position != -1);
}

static odb::Point findSegmentPos(const GSegment& seg)
{
  // Get the middle position of the segment
  int x = (seg.init_x + seg.final_x) / 2;
  int y = (seg.init_y + seg.final_y) / 2;
  return odb::Point(x, y);
}

// Use DFS to find segment in layer violations connect with pin
void RepairAntennas::findSegments(const GRoute& route,
                                  odb::dbITerm* iterm,
                                  const SegmentNodeIds& segment_ids,
                                  LayerToSegmentNodeVector& segment_graph,
                                  const int& num_nodes,
                                  const int& violation_layer,
                                  SegmentToJumperPos& segments_to_repair,
                                  odb::dbNet* db_net)
{
  // Init stack and vector of visited and parent position
  std::stack<std::pair<odb::dbTechLayer*, SegmentNode>> node_stack;
  std::vector<bool> visited(num_nodes, false);
  std::vector<odb::Point> parent_pos(num_nodes);

  // Get pos of iterm
  odb::dbInst* sink_inst = iterm->getInst();
  odb::Rect sink_bbox = getInstRect(sink_inst, iterm);
  odb::Point gate_pos(sink_bbox.xCenter(), sink_bbox.yCenter());
  // Convert position --> move pos on GCell middle
  gate_pos = grouter_->getPositionOnGrid(gate_pos);
  // Insert init segments to stack
  for (const auto& layer_it : segment_ids) {
    for (const auto& node_it : layer_it.second) {
      node_stack.push({layer_it.first, segment_graph[layer_it.first][node_it]});
      parent_pos[segment_graph[layer_it.first][node_it].node_id] = gate_pos;
    }
  }
  // Run DFS
  SegmentNode cur_node, adj_node;
  odb::dbTechLayer* cur_layer;
  while (!node_stack.empty()) {
    // Get next node
    cur_layer = node_stack.top().first;
    cur_node = node_stack.top().second;
    node_stack.pop();
    // If node was visited
    if (visited[cur_node.node_id]) {
      continue;
    }

    visited[cur_node.node_id] = true;

    int layer_level = cur_layer->getRoutingLevel();
    if (layer_level == violation_layer && cur_node.seg_id != -1) {
      // Candidate to add jumper - find pos checking len and capacity
      int jumper_pos;
      // Convert position --> move pos on tile middle
      parent_pos[cur_node.node_id]
          = grouter_->getPositionOnGrid(parent_pos[cur_node.node_id]);
      bool is_found = findPosToJumper(route,
                                      segment_graph,
                                      cur_node,
                                      parent_pos[cur_node.node_id],
                                      jumper_pos,
                                      db_net);
      // If jumper wasnt added, then explore adjacent segment nodes
      if (is_found) {
        segments_to_repair[cur_node.seg_id].insert(jumper_pos);
        continue;
      }
    }

    // Get all adjs from cur segment node
    for (const auto& adj_it : cur_node.adjs) {
      // Only segment nodes with layer below the violation layer or vias
      if (layer_level == 0
          || (layer_level != 0 && layer_level <= violation_layer)) {
        adj_node = segment_graph[adj_it.first][adj_it.second];
        node_stack.push({adj_it.first, adj_node});
        // Update parent position (x, y)
        if (cur_node.seg_id != -1) {
          parent_pos[adj_node.node_id] = findSegmentPos(route[cur_node.seg_id]);
        }
      }
    }
  }
}

void RepairAntennas::getViolations(
    const std::vector<ant::Violation>& violations,
    const int& max_routing_layer,
    std::vector<int>& violation_id_to_repair,
    int& max_layer_to_repair)
{
  odb::dbTech* tech = db_->getTech();
  int violation_id = 0;
  // Iterate all layers with violaions to check if jumper can be added
  for (const auto& violation : violations) {
    odb::dbTechLayer* violation_layer
        = tech->findRoutingLayer(violation.routing_level);
    odb::dbTechLayer* upper_layer
        = tech->findRoutingLayer(violation.routing_level + 2);
    // Check if the violation layer is valid and if it has layer + 2
    if (upper_layer && upper_layer->getRoutingLevel() <= max_routing_layer
        && violation_layer->getType() == odb::dbTechLayerType::ROUTING) {
      violation_id_to_repair.push_back(violation_id);
      max_layer_to_repair
          = std::max(max_layer_to_repair, violation.routing_level);
    }
    violation_id++;
  }
}

int RepairAntennas::addJumperOnSegments(
    const SegmentToJumperPos& segments_to_repair,
    GRoute& route,
    odb::dbNet* db_net)
{
  int jumper_by_net = 0;
  // Iterate all jumper positions on segments
  for (const auto& seg_it : segments_to_repair) {
    int last_pos_aux = -1;
    for (const auto& pos_it : seg_it.second) {
      const int seg_len = route[seg_it.first].length();
      if (seg_len < smaller_seg_size_) {
        break;
      }
      if (last_pos_aux != -1) {
        // Avoid overlap with last jumper position
        const int dist = abs(last_pos_aux - pos_it);
        if (dist > jumper_size_) {
          addJumper(route, seg_it.first, pos_it, db_net);
          jumper_by_net++;
        }
      } else {
        addJumper(route, seg_it.first, pos_it, db_net);
        jumper_by_net++;
      }
      last_pos_aux = pos_it;
    }
  }
  return jumper_by_net;
}

void RepairAntennas::jumperInsertion(NetRouteMap& routing,
                                     const int& tile_size,
                                     const int& max_routing_layer,
                                     std::vector<odb::dbNet*>& modified_nets)
{
  // Init jumper size
  tile_size_ = tile_size;
  jumper_size_ = 2 * tile_size_;
  smaller_seg_size_ = 5 * tile_size_;
  int total_jumpers = 0;
  int jumper_by_net, required_jumper_by_net;
  int net_with_jumpers = 0;
  // Iterate the antenna violations
  for (const auto& net_violations : antenna_violations_) {
    odb::dbNet* db_net = net_violations.first;
    const auto& violations = net_violations.second;
    std::vector<int> violation_id_to_repair;

    // Get available violations to add jumper
    int max_layer_to_repair = -1;
    getViolations(violations,
                  max_routing_layer,
                  violation_id_to_repair,
                  max_layer_to_repair);

    SegmentToJumperPos segments_to_repair;
    required_jumper_by_net = 0;
    // Check if has violation to repair with jumpers
    if (!violation_id_to_repair.empty()) {
      // Build graph of segments
      LayerToSegmentNodeVector segment_graph;
      const int num_nodes = buildSegmentGraph(
          routing[db_net], max_layer_to_repair, segment_graph);

      // Iterate all valid violations
      for (const int& violation_id : violation_id_to_repair) {
        const int layer_level = violations[violation_id].routing_level;
        // Iterate all pins in this violation
        for (const auto& iterm : violations[violation_id].gates) {
          SegmentNodeIds connected_seg_ids;
          // Get segments connect to pin with violations
          getSegmentsConnectedToPin(iterm, segment_graph, connected_seg_ids);
          findSegments(routing[db_net],
                       iterm,
                       connected_seg_ids,
                       segment_graph,
                       num_nodes,
                       layer_level,
                       segments_to_repair,
                       db_net);
        }
      }
      required_jumper_by_net = segments_to_repair.size();
    }
    if (required_jumper_by_net > 0) {
      // Add jumper in found segment positions
      jumper_by_net
          = addJumperOnSegments(segments_to_repair, routing[db_net], db_net);
      if (jumper_by_net > 0) {
        db_net->setJumpers(true);
        net_with_jumpers++;
        total_jumpers += jumper_by_net;
        modified_nets.push_back(db_net);
      }
    }
  }
  logger_->info(GRT,
                302,
                "Inserted {} jumpers for {} nets.",
                total_jumpers,
                net_with_jumpers);
}

}  // namespace grt
