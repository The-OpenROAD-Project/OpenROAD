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

#include "pdngen/PdnGen.hh"
#include "StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char *pdngen_tcl_inits[];
}

namespace pdngen {

extern "C" {
extern int Pdngen_Init(Tcl_Interp *interp);
}

PdnGen::PdnGen()
{
}

PdnGen::~PdnGen()
{
}

void
PdnGen::init(Tcl_Interp *tcl_interp,
	   odb::dbDatabase *db)
{
  db_ = db;

  // Define swig TCL commands.
  Pdngen_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(tcl_interp, sta::pdngen_tcl_inits);
}


}
