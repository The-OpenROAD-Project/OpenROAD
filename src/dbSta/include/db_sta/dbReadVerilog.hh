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

#ifndef DB_READ_VERILOG_H
#define DB_READ_VERILOG_H

namespace odb {
class dbDatabase;
}

namespace sta {
class NetworkReader;
}

namespace ord {

class dbVerilogNetwork;

using odb::dbDatabase;
using sta::NetworkReader;

dbVerilogNetwork *
makeDbVerilogNetwork();

void
initDbVerilogNetwork(OpenRoad *openroad);

void
deleteDbVerilogNetwork(dbVerilogNetwork *verilog_network);

// Read a hierarchical Verilog netlist into a OpenSTA concrete network
// objects. The hierarchical network is elaborated/flattened by the 
// link_design command and OpenDB objects are created from the flattened
// network.
void
dbReadVerilog(const char *filename,
	      dbVerilogNetwork *verilog_networku );

void
dbLinkDesign(const char *top_cell_name,
	     dbVerilogNetwork *verilog_network,
	     dbDatabase *db);

} // namespace
#endif
