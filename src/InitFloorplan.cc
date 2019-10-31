// Resizer, LEF/DEF gate resizer
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
#include "Machine.hh"
#include "Debug.hh"
#include "Error.hh"
#include "Report.hh"
#include "Vector.hh"
#include "PortDirection.hh"
#include "OpenDBNetwork.hh"
#include "opendb/db.h"
#include "opendb/dbTransform.h"
#include "InitFloorplan.hh"

namespace ord {

using std::string;

using sta::Vector;
using sta::Report;
using sta::Debug;
using sta::stringPrint;
using sta::StringVector;
using sta::stringEqual;
using sta::FileNotReadable;

using odb::dbDatabase;
using odb::dbChip;
using odb::dbBlock;
using odb::dbLib;
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
  void initFloorplan(const char *site_name,
		     const char *tracks_file,
		     bool auto_place_pins,
		     const char *pin_layer_name,
		     // Die area.
		     double die_lx,
		     double die_ly,
		     double die_ux,
		     double die_uy,
		     double core_lx,
		     double core_ly,
		     double core_ux,
		     double core_uy,
		     dbDatabase *db,
		     OpenDBNetwork *network);

protected:
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
  uint metersToDbu(double dist) const;
  double dbuToMeters(uint dist) const;

  dbDatabase *db_;
  OpenDBNetwork *network_;
  dbBlock *block_;
  Vector<Track> tracks_;
};

void
initFloorplan(const char *site_name,
	      const char *tracks_file,
	      bool auto_place_pins,
	      const char *pin_layer_name,
	      // Die area.
	      double die_lx,
	      double die_ly,
	      double die_ux,
	      double die_uy,
	      double core_lx,
	      double core_ly,
	      double core_ux,
	      double core_uy,
	      dbDatabase *db,
	      OpenDBNetwork *network)
{
  InitFloorplan init_fp;
  init_fp.initFloorplan(site_name,
			tracks_file,
			auto_place_pins, pin_layer_name,
			die_lx, die_ly, die_ux, die_uy,
			core_lx, core_ly, core_ux, core_uy,
			db, network);
}

void
InitFloorplan::initFloorplan(const char *site_name,
			     const char *tracks_file,
			     bool auto_place_pins,
			     const char *pin_layer_name,
			     // Die area.
			     double die_lx,
			     double die_ly,
			     double die_ux,
			     double die_uy,
			     double core_lx,
			     double core_ly,
			     double core_ux,
			     double core_uy,
			     dbDatabase *db,
			     OpenDBNetwork *network)
{
  db_ = db;
  network_ = network;

  Report *report = network_->report();
  dbChip *chip = db->getChip();
  if (chip) {
    block_ = chip->getBlock();
    dbTechLayer *pin_layer = nullptr;
    if (auto_place_pins && pin_layer_name) {
      dbTech *tech = db_->getTech();
      pin_layer = tech->findLayer(pin_layer_name);
      if (pin_layer == nullptr)
	report->warn("pin layer %s not found.\n", pin_layer_name);
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
}

void
InitFloorplan::makeRows(const char *site_name,
			double core_lx,
			double core_ly,
			double core_ux,
			double core_uy)
{
  Report *report = network_->report();
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
      report->printWarn("Warning: SITE %s not found.\n", site_name);
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
    dbTrackGrid *grid = dbTrackGrid::create(block_, layer);
    uint width;
    int track_count;
    switch (dir) {
    case 'X':
      width = metersToDbu(die_ux - die_lx);
      track_count = floor((width - offset) / pitch) + 1;
      grid->addGridPatternX(offset, track_count, pitch);
      break;
    case 'Y':
      width = metersToDbu(die_uy - die_ly);
      track_count = floor((width - offset) / pitch) + 1;
      grid->addGridPatternY(offset, track_count, pitch);
      break;
    default:
      internalError("unknown track direction\n");
    }
  }
}

void
InitFloorplan::readTracks(const char *tracks_file)
{
  Report *report = network_->report();
  Debug *debug = network_->debug();
  std::ifstream tracks_stream(tracks_file);
  if (tracks_stream.is_open()) {
    int line_count = 1;
    string line;
    while (getline(tracks_stream, line)) {
      StringVector tokens;
      split(line, " \t", tokens);
      if (tokens.size() == 4) {
	string layer = tokens[0];
	string dir_str = tokens[1];
	char dir = 'X';
	if (stringEqual(dir_str.c_str(), "X"))
	  dir = 'X';
	else if (stringEqual(dir_str.c_str(), "Y"))
	  dir = 'Y';
	else
	  report->warn("Warning: track file line %d direction must be X or Y'.\n",
		       line_count);
	// microns -> meters
	double offset = strtod(tokens[2].c_str(), nullptr) * 1e-6;
	double pitch = strtod(tokens[3].c_str(), nullptr) * 1e-6;
	tracks_.push_back(Track(layer, dir, offset, pitch));
	debugPrint4(debug, "track", 1, "%s %c %f %f\n", layer.c_str(), dir, offset, pitch);
      }
      else
	report->warn("Warning: track file line %d does not match 'layer X|Y offset pitch'.\n",
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
      dbBox::create(bpin, pin_layer, x, y, x, y);

      location += pin_dist;
    }
  }
}

// DBUs are nanometers.
uint
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
