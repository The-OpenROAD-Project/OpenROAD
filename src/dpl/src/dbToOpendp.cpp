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
#include "utl/Logger.h"

namespace dpl {

using std::string;
using std::vector;

using utl::DPL;

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
  have_one_site_cells_ = false;

  importClear();
  grid_->examineRows(block_);
  checkOneSiteDbMaster();
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

void Opendp::checkOneSiteDbMaster()
{
  vector<dbMaster*> masters;
  auto db_libs = db_->getLibs();
  for (auto db_lib : db_libs) {
    if (have_one_site_cells_) {
      break;
    }
    auto masters = db_lib->getMasters();
    for (auto db_master : masters) {
      if (isOneSiteCell(db_master)) {
        have_one_site_cells_ = true;
        break;
      }
    }
  }
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

void Opendp::makeMaster(Master* master, dbMaster* db_master)
{
  master->is_multi_row = grid_->isMultiHeight(db_master);
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
