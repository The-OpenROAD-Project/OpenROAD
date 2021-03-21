/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
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

#include "openroad/OpenRoad.hh"
#include "utl/Logger.h"

namespace ifp {

using std::string;
using std::abs;
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


class Track
{
public:
  Track(string layer,
	char dir,
	double offset,
	double pitch);
  string layer() const { return layer_; }
  char dir() const { return dir_; }
  double offset() const { return offset_; }
  double pitch() const { return pitch_; }

protected:
  string layer_;
  char dir_; 			// X or Y
  double offset_;		// meters
  double pitch_;		// meters
};

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
		     const char *tracks_file,
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
		     const char *tracks_file,
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
		     const char *site_name,
		     const char *tracks_file);
  double designArea();
  void makeRows(dbSite *site,
		int core_lx,
		int core_ly,
		int core_ux,
		int core_uy);
  dbSite *findSite(const char *site_name);
  Vector<Track> readTracks(const char *tracks_file);
  void makeTracks(const char *tracks_file,
		  Rect &die_area);
  void autoPlacePins(dbTechLayer *pin_layer,
		     Rect &core);
  int metersToMfgGrid(double dist) const;
  double dbuToMeters(int dist) const;

  void updateVoltageDomain(int core_lx,
                           int core_ly,
                           int core_ux,
                           int core_uy);

  dbDatabase *db_;
  dbBlock *block_;
  Logger *logger_;
  Vector<Track> tracks_;
};

void
initFloorplan(double util,
	      double aspect_ratio,
	      double core_space_bottom,
	      double core_space_top,
	      double core_space_left,
	      double core_space_right,
	      const char *site_name,
	      const char *tracks_file,
	      dbDatabase *db,
	      Logger *logger)
{
  InitFloorplan init_fp;
  init_fp.initFloorplan(util, aspect_ratio,
                        core_space_bottom, core_space_top,
                        core_space_left, core_space_right,
                        site_name, tracks_file,
                        db, logger);
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
	      const char *tracks_file,
	      dbDatabase *db,
              Logger *logger)
{
  InitFloorplan init_fp;
  init_fp.initFloorplan(die_lx, die_ly, die_ux, die_uy,
			core_lx, core_ly, core_ux, core_uy,
			site_name, tracks_file,
			db, logger);
}

void
InitFloorplan::initFloorplan(double util,
			     double aspect_ratio,
			     double core_space_bottom,
			     double core_space_top,
			     double core_space_left,
			     double core_space_right,
			     const char *site_name,
			     const char *tracks_file,
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
		    site_name, tracks_file);
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
			     const char *tracks_file,
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
		    site_name, tracks_file);

    }
  }
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
			     const char *tracks_file)
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
      // floor core lower left corner to multiple of site dx/dy.
      int clx = (metersToMfgGrid(core_lx) / site_dx) * site_dx;
      int cly = (metersToMfgGrid(core_ly) / site_dy) * site_dy;
      int cux = metersToMfgGrid(core_ux);
      int cuy = metersToMfgGrid(core_uy);
      makeRows(site, clx, cly, cux, cuy);
      updateVoltageDomain(clx, cly, cux, cuy);

      // Destroy existing tracks.
      for (odb::dbTrackGrid *grid : block_->getTrackGrids())
        dbTrackGrid::destroy(grid);

      if (tracks_file && tracks_file[0])
	makeTracks(tracks_file, die_area);
    }
    else
      logger_->warn(IFP, 9, "SITE {} not found.", site_name);
  }
}


// this function is used to create regions ( split overlapped rows and create new ones )
void
InitFloorplan::updateVoltageDomain(int core_lx,
                                   int core_ly,
                                   int core_ux,
                                   int core_uy)
{
  // this is hardcoded for now as a margin between voltage domains
  int MARGIN = 6;

  // checks if a group is defined as a voltage domain, if so it creates a region 
  for (dbGroup* domain : block_->getGroups()) {
    if (domain->getType()==dbGroup::VOLTAGE_DOMAIN) {
      dbRegion* domain_region = dbRegion::create(block_, domain->getName());

      auto domain_name = string(domain->getName());
      Rect domain_rect = domain->getBox();
      int temp_domain_xMin = metersToMfgGrid(domain_rect.xMin() / 1e+6);
      int temp_domain_yMin = metersToMfgGrid(domain_rect.yMin() / 1e+6);
      int temp_domain_xMax = metersToMfgGrid(domain_rect.xMax() / 1e+6);
      int temp_domain_yMax = metersToMfgGrid(domain_rect.yMax() / 1e+6);

      dbSet<dbRow> rows = block_->getRows();
      dbSet<dbRow>::iterator first_row = rows.begin();
      dbSite *site = first_row->getSite();
      int row_height_ = site->getHeight();
      int site_width_ = site->getWidth();

      int domain_yMin = temp_domain_yMin - temp_domain_yMin % row_height_;
      int domain_yMax = temp_domain_yMax - temp_domain_yMax % row_height_;
      int domain_xMin = temp_domain_xMin - temp_domain_xMin % site_width_;
      int domain_xMax = temp_domain_xMax - temp_domain_xMax % site_width_;

      dbBox::create(domain_region, domain_xMin, domain_yMin, domain_xMax, domain_yMax);

      int row_old_count = 0;
      for (dbRow* row_count : rows) {
        row_old_count += 1;
      }

      int row_processed = 0;
      for (dbRow *row_itr: rows) {
        row_processed += 1;
        if (row_processed > row_old_count) {
          break;
        }

        Rect row_rect;
        row_itr->getBBox(row_rect);
        int row_xMin = row_rect.xMin();
        int row_xMax = row_rect.xMax();
        int row_yMin = row_rect.yMin();
        int row_yMax = row_rect.yMax();

        string row_name = row_itr->getName();
        string row_number = row_name.substr(row_name.find("_") + 1);
        // check if the rows overlapped with the area of a defined voltage domains + margin
        if (row_yMax + MARGIN * row_height_ <= domain_yMin || row_yMin >= domain_yMax + MARGIN * row_height_) {
          continue;
        } else {
          dbOrientType orient = row_itr->getOrient();
          dbRow::destroy(row_itr);

          int row_1_xMax_sites = (domain_xMin - MARGIN * row_height_) / site_width_;
          int row_1_xMax = row_1_xMax_sites * site_width_;
          // in case there is at least one valid site width on the left, create left core rows 
          if (row_1_xMax > core_lx + site_width_)
          {
	    // warning message since tap cells might not be inserted
            if (row_1_xMax < core_lx + 10 * site_width_) {
              logger_->warn(IFP, 11, "left core row has less than 10 sites");   
            } 
            string row_1_new_name = "ROW_" + row_number + "_1";
            int row_1_sites = (row_1_xMax - core_lx) / site_width_;
            dbRow::create(block_, row_1_new_name.c_str(), site, core_lx, row_yMin, orient, dbRowDir::HORIZONTAL, row_1_sites, site_width_);
          }
          
          int row_2_xMin_sites = (domain_xMax + MARGIN * row_height_) / site_width_;
          int row_2_xMin = row_2_xMin_sites * site_width_;
          // in case there is at least one valid site width on the right, create right core rows 
          if (row_2_xMin + site_width_ < core_ux)
          {  
            if (row_2_xMin + 10 * site_width_ > core_ux) {
              logger_->warn(IFP, 12, "right core row has less than 10 sites"); 
            }   
            string row_2_new_name = "ROW_" + row_number + "_2";
            int row_2_sites = (core_ux - row_2_xMin) / site_width_;
            dbRow::create(block_, row_2_new_name.c_str(), site, row_2_xMin, row_yMin, orient, dbRowDir::HORIZONTAL, row_2_sites, site_width_);
          }

          int domain_row_sites = (domain_xMax - domain_xMin) / site_width_;
          // create domain rows if current iterations are not in margin area
          if (row_yMin >= domain_yMin && row_yMax <= domain_yMax) {
            string domain_row_name = "ROW_" + domain_name + "_" + row_number;
            dbRow::create(block_, domain_row_name.c_str(), site, domain_xMin, row_yMin, orient, dbRowDir::HORIZONTAL, domain_row_sites, site_width_);
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
      ? dbOrientType::MX  // FS
      : dbOrientType::R0; // N
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
InitFloorplan::makeTracks(const char *tracks_file,
			  Rect &die_area)
{
  Vector<Track> tracks = readTracks(tracks_file);
  dbTech *tech = db_->getTech();
  for (auto track : tracks) {
    int pitch = metersToMfgGrid(track.pitch());
    int offset = metersToMfgGrid(track.offset());
    char dir = track.dir();
    string layer_name = track.layer();
    dbTechLayer *layer = tech->findLayer(layer_name.c_str());
    if (layer) {
      dbTrackGrid *grid = block_->findTrackGrid(layer);
      if (grid == nullptr)
	grid = dbTrackGrid::create(block_, layer);
      int width;
      int track_count;
      switch (dir) {
      case 'X':
	width = die_area.dx();
	track_count = (width - offset) / pitch + 1;
	grid->addGridPatternX(die_area.xMin() + offset, track_count, pitch);
	break;
      case 'Y':
	width = die_area.dy();
	track_count = (width - offset) / pitch + 1;
	grid->addGridPatternY(die_area.yMin() + offset, track_count, pitch);
	break;
      default:
	logger_->critical(IFP, 3, "unknown track direction");
      }
    }
  }
}

Vector<Track>
InitFloorplan::readTracks(const char *tracks_file)
{
  Vector<Track> tracks;
  std::ifstream tracks_stream(tracks_file);
  if (tracks_stream.is_open()) {
    int line_count = 1;
    string line;
    dbTech *tech = db_->getTech();
    while (getline(tracks_stream, line)) {
      StringVector tokens;
      split(line, " \t", tokens);
      if (tokens.size() == 4) {
	string layer_name = tokens[0];
	dbTechLayer *layer = tech->findLayer(layer_name.c_str());
	if (layer) {
	  string dir_str = tokens[1];
	  char dir = 'X';
	  if (stringEqual(dir_str.c_str(), "X"))
	    dir = 'X';
	  else if (stringEqual(dir_str.c_str(), "Y"))
	    dir = 'Y';
	  else
	    logger_->error(IFP, 4, "track file line {} direction must be X or Y'.",
                           line_count);
	  // microns -> meters
	  double offset = strtod(tokens[2].c_str(), nullptr) * 1e-6;
	  double pitch = strtod(tokens[3].c_str(), nullptr) * 1e-6;
	  tracks.push_back(Track(layer_name, dir, offset, pitch));
	}
	else
	  logger_->error(IFP, 5, "layer {} not found.", layer_name);
      }
      else
	logger_->error(IFP, 6,
                       "track file line {} does not match 'layer X|Y offset pitch'.",
                       line_count);
      line_count++;
    }
    tracks_stream.close();
  }
  else
    logger_->error(IFP, 10, "Tracks file not readable.");
  return tracks;
}

Track::Track(string layer,
	     char dir,
	     double offset,
	     double pitch) :
  layer_(layer),
  dir_(dir),
  offset_(offset),
  pitch_(pitch)
{
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
