%module init_fp

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

%{

#include "Machine.hh"
#include "db_sta/dbSta.hh"
#include "init_fp/InitFloorplan.hh"

// Defined by OpenRoad.i
namespace ord {

odb::dbDatabase *
getDb();

sta::dbSta *
getSta();
}

%}

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%include "../../Exception.i"

%inline %{

void
init_floorplan_core(double die_lx,
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
  odb::dbDatabase *db = ord::getDb();
  sta::dbSta *sta = ord::getSta();
  sta::Report *report = sta->report();
  ord::initFloorplan(die_lx, die_ly, die_ux, die_uy,
		     core_lx, core_ly, core_ux, core_uy,
		     site_name, tracks_file,
		     db, report);
}

void
init_floorplan_util(double util,
		    double aspect_ratio,
		    double core_space,
		    const char *site_name,
		    const char *tracks_file)

{
  odb::dbDatabase *db = ord::getDb();
  sta::dbSta *sta = ord::getSta();
  sta::Report *report = sta->report();
  ord::initFloorplan(util, aspect_ratio, core_space,
		     site_name, tracks_file,
		     db, report);
}

void
auto_place_pins_cmd(const char *pin_layer)
{
  odb::dbDatabase *db = ord::getDb();
  sta::dbSta *sta = ord::getSta();
  sta::Report *report = sta->report();
  ord::autoPlacePins(pin_layer, db, report);
}

%} // inline
