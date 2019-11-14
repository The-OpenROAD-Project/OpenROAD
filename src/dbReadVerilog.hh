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

#ifndef VERILOG2DB_H
#define VERILOG2DB_H

namespace odb {
class dbDatabase;
}

namespace sta {
class Debug;
class Report;
class Network;
}

namespace ord {

using odb::dbDatabase;
using sta::NetworkReader;

void
dbReadVerilog(const char *filename,
	      NetworkReader *db_network);

void
dbLinkDesign(const char *top_cell_name,
	     dbDatabase *db);

} // namespace
#endif
