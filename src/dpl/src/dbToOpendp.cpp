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

#include "dpl/Opendp.h"
#include "utl/Logger.h"

namespace dpl {

using std::max;
using std::min;
using std::numeric_limits;
using std::string;
using std::vector;

using utl::DPL;

using odb::dbBox;
using odb::dbMaster;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbOrientType;
using odb::dbRegion;
using odb::dbSBox;
using odb::dbSigType;
using odb::dbSWire;
using odb::Rect;

static bool swapWidthHeight(dbOrientType orient);

void Opendp::importDb()
{
  block_ = db_->getChip()->getBlock();
  core_ = block_->getCoreArea();
  have_fillers_ = false;

  importClear();
  examineRows();
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

void Opendp::makeMaster(Master* master, dbMaster* db_master)
{
  const int master_height = db_master->getHeight();
  const bool is_multi_row
      = master_height != row_height_ && master_height % row_height_ == 0;

  master->is_multi_row = is_multi_row;
}

void Opendp::examineRows()
{
  auto rows = block_->getRows();
  if (!rows.empty()) {
    for (dbRow* db_row : rows) {
      dbSite* site = db_row->getSite();
      row_height_ = site->getHeight();
      site_width_ = site->getWidth();
    }
    row_site_count_ = divFloor(core_.dx(), site_width_);
    row_count_ = divFloor(core_.dy(), row_height_);
  } else
    logger_->error(DPL, 12, "no rows found.");
}

void Opendp::makeCells()
{
  auto db_insts = block_->getInsts();
  cells_.reserve(db_insts.size());
  for (auto db_inst : db_insts) {
    dbMaster* db_master = db_inst->getMaster();
    if (db_master->isCoreAutoPlaceable()) {
      cells_.push_back(Cell());
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
    if (isFiller(db_inst))
      have_fillers_ = true;
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
  if (swapWidthHeight(inst->getOrient()))
    std::swap(width, height);

  return Rect(loc_x, loc_y, loc_x + width, loc_y + height);
}

static bool swapWidthHeight(dbOrientType orient)
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
  // preallocate groups so it does not grow when push_back is called
  // because region cells point to them.
  auto db_groups = block_->getGroups();
  groups_.reserve(db_groups.size());
  for (auto db_group : db_groups) {
    dbRegion* parent = db_group->getRegion();
    if (parent) {
      groups_.emplace_back(Group());
      struct Group& group = groups_.back();
      string group_name = db_group->getName();
      group.name = group_name;
      group.boundary.mergeInit();
      auto boundaries = parent->getBoundaries();
      for (dbBox* boundary : boundaries) {
        Rect box = boundary->getBox();
        box = box.intersect(core_);
        // offset region to core origin
        box.moveDelta(-core_.xMin(), -core_.yMin());

        group.regions.push_back(box);
        group.boundary.merge(box);
      }

      for (auto db_inst : db_group->getInsts()) {
        Cell* cell = db_inst_map_[db_inst];
        group.cells_.push_back(cell);
        cell->group_ = &group;
      }
    }
  }
}

}  // namespace dpl
