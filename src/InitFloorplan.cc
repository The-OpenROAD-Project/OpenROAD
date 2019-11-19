// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <iostream>
#include <fstream>
#include <cmath>
#include "Machine.hh"
#include "Error.hh"
#include "Report.hh"
#include "StringUtil.hh"
#include "Vector.hh"
#include "PortDirection.hh"
#include "opendb/db.h"
#include "opendb/dbTransform.h"
#include "InitFloorplan.hh"

namespace ord {

using std::string;
using std::abs;

using sta::Vector;
using sta::Report;
using sta::stringPrint;
using sta::StringVector;
using sta::stringEqual;
using sta::FileNotReadable;

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
using odb::adsRect;
using odb::dbOrientType;
using odb::dbTechLayerDir;
using odb::dbTrackGrid;
using odb::dbPlacementStatus;
using odb::dbTransform;
using odb::dbBox;

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
		     double core_space,
		     const char *site_name,
		     const char *tracks_file,
		     bool auto_place_pins,
		     const char *pin_layer_name,
		     dbDatabase *db,
		     Report *report);
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
		     bool auto_place_pins,
		     const char *pin_layer_name,
		     dbDatabase *db,
		     Report *report);

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
		     const char *tracks_file,
		     bool auto_place_pins,
		     const char *pin_layer_name);
  double designArea();
  void makeRows(const char *site_name,
		double core_lx,
		double core_ly,
		double core_ux,
		double core_uy);
  dbSite *findSite(const char *site_name);
  void makeTracks(const char *tracks_file,
		  double die_lx,
		  double die_ly,
		  double die_ux,
		  double die_uy);
  void makeTracks(double die_lx,
		  double die_ly,
		  double die_ux,
		  double die_uy);
  void readTracks(const char *tracks_file);
  void autoPlacePins(dbTechLayer *pin_layer,
		     double core_lx,
		     double core_ly,
		     double core_ux,
		     double core_uy);
  int metersToDbu(double dist) const;
  double dbuToMeters(uint dist) const;

  dbDatabase *db_;
  Report *report_;
  dbBlock *block_;
  Vector<Track> tracks_;
};

void
initFloorplan(double util,
	      double aspect_ratio,
	      double core_space,
	      const char *site_name,
	      const char *tracks_file,
	      bool auto_place_pins,
	      const char *pin_layer_name,
	      dbDatabase *db,
	      Report *report)
{
  InitFloorplan init_fp;
  init_fp.initFloorplan(util, aspect_ratio, core_space,
			site_name, tracks_file,
			auto_place_pins, pin_layer_name,
			db, report);
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
	      bool auto_place_pins,
	      const char *pin_layer_name,
	      dbDatabase *db,
	      Report *report)
{
  InitFloorplan init_fp;
  init_fp.initFloorplan(die_lx, die_ly, die_ux, die_uy,
			core_lx, core_ly, core_ux, core_uy,
			site_name, tracks_file,
			auto_place_pins, pin_layer_name,
			db, report);
}

void
InitFloorplan::initFloorplan(double util,
			     double aspect_ratio,
			     double core_space,
			     const char *site_name,
			     const char *tracks_file,
			     bool auto_place_pins,
			     const char *pin_layer_name,
			     dbDatabase *db,
			     Report *report)
{
  report_ = report;
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

      double core_lx = core_space;
      double core_ly = core_space;
      double core_ux = core_space + core_width;
      double core_uy = core_space + core_height;
      double die_lx = 0.0;
      double die_ly = 0.0;
      double die_ux = core_width + core_space * 2.0;
      double die_uy = core_height + core_space * 2.0;
      initFloorplan(die_lx, die_ly, die_ux, die_uy,
		    core_lx, core_ly, core_ux, core_uy,
		    site_name, tracks_file,
		    auto_place_pins, pin_layer_name);
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
			     bool auto_place_pins,
			     const char *pin_layer_name,
			     dbDatabase *db,
			     Report *report)
{
  report_ = report;
  db_ = db;
  dbChip *chip = db_->getChip();
  if (chip) {
    block_ = chip->getBlock();
    if (block_) {
      initFloorplan(die_lx, die_ly, die_ux, die_uy,
		    core_lx, core_ly, core_ux, core_uy,
		    site_name, tracks_file,
		    auto_place_pins, pin_layer_name);
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
			     const char *tracks_file,
			     bool auto_place_pins,
			     const char *pin_layer_name)
{
  dbTechLayer *pin_layer = nullptr;
  if (pin_layer_name[0] == '\0')
    pin_layer_name = nullptr;
  if (auto_place_pins && pin_layer_name) {
    dbTech *tech = db_->getTech();
    pin_layer = tech->findLayer(pin_layer_name);
    if (pin_layer == nullptr)
      report_->warn("pin layer %s not found.\n", pin_layer_name);
  }

  adsRect die_area(metersToDbu(die_lx),
		   metersToDbu(die_ly),
		   metersToDbu(die_ux),
		   metersToDbu(die_uy));
  block_->setDieArea(die_area);
  makeRows(site_name, core_lx, core_ly, core_ux, core_uy);
  if (tracks_file && tracks_file[0])
    makeTracks(tracks_file, die_lx, die_ly, die_ux, die_uy);
  else
    makeTracks(die_lx, die_ly, die_ux, die_uy);
  if (auto_place_pins && pin_layer)
    autoPlacePins(pin_layer, core_lx, core_ly, core_ux, core_uy);
}

void
InitFloorplan::makeRows(const char *site_name,
			double core_lx,
			double core_ly,
			double core_ux,
			double core_uy)
{
  if (site_name && site_name[0]
      &&  core_lx >= 0.0 && core_lx >= 0.0 && core_ux >= 0.0 && core_uy >= 0.0) {
    dbSite *site = findSite(site_name);
    if (site) {
      uint site_dx = site->getWidth();
      uint site_dy = site->getHeight();
      double core_dx = abs(core_ux - core_lx);
      double core_dy = abs(core_uy - core_ly);
      int rows_x = floor(core_dx / dbuToMeters(site_dx));
      int rows_y = floor(core_dy / dbuToMeters(site_dy));

      uint core_lx_dbu = metersToDbu(core_lx);
      uint y = metersToDbu(core_ly);;
      for (int row = 0; row < rows_y; row++) {
	dbOrientType orient = (row % 2 == 0)
	  ? dbOrientType::MX  // FS
	  : dbOrientType::R0; // N
	const char *row_name = stringPrint("ROW_%d", row);
	dbRow::create(block_, row_name, site, core_lx_dbu, y, orient,
		      dbRowDir::HORIZONTAL, rows_x, site_dx);
	y += site_dy;
      }
    }
    else
      report_->printWarn("Warning: SITE %s not found.\n", site_name);
  }
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
			  double die_lx,
			  double die_ly,
			  double die_ux,
			  double die_uy)
{
  readTracks(tracks_file);
  dbTech *tech = db_->getTech();
  for (auto track : tracks_) {
    double offset = track.offset();
    double pitch = track.pitch();
    char dir = track.dir();
    const char *layer_name = track.layer().c_str();
    dbTechLayer *layer = tech->findLayer(layer_name);
    if (layer) {
      dbTrackGrid *grid = block_->findTrackGrid(layer);
      if (grid == nullptr)
	grid = dbTrackGrid::create(block_, layer);
      double width;
      int track_count;
      int pitch_dbu = metersToDbu(pitch);
      int offset_dbu = metersToDbu(offset);
      switch (dir) {
      case 'X':
	width = die_ux - die_lx;
	track_count = floor((width - offset) / pitch) + 1;
	grid->addGridPatternX(offset_dbu, track_count, pitch_dbu);
	break;
      case 'Y':
	width = die_uy - die_ly;
	track_count = floor((width - offset) / pitch) + 1;
	grid->addGridPatternY(offset_dbu, track_count, pitch_dbu);
	break;
      default:
	internalError("unknown track direction\n");
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
	    report_->warn("Warning: track file line %d direction must be X or Y'.\n",
			  line_count);
	  // microns -> meters
	  double offset = strtod(tokens[2].c_str(), nullptr) * 1e-6;
	  double pitch = strtod(tokens[3].c_str(), nullptr) * 1e-6;
	  tracks_.push_back(Track(layer_name, dir, offset, pitch));
	}
	else
	  report_->fileWarn(tracks_file, line_count, "layer %s not found.\n",
			    layer_name.c_str());
      }
      else
	report_->warn("Warning: track file line %d does not match 'layer X|Y offset pitch'.\n",
		      line_count);
      line_count++;
    }
    tracks_stream.close();
  }
  else
    throw FileNotReadable(tracks_file);
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
InitFloorplan::makeTracks(double die_lx,
			  double die_ly,
			  double die_ux,
			  double die_uy)
{
  dbTech *tech = db_->getTech();
  dbSet<dbTechLayer> layers = tech->getLayers();
  for (auto layer : layers) {
    double pitch = layer->getPitch();
    // FIXME is offset a lef 5.7 construct?
    //double offset = layer->hasOffset() ? layer.hasOffset() * 1e-6 : pitch;
    double offset = pitch;
    dbTechLayerDir layer_dir = layer->getDirection();
    dbTrackGrid *grid;
    uint width;
    int track_count;
    switch (layer_dir) {
    case dbTechLayerDir::HORIZONTAL:
      grid = dbTrackGrid::create(block_, layer);
      width = metersToDbu(die_ux - die_lx);
      track_count = floor((width - offset) / pitch) + 1;
      grid->addGridPatternX(offset, track_count, pitch);
      break;
    case dbTechLayerDir::VERTICAL:
      grid = dbTrackGrid::create(block_, layer);
      width = metersToDbu(die_uy - die_ly);
      track_count = floor((width - offset) / pitch) + 1;
      grid->addGridPatternY(offset, track_count, pitch);
      break;
    case dbTechLayerDir::NONE:
      break;
    }
  }
}

////////////////////////////////////////////////////////////////

void
InitFloorplan::autoPlacePins(dbTechLayer *pin_layer,
			     double core_lx,
			     double core_ly,
			     double core_ux,
			     double core_uy)
{
  dbSet<dbBTerm> bterms = block_->getBTerms();
  int pin_count = bterms.size();

  if (pin_count > 0) {
    double dx = abs(core_ux - core_lx);
    double dy = abs(core_uy - core_ly);
    double perimeter = dx * 2 + dy * 2;
    double location = 0.0;
    double pin_dist = perimeter / pin_count;

    for (dbBTerm *bterm : bterms) {
      double x, y;
      dbOrientType orient;
      if (location < dx) {
	// bottom
	x = core_lx + location;
	y = core_ly;
	orient = dbOrientType::R180; // S
      }
      else if (location < (dx + dy)) {
	// right
	x = core_ux;
	y = core_ly + (location - dx);
	orient = dbOrientType::R270; // E
      }
      else if (location < (dx * 2 + dy)) {
	// top
	x = core_ux - (location - (dx + dy));
	y = core_uy;
	orient = dbOrientType::R0; // N
      }
      else {
	// left
	x = core_lx;
	y = core_uy - (location - (dx * 2 + dy));
	orient = dbOrientType::R90; // W
      }

      dbBPin *bpin = dbBPin::create(bterm);
      bpin->setPlacementStatus(dbPlacementStatus::FIRM);
      int x_dbu = metersToDbu(x);
      int y_dbu = metersToDbu(y);
      dbBox::create(bpin, pin_layer, x_dbu, y_dbu, x_dbu, y_dbu);

      location += pin_dist;
    }
  }
}

// DBUs are nanometers.
int
InitFloorplan::metersToDbu(double dist) const
{
  dbTech *tech = db_->getTech();
  if (tech->hasManufacturingGrid()) {
    int grid = tech->getManufacturingGrid();
    return round(round(dist * 1e9 / grid) * grid);
  }
  else
    return round(dist * 1e9);
}

// DBUs are nanometers.
double
InitFloorplan::dbuToMeters(uint dist) const
{
  return dist * 1E-9;
}

}
