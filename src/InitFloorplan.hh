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

#ifndef INIT_FLOORPLAN_H
#define INIT_FLOORPLAN_H

namespace odb {
class dbDatabase;
}

namespace sta {
class OpenDbNetwork;
}

namespace ord {

using odb::dbDatabase;
using sta::OpenDBNetwork;

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
	      OpenDBNetwork *network);

} // namespace
#endif
