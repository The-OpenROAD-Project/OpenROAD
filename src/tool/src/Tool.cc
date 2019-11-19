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

#include "tool/Tool.hh"
#include "StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char *tool_tcl_inits[];
}

namespace tool {

extern "C" {
extern int Tool_Init(Tcl_Interp *interp);
}

Tool::Tool() :
  param1_(0.0),
  flag1_(false)
{
}

Tool::~Tool()
{
}

void
Tool::init(Tcl_Interp *tcl_interp,
	   odb::dbDatabase *db)
{
  db_ = db;

  // Define swig TCL commands.
  Tool_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(tcl_interp, sta::tool_tcl_inits);
}

void
Tool::run(const char *pos_arg1)
{
  printf("Gotta pos_arg1 %s\n", pos_arg1);
  printf("Gotta param1 %f\n", param1_);
  printf("Gotta flag1 %s\n", flag1_ ? "true" : "false");
}

void
Tool::setParam1(double param1)
{
  param1_ = param1;
}

void
Tool::setFlag1(bool flag1)
{
  flag1_ = flag1;
}

}
