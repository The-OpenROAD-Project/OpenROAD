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
#include "utility/Logger.h"

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
  void makeTracks(const char *tracks_file,
		  Rect &die_area);
  void makeTracks(Rect &die_area);
  void readTracks(const char *tracks_file);
  void autoPlacePins(dbTechLayer *pin_layer,
		     Rect &core);
  int metersToMfgGrid(double dist) const;
  double dbuToMeters(int dist) const;

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

      // Destroy existing tracks.
      for (odb::dbTrackGrid *grid : block_->getTrackGrids())
        dbTrackGrid::destroy(grid);

      if (tracks_file && tracks_file[0])
	makeTracks(tracks_file, die_area);
      else
	makeTracks(die_area);
    }
    else
      logger_->warn(IFP, 9, "SITE {} not found.", site_name);
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
  readTracks(tracks_file);
  dbTech *tech = db_->getTech();
  for (auto track : tracks_) {
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

void
InitFloorplan::readTracks(const char *tracks_file)
{
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
	  tracks_.push_back(Track(layer_name, dir, offset, pitch));
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

void
InitFloorplan::makeTracks(Rect &die_area)
{
  dbTech *tech = db_->getTech();
  dbSet<dbTechLayer> layers = tech->getLayers();
  for (auto layer : layers) {
    if (layer->getType() == dbTechLayerType::ROUTING) {
      dbTrackGrid *grid;
      uint width;
      int offset, pitch, track_count;
      // vertical
      grid = block_->findTrackGrid(layer);
      if (grid == nullptr)
        grid = dbTrackGrid::create(block_, layer);
      width = die_area.dx();
      pitch = layer->getPitchX();
      if (pitch) {
        offset = layer->getOffsetX();
        if (offset == 0)
          offset = pitch;
        track_count = floor((width - offset) / pitch) + 1;
        grid->addGridPatternX(offset, track_count, pitch);
      }
      else
        logger_->error(IFP, 7, "layer {} has zero X pitch.", layer->getConstName());

      // horizontal
      grid = block_->findTrackGrid(layer);
      if (grid == nullptr)
        grid = dbTrackGrid::create(block_, layer);
      width = die_area.dy();
      pitch = layer->getPitchY();
      if (pitch) {
        offset = layer->getOffsetY();
        if (offset == 0)
          offset = pitch;
        track_count = floor((width - offset) / pitch) + 1;
        grid->addGridPatternY(offset, track_count, pitch);
      }
      else
        logger_->error(IFP, 8, "layer {} has zero Y pitch.",
                       layer->getConstName());
    }
  }
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
