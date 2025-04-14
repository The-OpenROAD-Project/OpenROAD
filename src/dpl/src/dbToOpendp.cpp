// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cfloat>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "dpl/Grid.h"
#include "dpl/Objects.h"
#include "dpl/Opendp.h"
#include "dpl/PlacementDRC.h"
#include "odb/util.h"
#include "utl/Logger.h"

namespace dpl {

using std::string;
using std::vector;

using odb::dbBox;
using odb::dbMaster;
using odb::dbOrientType;
using odb::dbRegion;
using odb::Rect;

void Opendp::importDb()
{
  block_ = db_->getChip()->getBlock();
  grid_->initBlock(block_);
  have_fillers_ = false;

  disallow_one_site_gaps_ = !odb::hasOneSiteMaster(db_);

  importClear();
  grid_->examineRows(block_);
  initPlacementDRC();
  makeMacros();
  makeCells();
  makeGroups();
}

void Opendp::importClear()
{
  db_master_map_.clear();
  cells_.clear();
  groups_.clear();
  db_inst_map_.clear();
  deleteGrid();
  have_multi_row_cells_ = false;
}

void Opendp::makeMacros()
{
  vector<dbMaster*> masters;
  block_->getMasters(masters);
  for (auto db_master : masters) {
    Master& master = db_master_map_[db_master];
    makeMaster(&master, db_master);
  }
}

namespace edge_calc {
/**
 * @brief Calculates the difference between the passed parent_segment and the
 * vector segs The parent segment containts all the segments in the segs vector.
 * This function computes the difference between the parent segment and the
 * child segments. It first sorts the segs vector and merges intersecting ones.
 * Then it calculates the difference and returns a list of segments.
 */
std::vector<Rect> difference(const Rect& parent_segment,
                             const std::vector<Rect>& segs)
{
  if (segs.empty()) {
    return {parent_segment};
  }
  bool is_horizontal = parent_segment.yMin() == parent_segment.yMax();
  std::vector<Rect> sorted_segs = segs;
  // Sort segments by start coordinate
  std::sort(
      sorted_segs.begin(),
      sorted_segs.end(),
      [is_horizontal](const Rect& a, const Rect& b) {
        return (is_horizontal ? a.xMin() < b.xMin() : a.yMin() < b.yMin());
      });
  // Merge overlapping segments
  auto prev_seg = sorted_segs.begin();
  auto curr_seg = prev_seg;
  for (++curr_seg; curr_seg != sorted_segs.end();) {
    if (curr_seg->intersects(*prev_seg)) {
      prev_seg->merge(*curr_seg);
      curr_seg = sorted_segs.erase(curr_seg);
    } else {
      prev_seg = curr_seg++;
    }
  }
  // Get the difference
  const int start
      = is_horizontal ? parent_segment.xMin() : parent_segment.yMin();
  const int end = is_horizontal ? parent_segment.xMax() : parent_segment.yMax();
  int current_pos = start;
  std::vector<Rect> result;
  for (const Rect& seg : sorted_segs) {
    int seg_start = is_horizontal ? seg.xMin() : seg.yMin();
    int seg_end = is_horizontal ? seg.xMax() : seg.yMax();
    if (seg_start > current_pos) {
      if (is_horizontal) {
        result.emplace_back(current_pos,
                            parent_segment.yMin(),
                            seg_start,
                            parent_segment.yMax());
      } else {
        result.emplace_back(parent_segment.xMin(),
                            current_pos,
                            parent_segment.xMax(),
                            seg_start);
      }
    }
    current_pos = seg_end;
  }
  // Add the remaining end segment if it exists
  if (current_pos < end) {
    if (is_horizontal) {
      result.emplace_back(
          current_pos, parent_segment.yMin(), end, parent_segment.yMax());
    } else {
      result.emplace_back(
          parent_segment.xMin(), current_pos, parent_segment.xMax(), end);
    }
  }

  return result;
}

Rect getBoundarySegment(const Rect& bbox,
                        const odb::dbMasterEdgeType::EdgeDir dir)
{
  Rect segment(bbox);
  switch (dir) {
    case odb::dbMasterEdgeType::RIGHT:
      segment.set_xlo(bbox.xMax());
      break;
    case odb::dbMasterEdgeType::LEFT:
      segment.set_xhi(bbox.xMin());
      break;
    case odb::dbMasterEdgeType::TOP:
      segment.set_ylo(bbox.yMax());
      break;
    case odb::dbMasterEdgeType::BOTTOM:
      segment.set_yhi(bbox.yMin());
      break;
  }
  return segment;
}

}  // namespace edge_calc

void Opendp::makeMaster(Master* master, dbMaster* db_master)
{
  master->setMultiRow(grid_->isMultiHeight(db_master));
  master->clearEdges();
  if (!drc_engine_->hasCellEdgeSpacingTable()) {
    return;
  }
  if (db_master->getType()
      == odb::dbMasterType::CORE_SPACER) {  // Skip fillcells
    return;
  }
  Rect bbox;
  db_master->getPlacementBoundary(bbox);
  std::map<odb::dbMasterEdgeType::EdgeDir, std::vector<Rect>> typed_segs;
  int num_rows = grid_->gridHeight(db_master).v;
  for (auto edge : db_master->getEdgeTypes()) {
    auto dir = edge->getEdgeDir();
    Rect edge_rect = edge_calc::getBoundarySegment(bbox, dir);
    if (dir == odb::dbMasterEdgeType::TOP
        || dir == odb::dbMasterEdgeType::BOTTOM) {
      if (edge->getRangeBegin() != -1) {
        edge_rect.set_xlo(edge_rect.xMin() + edge->getRangeBegin());
        edge_rect.set_xhi(edge_rect.xMin() + edge->getRangeEnd());
      }
    } else {
      auto dy = edge_rect.dy();
      auto row_height = dy / num_rows;
      auto half_row_height = row_height / 2;
      if (edge->getCellRow() != -1) {
        edge_rect.set_ylo(edge_rect.yMin()
                          + (edge->getCellRow() - 1) * row_height);
        edge_rect.set_yhi(
            std::min(edge_rect.yMax(), edge_rect.yMin() + row_height));
      } else if (edge->getHalfRow() != -1) {
        edge_rect.set_ylo(edge_rect.yMin()
                          + (edge->getHalfRow() - 1) * half_row_height);
        edge_rect.set_yhi(
            std::min(edge_rect.yMax(), edge_rect.yMin() + half_row_height));
      }
    }
    typed_segs[dir].push_back(edge_rect);
    const auto edge_idx = drc_engine_->getEdgeTypeIdx(edge->getEdgeType());
    if (edge_idx != -1) {
      // consider only edge types defined in the spacing table
      master->addEdge(MasterEdge(edge_idx, edge_rect));
    }
  }
  const auto default_edge_idx = drc_engine_->getEdgeTypeIdx("DEFAULT");
  if (default_edge_idx == -1) {
    return;
  }
  // Add the remaining DEFAULT un-typed segments
  for (size_t dir_idx = 0; dir_idx <= 3; dir_idx++) {
    const auto dir = (odb::dbMasterEdgeType::EdgeDir) dir_idx;
    const auto parent_seg = edge_calc::getBoundarySegment(bbox, dir);
    const auto default_segs
        = edge_calc::difference(parent_seg, typed_segs[dir]);
    for (const auto& seg : default_segs) {
      master->addEdge(MasterEdge(default_edge_idx, seg));
    }
  }
}

void Opendp::initPlacementDRC()
{
  drc_engine_ = std::make_unique<PlacementDRC>(grid_.get(), db_->getTech());
}

void Opendp::makeCells()
{
  auto db_insts = block_->getInsts();
  cells_.reserve(db_insts.size());
  for (auto db_inst : db_insts) {
    dbMaster* db_master = db_inst->getMaster();
    if (db_master->isCoreAutoPlaceable()) {
      cells_.emplace_back();
      Node& cell = cells_.back();
      cell.setDbInst(db_inst);
      db_inst_map_[db_inst] = &cell;

      Rect bbox = getBbox(db_inst);
      cell.setWidth(DbuX{bbox.dx()});
      cell.setHeight(DbuY{bbox.dy()});
      cell.setLeft(DbuX{bbox.xMin()});
      cell.setBottom(DbuY{bbox.yMin()});
      cell.setOrient(db_inst->getOrient());
      // Node is already placed if it is FIXED.
      cell.setFixed(db_inst->isFixed());
      cell.setPlaced(cell.isFixed());

      Master& master = db_master_map_[db_master];
      cell.setMaster(&master);
      // We only want to set this if we have multi-row cells to
      // place and not whenever we see a placed block.
      if (master.isMultiRow() && db_master->isCore()) {
        have_multi_row_cells_ = true;
      }
    }
    if (isFiller(db_inst)) {
      have_fillers_ = true;
    }
  }
}

static bool swapWidthHeight(const dbOrientType& orient)
{
  switch (orient.getValue()) {
    case dbOrientType::R90:
    case dbOrientType::MXR90:
    case dbOrientType::R270:
    case dbOrientType::MYR90:
      return true;
    case dbOrientType::R0:
    case dbOrientType::R180:
    case dbOrientType::MY:
    case dbOrientType::MX:
      return false;
  }
  // gcc warning
  return false;
}

Rect Opendp::getBbox(dbInst* inst)
{
  dbMaster* master = inst->getMaster();

  int loc_x, loc_y;
  inst->getLocation(loc_x, loc_y);
  // Shift by core lower left.
  loc_x -= grid_->getCore().xMin();
  loc_y -= grid_->getCore().yMin();

  int width = master->getWidth();
  int height = master->getHeight();
  if (swapWidthHeight(inst->getOrient())) {
    std::swap(width, height);
  }

  return Rect(loc_x, loc_y, loc_x + width, loc_y + height);
}

void Opendp::makeGroups()
{
  regions_rtree_.clear();
  // preallocate groups so it does not grow when push_back is called
  // because region cells point to them.
  auto db_groups = block_->getGroups();
  int reserve_size = 0;
  for (auto db_group : db_groups) {
    if (db_group->getRegion()) {
      std::unordered_set<DbuY> unique_heights;
      for (auto db_inst : db_group->getInsts()) {
        unique_heights.insert(db_inst_map_[db_inst]->getHeight());
      }
      reserve_size += unique_heights.size();
    }
  }
  reserve_size = std::max(reserve_size, (int) db_groups.size());
  groups_.reserve(reserve_size);

  for (auto db_group : db_groups) {
    dbRegion* region = db_group->getRegion();
    if (!region) {
      continue;
    }
    std::set<DbuY> unique_heights;
    std::map<DbuY, Group*> cell_height_to_group_map;
    for (auto db_inst : db_group->getInsts()) {
      unique_heights.insert(db_inst_map_[db_inst]->getHeight());
    }
    int index = 0;
    for (auto height : unique_heights) {
      groups_.emplace_back();
      Group& group = groups_.back();
      string group_name
          = string(db_group->getName()) + "_" + std::to_string(index++);
      group.setName(group_name);
      Rect bbox;
      bbox.mergeInit();
      cell_height_to_group_map[height] = &group;

      for (dbBox* boundary : region->getBoundaries()) {
        Rect box = boundary->getBox();
        const Rect core = grid_->getCore();
        box = box.intersect(core);
        // offset region to core origin
        box.moveDelta(-core.xMin(), -core.yMin());
        if (height == *(unique_heights.begin())) {
          bgBox bbox(
              bgPoint(box.xMin(), box.yMin()),
              bgPoint(
                  box.xMax() - 1,
                  box.yMax() - 1));  /// the -1 is to prevent imaginary overlaps
                                     /// where a region ends and another starts
          regions_rtree_.insert(bbox);
        }
        group.addRect(box);
        bbox.merge(box);
      }
      group.setBoundary(bbox);
    }

    for (auto db_inst : db_group->getInsts()) {
      Node* cell = db_inst_map_[db_inst];
      Group* group = cell_height_to_group_map[cell->getHeight()];
      group->addCell(cell);
      cell->setGroup(group);
    }
  }
}

}  // namespace dpl
