// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "PadPlacer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"
#include "utl/Logger.h"

namespace pad {

PadPlacer::PadPlacer(utl::Logger* logger,
                     odb::dbBlock* block,
                     const std::vector<odb::dbInst*>& insts,
                     const odb::Direction2D::Value& edge,
                     odb::dbRow* row)
    : logger_(logger), block_(block), insts_(insts), edge_(edge), row_(row)
{
  populateInstWidths();
}

void PadPlacer::populateInstWidths()
{
  const odb::dbTransform xform(row_->getOrient());
  inst_widths_.clear();

  for (auto* inst : getInsts()) {
    auto* master = inst->getMaster();
    odb::Rect inst_bbox;
    master->getPlacementBoundary(inst_bbox);
    xform.apply(inst_bbox);

    switch (getRowEdge()) {
      case odb::Direction2D::North:
      case odb::Direction2D::South:
        inst_widths_[inst] = inst_bbox.dx();
        break;
      case odb::Direction2D::West:
      case odb::Direction2D::East:
        inst_widths_[inst] = inst_bbox.dy();
        break;
    }
  }
}

int PadPlacer::getRowStart() const
{
  const odb::Rect row_bbox = getRow()->getBBox();

  int row_start = 0;
  switch (getRowEdge()) {
    case odb::Direction2D::North:
    case odb::Direction2D::South:
      row_start = row_bbox.xMin();
      break;
    case odb::Direction2D::West:
    case odb::Direction2D::East:
      row_start = row_bbox.yMin();
      break;
  }

  return row_start;
}

int PadPlacer::getRowEnd() const
{
  const odb::Rect row_bbox = getRow()->getBBox();

  int row_end = 0;
  switch (getRowEdge()) {
    case odb::Direction2D::North:
    case odb::Direction2D::South:
      row_end = row_bbox.xMax();
      break;
    case odb::Direction2D::West:
    case odb::Direction2D::East:
      row_end = row_bbox.yMax();
      break;
  }

  return row_end;
}

int PadPlacer::getRowWidth() const
{
  const odb::Rect row_bbox = getRow()->getBBox();

  int row_width = 0;
  switch (getRowEdge()) {
    case odb::Direction2D::North:
    case odb::Direction2D::South:
      row_width = row_bbox.dx();
      break;
    case odb::Direction2D::West:
    case odb::Direction2D::East:
      row_width = row_bbox.dy();
      break;
  }

  return row_width;
}

int PadPlacer::getTotalInstWidths() const
{
  int width = 0;
  for (const auto& [inst, inst_w] : inst_widths_) {
    width += inst_w;
  }
  return width;
}

int PadPlacer::snapToRowSite(int location) const
{
  const odb::Point origin = row_->getOrigin();

  const double spacing = row_->getSpacing();
  int relative_location;
  if (row_->getDirection() == odb::dbRowDir::HORIZONTAL) {
    relative_location = location - origin.x();
  } else {
    relative_location = location - origin.y();
  }

  int site_count = std::round(relative_location / spacing);
  site_count = std::max(0, site_count);
  site_count = std::min(site_count, row_->getSiteCount());

  return site_count;
}

int PadPlacer::placeInstance(int index,
                             odb::dbInst* inst,
                             const odb::dbOrientType& base_orient,
                             bool allow_overlap,
                             bool allow_shift) const
{
  const int origin_offset = index * row_->getSpacing();

  const odb::Rect row_bbox = row_->getBBox();

  odb::dbTransform xform(base_orient);
  xform.concat(row_->getOrient());
  inst->setOrient(xform.getOrient());
  const odb::Rect inst_bbox = inst->getBBox()->getBox();

  odb::Point index_pt;
  switch (edge_) {
    case odb::Direction2D::North:
      index_pt = odb::Point(row_bbox.xMin() + origin_offset,
                            row_bbox.yMax() - inst_bbox.dy());
      break;
    case odb::Direction2D::South:
      index_pt = odb::Point(row_bbox.xMin() + origin_offset, row_bbox.yMin());
      break;
    case odb::Direction2D::West:
      index_pt = odb::Point(row_bbox.xMin(), row_bbox.yMin() + origin_offset);
      break;
    case odb::Direction2D::East:
      index_pt = odb::Point(row_bbox.xMax() - inst_bbox.dx(),
                            row_bbox.yMin() + origin_offset);
      break;
  }

  inst->setLocation(index_pt.x(), index_pt.y());
  const odb::Rect inst_rect = inst->getBBox()->getBox();
  auto* block = getBlock();
  const double dbus = block->getDbUnitsPerMicron();

  // Check if its in the row
  bool outofrow = false;
  switch (edge_) {
    case odb::Direction2D::North:
    case odb::Direction2D::South:
      if (row_bbox.xMin() > inst_rect.xMin()
          || row_bbox.xMax() < inst_rect.xMax()) {
        if (!allow_overlap) {
          outofrow = true;
        }
      }
      break;
    case odb::Direction2D::West:
    case odb::Direction2D::East:
      if (row_bbox.yMin() > inst_rect.yMin()
          || row_bbox.yMax() < inst_rect.yMax()) {
        if (!allow_overlap) {
          outofrow = true;
        }
      }
      break;
  }
  if (outofrow) {
    logger_->error(utl::PAD,
                   119,
                   "Unable to place {} ({}) at ({:.3f}um, {:.3f}um) - "
                   "({:.3f}um, {:.3f}um) as it is not inside the row {} "
                   "({:.3f}um, {:.3f}um) - "
                   "({:.3f}um, {:.3f}um)",
                   inst->getName(),
                   inst->getMaster()->getName(),
                   inst_rect.xMin() / dbus,
                   inst_rect.yMin() / dbus,
                   inst_rect.xMax() / dbus,
                   inst_rect.yMax() / dbus,
                   row_->getName(),
                   row_bbox.xMin() / dbus,
                   row_bbox.yMin() / dbus,
                   row_bbox.xMax() / dbus,
                   row_bbox.yMax() / dbus);
  }

  const auto check_obs = checkInstancePlacement(inst);
  if (allow_shift && check_obs) {
    const auto& [check_inst, check_rect] = *check_obs;

    int obs_index = index;
    switch (edge_) {
      case odb::Direction2D::North:
      case odb::Direction2D::South:
        obs_index = snapToRowSite(check_rect.xMin());
        break;
      case odb::Direction2D::West:
      case odb::Direction2D::East:
        obs_index = snapToRowSite(check_rect.yMin());
        break;
    }

    int next_index = std::max(index + 1, obs_index);

    debugPrint(logger_,
               utl::PAD,
               "Place",
               2,
               "Shift required for {} to avoid {} ({} -> {})",
               inst->getName(),
               check_inst ? check_inst->getName() : "blockage",
               index,
               next_index);

    return placeInstance(
        next_index, inst, base_orient, allow_overlap, allow_shift);
  }
  if (!allow_overlap && check_obs) {
    const auto& [check_inst, obs_rect] = *check_obs;
    odb::Rect check_rect;
    if (check_inst == nullptr) {
      check_rect = obs_rect;
    } else {
      check_rect = check_inst->getBBox()->getBox();
    }
    logger_->error(utl::PAD,
                   1,
                   "Unable to place {} ({}) at ({:.3f}um, {:.3f}um) - "
                   "({:.3f}um, {:.3f}um) as it "
                   "overlaps with {} at ({:.3f}um, {:.3f}um) - "
                   "({:.3f}um, {:.3f}um)",
                   inst->getName(),
                   inst->getMaster()->getName(),
                   inst_rect.xMin() / dbus,
                   inst_rect.yMin() / dbus,
                   inst_rect.xMax() / dbus,
                   inst_rect.yMax() / dbus,
                   check_inst ? fmt::format("{} ({})",
                                            check_inst->getName(),
                                            check_inst->getMaster()->getName())
                              : "blockage",
                   check_rect.xMin() / dbus,
                   check_rect.yMin() / dbus,
                   check_rect.xMax() / dbus,
                   check_rect.yMax() / dbus);
  }
  inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);

  int next_pos = index;
  switch (edge_) {
    case odb::Direction2D::North:
    case odb::Direction2D::South:
      next_pos += static_cast<int>(
          std::ceil(static_cast<double>(inst_bbox.dx()) / row_->getSpacing()));
      break;
    case odb::Direction2D::West:
    case odb::Direction2D::East:
      next_pos += static_cast<int>(
          std::ceil(static_cast<double>(inst_bbox.dy()) / row_->getSpacing()));
      break;
  }
  return next_pos;
}

std::optional<std::pair<odb::dbInst*, odb::Rect>>
PadPlacer::checkInstancePlacement(odb::dbInst* inst) const
{
  std::set<odb::dbInst*> covers;
  auto* block = getBlock();
  if (block) {
    const odb::Rect inst_rect = inst->getBBox()->getBox();
    for (odb::dbBlockage* blockage : block->getBlockages()) {
      if (blockage->getBBox()->getBox().overlaps(inst_rect)) {
        return std::make_pair(
            nullptr, blockage->getBBox()->getBox().intersect(inst_rect));
      }
    }
    for (auto* check_inst : block->getInsts()) {
      if (check_inst == inst) {
        continue;
      }
      if (!check_inst->isFixed()) {
        continue;
      }
      if (check_inst->getMaster()->isCover()) {
        covers.insert(check_inst);
        continue;
      }
      if (check_inst->getBBox()->getBox().overlaps(inst_rect)) {
        return std::make_pair(
            check_inst, check_inst->getBBox()->getBox().intersect(inst_rect));
      }
    }
  }

  // Check if inst overlaps with bumps
  std::map<odb::dbTechLayer*, std::set<std::pair<odb::Rect, odb::dbNet*>>>
      check_shapes;
  if (!covers.empty()) {
    // populate map as needed
    const auto xform = inst->getTransform();
    for (auto* obs : inst->getMaster()->getObstructions()) {
      odb::Rect obs_rect = obs->getBox();
      xform.apply(obs_rect);
      odb::dbTechLayer* layer = obs->getTechLayer();
      if (layer != nullptr) {
        odb::Rect bloat;
        obs_rect.bloat(layer->getSpacing(), bloat);
        obs_rect = bloat;
      }
      check_shapes[obs->getTechLayer()].emplace(obs_rect, nullptr);
    }
    for (auto* iterm : inst->getITerms()) {
      for (const auto& [layer, box] : iterm->getGeometries()) {
        odb::Rect term_rect = box;
        if (layer != nullptr) {
          odb::Rect bloat;
          box.bloat(layer->getSpacing(), bloat);
          term_rect = bloat;
        }
        check_shapes[layer].emplace(term_rect, iterm->getNet());
      }
    }
  }

  for (odb::dbInst* check_inst : covers) {
    const auto xform = check_inst->getTransform();
    for (auto* obs : check_inst->getMaster()->getObstructions()) {
      odb::Rect obs_rect = obs->getBox();
      xform.apply(obs_rect);
      for (const auto& [inst_rect, check_net] :
           check_shapes[obs->getTechLayer()]) {
        if (inst_rect.intersects(obs_rect)) {
          return std::make_pair(check_inst, inst_rect.intersect(obs_rect));
        }
      }
    }

    for (auto* iterm : check_inst->getITerms()) {
      for (const auto& [layer, box] : iterm->getGeometries()) {
        for (const auto& [inst_rect, check_net] : check_shapes[layer]) {
          const bool nets_match
              = iterm->getNet() == check_net
                && (check_net != nullptr || iterm->getNet() != nullptr);
          if (!nets_match && inst_rect.intersects(box)) {
            return std::make_pair(check_inst, inst_rect.intersect(box));
          }
        }
      }
    }
  }

  return {};
}

///////////////////////////////////////////

UniformPadPlacer::UniformPadPlacer(utl::Logger* logger,
                                   odb::dbBlock* block,
                                   const std::vector<odb::dbInst*>& insts,
                                   const odb::Direction2D::Value& edge,
                                   odb::dbRow* row,
                                   std::optional<int> max_spacing)
    : PadPlacer(logger, block, insts, edge, row), max_spacing_(max_spacing)
{
}

void UniformPadPlacer::place()
{
  const bool gui_debug
      = getLogger()->debugCheck(utl::PAD, "Place", 1) && gui::Gui::enabled();

  float initial_target_spacing
      = static_cast<float>(getRowWidth() - getTotalInstWidths())
        / (getInsts().size() + 1);
  if (max_spacing_) {
    if (*max_spacing_ < initial_target_spacing) {
      initial_target_spacing = *max_spacing_;
    }
  }
  const int site_width = std::min(getRow()->getSite()->getWidth(),
                                  getRow()->getSite()->getHeight());
  const int target_spacing
      = std::floor(initial_target_spacing / site_width) * site_width;
  int offset = getRowStart() + target_spacing;

  const double dbus = getBlock()->getDbUnitsPerMicron();
  debugPrint(getLogger(),
             utl::PAD,
             "Place",
             1,
             "Placing pads with uniform: spacing {:.4f} um",
             target_spacing / dbus);

  for (auto* inst : getInsts()) {
    debugPrint(getLogger(),
               utl::PAD,
               "Place",
               1,
               "Placing {} at {:.4f}um",
               inst->getName(),
               offset / dbus);

    placeInstance(snapToRowSite(offset),
                  inst,
                  odb::dbOrientType::R0,
                  /* allow_overlap */ false,
                  /* allow_shift */ true);
    offset += getInstWidths().at(inst);
    offset += target_spacing;

    if (gui_debug) {
      gui::Gui::get()->pause();
    }
  }
}

///////////////////////////////////////////

CheckerOnlyPadPlacer::CheckerOnlyPadPlacer(utl::Logger* logger,
                                           odb::dbBlock* block,
                                           odb::dbRow* row)
    : PadPlacer(logger, block, {}, odb::Direction2D::North, row)
{
}

bool CheckerOnlyPadPlacer::check(odb::dbInst* inst) const
{
  return !checkInstancePlacement(inst);
}

///////////////////////////////////////////

SingleInstPadPlacer::SingleInstPadPlacer(utl::Logger* logger,
                                         odb::dbBlock* block,
                                         const odb::Direction2D::Value& edge,
                                         odb::dbRow* row)
    : PadPlacer(logger, block, {}, edge, row)
{
}

void SingleInstPadPlacer::place(odb::dbInst* inst,
                                int location,
                                const odb::dbOrientType& base_orient,
                                bool allow_overlap) const
{
  placeInstance(snapToRowSite(location), inst, base_orient, allow_overlap);
}

///////////////////////////////////////////

BumpAlignedPadPlacer::BumpAlignedPadPlacer(
    utl::Logger* logger,
    odb::dbBlock* block,
    const std::vector<odb::dbInst*>& insts,
    const odb::Direction2D::Value& edge,
    odb::dbRow* row)
    : PadPlacer(logger, block, insts, edge, row)
{
}

void BumpAlignedPadPlacer::place()
{
  const double dbus = getBlock()->getDbUnitsPerMicron();

  int offset = getRowStart();

  const bool gui_debug
      = getLogger()->debugCheck(utl::PAD, "Place", 1) && gui::Gui::enabled();

  auto& insts = getInsts();

  int max_travel = getRowWidth() - getTotalInstWidths();
  // iterate over pads in order
  for (auto itr = insts.begin(); itr != insts.end();) {
    // get bump aligned pad group
    const std::map<odb::dbInst*, odb::dbITerm*> min_terms
        = getBumpAlignmentGroup(offset, itr, insts.end());

    // build position map
    // for pads connected to bumps, this will ensure they are centered by the
    // bump if pad is not connected to a bump, place in the next available
    // position
    odb::dbInst* inst = *itr;
    std::map<odb::dbInst*, int> inst_pos;
    if (!min_terms.empty()) {
      int group_center = 0;
      switch (getRowEdge()) {
        case odb::Direction2D::North:
        case odb::Direction2D::South:
          group_center = min_terms.at(inst)->getBBox().xCenter();
          break;
        case odb::Direction2D::West:
        case odb::Direction2D::East:
          group_center = min_terms.at(inst)->getBBox().yCenter();
          break;
      }

      // group width
      int group_width = 0;
      for (const auto& [ginst, giterm] : min_terms) {
        group_width += getInstWidths().at(ginst);
      }

      // build positions with bump as the center
      int target_offset = group_center - (group_width / 2);
      for (const auto& [ginst, giterm] : min_terms) {
        inst_pos[ginst] = target_offset;
        target_offset += getInstWidths().at(ginst);
      }
    } else {
      // request next availble position
      inst_pos[inst] = 0;
    }

    // place pads
    for (const auto& [ginst, pos] : inst_pos) {
      // compute next available position
      const int select_pos
          = offset + std::min(max_travel, std::max(pos - offset, 0));

      // adjust max travel to remove the slack used by this pad
      max_travel -= select_pos - offset;

      debugPrint(getLogger(),
                 utl::PAD,
                 "Place",
                 1,
                 "Placing {} at {:.4f}um with a remaining spare gap {:.4f}um",
                 inst->getName(),
                 select_pos / dbus,
                 max_travel / dbus);

      offset = getRowStart();
      offset += placeInstance(snapToRowSite(select_pos),
                              ginst,
                              odb::dbOrientType::R0,
                              /* allow_overlap */ false,
                              /* allow_shift */ true)
                * getRow()->getSpacing();

      performPadFlip(ginst);

      if (gui_debug) {
        gui::Gui::get()->pause();
      }
    }

    // more iterator to next unplaced pad
    std::advance(itr, inst_pos.size());
  }
}

int64_t BumpAlignedPadPlacer::computePadBumpDistance(odb::dbInst* inst,
                                                     int inst_width,
                                                     odb::dbITerm* bump,
                                                     int center_pos) const
{
  const odb::Point row_center = getRow()->getBBox().center();
  const odb::Point center = bump->getBBox().center();

  switch (getRowEdge()) {
    case odb::Direction2D::North:
    case odb::Direction2D::South:
      return odb::Point::squaredDistance(
          odb::Point(center_pos + (inst_width / 2), row_center.y()), center);
    case odb::Direction2D::West:
    case odb::Direction2D::East:
      return odb::Point::squaredDistance(
          odb::Point(row_center.x(), center_pos + (inst_width / 2)), center);
  }

  return std::numeric_limits<int64_t>::max();
}

std::map<odb::dbInst*, odb::dbITerm*>
BumpAlignedPadPlacer::getBumpAlignmentGroup(
    int offset,
    const std::vector<odb::dbInst*>::const_iterator& itr,
    const std::vector<odb::dbInst*>::const_iterator& inst_end) const
{
  odb::dbInst* inst = *itr;

  // build map of bump aligned pads (ie. pads connected to bumps in the same row
  // or column)
  std::map<odb::dbInst*, odb::dbITerm*> min_terms;
  auto sitr = itr;
  for (; sitr != inst_end; sitr++) {
    odb::dbInst* check_inst = *sitr;

    auto find_assignment = iterm_connections_.find(check_inst);
    if (find_assignment != iterm_connections_.end()) {
      odb::dbITerm* min_dist = *find_assignment->second.begin();

      // find closest bump to the current offset in the row
      for (auto* iterm : find_assignment->second) {
        if (computePadBumpDistance(
                check_inst, getInstWidths().at(check_inst), min_dist, offset)
            > computePadBumpDistance(
                check_inst, getInstWidths().at(check_inst), iterm, offset)) {
          min_dist = iterm;
        }
      }

      if (sitr != itr) {
        // no longer the first pad in group, check if bumps are in same
        // column/row
        bool keep = true;
        switch (getRowEdge()) {
          case odb::Direction2D::North:
          case odb::Direction2D::South:
            keep = min_terms[inst]->getBBox().xCenter()
                   == min_dist->getBBox().xCenter();
            break;
          case odb::Direction2D::West:
          case odb::Direction2D::East:
            keep = min_terms[inst]->getBBox().yCenter()
                   == min_dist->getBBox().yCenter();
            break;
        }

        if (!keep) {
          break;
        }
      }

      min_terms[check_inst] = min_dist;
    } else {
      break;
    }
  }

  if (getLogger()->debugCheck(utl::PAD, "Place", 1)) {
    getLogger()->debug(
        utl::PAD, "Place", "Pad group size {}", min_terms.size());
    for (const auto& [dinst, diterm] : min_terms) {
      getLogger()->debug(
          utl::PAD, "Place", " {} -> {}", dinst->getName(), diterm->getName());
    }
  }

  return min_terms;
}

int64_t BumpAlignedPadPlacer::estimateWirelengths(
    odb::dbInst* inst,
    const std::set<odb::dbITerm*>& iterms) const
{
  std::map<odb::dbITerm*, odb::dbITerm*> terms;
  for (auto* iterm : iterms) {
    for (auto* net_iterm : iterm->getNet()->getITerms()) {
      if (net_iterm->getInst() == inst) {
        terms[net_iterm] = iterm;
      }
    }
  }

  int64_t dist = 0;

  for (const auto& [term0, term1] : terms) {
    dist += odb::Point::manhattanDistance(term0->getBBox().center(),
                                          term1->getBBox().center());
  }

  return dist;
}

void BumpAlignedPadPlacer::performPadFlip(odb::dbInst* inst) const
{
  const double dbus = getBlock()->getDbUnitsPerMicron();

  // check if flipping improves wirelength
  auto find_assignment = iterm_connections_.find(inst);
  if (find_assignment != iterm_connections_.end()) {
    const auto& pins = find_assignment->second;
    if (pins.size() > 1) {
      // only need to check if pad has more than one connection
      const int64_t start_wirelength = estimateWirelengths(inst, pins);
      const auto start_orient = inst->getOrient();

      // try flipping pad
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      switch (getRowEdge()) {
        case odb::Direction2D::North:
        case odb::Direction2D::South:
          inst->setLocationOrient(start_orient.flipY());
          break;
        case odb::Direction2D::West:
        case odb::Direction2D::East:
          inst->setLocationOrient(start_orient.flipX());
          break;
      }

      // get new wirelength
      const int64_t flipped_wirelength = estimateWirelengths(inst, pins);
      const bool undo = flipped_wirelength > start_wirelength;

      if (undo) {
        // since new wirelength was longer, restore the original orientation
        inst->setLocationOrient(start_orient);
      }
      debugPrint(getLogger(),
                 utl::PAD,
                 "Place",
                 1,
                 "Flip check for {}: non flipped wirelength {:.4f}um: "
                 "flipped wirelength {:.4f}um: {}",
                 inst->getName(),
                 start_wirelength / dbus,
                 flipped_wirelength / dbus,
                 undo ? "undo" : "keep");
      inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
    }
  }
}

}  // namespace pad
