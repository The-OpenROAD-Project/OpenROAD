// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "power_cells.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "domain.h"
#include "grid.h"
#include "odb/db.h"
#include "pdn/PdnGen.hh"
#include "shape.h"
#include "straps.h"
#include "utl/Logger.h"

namespace pdn {

PowerCell::PowerCell(utl::Logger* logger,
                     odb::dbMaster* master,
                     odb::dbMTerm* control,
                     odb::dbMTerm* acknowledge,
                     odb::dbMTerm* switched_power,
                     odb::dbMTerm* alwayson_power,
                     odb::dbMTerm* ground)
    : logger_(logger),
      master_(master),
      control_(control),
      acknowledge_(acknowledge),
      switched_power_(switched_power),
      alwayson_power_(alwayson_power),
      ground_(ground)
{
}

std::string PowerCell::getName() const
{
  return master_->getName();
}

void PowerCell::report() const
{
  logger_->report("Switched power cell: {}", master_->getName());
  logger_->report("  Control pin: {}", control_->getName());
  if (acknowledge_ != nullptr) {
    logger_->report("  Acknowledge pin: {}", acknowledge_->getName());
  }
  logger_->report("  Switched power pin: {}", switched_power_->getName());
  logger_->report("  Always on power pin: {}", alwayson_power_->getName());
  logger_->report("  Ground pin: {}", ground_->getName());
}

void PowerCell::populateAlwaysOnPinPositions(int site_width)
{
  alwayson_power_positions_.clear();

  for (auto* pin : alwayson_power_->getMPins()) {
    for (auto* box : pin->getGeometry()) {
      odb::Rect bbox = box->getBox();

      const auto pin_pos = getRectAsSiteWidths(bbox, site_width, 0);
      alwayson_power_positions_.insert(pin_pos.begin(), pin_pos.end());
    }
  }
}

std::set<int> PowerCell::getRectAsSiteWidths(const odb::Rect& rect,
                                             int site_width,
                                             int offset)
{
  std::set<int> pos;
  const int x_start
      = std::ceil(static_cast<double>(rect.xMin() - offset) / site_width)
        * site_width;
  const int x_end
      = std::floor(static_cast<double>(rect.xMax() - offset) / site_width)
        * site_width;
  for (int x = x_start; x <= x_end; x += site_width) {
    pos.insert(x + offset);
  }
  return pos;
}

bool PowerCell::appliesToRow(odb::dbRow* row) const
{
  // needs to be string compare because site pointers can come from different
  // libraries
  return master_->getSite()->getName() == row->getSite()->getName();
}

//////////

GridSwitchedPower::GridSwitchedPower(Grid* grid,
                                     PowerCell* cell,
                                     odb::dbNet* control,
                                     PowerSwitchNetworkType network)
    : grid_(grid), cell_(cell), control_(control), network_(network)
{
  if (network_ == PowerSwitchNetworkType::DAISY && !cell->hasAcknowledge()) {
    grid->getLogger()->error(
        utl::PDN,
        198,
        "{} requires the power cell to have an acknowledge pin.",
        toString(DAISY));
  }
}

std::string GridSwitchedPower::toString(PowerSwitchNetworkType type)
{
  switch (type) {
    case STAR:
      return "STAR";
    case DAISY:
      return "DAISY";
  }
  return "unknown";
}

PowerSwitchNetworkType GridSwitchedPower::fromString(const std::string& type,
                                                     utl::Logger* logger)
{
  if (type == "STAR") {
    return STAR;
  }
  if (type == "DAISY") {
    return DAISY;
  }

  logger->error(utl::PDN, 197, "Unrecognized network type: {}", type);
  return STAR;
}

void GridSwitchedPower::report() const
{
  auto* logger = grid_->getLogger();
  logger->report("Switched power cell: {}", cell_->getName());
  std::string control_net = control_ ? control_->getName() : "undefined";
  logger->report("  Control net: {}", control_net);
  logger->report("  Network type: {}", toString(network_));
}

GridSwitchedPower::InstTree GridSwitchedPower::buildInstanceSearchTree() const
{
  InstTree exisiting_insts;
  for (auto* inst : grid_->getBlock()->getInsts()) {
    if (!inst->getPlacementStatus().isFixed()) {
      continue;
    }

    exisiting_insts.insert(inst);
  }

  return exisiting_insts;
}

Shape::ShapeTree GridSwitchedPower::buildStrapTargetList(Straps* target) const
{
  const odb::dbNet* alwayson = grid_->getDomain()->getAlwaysOnPower();
  const auto& target_shapes = target->getShapes();

  Shape::ShapeTree targets;
  for (auto& shape : target_shapes.at(target->getLayer())) {
    if (shape->getNet() != alwayson) {
      continue;
    }
    targets.insert(shape);
  }

  return targets;
}

GridSwitchedPower::RowTree GridSwitchedPower::buildRowTree() const
{
  RowTree row_search;
  for (auto* row : grid_->getDomain()->getRows()) {
    row_search.insert(row);
  }

  return row_search;
}

std::set<odb::dbRow*> GridSwitchedPower::getInstanceRows(
    odb::dbInst* inst,
    const RowTree& row_search) const
{
  std::set<odb::dbRow*> rows;

  odb::Rect box = inst->getBBox()->getBox();

  for (auto itr = row_search.qbegin(bgi::intersects(box));
       itr != row_search.qend();
       itr++) {
    auto* row = *itr;
    odb::Rect row_box = row->getBBox();

    if (row_box.overlaps(box)) {
      rows.insert(row);
    }
  }

  return rows;
}

void GridSwitchedPower::build()
{
  if (!insts_.empty()) {
    // power switches already built and need to be ripped up to try again
    grid_->getLogger()->warn(
        utl::PDN,
        222,
        "Power switch insertion has already run. To reset use -ripup option.");
    return;
  }

  odb::Rect core_area = grid_->getBlock()->getCoreArea();

  auto* target = getLowestStrap();
  if (target == nullptr) {
    grid_->getLogger()->error(
        utl::PDN, 220, "Unable to find a strap to connect power switched to.");
  }

  const InstTree exisiting_insts = buildInstanceSearchTree();

  odb::dbNet* switched = grid_->getDomain()->getSwitchedPower();
  odb::dbNet* alwayson = grid_->getDomain()->getAlwaysOnPower();
  odb::dbNet* ground = grid_->getDomain()->getGround();

  odb::dbRegion* region = grid_->getDomain()->getRegion();

  const Shape::ShapeTree targets = buildStrapTargetList(target);
  const RowTree row_search = buildRowTree();

  bool found_row = false;

  for (auto* row : grid_->getDomain()->getRows()) {
    if (!cell_->appliesToRow(row)) {
      continue;
    }
    found_row = true;

    const int site_width = row->getSite()->getWidth();
    cell_->populateAlwaysOnPinPositions(site_width);
    const std::string inst_prefix = inst_prefix_ + row->getName() + "_";
    int idx = 0;

    debugPrint(grid_->getLogger(),
               utl::PDN,
               "PowerSwitch",
               2,
               "Adding power switches in row: {}",
               row->getName());

    odb::Rect bbox = row->getBBox();
    std::vector<odb::Rect> straps;
    for (auto itr = targets.qbegin(bgi::intersects(bbox));
         itr != targets.qend();
         itr++) {
      const auto& shape = *itr;
      straps.push_back(shape->getRect());
    }

    std::ranges::sort(straps, [](const odb::Rect& lhs, const odb::Rect& rhs) {
      return lhs.xMin() < rhs.xMin();
    });

    for (const auto& strap : straps) {
      const std::string new_name = inst_prefix + std::to_string(idx++);
      auto* inst = odb::dbInst::create(
          grid_->getBlock(), cell_->getMaster(), new_name.c_str(), true);
      if (inst == nullptr) {
        inst = grid_->getBlock()->findInst(new_name.c_str());
        if (inst->getMaster() != cell_->getMaster()) {
          grid_->getLogger()->error(utl::PDN,
                                    221,
                                    "Instance {} should be {}, but is {}.",
                                    new_name,
                                    cell_->getMaster()->getName(),
                                    inst->getMaster()->getName());
        }
      }

      if (region != nullptr) {
        region->addInst(inst);
      }

      debugPrint(grid_->getLogger(),
                 utl::PDN,
                 "PowerSwitch",
                 3,
                 "Adding switch {}",
                 new_name);

      const auto locations = computeLocations(strap, site_width, core_area);
      inst->setLocation(*locations.begin(), bbox.yMin());
      inst->setLocationOrient(row->getOrient());

      const auto inst_rows = getInstanceRows(inst, row_search);
      if (inst_rows.size() < 2) {
        // inst is not in multiple rows, so remove
        odb::dbInst::destroy(inst);
        const double dbu_to_microns = grid_->getBlock()->getDbUnitsPerMicron();
        grid_->getLogger()->warn(
            utl::PDN,
            223,
            "Unable to insert power switch ({}) at ({:.4f}, {:.4f}), due to "
            "lack of available rows.",
            new_name,
            *locations.begin() / dbu_to_microns,
            bbox.yMin() / dbu_to_microns);
        continue;
      }

      inst->getITerm(cell_->getGroundPin())->connect(ground);
      inst->getITerm(cell_->getAlwaysOnPowerPin())->connect(alwayson);
      inst->getITerm(cell_->getSwitchedPowerPin())->connect(switched);

      insts_[inst] = InstanceInfo{locations, inst_rows};
    }
  }

  if (!found_row) {
    grid_->getLogger()->error(utl::PDN,
                              240,
                              "No rows found that match the power cell: {}.",
                              cell_->getMaster()->getName());
  }

  updateControlNetwork();

  checkAndFixOverlappingInsts(exisiting_insts);

  for (const auto& [inst, inst_info] : insts_) {
    inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  }
}

void GridSwitchedPower::updateControlNetwork()
{
  switch (network_) {
    case STAR:
      updateControlNetworkSTAR();
      break;
    case DAISY:
      updateControlNetworkDAISY(true);
      break;
  }
}

void GridSwitchedPower::updateControlNetworkSTAR()
{
  for (const auto& [inst, inst_info] : insts_) {
    inst->getITerm(cell_->getControlPin())->connect(control_);
  }
}

void GridSwitchedPower::updateControlNetworkDAISY(const bool order_by_x)
{
  std::map<int, std::vector<odb::dbInst*>> inst_order;

  for (const auto& [inst, inst_info] : insts_) {
    int loc;
    int x, y;
    inst->getLocation(x, y);
    if (order_by_x) {
      loc = x;
    } else {
      loc = y;
    }

    inst_order[loc].push_back(inst);
  }

  for (auto& [pos, insts] : inst_order) {
    std::ranges::sort(insts, [order_by_x](odb::dbInst* lhs, odb::dbInst* rhs) {
      int lhs_x, lhs_y;
      lhs->getLocation(lhs_x, lhs_y);
      int rhs_x, rhs_y;
      rhs->getLocation(rhs_x, rhs_y);

      if (order_by_x) {
        return lhs_y < rhs_y;
      }
      return lhs_x < rhs_x;
    });
  }

  auto get_next_ack = [this](const std::string& inst_name) {
    std::string net_name
        = inst_name + "_" + cell_->getAcknowledgePin()->getName();
    auto* ack = odb::dbNet::create(grid_->getBlock(), net_name.c_str());
    if (ack == nullptr) {
      return grid_->getBlock()->findNet(net_name.c_str());
    }
    return ack;
  };

  odb::dbNet* control = control_;
  for (const auto& [pos, insts] : inst_order) {
    odb::dbNet* next_control = nullptr;
    for (auto* inst : insts) {
      odb::dbNet* ack = get_next_ack(inst->getName());

      inst->getITerm(cell_->getControlPin())->connect(control);
      inst->getITerm(cell_->getAcknowledgePin())->connect(ack);

      control = ack;
      if (next_control == nullptr) {
        next_control = ack;
      }
    }
    control = next_control;
  }

  // remove dangling signals
  for (const auto& [inst, inst_info] : insts_) {
    odb::dbNet* net = inst->getITerm(cell_->getAcknowledgePin())->getNet();
    if (net == nullptr) {
      continue;
    }
    if (net->getITermCount() < 2) {
      odb::dbNet::destroy(net);
    }
  }
}

void GridSwitchedPower::checkAndFixOverlappingInsts(const InstTree& insts)
{
  // needs to check for bounds of the rows
  for (const auto& [inst, inst_info] : insts_) {
    auto* overlapping = checkOverlappingInst(inst, insts);
    if (overlapping == nullptr) {
      continue;
    }
    debugPrint(grid_->getLogger(),
               utl::PDN,
               "PowerSwitch",
               2,
               "Power switch {} overlaps with {}",
               inst->getName(),
               overlapping->getName());

    int x, y;
    inst->getLocation(x, y);
    bool fixed = false;
    // start by checking if this can be resolved by moving the power switch
    for (int new_pos : inst_info.sites) {
      if (new_pos == x) {
        continue;
      }

      inst->setLocation(new_pos, y);
      if (!checkInstanceOverlap(inst, overlapping)) {
        debugPrint(
            grid_->getLogger(),
            utl::PDN,
            "PowerSwitch",
            3,
            "Fixed by moving {} to ({}, {})",
            inst->getName(),
            new_pos
                / static_cast<double>(grid_->getBlock()->getDbUnitsPerMicron()),
            y / static_cast<double>(grid_->getBlock()->getDbUnitsPerMicron()));
        fixed = true;
        break;
      }
    }
    if (fixed) {
      continue;
    }
    // restore original position
    inst->setLocation(x, y);
    // next find minimum shift of other cell
    const int pws_min = *inst_info.sites.begin();
    const int pws_max = *inst_info.sites.rbegin();
    const int pws_width = cell_->getMaster()->getWidth();

    int overlap_y;
    overlapping->getLocation(x, overlap_y);
    const int other_width = overlapping->getMaster()->getWidth();

    const int other_avg = x + other_width / 2;
    const int pws_min_avg = pws_min + pws_width / 2;
    const int pws_max_avg = pws_max + pws_width / 2;

    const int pws_min_displacement = std::abs(pws_min_avg - other_avg);
    const int pws_max_displacement = std::abs(pws_max_avg - other_avg);

    int pws_new_loc, other_new_loc;
    if (pws_min_displacement < pws_max_displacement) {
      pws_new_loc = pws_min;
      other_new_loc = pws_new_loc + pws_width;
    } else {
      pws_new_loc = pws_max;
      other_new_loc = pws_new_loc - other_width;
    }

    inst->setLocation(pws_new_loc, y);
    // Allow us to move fixed tapcells
    auto prev_status = overlapping->getPlacementStatus();
    overlapping->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    overlapping->setLocation(other_new_loc, overlap_y);
    overlapping->setPlacementStatus(prev_status);
    debugPrint(grid_->getLogger(),
               utl::PDN,
               "PowerSwitch",
               3,
               "Fixed by moving {} to ({}, {}) and {} to ({}, {})",
               inst->getName(),
               pws_new_loc,
               y,
               overlapping->getName(),
               other_new_loc,
               overlap_y);
  }
}

bool GridSwitchedPower::checkInstanceOverlap(odb::dbInst* inst0,
                                             odb::dbInst* inst1) const
{
  odb::Rect inst0_bbox = inst0->getBBox()->getBox();
  odb::Rect inst1_bbox = inst1->getBBox()->getBox();

  return inst0_bbox.overlaps(inst1_bbox);
}

odb::dbInst* GridSwitchedPower::checkOverlappingInst(
    odb::dbInst* cell,
    const InstTree& insts) const
{
  odb::Rect bbox = cell->getBBox()->getBox();

  for (auto itr = insts.qbegin(bgi::intersects(bbox)); itr != insts.qend();
       itr++) {
    auto* other_inst = *itr;
    if (checkInstanceOverlap(cell, other_inst)) {
      return other_inst;
    }
  }

  return nullptr;
}

void GridSwitchedPower::ripup()
{
  for (const auto& [inst, inst_info] : insts_) {
    if (cell_->hasAcknowledge()) {
      auto* net = inst->getITerm(cell_->getAcknowledgePin())->getNet();
      if (net != nullptr) {
        odb::dbNet::destroy(net);
      }
    }

    odb::dbInst::destroy(inst);
  }
  insts_.clear();
}

Straps* GridSwitchedPower::getLowestStrap() const
{
  Straps* target = nullptr;

  for (const auto& strap : grid_->getStraps()) {
    if (strap->type() != GridComponent::Strap) {
      continue;
    }

    if (target == nullptr) {
      target = strap.get();
    } else {
      const int target_level = target->getLayer()->getRoutingLevel();
      const int strap_level = strap->getLayer()->getRoutingLevel();

      if (target_level == strap_level) {
        if (target->getShapeCount() < strap->getShapeCount()) {
          // use the one with more shapes
          target = strap.get();
        }
      } else if (target_level > strap_level) {
        target = strap.get();
      }
    }
  }

  return target;
}

Shape::ShapeTreeMap GridSwitchedPower::getShapes() const
{
  odb::dbNet* alwayson = grid_->getDomain()->getAlwaysOnPower();

  Shape::ShapeTreeMap shapes;

  for (const auto& [inst, inst_info] : insts_) {
    for (const auto& [layer, inst_shapes] :
         InstanceGrid::getInstancePins(inst)) {
      auto& layer_shapes = shapes[layer];
      for (const auto& shape : inst_shapes) {
        if (shape->getNet() == alwayson) {
          layer_shapes.insert(shape);
        }
      }
    }
  }

  return shapes;
}

std::set<int> GridSwitchedPower::computeLocations(
    const odb::Rect& strap,
    int site_width,
    const odb::Rect& corearea) const
{
  const auto& pin_pos = cell_->getAlwaysOnPowerPinPositions();
  const int min_pin = *pin_pos.begin();
  const int max_pin = *pin_pos.rbegin();

  std::set<int> pos;
  for (auto strap_pos :
       PowerCell::getRectAsSiteWidths(strap, site_width, corearea.xMin())) {
    for (auto pin : cell_->getAlwaysOnPowerPinPositions()) {
      const int new_pos = strap_pos - pin;

      const int new_min_pin = new_pos + min_pin;
      const int new_max_pin = new_pos + max_pin;

      if (new_min_pin >= strap.xMin() && new_max_pin <= strap.xMax()) {
        // pin is completely inside strap
        pos.insert(new_pos);
      } else if (new_min_pin <= strap.xMin() && new_max_pin >= strap.xMax()) {
        // pin is completely overlapping strap
        pos.insert(new_pos);
      }
    }
  }

  return pos;
}

}  // namespace pdn
