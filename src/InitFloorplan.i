%module InitFloorplan

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
#include "InitFloorplan.hh"

using namespace sta;

%}

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%inline %{

void
init_floorplan_cmd(const char *site_name,
		   const char *tracks_file,
		   bool auto_place_pins,
		   const char *pin_layer_name,
		   double die_lx,
		   double die_ly,
		   double die_ux,
		   double die_uy,
		   double core_lx,
		   double core_ly,
		   double core_ux,
		   double core_uy)

{
  odb::dbDatabase *db = getDb();
  sta::OpenDBNetwork *network = getDbNetwork();
  ord::initFloorplan(site_name, tracks_file,
		     auto_place_pins, pin_layer_name,
		     die_lx, die_ly, die_ux, die_uy,
		     core_lx, core_ly, core_ux, core_uy,
		     db, network);
}

%} // inline
