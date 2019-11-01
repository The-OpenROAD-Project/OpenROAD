// OpenStaDB, OpenSTA on OpenDB
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

#include <stdio.h>
#include "Machine.hh"
#include "StringUtil.hh"
#include "StaMain.hh"
#include "sta_db/StaDb.hh"
#include "openroad/Version.hh"
#include "openroad/OpenRoad.hh"

using sta::stringEq;
using sta::StaDb;
using sta::staMain;
using sta::showUsage;

// Swig uses C linkage for init functions.
extern "C" {
extern int Opensta_db_Init(Tcl_Interp *interp);
}

namespace sta {
extern const char *openroad_tcl_inits[];
}

int
main(int argc,
     char *argv[])
{
  if (argc == 2 && stringEq(argv[1], "-help")) {
    showUsage(argv[0]);
    return 0;
  }
  else if (argc == 2 && stringEq(argv[1], "-version")) {
    printf("%s %s\n", OPENROAD_VERSION, OPENROAD_GIT_SHA1);
    return 0;
  }
  else {
    ord::OpenRoad *ord = ord::OpenRoad::openRoad();
    StaDb *sta = ord->getSta();
    staMain(sta, argc, argv, Opensta_db_Init, sta::openroad_tcl_inits);
    return 0;
  }
}
