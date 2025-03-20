// Copyright (c) 2021, The Regents of the University of California
// All rights reserved.
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

#include "ord/OpenRoad.hh"
#include "tool/Tool.hh"
#include "tool/MakeTool.hh"

namespace ord {

tool::Tool *
makeTool()
{
  return new tool::Tool;
}

void
deleteTool(tool::Tool *tool)
{
  delete tool;
}

void
initTool(OpenRoad *openroad)
{
  openroad->getTool()->init(openroad->tclInterp(),
			    openroad->getDb());
}

}
