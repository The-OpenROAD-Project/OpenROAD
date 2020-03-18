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
using std::pair;
using std::string;
using std::to_string;
using std::vector;

using ord::error;
using ord::warn;

using odb::adsRect;
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
using odb::dbMasterType;

static bool swapWidthHeight(dbOrientType orient);
static bool placeMasterType(dbMasterType type);

void Opendp::dbToOpendp() {
  // LEF
  for(auto db_lib : db_->getLibs())
    makeMacros(db_lib);

  block_ = db_->getChip()->getBlock();
  core_ = ord::getCore(block_);

  examineRows();
  makeCells();
  makeGroups();
  findRowPower();
}

void Opendp::makeMacros(dbLib *db_lib) {
  auto db_masters = db_lib->getMasters();
  for(auto db_master : db_masters) {
    struct Macro &macro = db_master_map_[db_master];
    defineTopPower(macro, db_master);
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

  int power_y_max = power ? find_ymax(power) : 0;
  int gnd_y_max = gnd ? find_ymax(gnd) : 0;
  macro.top_power_ = (power_y_max > gnd_y_max) ? VDD : VSS;

  macro.is_multi_row_ = power && gnd
    && (power->getMPins().size() > 1 || gnd->getMPins().size() > 1);
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
  auto db_insts = block_->getInsts();
  cells_.reserve(db_insts.size());
  for(auto db_inst : db_insts) {
    dbMaster *master = db_inst->getMaster();
    // Ignore PAD/COVER/RING/ENDCAP instances.
    if (placeMasterType(master->getType())) {
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
      initLocation(&cell, init_x, init_y);
      // Shift by core lower left.
      cell.x_ = init_x;
      cell.y_ = init_y;
      cell.orient_ = db_inst->getOrient();
      cell.is_placed_ = isFixed(&cell);
    }
  }
}

// Use switch so if new types are added we get a compiler warning.
static bool placeMasterType(dbMasterType type) {
  switch (type) {
  case dbMasterType::CORE:
  case dbMasterType::CORE_FEEDTHRU:
  case dbMasterType::CORE_TIEHIGH:
  case dbMasterType::CORE_TIELOW:
  case dbMasterType::CORE_SPACER:
  case dbMasterType::BLOCK:
    return true;
  case dbMasterType::NONE:
  case dbMasterType::COVER:
  case dbMasterType::RING:
  case dbMasterType::PAD:
  case dbMasterType::PAD_INPUT:
  case dbMasterType::PAD_OUTPUT:
  case dbMasterType::PAD_INOUT:
  case dbMasterType::PAD_POWER:
  case dbMasterType::PAD_SPACER:
  case dbMasterType::ENDCAP:
  case dbMasterType::ENDCAP_PRE:
  case dbMasterType::ENDCAP_POST:
  case dbMasterType::ENDCAP_TOPLEFT:
  case dbMasterType::ENDCAP_TOPRIGHT:
  case dbMasterType::ENDCAP_BOTTOMLEFT:
  case dbMasterType::ENDCAP_BOTTOMRIGHT:
    return false;
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
	adsRect box;
	boundary->getBox(box);
	box = box.intersect(core_);
	// offset region to core origin
	box.moveDelta(-core_.xMin(), -core_.yMin());

	group.regions.push_back(box);
	group.boundary.merge(box);
      }

      for (auto db_inst : db_region->getRegionInsts()) {
	Cell *cell = db_inst_map_[db_inst];
	group.siblings.push_back(cell);
	cell->group_ = &group;
      }
    }
  }
}

void Opendp::findRowPower() {
  initial_power_ = power::undefined;
  const char *power_net_name = "VDD";
  int min_vdd_y = numeric_limits<int>::max();
  bool found_vdd = false;
  for(dbNet *net : block_->getNets()) {
    if (net->isSpecial()) {
      const char *net_name = net->getConstName();
      if (strcasecmp(net_name, power_net_name) == 0) {
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
  else
    warn("could not find power special net");
}

}  // namespace opendp
