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

#include "Machine.hh"
#include "openroad/OpenRoad.hh"
#include "resizer/Resizer.hh"
#include "resizer/MakeResizer.hh"

namespace ord {

using sta::dbSta;
using sta::Resizer;

Resizer *
makeResizer()
{
  return new Resizer;
}

void
deleteResizer(Resizer *resizer)
{
  delete resizer;
}

void
initResizer(OpenRoad *openroad)
{
  openroad->getResizer()->init(openroad->tclInterp(),
			       openroad->getDb(),
			       openroad->getSta());
}

}
