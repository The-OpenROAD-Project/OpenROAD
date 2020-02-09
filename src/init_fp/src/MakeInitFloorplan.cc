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

#include <tcl.h>
#include "openroad/OpenRoad.hh"
#include "StaMain.hh"
#include "init_fp/MakeInitFloorplan.hh"

namespace sta {

extern "C" {
extern int Init_fp_Init(Tcl_Interp *interp);
}

extern const char *init_fp_tcl_inits[];

}

namespace ord {

void
initInitFloorplan(OpenRoad *openroad)
{
  Tcl_Interp *interp = openroad->tclInterp();
  // Define swig TCL commands.
  sta::Init_fp_Init(interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(interp, sta::init_fp_tcl_inits);
}

} // namespace ord
