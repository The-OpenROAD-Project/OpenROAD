// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "PadPlacer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/geometry/index/predicates.hpp"
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
  populateObstructions();
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

int PadPlacer::convertRowIndexToPos(int index) const
{
  const odb::Point origin = row_->getOrigin();

  int start;
  if (row_->getDirection() == odb::dbRowDir::HORIZONTAL) {
    start = origin.x();
  } else {
    start = origin.y();
  }

  return start + (index * row_->getSpacing());
}

int PadPlacer::placeInstance(int index,
                             odb::dbInst* inst,
                             const odb::dbOrientType& base_orient,
                             bool allow_overlap,
                             bool allow_shift,
                             bool check) const
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

  if (check) {
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
                     check_inst
                         ? fmt::format("{} ({})",
                                       check_inst->getName(),
                                       check_inst->getMaster()->getName())
                         : "blockage",
                     check_rect.xMin() / dbus,
                     check_rect.yMin() / dbus,
                     check_rect.xMax() / dbus,
                     check_rect.yMax() / dbus);
    }
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

PadPlacer::LayerTermObsTree PadPlacer::getInstanceObstructions(
    odb::dbInst* inst,
    bool bloat) const
{
  std::map<odb::dbTechLayer*, std::vector<TermObsValue>> shapes;

  // populate map as needed
  const auto xform = inst->getTransform();
  for (auto* obs : inst->getMaster()->getObstructions()) {
    odb::Rect obs_rect = obs->getBox();
    xform.apply(obs_rect);
    odb::dbTechLayer* layer = obs->getTechLayer();
    if (bloat && layer != nullptr) {
      odb::Rect bloat_rect;
      obs_rect.bloat(layer->getSpacing(), bloat_rect);
      obs_rect = bloat_rect;
    }
    shapes[obs->getTechLayer()].emplace_back(obs_rect, nullptr, inst);
  }
  for (auto* iterm : inst->getITerms()) {
    for (const auto& [layer, box] : iterm->getGeometries()) {
      odb::Rect term_rect = box;
      if (bloat && layer != nullptr) {
        odb::Rect bloat_rect;
        box.bloat(layer->getSpacing(), bloat_rect);
        term_rect = bloat_rect;
      }
      shapes[layer].emplace_back(term_rect, iterm->getNet(), inst);
    }
  }

  LayerTermObsTree rshapes;
  for (const auto& [layer, layer_shapes] : shapes) {
    if (layer == nullptr) {
      continue;
    }
    rshapes[layer] = TermObsTree(layer_shapes.begin(), layer_shapes.end());
  }
  return rshapes;
}

void PadPlacer::populateObstructions()
{
  blockage_obstructions_.clear();
  instance_obstructions_.clear();
  term_obstructions_.clear();

  const odb::Rect row = row_->getBBox();

  std::set<odb::dbInst*> covers;
  auto* block = getBlock();
  if (block) {
    // Get placement blockages
    for (odb::dbBlockage* blockage : block->getBlockages()) {
      if (blockage->getBBox()->getBox().overlaps(row)) {
        blockage_obstructions_.insert(blockage->getBBox()->getBox());
      }
    }
    // Get obstructions that might interfere with RDL routing
    for (odb::dbObstruction* obs : block->getObstructions()) {
      if (obs->getBBox()->getBox().overlaps(row)) {
        term_obstructions_[obs->getBBox()->getTechLayer()].insert(
            {obs->getBBox()->getBox(), nullptr, nullptr});
      }
    }
    for (auto* check_inst : block->getInsts()) {
      if (!check_inst->isFixed()) {
        continue;
      }
      if (check_inst->getBBox()->getBox().overlaps(row)) {
        if (check_inst->getMaster()->isCover()) {
          covers.insert(check_inst);
          continue;
        }
        instance_obstructions_.insert(
            {check_inst->getBBox()->getBox(), check_inst});
      }
    }
  }

  for (odb::dbInst* check_inst : covers) {
    for (const auto& [layer, shapes] : getInstanceObstructions(check_inst)) {
      term_obstructions_[layer].insert(shapes.begin(), shapes.end());
    }
  }
}

void PadPlacer::addInstanceObstructions(odb::dbInst* inst)
{
  instance_obstructions_.insert({inst->getBBox()->getBox(), inst});
  if (inst->getMaster()->isCover()) {
    for (const auto& [layer, shapes] : getInstanceObstructions(inst)) {
      term_obstructions_[layer].insert(shapes.begin(), shapes.end());
    }
  }
}

std::optional<std::pair<odb::dbInst*, odb::Rect>>
PadPlacer::checkInstancePlacement(odb::dbInst* inst,
                                  bool return_intersect) const
{
  const odb::Rect inst_rect = inst->getBBox()->getBox();
  for (auto itr = blockage_obstructions_.qbegin(
           boost::geometry::index::intersects(inst_rect));
       itr != blockage_obstructions_.qend();
       itr++) {
    if (itr->overlaps(inst_rect)) {
      debugPrint(getLogger(),
                 utl::PAD,
                 "Check",
                 2,
                 "{} blocked by blockage: {}",
                 inst->getName(),
                 *itr);
      return std::make_pair(
          nullptr, return_intersect ? itr->intersect(inst_rect) : *itr);
    }
  }
  for (auto itr = instance_obstructions_.qbegin(
           boost::geometry::index::intersects(inst_rect));
       itr != instance_obstructions_.qend();
       itr++) {
    const auto& [check_rect, check_inst] = *itr;
    if (check_rect.overlaps(inst_rect)) {
      if (check_inst == inst) {
        continue;
      }
      debugPrint(getLogger(),
                 utl::PAD,
                 "Check",
                 2,
                 "{} blocked by fixed instance: {}",
                 inst->getName(),
                 check_inst->getName());
      return std::make_pair(
          check_inst,
          return_intersect ? check_rect.intersect(inst_rect) : check_rect);
    }
  }

  for (const auto& [layer, term_shapes] : getInstanceObstructions(inst, true)) {
    if (term_obstructions_.find(layer) == term_obstructions_.end()) {
      continue;
    }

    const auto& obs_layer = term_obstructions_.at(layer);

    for (const auto& [term_shape, term_net, term_inst] : term_shapes) {
      for (auto itr
           = obs_layer.qbegin(boost::geometry::index::intersects(term_shape));
           itr != obs_layer.qend();
           itr++) {
        const auto& [check_rect, check_net, check_inst] = *itr;
        if (check_inst == inst) {
          continue;
        }
        const bool nets_match
            = term_net == check_net
              && (check_net != nullptr || term_net != nullptr);
        if (!nets_match) {
          debugPrint(
              getLogger(),
              utl::PAD,
              "Check",
              2,
              "{} ({} / {}) blocked by terminal obstruction: {} / {}",
              inst->getName(),
              term_shape,
              term_net == nullptr ? "unconnected" : term_net->getName(),
              check_rect,
              check_net == nullptr ? "unconnected" : check_net->getName());
          return std::make_pair(
              check_inst,
              return_intersect ? check_rect.intersect(inst_rect) : check_rect);
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
    initial_target_spacing
        = std::min<float>(*max_spacing_, initial_target_spacing);
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
    addInstanceObstructions(inst);
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
                                bool allow_overlap)
{
  placeInstance(snapToRowSite(location), inst, base_orient, allow_overlap);
  addInstanceObstructions(inst);
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
      addInstanceObstructions(ginst);

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

///////////////////////////////////////////

PlacerPadPlacer::PlacerPadPlacer(utl::Logger* logger,
                                 odb::dbBlock* block,
                                 const std::vector<odb::dbInst*>& insts,
                                 const odb::Direction2D::Value& edge,
                                 odb::dbRow* row)
    : PadPlacer(logger, block, insts, edge, row)
{
}

void PlacerPadPlacer::placeInstances(
    const std::map<odb::dbInst*, int>& positions,
    bool center_ref) const
{
  for (const auto& [inst, pos] : positions) {
    placeInstanceSimple(inst, pos, center_ref);
  }
}

void PlacerPadPlacer::placeInstances(
    const std::map<odb::dbInst*, std::unique_ptr<InstAnchors>>& positions) const
{
  for (const auto& [inst, anchor] : positions) {
    placeInstanceSimple(inst, anchor->center, true);
  }
}

void PlacerPadPlacer::placeInstanceSimple(odb::dbInst* inst,
                                          int position,
                                          bool center_ref) const
{
  placeInstance(
      snapToRowSite(position - (center_ref ? getInstWidths().at(inst) / 2 : 0)),
      inst,
      odb::dbOrientType::R0,
      /* allow_overlap */ false,
      /* allow_shift */ false,
      /* check */ false);
  inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
}

std::map<odb::dbInst*, int> PlacerPadPlacer::initialPoolMapping() const
{
  const auto& insts = getInsts();

  std::vector<int> position(insts.size());
  for (int i = 0; i < insts.size(); i++) {
    odb::dbInst* inst = insts[i];
    if (ideal_positions_.find(inst) == ideal_positions_.end()) {
      // This instance has no preference so shift down
      if (i == 0) {
        // Start at row beginning
        position[i] = getRowStart(inst);
      } else if (i + 1 == insts.size()) {
        // End at row end
        position[i] = getRowEnd(inst);
      } else {
        if (i == 1) {
          // nothing to spread out
          position[i] = position[i - 1];
        } else {
          // try to spread these out a little
          position[i] = 0.5 * (position[i - 1] + position[i - 2]);
        }
      }
    } else {
      position[i] = ideal_positions_.at(inst);
    }
  }

  std::map<odb::dbInst*, int> mapping;
  for (int i = 0; i < insts.size(); i++) {
    odb::dbInst* inst = insts[i];
    mapping[inst] = position[i];
  }

  if (getLogger()->debugCheck(utl::PAD, "PAVA", 2)) {
    const double dbus = getBlock()->getDbUnitsPerMicron();
    int idx = 0;
    getLogger()->debug(utl::PAD, "PAVA", "Pool mapping ({}):", insts.size());
    for (auto* inst : insts) {
      getLogger()->debug(
          utl::PAD,
          "PAVA",
          "  {:>5}: {} at {:.4f}um (ideal: {})",
          ++idx,
          inst->getName(),
          mapping[inst] / dbus,
          ideal_positions_.find(inst) == ideal_positions_.end()
              ? "N/A"
              : fmt::format("{:.4f}um", ideal_positions_.at(inst) / dbus));
    }
  }

  return mapping;
}

void PlacerPadPlacer::debugPause(const std::string& msg) const
{
  if (gui::Gui::enabled() && getLogger()->debugCheck(utl::PAD, "Pause", 1)) {
    debugPrint(getLogger(), utl::PAD, "Pause", 1, msg);
    auto* gui = gui::Gui::get();
    gui->clearHighlights();
    for (const auto& [inst, iterms] : iterm_connections_) {
      for (auto* iterm : iterms) {
        if (iterm->getNet() != nullptr) {
          gui->addNetToHighlightSet(iterm->getNet()->getConstName());
        }
      }
    }
    gui::Gui::get()->pause();
  }
}

void PlacerPadPlacer::place()
{
  if (gui::Gui::enabled() && getLogger()->debugCheck(utl::PAD, "Place", 1)) {
    chart_ = gui::Gui::get()->addChart(
        fmt::format("PAD ({})", getRow()->getName()),
        "Iteration",
        {"RDL Estimate (μm)", "Move (μm)"});
    chart_->setXAxisFormat("%d");
    chart_->setYAxisFormats({"%.2e", "%.1f"});
  }

  // Determine ideal positions
  computeIdealPostions();
  debugPause("Ideal pad positions");

  // resolve ordering for pads
  const auto pava_positions = poolAdjacentViolators(initialPoolMapping());
  debugPause("Ordered ideal pad positions");

  // Perform iterative spreading
  const auto pad_positions = padSpreading(pava_positions);
  placeInstances(pad_positions, false);
  debugPause("Final placement");

  // Fixed pad locations
  for (auto* inst : getInsts()) {
    placeInstance(snapToRowSite(pad_positions.at(inst)),
                  inst,
                  odb::dbOrientType::R0,
                  false,
                  false);
    // TODO possibly build in pad flipping
    addInstanceObstructions(inst);
  }
}

int PlacerPadPlacer::getNearestLegalPosition(odb::dbInst* inst,
                                             int target,
                                             bool round_down,
                                             bool round_up) const
{
  placeInstanceSimple(inst, target, true);
  const auto ideal_obs = checkInstancePlacement(inst, false);
  if (ideal_obs) {
    const int half_width = getInstWidths().at(inst) / 2;
    int start, end;
    if (isRowHorizontal()) {
      start = ideal_obs.value().second.xMin() - half_width;
      end = ideal_obs.value().second.xMax() + half_width;
    } else {
      start = ideal_obs.value().second.yMin() - half_width;
      end = ideal_obs.value().second.yMax() + half_width;
    }
    // Handle obs near row ends
    if (start < getRowStart(inst)) {
      return end;
    }
    if (end > getRowEnd(inst)) {
      return start;
    }
    if (round_down) {
      return start;
    }
    if (round_up) {
      return end;
    }
    if ((target - start) < (end - target)) {
      return start;
    }
    return end;
  }
  return target;
}

void PlacerPadPlacer::addChartData(int itr, int64_t rdl_length, int move) const
{
  if (chart_) {
    const double dbus = getBlock()->getDbUnitsPerMicron();
    chart_->addPoint(itr, {rdl_length / dbus, move / dbus});
  }
}

void PlacerPadPlacer::computeIdealPostions()
{
  ideal_positions_.clear();

  for (odb::dbInst* inst : getInsts()) {
    if (iterm_connections_.find(inst) == iterm_connections_.end()) {
      // no constraint
      continue;
    }

    std::vector<int> ideal_pos;
    for (odb::dbITerm* iterm : iterm_connections_.at(inst)) {
      switch (getRowEdge()) {
        case odb::Direction2D::North:
        case odb::Direction2D::South:
          ideal_pos.push_back(iterm->getBBox().xCenter());
          break;
        case odb::Direction2D::West:
        case odb::Direction2D::East:
          ideal_pos.push_back(iterm->getBBox().yCenter());
          break;
      }
    }

    if (!ideal_pos.empty()) {
      float ideal = 0;
      for (auto pos : ideal_pos) {
        ideal += pos;
      }
      ideal_positions_[inst]
          = getNearestLegalPosition(inst, std::round(ideal / ideal_pos.size()));
    }
  }

  placeInstances(ideal_positions_, true);
  addChartData(-2, estimateWirelengths(), 0);
}

std::map<odb::dbInst*, int> PlacerPadPlacer::poolAdjacentViolators(
    const std::map<odb::dbInst*, int>& initial_positions) const
{
  const double dbus = getBlock()->getDbUnitsPerMicron();
  const auto& insts = getInsts();
  std::vector<float> weights(insts.size());
  std::ranges::fill(weights, 1.0);

  std::vector<int> position(insts.size());
  for (int i = 0; i < insts.size(); i++) {
    odb::dbInst* inst = insts[i];
    position[i] = initial_positions.at(inst);
  }

  for (int k = 0; k < insts.size(); k++) {
    bool updated = false;

    debugPrint(getLogger(), utl::PAD, "PAVA", 1, "Itr {}: start", k);

    // Run PAVA
    for (int i = 1; i < insts.size(); i++) {
      const int current_pos = position[i];
      const int previous_pos = position[i - 1];

      if (current_pos >= previous_pos) {
        continue;
      }

      updated = true;
      // Calculate new value
      const float total_weight = weights[i] + weights[i - 1];
      const int pooled_value = std::round(
          (weights[i] * current_pos + weights[i - 1] * previous_pos)
          / total_weight);

      // Update positions
      position[i] = pooled_value;
      position[i - 1] = pooled_value;

      debugPrint(getLogger(),
                 utl::PAD,
                 "PAVA",
                 2,
                 "Ordering swap: {} ({:.4f}um {:.0f}x) and {} ({:.4f}um "
                 "{:.0f}x) to {:.4f}um",
                 insts[i]->getName(),
                 current_pos / dbus,
                 weights[i],
                 insts[i - 1]->getName(),
                 previous_pos / dbus,
                 weights[i - 1],
                 pooled_value / dbus);

      // Update weights
      weights[i] += weights[i - 1];
      weights[i - 1] += weights[i];
    }

    // Check for legal positions
    for (int i = 0; i < insts.size(); i++) {
      const int pos = position[i];
      odb::dbInst* inst = insts[i];
      const int legal_pos = getNearestLegalPosition(inst, pos);
      if (legal_pos != pos) {
        debugPrint(getLogger(),
                   utl::PAD,
                   "PAVA",
                   2,
                   "Legal position swap: {} from {:.4f}um to {:.4f}um",
                   inst->getName(),
                   pos / dbus,
                   legal_pos / dbus);
        updated = true;
        position[i] = legal_pos;
      }
    }

    debugPrint(getLogger(),
               utl::PAD,
               "PAVA",
               1,
               "Itr {}: end: updated? {}",
               k,
               updated);

    std::map<odb::dbInst*, int> positions;
    for (int i = 0; i < insts.size(); i++) {
      positions[insts[i]] = position[i];
    }
    placeInstances(positions, true);
    debugCheckPlacement();
    debugPause(fmt::format("PAVA itr: {}", k));

    if (!updated) {
      break;
    }
  }

  std::map<odb::dbInst*, int> positions;

  for (int i = 0; i < insts.size(); i++) {
    odb::dbInst* inst = insts[i];
    positions[inst] = position[i];
    debugPrint(getLogger(),
               utl::PAD,
               "PAVA",
               2,
               "{} / {}: position {:.3f}um, weight {:.0f}x",
               i,
               inst->getName(),
               position[i] / dbus,
               weights[i]);
  }

  placeInstances(positions, true);
  addChartData(-1, estimateWirelengths(), 0);

  return positions;
}

bool PlacerPadPlacer::padSpreading(
    std::map<odb::dbInst*, std::unique_ptr<InstAnchors>>& positions,
    const std::map<odb::dbInst*, int>& initial_positions,
    int itr,
    float spring,
    float repel,
    float damper) const
{
  bool has_violations = false;

  const auto& insts = getInsts();
  const double dbus = getBlock()->getDbUnitsPerMicron();
  const int site_width = getRow()->getSpacing();

  std::map<int, int> failed_tunnel_positions;

  int total_move = 0;
  int net_move = 0;
  debugPrint(getLogger(),
             utl::PAD,
             "Place",
             1,
             "Itr {} start: spring coeff {:.3f}, repel coeff {:.3f}, damper "
             "coeff {:.3f}",
             itr,
             spring,
             repel,
             damper);

  for (int i = 0; i < insts.size(); i++) {
    odb::dbInst* prev = nullptr;
    odb::dbInst* curr = insts[i];
    odb::dbInst* next = nullptr;
    if (i > 0) {
      prev = insts[i - 1];
    }
    if (i < insts.size() - 1) {
      next = insts[i + 1];
    }

    // Get positions
    const int prev_pos
        = prev == nullptr ? getRowStart(curr) : positions[prev]->center;
    const int curr_pos = positions[curr]->center;
    const int next_pos
        = next == nullptr ? getRowEnd(curr) : positions[next]->center;

    // Get target position
    const int target_pos = initial_positions.at(curr);

    // Calculate "forces"

    // Spring force to target
    const int spring_delta = target_pos - curr_pos;
    const float spring_force = spring_delta * spring;

    // Repulsive force from previous pad
    float repel_prev_force = 0;
    if (prev != nullptr) {
      const int overlap = positions[curr]->overlap(positions[prev]);
      if (overlap > 0) {
        has_violations = true;
        repel_prev_force = (overlap + site_width) * repel;
      }
    }

    // Repulsive force from next pad
    float repel_next_force = 0;
    if (next != nullptr) {
      const int overlap = positions[next]->overlap(positions[curr]);
      if (overlap > 0) {
        has_violations = true;
        repel_next_force = -(overlap + site_width) * repel;
      }
    }
    debugPrint(getLogger(),
               utl::PAD,
               "Place",
               3,
               "{} / {}: ({:.3f}um, {:.3f}um), ({:.3f}um, {:.3f}um), "
               "({:.3f}um, {:.3f}um)",
               itr,
               curr->getName(),
               (prev == nullptr ? prev_pos : positions[prev]->min) / dbus,
               (prev == nullptr ? prev_pos : positions[prev]->max) / dbus,
               positions[curr]->min / dbus,
               positions[curr]->max / dbus,
               (next == nullptr ? next_pos : positions[next]->min) / dbus,
               (next == nullptr ? next_pos : positions[next]->max) / dbus);

    // Total force
    const float total_force
        = spring_force + repel_prev_force + repel_next_force;

    // Apply a damped movement
    const float target_move = total_force * damper;
    const float abs_target_move = std::abs(target_move);
    int move_by = ((total_force < 0) ? -1 : 1)
                  * std::ceil(abs_target_move / site_width) * site_width;

    // jump logic
    // if there is only a force pulling into blockage, jump if there is no
    // opposing force
    //    clamp to next or previous position or row start and end
    // otherwise clamp to nearest legal position
    const int check_move
        = std::max(prev_pos, std::min(next_pos, curr_pos + move_by));
    if (move_by != 0) {
      const auto& [tunnel_move, tunnel_ideal] = getTunnelingPosition(
          curr, check_move, move_by > 0, prev_pos, curr_pos, next_pos, itr);
      move_by = tunnel_move - curr_pos;
      if (tunnel_move != tunnel_ideal) {
        failed_tunnel_positions.emplace(i, tunnel_ideal);
      }
    }

    // Record stats
    net_move += move_by;
    total_move += std::abs(move_by);

    const int move_to = convertRowIndexToPos(snapToRowSite(
                            curr_pos + move_by - (positions[curr]->width / 2)))
                        + (positions[curr]->width / 2);
    positions[curr]->setLocation(std::max(prev_pos, std::min(next_pos, move_to))
                                 - (positions[curr]->width / 2));
    debugPrint(
        getLogger(),
        utl::PAD,
        "Place",
        3,
        "{} / {}: {:.4f}um -> {:.4f}um (idx: {}) based on {:.6f} s, {:.6f} p, "
        "{:.6f} n, {:.6f} t",
        itr,
        curr->getName(),
        curr_pos / dbus,
        positions[curr]->center / dbus,
        snapToRowSite(positions[curr]->min),
        spring_force,
        repel_prev_force,
        repel_next_force,
        total_force);
  }

  // Push instances causing tunneling to fail, these are instances that are too
  // far from the instance that needs to tunnel/jump over the obstruction and
  // therefore needs to push instances under the obstruction over to make room.
  for (const auto& [inst_idx, ideal_pos] : failed_tunnel_positions) {
    const int delta_pos = ideal_pos - positions[insts[inst_idx]]->center;
    const int push = ((delta_pos < 0) ? -1 : 1)
                     * std::ceil((damper * std::abs(delta_pos)) / site_width)
                     * site_width;
    std::vector<odb::dbInst*> push_insts;  // instances to push
    int last_idx = inst_idx;
    int bound_pos;
    if (delta_pos > 0) {
      for (int j = inst_idx + 1; j < insts.size(); j++) {
        if (positions[insts[j]]->center <= ideal_pos) {
          push_insts.push_back(insts[j]);
          last_idx = j;
        }
      }
      bound_pos = last_idx == insts.size()
                      ? getRowEnd(insts[last_idx])
                      : positions[insts[last_idx + 1]]->center;
    } else {
      for (int j = inst_idx - 1; j >= 0; j--) {
        if (positions[insts[j]]->center >= ideal_pos) {
          push_insts.push_back(insts[j]);
          last_idx = j;
        }
      }
      bound_pos = last_idx == 0 ? getRowStart(insts[last_idx])
                                : positions[insts[last_idx - 1]]->center;
    }
    debugPrint(getLogger(),
               utl::PAD,
               "Place",
               2,
               "{} / {}: Unable to tunnel to {:.4f}um, stuck at {:.4f}um, push "
               "boundary at {:.4f}um, desired push {:.4f}um applies to: {}",
               itr,
               insts[inst_idx]->getName(),
               ideal_pos / dbus,
               positions[insts[inst_idx]]->center / dbus,
               bound_pos / dbus,
               push / dbus,
               instNameList(push_insts));
    // push instances
    for (odb::dbInst* inst : push_insts) {
      const int curr_pos = positions[inst]->center;
      int desired_pos;
      if (push < 0) {
        desired_pos = std::max(bound_pos, curr_pos + push);
      } else {
        desired_pos = std::min(bound_pos, curr_pos + push);
      }
      // new position
      const int move_to = convertRowIndexToPos(snapToRowSite(
                              desired_pos - (positions[inst]->width / 2)))
                          + (positions[inst]->width / 2);
      positions[inst]->setLocation(move_to - (positions[inst]->width / 2));

      // Record stats
      const int delta_move = move_to - curr_pos;
      net_move += delta_move;
      total_move += std::abs(delta_move);
    }
    has_violations = true;
  }

  placeInstances(positions);
  addChartData(itr, estimateWirelengths(), total_move);
  debugPrint(getLogger(),
             utl::PAD,
             "Place",
             1,
             "Itr {} end: continue {}, total move {:.3f}um, net move {:.3f}um",
             itr,
             has_violations,
             total_move / dbus,
             net_move / dbus);
  debugCheckPlacement();

  if (itr % (getLogger()->debugCheck(utl::PAD, "Pause", 2) ? 100 : 500) == 0) {
    debugPause(fmt::format("Iterative pad spreading: {}", itr));
  }

  return !has_violations;
}

std::map<odb::dbInst*, int> PlacerPadPlacer::padSpreading(
    const std::map<odb::dbInst*, int>& initial_positions) const
{
  const auto& inst_widths = getInstWidths();

  // Snap all positions to row index
  std::map<odb::dbInst*, std::unique_ptr<InstAnchors>> positions;
  for (const auto& [inst, pos] : initial_positions) {
    auto anchors = std::make_unique<InstAnchors>();
    anchors->width = inst_widths.at(inst);
    const int half_width = inst_widths.at(inst) / 2;
    anchors->setLocation(convertRowIndexToPos(snapToRowSite(pos - half_width)));
    positions[inst] = std::move(anchors);
  }

  for (int k = 0; k < kMaxIterations; k++) {
    // Update coeff schedule
    const float kRepel1 = kRepelStart
                          + ((kRepelEnd - kRepelStart) * k
                             / static_cast<float>(kMaxIterations));
    const float kString2
        = k > kSpringIterInfluence
              ? kSpringStart * (kSpringItrRange - (k - kSpringIterInfluence))
                    / static_cast<float>(kSpringItrRange)
              : kSpringStart;
    const float kSpring1 = k > kSpringIterEnd ? 0 : kString2;

    if (padSpreading(
            positions, initial_positions, k, kSpring1, kRepel1, kDamper)) {
      break;
    }
  }

  // convert to regular placement information
  std::map<odb::dbInst*, int> real_positions;
  for (const auto& [inst, anchor] : positions) {
    real_positions[inst] = anchor->min;
  }

  return real_positions;
}

int64_t PlacerPadPlacer::estimateWirelengths() const
{
  int64_t length = 0;
  for (odb::dbInst* inst : getInsts()) {
    if (iterm_connections_.find(inst) == iterm_connections_.end()) {
      continue;
    }
    std::vector<int64_t> lengths;
    const odb::Point inst_center = inst->getBBox()->getBox().center();
    for (odb::dbITerm* iterm : iterm_connections_.at(inst)) {
      const odb::Point iterm_center = iterm->getBBox().center();
      lengths.push_back(odb::Point::squaredDistance(inst_center, iterm_center));
    }

    if (lengths.empty()) {
      continue;
    }
    length += std::sqrt(*std::min(lengths.begin(), lengths.end()));
  }

  if (getLogger()->debugCheck(utl::PAD, "RDLEstimate", 1)) {
    const int routes = getNumberOfRoutes();
    const double dbus = getBlock()->getDbUnitsPerMicron();
    debugPrint(
        getLogger(),
        utl::PAD,
        "RDLEstimate",
        1,
        "Estimated RDL routing length {:.4f}um with {} routes (avg: {:.4f}um)",
        length / dbus,
        routes,
        routes > 0 ? (length / dbus) / routes : 0);
  }

  return length;
}

int PlacerPadPlacer::getNumberOfRoutes() const
{
  int count = 0;
  for (odb::dbInst* inst : getInsts()) {
    if (iterm_connections_.find(inst) != iterm_connections_.end()) {
      count += 1;
    }
  }
  return count;
}

int PlacerPadPlacer::getRowStart(odb::dbInst* inst) const
{
  return PadPlacer::getRowStart() + (getInstWidths().at(inst) / 2);
}

int PlacerPadPlacer::getRowEnd(odb::dbInst* inst) const
{
  return PadPlacer::getRowEnd() - (getInstWidths().at(inst) / 2);
}

std::pair<int, int> PlacerPadPlacer::getTunnelingPosition(odb::dbInst* inst,
                                                          int target,
                                                          bool move_up,
                                                          int low_bound,
                                                          int curr_pos,
                                                          int high_bound,
                                                          int itr) const
{
  const double dbus = getBlock()->getDbUnitsPerMicron();
  placeInstanceSimple(inst, target, true);
  const int ideal = getNearestLegalPosition(inst, target, !move_up, move_up);
  if (move_up) {
    // pad is moving up in the row
    if (ideal != target) {
      if (ideal > high_bound) {
        // check if high_bound is safe to pin to
        placeInstanceSimple(inst, high_bound, true);
        if (checkInstancePlacement(inst)) {
          // inst is obstructed, so move to the nearest edge of the obstruction
          const int next = getNearestLegalPosition(inst, target, true, false);
          if (next <= high_bound) {
            debugPrint(getLogger(),
                       utl::PAD,
                       "Place",
                       2,
                       "{} / {}: Tunneling past obstruction (up) {:.4f} um -> "
                       "{:.4f} um, blocked by lower bound {:.4f}um, pinning to "
                       "blockage ({:.4f}um , {:.4f}um , {:.4f}um)",
                       itr,
                       inst->getName(),
                       target / dbus,
                       next / dbus,
                       high_bound / dbus,
                       low_bound / dbus,
                       curr_pos / dbus,
                       high_bound / dbus);
            return {next, ideal};
          }
          debugPrint(getLogger(),
                     utl::PAD,
                     "Place",
                     2,
                     "{} / {}: Tunneling past obstruction (up) {:.4f} um -> "
                     "{:.4f} um, blocked by lower bound {:.4f}um ({:.4f}um , "
                     "{:.4f}um , {:.4f}um)",
                     itr,
                     inst->getName(),
                     target / dbus,
                     curr_pos / dbus,
                     high_bound / dbus,
                     low_bound / dbus,
                     curr_pos / dbus,
                     high_bound / dbus);
          return {curr_pos, ideal};  // stay in the same position
        }
      }
      debugPrint(getLogger(),
                 utl::PAD,
                 "Place",
                 2,
                 "{} / {}: Tunneling past obstruction (up) {:.4f} um -> {:.4f} "
                 "um ({:.4f}um , {:.4f}um , {:.4f}um)",
                 itr,
                 inst->getName(),
                 target / dbus,
                 ideal / dbus,
                 low_bound / dbus,
                 curr_pos / dbus,
                 high_bound / dbus);
      return {ideal, ideal};
    }
  }
  // pad is moving down in the row
  if (ideal != target) {
    if (ideal < low_bound) {
      // check if low_bound is safe to pin to
      placeInstanceSimple(inst, low_bound, true);
      if (checkInstancePlacement(inst)) {
        const int next = getNearestLegalPosition(inst, target, true, false);
        if (next >= low_bound) {
          // inst is obstructed, so move to the nearest edge of the obstruction
          debugPrint(getLogger(),
                     utl::PAD,
                     "Place",
                     2,
                     "{} / {}: Tunneling past obstruction (down) {:.4f} um -> "
                     "{:.4f} um, blocked by lower bound, pinning to blockage "
                     "{:.4f}um ({:.4f}um , {:.4f}um , {:.4f}um)",
                     itr,
                     inst->getName(),
                     target / dbus,
                     next / dbus,
                     high_bound / dbus,
                     low_bound / dbus,
                     curr_pos / dbus,
                     high_bound / dbus);
          return {next, ideal};
        }
        debugPrint(getLogger(),
                   utl::PAD,
                   "Place",
                   2,
                   "{} / {}: Tunneling past obstruction (down) {:.4f} um -> "
                   "{:.4f} um, blocked by lower bound {:.4f}um ({:.4f}um , "
                   "{:.4f}um , {:.4f}um)",
                   itr,
                   inst->getName(),
                   target / dbus,
                   curr_pos / dbus,
                   low_bound / dbus,
                   low_bound / dbus,
                   curr_pos / dbus,
                   high_bound / dbus);
        return {curr_pos, ideal};
      }
    }
    debugPrint(getLogger(),
               utl::PAD,
               "Place",
               2,
               "{} / {}: Tunneling past obstruction (down) {:.4f} um -> {:.4f} "
               "um ({:.4f}um , {:.4f}um , {:.4f}um)",
               itr,
               inst->getName(),
               target / dbus,
               ideal / dbus,
               low_bound / dbus,
               curr_pos / dbus,
               high_bound / dbus);
    return {ideal, ideal};
  }

  return {target, target};
}

void PlacerPadPlacer::debugCheckPlacement() const
{
  if (!getLogger()->debugCheck(utl::PAD, "Check", 1)) {
    return;
  }

  bool pause = false;
  for (auto* inst : getInsts()) {
    if (checkInstancePlacement(inst)) {
      debugPrint(getLogger(),
                 utl::PAD,
                 "Check",
                 1,
                 "Invalid placement for {}",
                 inst->getName());
      pause = true;
    }
  }

  if (pause && gui::Gui::enabled()) {
    getLogger()->report("Pausing GUI due to invalid positions of pads");
    gui::Gui::get()->pause();
  }
}

std::string PlacerPadPlacer::instNameList(
    const std::vector<odb::dbInst*>& insts) const
{
  std::string inst_string;
  for (auto* inst : insts) {
    if (!inst_string.empty()) {
      inst_string += ", ";
    }
    inst_string += inst->getName();
  }
  return inst_string;
}

//////////////////////////

int PlacerPadPlacer::InstAnchors::overlap(
    const std::unique_ptr<InstAnchors>& other) const
{
  return overlap(other.get());
}

int PlacerPadPlacer::InstAnchors::overlap(InstAnchors* other) const
{
  if (other->max > min) {
    return other->max - min;
  }
  return 0;
}

}  // namespace pad
