/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
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

#include <limits>
#include <cfloat>
#include <algorithm>
#include "openroad/Error.hh"
#include "openroad/OpenRoad.hh"
#include "opendp/Opendp.h"

namespace opendp {

using std::cerr;
using std::cout;
using std::endl;
using std::fixed;
using std::ifstream;
using std::max;
using std::min;
using std::numeric_limits;
using std::ofstream;
using std::string;
using std::to_string;
using std::vector;

using ord::error;
using ord::warn;

using odb::Rect;
using odb::dbBox;
using odb::dbNet;
using odb::dbMaster;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbOrientType;
using odb::dbPlacementStatus;
using odb::dbRowDir;
using odb::dbSigType;
using odb::dbRegion;
using odb::dbSWire;
using odb::dbSBox;

static bool swapWidthHeight(dbOrientType orient);

void Opendp::importDb() {
  block_ = db_->getChip()->getBlock();
  core_ = ord::getCore(block_);

  importClear();
  makeMacros();
  examineRows();
  makeCells();
  makeGroups();
  findRowPower();

  Power row_power = (initial_power_ == undefined) ? macro_top_power_ : initial_power_;
  row0_top_power_is_vdd_ = (row_power == VDD);
}

void Opendp::importClear() {
  db_master_map_.clear();
  cells_.clear();
  groups_.clear();
  db_master_map_.clear();
  db_inst_map_.clear();
  deleteGrid(grid_);
  grid_ = nullptr;
}

void Opendp::makeMacros() {
  macro_top_power_ = undefined;
  vector<dbMaster*> masters;
  block_->getMasters(masters);
  for(auto master : masters) {
    struct Macro &macro = db_master_map_[master];
    defineTopPower(macro, master);
  }
}

void Opendp::defineTopPower(Macro &macro,
			    dbMaster *master) {
  dbMTerm *power = nullptr;
  dbMTerm *gnd = nullptr;
  for(dbMTerm *mterm : master->getMTerms()) {
    dbSigType sig_type = mterm->getSigType();
    if(sig_type == dbSigType::POWER)
      power = mterm;
    else if(sig_type == dbSigType::GROUND)
      gnd = mterm;
  }

  if (power && gnd) {
    bool is_multi_row = power->getMPins().size() > 1 || gnd->getMPins().size() > 1;
    macro.is_multi_row_ = is_multi_row;

    int power_y_max = find_ymax(power);
    int gnd_y_max = find_ymax(gnd);
    Power top_power = (power_y_max > gnd_y_max) ? VDD : VSS;
    macro.top_power_ = top_power;
    if(!is_multi_row)
      macro_top_power_ = top_power;
  }
}

int Opendp::find_ymax(dbMTerm *mterm) {
  int ymax = 0;
  for(dbMPin *mpin : mterm->getMPins()) {
    for(dbBox *box : mpin->getGeometry()) ymax = max(ymax, box->yMax());
  }
  return ymax;
}

void Opendp::examineRows() {

  auto rows = block_->getRows();
  if (!rows.empty()) {
    int bottom_row_y = numeric_limits<int>::max();
    dbRow *bottom_row = nullptr;
    for (dbRow *db_row : rows) {
      dbSite *site = db_row->getSite();
      row_height_ = site->getHeight();
      site_width_ = site->getWidth();

      int row_x, row_y;
      db_row->getOrigin(row_x, row_y);
      if (row_y < bottom_row_y) {
	bottom_row_y = row_y;
	bottom_row = db_row;
      }
    }
    row_site_count_ = divFloor(core_.dx(), site_width_);
    row_count_ = divFloor(core_.dy(), row_height_);

    dbOrientType orient = bottom_row->getOrient();
    row0_orient_is_r0_ = (bottom_row->getOrient() == dbOrientType::R0);
  }
  else
    error("no rows found.");
}

void Opendp::makeCells() {
  multi_row_inst_count_ = 0;

  auto db_insts = block_->getInsts();
  cells_.reserve(db_insts.size());
  for(auto db_inst : db_insts) {
    dbMaster *master = db_inst->getMaster();
    if (isPlacedType(master->getType())) {
      cells_.push_back(Cell());
      Cell &cell = cells_.back();
      cell.db_inst_ = db_inst;
      db_inst_map_[db_inst] = &cell;

      int width = master->getWidth();
      int height = master->getHeight();
      if(swapWidthHeight(db_inst->getOrient())) std::swap(width, height);
      cell.width_ = width;
      cell.height_ = height;

      int init_x, init_y;
      initialLocation(&cell, init_x, init_y);
      // Shift by core lower left.
      cell.x_ = init_x;
      cell.y_ = init_y;
      cell.orient_ = db_inst->getOrient();
      cell.is_placed_ = isFixed(&cell);

      Macro &macro = db_master_map_[master];
      if (macro.is_multi_row_)
	multi_row_inst_count_++;
    }
  }
}

static bool swapWidthHeight(dbOrientType orient) {
  switch(orient) {
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
}

// OpenDB represents groups as regions with the parent pointing to
// the region.
// DEF GROUP => dbRegion with instances, no boundary, parent->region
// DEF REGION => dbRegion no instances, boundary, parent = null
void Opendp::makeGroups() {
  // preallocate groups so it does not grow when push_back is called
  // because region cells point to them.
  auto db_regions = block_->getRegions();
  groups_.reserve(block_->getRegions().size());
  for(auto db_region : db_regions) {
    dbRegion* parent = db_region->getParent();
    if (parent) {
      groups_.push_back(Group());
      struct Group &group = groups_.back();
      string group_name = db_region->getName();
      group.name = group_name.c_str();
      group.boundary.mergeInit();
      auto boundaries = db_region->getParent()->getBoundaries();
      for(dbBox *boundary : boundaries) {
	Rect box;
	boundary->getBox(box);
	box = box.intersect(core_);
	// offset region to core origin
	box.moveDelta(-core_.xMin(), -core_.yMin());

	group.regions.push_back(box);
	group.boundary.merge(box);
      }

      for (auto db_inst : db_region->getRegionInsts()) {
	Cell *cell = db_inst_map_[db_inst];
	group.cells_.push_back(cell);
	cell->group_ = &group;
      }
    }
  }
}

void Opendp::findRowPower() {
  initial_power_ = Power::undefined;
  int min_vdd_y = numeric_limits<int>::max();
  bool found_vdd = false;
  for(dbNet *net : block_->getNets()) {
    if (net->isSpecial()) {
      const char *net_name = net->getConstName();
      if (strcasecmp(net_name, power_net_name_) == 0) {
	for (dbSWire *swire : net->getSWires()) {
	  for (dbSBox *sbox : swire->getWires()) {
	    min_vdd_y = min(min_vdd_y, sbox->yMin());
	    found_vdd = true;
	  }
	}
      }
    }
  }
  if (found_vdd)
    initial_power_ = divRound(min_vdd_y, row_height_) % 2 == 0 ? VDD : VSS;
}

void Opendp::reportImportWarnings() {
  if(macro_top_power_ == Power::undefined) {
    warn("Cannot find MACRO with VDD/VSS pins.");
  }
  if (initial_power_ == Power::undefined)
    warn("could not find power special net");
}

}  // namespace opendp
