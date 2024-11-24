// Copyright (c) 2024, The Regents of the University of California
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

#include "ora/MakeOra.hh"

#include "ora/Ora.hh"
#include "ord/OpenRoad.hh"

namespace ord {

ora::Ora* makeOra()
{
  return new ora::Ora;
}

void deleteOra(ora::Ora* ora)
{
  delete ora;
}

void initOra(OpenRoad* openroad)
{
  utl::Logger* logger = openroad->getLogger();
  openroad->getOra()->init(openroad->tclInterp(), openroad->getDb(), logger);
}

}  // namespace ord
