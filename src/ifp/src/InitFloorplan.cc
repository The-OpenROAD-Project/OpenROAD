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
//
///////////////////////////////////////////////////////////////////////////////

#include "ifp/InitFloorplan.hh"

#include <iostream>
#include <fstream>
#include <cmath>

#include "opendb/db.h"
#include "opendb/dbTransform.h"

#include "sta/StringUtil.hh"
#include "sta/Vector.hh"
#include "sta/PortDirection.hh"

#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

namespace ifp {

using std::string;
using std::abs;
using std::ceil;
using std::round;

using sta::Vector;
using sta::stdstrPrint;
using sta::StringVector;
using sta::stringEqual;

using utl::Logger;
using utl::IFP;

using odb::dbDatabase;
using odb::dbChip;
using odb::dbBlock;
using odb::dbLib;
using odb::dbMaster;
using odb::dbInst;
using odb::dbTech;
using odb::dbBTerm;
using odb::dbBPin;
using odb::dbSet;
using odb::dbSite;
using odb::dbRow;
using odb::dbRowDir;
using odb::dbTechLayer;
using odb::Rect;
using odb::dbOrientType;
using odb::dbTechLayerDir;
using odb::dbTrackGrid;
using odb::dbPlacementStatus;
using odb::dbTransform;
using odb::dbBox;
using odb::dbTechLayerType;
using odb::dbGroup;
using odb::dbRegion;

class InitFloorplan
{
public:
  InitFloorplan() {}
  void initFloorplan(double util,
		     double aspect_ratio,
		     double core_space_bottom,
		     double core_space_top,
		     double core_space_left,
		     double core_space_right,
		     const char *site_name,
		     dbDatabase *db,
		     Logger *logger);

  void initFloorplan(double die_lx,
		     double die_ly,
		     double die_ux,
		     double die_uy,
		     double core_lx,
		     double core_ly,
		     double core_ux,
		     double core_uy,
		     const char *site_name,
		     dbDatabase *db,
		     Logger *logger );

  void autoPlacePins(const char *pin_layer_name,
		     dbDatabase *db,
		     Logger *logger);

protected:
  void initFloorplan(double die_lx,
		     double die_ly,
		     double die_ux,
		     double die_uy,
		     double core_lx,
		     double core_ly,
		     double core_ux,
		     double core_uy,
		     const char *site_name);
  double designArea();
  void makeRows(dbSite *site,
		int core_lx,
		int core_ly,
		int core_ux,
		int core_uy);
  dbSite *findSite(const char *site_name);
  void makeTracks(const char *tracks_file,
		  Rect &die_area);
  void autoPlacePins(dbTechLayer *pin_layer,
		     Rect &core);
  int metersToMfgGrid(double dist) const;
  double dbuToMeters(int dist) const;
  void updateVoltageDomain(dbSite *site,
			   int core_lx,
            		   int core_ly,
            		   int core_ux,
            		   int core_uy);

  dbDatabase *db_;
  dbBlock *block_;
  Logger *logger_;
};

void
initFloorplan(double util,
	      double aspect_ratio,
	      double core_space_bottom,
	      double core_space_top,
	      double core_space_left,
	      double core_space_right,
	      const char *site_name,
	      dbDatabase *db,
	      Logger *logger)
{
  InitFloorplan init_fp;
  init_fp.initFloorplan(util, aspect_ratio,
                        core_space_bottom, core_space_top,
                        core_space_left, core_space_right,
                        site_name, db, logger);
}

void
initFloorplan(double die_lx,
	      double die_ly,
	      double die_ux,
	      double die_uy,
	      double core_lx,
	      double core_ly,
	      double core_ux,
	      double core_uy,
	      const char *site_name,
	      dbDatabase *db,
              Logger *logger)
{
  InitFloorplan init_fp;
  init_fp.initFloorplan(die_lx, die_ly, die_ux, die_uy,
			core_lx, core_ly, core_ux, core_uy,
			site_name, db, logger);
}

void
InitFloorplan::initFloorplan(double util,
			     double aspect_ratio,
			     double core_space_bottom,
			     double core_space_top,
			     double core_space_left,
			     double core_space_right,
			     const char *site_name,
			     dbDatabase *db,
			     Logger *logger)
{
  logger_ = logger;
  db_ = db;
  dbChip *chip = db_->getChip();
  if (chip) {
    block_ = chip->getBlock();
    if (block_) {
      // In microns.
      double design_area = designArea();
      double core_area = design_area / util;
      double core_width = std::sqrt(core_area / aspect_ratio);
      double core_height = core_width * aspect_ratio;

      double core_lx = core_space_left;
      double core_ly = core_space_bottom;
      double core_ux = core_lx + core_width;
      double core_uy = core_ly + core_height;
      double die_lx = 0.0;
      double die_ly = 0.0;
      double die_ux = core_width + core_space_left + core_space_right ;
      double die_uy = core_height + core_space_top + core_space_bottom;
      initFloorplan(die_lx, die_ly, die_ux, die_uy,
		    core_lx, core_ly, core_ux, core_uy,
		    site_name);
    }
  }
}

double
InitFloorplan::designArea()
{
  double design_area = 0.0;
  for (dbInst *inst : block_->getInsts()) {
    dbMaster *master = inst->getMaster();
    double area = dbuToMeters(master->getHeight()) * dbuToMeters(master->getWidth());
    design_area += area;
  }
  return design_area;
}

void
InitFloorplan::initFloorplan(double die_lx,
			     double die_ly,
			     double die_ux,
			     double die_uy,
			     double core_lx,
			     double core_ly,
			     double core_ux,
			     double core_uy,
			     const char *site_name,
			     dbDatabase *db,
			     Logger *logger)
{
  logger_ = logger;
  db_ = db;
  dbChip *chip = db_->getChip();
  if (chip) {
    block_ = chip->getBlock();
    if (block_) {
      initFloorplan(die_lx, die_ly, die_ux, die_uy,
		    core_lx, core_ly, core_ux, core_uy,
		    site_name);

    }
  }
}

static int
divCeil(int dividend, int divisor)
{
  return ceil(static_cast<double>(dividend) / divisor);
}

void
InitFloorplan::initFloorplan(double die_lx,
			     double die_ly,
			     double die_ux,
			     double die_uy,
			     double core_lx,
			     double core_ly,
			     double core_ux,
			     double core_uy,
			     const char *site_name)
{
  Rect die_area(metersToMfgGrid(die_lx),
                metersToMfgGrid(die_ly),
                metersToMfgGrid(die_ux),
                metersToMfgGrid(die_uy));
  block_->setDieArea(die_area);

  if (site_name && site_name[0]
      &&  core_lx >= 0.0 && core_lx >= 0.0 && core_ux >= 0.0 && core_uy >= 0.0) {
    dbSite *site = findSite(site_name);
    if (site) {
      // Destroy any existing rows.
      auto rows = block_->getRows();
      for (dbSet<dbRow>::iterator row_itr = rows.begin();
           row_itr != rows.end() ;
           row_itr = dbRow::destroy(row_itr)) ;

      uint site_dx = site->getWidth();
      uint site_dy = site->getHeight();
      // core lower left corner to multiple of site dx/dy.
      int clx = divCeil(metersToMfgGrid(core_lx), site_dx) * site_dx;
      int cly = divCeil(metersToMfgGrid(core_ly), site_dy) * site_dy;
      int cux = metersToMfgGrid(core_ux);
      int cuy = metersToMfgGrid(core_uy);
      makeRows(site, clx, cly, cux, cuy);
      updateVoltageDomain(site, clx, cly, cux, cuy);
    }
    else
      logger_->warn(IFP, 9, "SITE {} not found.", site_name);
  }
}


// this function is used to create regions ( split overlapped rows and create new ones )
void
InitFloorplan::updateVoltageDomain(dbSite *site,
				   int core_lx,
				   int core_ly,
				   int core_ux,
				   int core_uy)
{
  //The unit for power_domain_y_space is the site height. The real space is power_domain_y_space * site_dy
  static constexpr int power_domain_y_space = 6;
  uint site_dy = site->getHeight();
  uint site_dx = site->getWidth();
  
  // checks if a group is defined as a voltage domain, if so it creates a region 
  for (dbGroup *group: block_->getGroups()) {
    if (group->getType() == dbGroup::VOLTAGE_DOMAIN) {
      dbRegion *domain_region = dbRegion::create(block_, group->getName());

      string domain_name = group->getName();
      Rect domain_rect = group->getBox();
      int domain_xMin = domain_rect.xMin();
      int domain_yMin = domain_rect.yMin();
      int domain_xMax = domain_rect.xMax();
      int domain_yMax = domain_rect.yMax();
      dbBox::create(domain_region, domain_xMin, domain_yMin, domain_xMax, domain_yMax);

      dbSet<dbRow> rows = block_->getRows();
      int total_row_count = rows.size();

      
      dbSet<dbRow>::iterator row_itr = rows.begin();
      for (int row_processed = 0;
           row_processed < total_row_count;
           row_processed++) {          

        dbRow *row = *row_itr;
        Rect row_bbox;
        row->getBBox(row_bbox);
        int row_xMin = row_bbox.xMin();
        int row_xMax = row_bbox.xMax();
        int row_yMin = row_bbox.yMin();
        int row_yMax = row_bbox.yMax();

        // check if the rows overlapped with the area of a defined voltage domains + margin
        if (row_yMax + power_domain_y_space * site_dy <= domain_yMin || 
            row_yMin >= domain_yMax + power_domain_y_space * site_dy) {
          row_itr++;
        } else {
          
          string row_name = row->getName();
          dbOrientType orient = row->getOrient();
          row_itr = dbRow::destroy(row_itr);
          
          // lcr stands for left core row
          int lcr_xMax = domain_xMin - power_domain_y_space * site_dy;
          // in case there is at least one valid site width on the left, create left core rows 
          if (lcr_xMax > core_lx + site_dx)
          {
            string lcr_name = row_name + "_1";
	        // warning message since tap cells might not be inserted
            if (lcr_xMax < core_lx + 10 * site_dx) {
              logger_->warn(IFP, 26, "left core row: {} has less than 10 sites", lcr_name);   
            } 
            int lcr_sites = (lcr_xMax - core_lx) / site_dx;
            dbRow::create(block_, lcr_name.c_str(), site, core_lx, row_yMin, orient, 
                  	  dbRowDir::HORIZONTAL, lcr_sites, site_dx);
          }
          
          // rcr stands for right core row
          // rcr_dx_site_number is the max number of site_dx that is less than power_domain_y_space * site_dy. This helps align the rcr_xMin on the multiple of site_dx. 
          int rcr_dx_site_number = (power_domain_y_space * site_dy) / site_dx; 
          int rcr_xMin = domain_xMax + rcr_dx_site_number * site_dx; 

          // in case there is at least one valid site width on the right, create right core rows 
          if (rcr_xMin + site_dx < core_ux)
          {  
            string rcr_name = row_name + "_2";
            if (rcr_xMin + 10 * site_dx > core_ux) {
              logger_->warn(IFP, 27, "right core row: {} has less than 10 sites", rcr_name); 
            }   
            int rcr_sites = (core_ux - rcr_xMin) / site_dx;
            dbRow::create(block_, rcr_name.c_str(), site, rcr_xMin, row_yMin, orient, 
                  	  dbRowDir::HORIZONTAL, rcr_sites, site_dx);
          }

          int domain_row_sites = (domain_xMax - domain_xMin) / site_dx;
          // create domain rows if current iterations are not in margin area
          if (row_yMin >= domain_yMin && row_yMax <= domain_yMax) {
            string domain_row_name = row_name + "_" + domain_name;
            dbRow::create(block_, domain_row_name.c_str(), site, domain_xMin, row_yMin, orient, 
                  	  dbRowDir::HORIZONTAL, domain_row_sites, site_dx);
          }
        }
      }
    }
  }
}

void
InitFloorplan::makeRows(dbSite *site,
			int core_lx,
			int core_ly,
			int core_ux,
			int core_uy)
{
  int core_dx = abs(core_ux - core_lx);
  int core_dy = abs(core_uy - core_ly);
  uint site_dx = site->getWidth();
  uint site_dy = site->getHeight();
  int rows_x = core_dx / site_dx;
  int rows_y = core_dy / site_dy;

  int y = core_ly;
  for (int row = 0; row < rows_y; row++) {
    dbOrientType orient = (row % 2 == 0)
      ? dbOrientType::R0        // N
      : dbOrientType::MX;       // FS
    string row_name = stdstrPrint("ROW_%d", row);
    dbRow::create(block_, row_name.c_str(), site, core_lx, y, orient,
		  dbRowDir::HORIZONTAL, rows_x, site_dx);
    y += site_dy;
  }
  logger_->info(IFP, 1, "Added {} rows of {} sites.", rows_y, rows_x);
}

dbSite *
InitFloorplan::findSite(const char *site_name)
{
  for (dbLib *lib : db_->getLibs()) {
    dbSite *site = lib->findSite(site_name);
    if (site)
      return site;
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////

void
autoPlacePins(const char *pin_layer_name,
	      dbDatabase *db,
	      Logger *logger)
{
  InitFloorplan init_fp;
  init_fp.autoPlacePins(pin_layer_name, db, logger);
}

void
InitFloorplan::autoPlacePins(const char *pin_layer_name,
			     dbDatabase *db,
			     Logger *logger)
{
  logger_ = logger;
  db_ = db;
  dbChip *chip = db_->getChip();
  if (chip) {
    block_ = chip->getBlock();
    if (block_) {
      dbTech *tech = db_->getTech();
      dbTechLayer *pin_layer = tech->findLayer(pin_layer_name);
      if (pin_layer) {
	Rect core = ord::getCore(block_);
	autoPlacePins(pin_layer, core);
      }
      else
	logger_->warn(IFP, 2, "pin layer {} not found.", pin_layer_name);
    }
  }
}

void
InitFloorplan::autoPlacePins(dbTechLayer *pin_layer,
			     Rect &core)
{
  dbSet<dbBTerm> bterms = block_->getBTerms();
  int pin_count = bterms.size();

  if (pin_count > 0) {
    int dx = core.dx();
    int dy = core.dy();
    int perimeter = dx * 2 + dy * 2;
    double location = 0.0;
    int pin_dist = perimeter / pin_count;

    for (dbBTerm *bterm : bterms) {
      int x, y;
      dbOrientType orient;
      if (location < dx) {
	// bottom
	x = core.xMin() + location;
	y = core.yMin();
	orient = dbOrientType::R180; // S
      }
      else if (location < (dx + dy)) {
	// right
	x = core.xMax();
	y = core.yMin() + (location - dx);
	orient = dbOrientType::R270; // E
      }
      else if (location < (dx * 2 + dy)) {
	// top
	x = core.xMax() - (location - (dx + dy));
	y = core.yMax();
	orient = dbOrientType::R0; // N
      }
      else {
	// left
	x = core.xMin();
	y = core.yMax() - (location - (dx * 2 + dy));
	orient = dbOrientType::R90; // W
      }

      // Delete existing BPins.
      dbSet<dbBPin> bpins = bterm->getBPins();
      for (auto bpin_itr = bpins.begin(); bpin_itr != bpins.end(); )
	bpin_itr = dbBPin::destroy(bpin_itr);

      dbBPin *bpin = dbBPin::create(bterm);
      bpin->setPlacementStatus(dbPlacementStatus::FIRM);
      dbBox::create(bpin, pin_layer, x, y, x, y);

      location += pin_dist;
    }
  }
}

int
InitFloorplan::metersToMfgGrid(double dist) const
{
  dbTech *tech = db_->getTech();
  int dbu = tech->getDbUnitsPerMicron();
  if (tech->hasManufacturingGrid()) {
    int grid = tech->getManufacturingGrid();
    return round(round(dist * dbu * 1e+6 / grid) * grid);
  }
  else
    return round(dist * 1e+9);
}

double
InitFloorplan::dbuToMeters(int dist) const
{
  dbTech *tech = db_->getTech();
  int dbu = tech->getDbUnitsPerMicron();
  return dist / (dbu * 1e+6);
}

} // namespace
