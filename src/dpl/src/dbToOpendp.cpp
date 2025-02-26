/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cfloat>
#include <limits>
#include <string>
#include <unordered_set>

#include "Grid.h"
#include "Objects.h"
#include "dpl/Opendp.h"
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
  makeCellEdgeSpacingTable();
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
    struct Master& master = db_master_map_[db_master];
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
  master->is_multi_row = grid_->isMultiHeight(db_master);
  master->edges_.clear();
  if (edge_spacing_table_.empty()) {
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
    if (edge_types_indices_.find(edge->getEdgeType())
        != edge_types_indices_.end()) {
      // consider only edge types defined in the spacing table
      master->edges_.emplace_back(edge_types_indices_[edge->getEdgeType()],
                                  edge_rect);
    }
  }
  if (edge_types_indices_.find("DEFAULT") == edge_types_indices_.end()) {
    return;
  }
  // Add the remaining DEFAULT un-typed segments
  for (size_t dir_idx = 0; dir_idx <= 3; dir_idx++) {
    const auto dir = (odb::dbMasterEdgeType::EdgeDir) dir_idx;
    const auto parent_seg = edge_calc::getBoundarySegment(bbox, dir);
    const auto default_segs
        = edge_calc::difference(parent_seg, typed_segs[dir]);
    for (const auto& seg : default_segs) {
      master->edges_.emplace_back(0, seg);
    }
  }
}

void Opendp::makeCellEdgeSpacingTable()
{
  auto spacing_rules = db_->getTech()->getCellEdgeSpacingTable();
  if (spacing_rules.empty()) {
    return;
  }
  for (auto rule : spacing_rules) {
    edge_types_indices_.try_emplace(rule->getFirstEdgeType(),
                                    edge_types_indices_.size());
    edge_types_indices_.try_emplace(rule->getSecondEdgeType(),
                                    edge_types_indices_.size());
  }
  // Resize
  const size_t size = edge_types_indices_.size();
  edge_spacing_table_.resize(size);
  for (size_t i = 0; i < size; i++) {
    edge_spacing_table_[i].resize(size, EdgeSpacingEntry(0, false, false));
  }
  // Fill Table
  for (auto rule : spacing_rules) {
    std::string first_edge = rule->getFirstEdgeType();
    std::string second_edge = rule->getSecondEdgeType();
    const int spc = rule->getSpacing();
    const bool exact = rule->isExact();
    const bool except_abutted = rule->isExceptAbutted();
    const EdgeSpacingEntry entry(spc, exact, except_abutted);
    const int idx1 = edge_types_indices_[first_edge];
    const int idx2 = edge_types_indices_[second_edge];
    edge_spacing_table_[idx1][idx2] = entry;
    edge_spacing_table_[idx2][idx1] = entry;
  }
}

bool Opendp::hasCellEdgeSpacingTable() const
{
  return !edge_spacing_table_.empty();
}

int Opendp::getMaxSpacing(int edge_idx) const
{
  return std::max_element(edge_spacing_table_[edge_idx].begin(),
                          edge_spacing_table_[edge_idx].end())
      ->spc;
}

void Opendp::makeCells()
{
  auto db_insts = block_->getInsts();
  cells_.reserve(db_insts.size());
  for (auto db_inst : db_insts) {
    dbMaster* db_master = db_inst->getMaster();
    if (db_master->isCoreAutoPlaceable()) {
      cells_.emplace_back();
      Cell& cell = cells_.back();
      cell.db_inst_ = db_inst;
      db_inst_map_[db_inst] = &cell;

      Rect bbox = getBbox(db_inst);
      cell.width_ = DbuX{bbox.dx()};
      cell.height_ = DbuY{bbox.dy()};
      cell.x_ = DbuX{bbox.xMin()};
      cell.y_ = DbuY{bbox.yMin()};
      cell.orient_ = db_inst->getOrient();
      // Cell is already placed if it is FIXED.
      cell.is_placed_ = cell.isFixed();

      Master& master = db_master_map_[db_master];
      // We only want to set this if we have multi-row cells to
      // place and not whenever we see a placed block.
      if (master.is_multi_row && db_master->isCore()) {
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
        unique_heights.insert(db_inst_map_[db_inst]->height_);
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
    map<DbuY, Group*> cell_height_to_group_map;
    for (auto db_inst : db_group->getInsts()) {
      unique_heights.insert(db_inst_map_[db_inst]->height_);
    }
    int index = 0;
    for (auto height : unique_heights) {
      groups_.emplace_back();
      struct Group& group = groups_.back();
      string group_name
          = string(db_group->getName()) + "_" + std::to_string(index++);
      group.name = std::move(group_name);
      group.boundary.mergeInit();
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
        group.region_boundaries.push_back(box);
        group.boundary.merge(box);
      }
    }

    for (auto db_inst : db_group->getInsts()) {
      Cell* cell = db_inst_map_[db_inst];
      Group* group = cell_height_to_group_map[cell->height_];
      group->cells_.push_back(cell);
      cell->group_ = group;
    }
  }
}

}  // namespace dpl
