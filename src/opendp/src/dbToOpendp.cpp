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
#include "opendp/Opendp.h"

namespace opendp {

using std::cerr;
using std::cout;
using std::endl;
using std::fixed;
using std::ifstream;
using std::make_pair;
using std::max;
using std::min;
using std::numeric_limits;
using std::ofstream;
using std::pair;
using std::string;
using std::to_string;
using std::vector;

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

static bool swapWidthHeight(dbOrientType orient);

void Opendp::dbToOpendp() {
  // Clearing pre-built structure will enable
  // multiple execution of the legalize_placement command.
  clear();

  // LEF
  for(auto db_lib : db_->getLibs()) {
    make_macros(db_lib);
  }

  block_ = db_->getChip()->getBlock();
  findCore();
  // make rows in CoreArea;
  make_core_rows();
  make_cells();
  makeGroups();
  findInitialPower();
}

void Opendp::make_macros(dbLib *db_lib) {
  auto db_masters = db_lib->getMasters();
  macros_.reserve(db_masters.size());
  for(auto db_master : db_masters) {
    macros_.push_back(Macro());
    struct Macro &macro = macros_.back();
    db_master_map_[db_master] = &macro;

    macro.db_master = db_master;
    macro_define_top_power(&macro);
  }
}

// - - - - - - - define multi row cell & define top power - - - - - - - - //
void Opendp::macro_define_top_power(Macro *myMacro) {
  dbMaster *master = myMacro->db_master;

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
  if(power_y_max > gnd_y_max)
    myMacro->top_power = VDD;
  else
    myMacro->top_power = VSS;

  if(power && gnd) {
    if(power->getMPins().size() > 1 || gnd->getMPins().size() > 1)
      myMacro->isMulti = true;
  }
}

int Opendp::find_ymax(dbMTerm *mterm) {
  int ymax = 0;
  for(dbMPin *mpin : mterm->getMPins()) {
    for(dbBox *box : mpin->getGeometry()) ymax = max(ymax, box->yMax());
  }
  return ymax;
}

void Opendp::findCore() {
  core_.mergeInit();
  auto db_rows = block_->getRows();
  for(auto db_row : db_rows) {
    int orig_x, orig_y;
    db_row->getOrigin(orig_x, orig_y);
    int site_count = db_row->getSiteCount();

    dbSite *site = db_row->getSite();
    row_height_ = site->getHeight();
    site_width_ = site->getWidth();

    adsRect row_bbox;
    db_row->getBBox(row_bbox);
    core_.merge(row_bbox);
  }
}

// Generate new rows to fill core area.
void Opendp::make_core_rows() {
  row_site_count_ = coreGridWidth();
  int row_count = coreGridHeight();

  int bottom_row_y = numeric_limits<int>::max();
  dbRow *bottom_row = nullptr;
  for (dbRow *db_row : block_->getRows()) {
    int row_x, row_y;
    db_row->getOrigin(row_x, row_y);
    if (row_y < bottom_row_y) {
      bottom_row_y = row_y;
      bottom_row = db_row;
    }
  }
  if (bottom_row) {
    dbOrientType orient = bottom_row->getOrient();

    rows_.reserve(row_count);
    rows_.clear();
    for(int i = 0; i < row_count; i++) {
      Row row;
      row.origX = core_.xMin();
      row.origY = core_.yMin() + i * row_height_;
      row.orient = orient;
      rows_.push_back(row);

      // orient flips. e.g. R0 -> MX -> R0 -> MX -> ...
      orient = (orient == dbOrientType::R0) ? dbOrientType::MX : dbOrientType::R0;
    }
  }
  else
    cerr << "Error: no rows found.";
}

void Opendp::make_cells() {
  auto db_insts = block_->getInsts();
  cells_.reserve(db_insts.size());
  for(auto db_inst : db_insts) {
    cells_.push_back(Cell());
    Cell &cell = cells_.back();
    cell.db_inst = db_inst;
    db_inst_map_[db_inst] = &cell;

    dbMaster *master = db_inst->getMaster();
    auto miter = db_master_map_.find(master);
    if(miter != db_master_map_.end()) {
      Macro *macro = miter->second;
      cell.cell_macro = macro;

      int width = master->getWidth();
      int height = master->getHeight();
      if(swapWidthHeight(db_inst->getOrient())) std::swap(width, height);
      cell.width = width;
      cell.height = height;

      // Shift by core lower left.
      int x, y;
      db_inst->getLocation(x, y);
      cell.init_x_coord = std::max(0, x - core_.xMin());
      cell.init_y_coord = std::max(0, y - core_.yMin());

      // fixed cells
      if(isFixed(&cell)) {
        // Shift by core lower left.
        cell.x_coord = x - core_.xMin();
        cell.y_coord = y - core_.yMin();
        cell.is_placed = true;
      }
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
	cell->cell_group = &group;
      }
      // Reverse instances for compatibility with standalone ordering.
      // reverse(group.siblings.begin(), group.siblings.end());
    }
  }
}

void Opendp::findInitialPower() {
  const char *power_net_name = "VDD";
  int min_vdd_y = numeric_limits<int>::max();
  bool found_vdd = false;
  for(dbNet *net : block_->getNets()) {
    if (net->isSpecial()) {
      const char *net_name = net->getConstName();
      if (strcasecmp(net_name, power_net_name) == 0) {
	for (dbSWire *swire : net->getSWires()) {
	  for (dbSBox *sbox : swire->getWires()) {
#ifdef ODP_DEBUG
	    adsRect box;
	    printf("wire box %d %d %d %d\n",
		   sbox->yMin(), sbox->xMin(),
		   sbox->xMax(), sbox->yMax());
#endif
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
    cerr << "Error: could not find power special net " << power_net_name << endl;
}

}  // namespace opendp
