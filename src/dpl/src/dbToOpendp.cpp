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

static bool swapWidthHeight(const dbOrientType& orient);

void Opendp::importDb()
{
  block_ = db_->getChip()->getBlock();
  core_ = block_->getCoreArea();
  have_fillers_ = false;
  have_one_site_cells_ = false;

  importClear();
  examineRows();
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
  const int master_height = db_master->getHeight();
  master->is_multi_row
      = (master_height != row_height_ && master_height % row_height_ == 0);
}

void Opendp::examineRows()
{
  std::vector<dbRow*> rows;
  auto block_rows = block_->getRows();
  rows.reserve(block_rows.size());
  for (auto* row : block_rows) {
    if (row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    rows.push_back(row);
  }
  if (rows.empty()) {
    logger_->error(DPL, 12, "no rows found.");
  }

  int min_row_height_ = std::numeric_limits<int>::max();
  int min_site_width_ = std::numeric_limits<int>::max();

  for (dbRow* db_row : rows) {
    dbSite* site = db_row->getSite();
    if (site->isHybrid()) {
      continue;
    }
    min_row_height_
        = std::min(min_row_height_, static_cast<int>(site->getHeight()));
    min_site_width_
        = std::min(min_site_width_, static_cast<int>(site->getWidth()));
  }
  if (min_row_height_ == std::numeric_limits<int>::max()) {
    // this means only hybrid sites exist, which breaks most of DPL mapping
    // logic. We still need this empty one-site grid to operate smoothly.
    // TODO(mina1460): create this grid even if no cells exist, just to be safe.
    logger_->error(
        DPL,
        129,
        "Only hybrid sites exist. Please add at least one non-hybrid site.");
  }
  row_height_ = min_row_height_;
  site_width_ = min_site_width_;
  row_site_count_ = divFloor(core_.dx(), site_width_);
  row_count_ = divFloor(core_.dy(), row_height_);
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
      cell.width_ = bbox.dx();
      cell.height_ = bbox.dy();
      cell.x_ = bbox.xMin();
      cell.y_ = bbox.yMin();
      cell.orient_ = db_inst->getOrient();
      // Cell is already placed if it is FIXED.
      cell.is_placed_ = isFixed(&cell);

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

Rect Opendp::getBbox(dbInst* inst)
{
  dbMaster* master = inst->getMaster();

  int loc_x, loc_y;
  inst->getLocation(loc_x, loc_y);
  // Shift by core lower left.
  loc_x -= core_.xMin();
  loc_y -= core_.yMin();

  int width = master->getWidth();
  int height = master->getHeight();
  if (swapWidthHeight(inst->getOrient())) {
    std::swap(width, height);
  }

  return Rect(loc_x, loc_y, loc_x + width, loc_y + height);
}

static bool swapWidthHeight(const dbOrientType& orient)
{
  switch (orient) {
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

void Opendp::makeGroups()
{
  regions_rtree.clear();
  // preallocate groups so it does not grow when push_back is called
  // because region cells point to them.
  auto db_groups = block_->getGroups();
  int reserve_size = 0;
  for (auto db_group : db_groups) {
    dbRegion* parent = db_group->getRegion();
    std::unordered_set<int> unique_heights;
    if (parent) {
      for (auto db_inst : db_group->getInsts()) {
        unique_heights.insert(db_inst_map_[db_inst]->height_);
      }
      reserve_size += unique_heights.size();
    }
  }
  reserve_size = std::max(reserve_size, (int) db_groups.size());
  groups_.reserve(reserve_size);

  for (auto db_group : db_groups) {
    dbRegion* parent = db_group->getRegion();
    if (parent) {
      std::set<int> unique_heights;
      map<int, Group*> cell_height_to_group_map;
      for (auto db_inst : db_group->getInsts()) {
        unique_heights.insert(db_inst_map_[db_inst]->height_);
      }
      int index = 0;
      for (auto height : unique_heights) {
        groups_.emplace_back(Group());
        struct Group& group = groups_.back();
        string group_name
            = string(db_group->getName()) + "_" + std::to_string(index++);
        group.name = group_name;
        group.boundary.mergeInit();
        cell_height_to_group_map[height] = &group;
        auto boundaries = parent->getBoundaries();

        for (dbBox* boundary : boundaries) {
          Rect box = boundary->getBox();
          box = box.intersect(core_);
          // offset region to core origin
          box.moveDelta(-core_.xMin(), -core_.yMin());
          if (height == *(unique_heights.begin())) {
            bgBox bbox(
                bgPoint(box.xMin(), box.yMin()),
                bgPoint(box.xMax() - 1,
                        box.yMax()
                            - 1));  /// the -1 is to prevent imaginary overlaps
                                    /// where a region ends and another starts
            regions_rtree.insert(bbox);
          }
          group.regions.push_back(box);
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
}

}  // namespace dpl
