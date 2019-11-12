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
#include "db_sta/dbSta.hh"
#include "resizer/Resizer.hh"
#include "openroad/Version.hh"
#include "openroad/OpenRoad.hh"

using ord::OpenRoad;

using odb::dbDatabase;

using sta::stringEq;
using sta::Sta;
using sta::dbSta;
using sta::initSta;
using sta::showUsage;
using sta::staSetupAppInit;
using sta::staTclAppInit;
using sta::parseThreadsArg;

using sta::Resizer;

// Swig uses C linkage for init functions.
extern "C" {
extern int Openroad_Init(Tcl_Interp *interp);
extern int Opendbtcl_Init(Tcl_Interp *interp);
extern int Dbsta_Init(Tcl_Interp *interp);
extern int Resizer_Init(Tcl_Interp *interp);
}

namespace sta {
extern const char *openroad_tcl_inits[];
}

static int
swigInit(Tcl_Interp *interp)
{
  Openroad_Init(interp);
  Opendbtcl_Init(interp);
  Dbsta_Init(interp);
  Resizer_Init(interp);
  return 1;
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
    dbDatabase *db = dbDatabase::create();
    OpenRoad *openroad = new OpenRoad(db);
    OpenRoad::setOpenRoad(openroad);

    initSta();
    dbSta *sta = openroad->getSta();
    Sta::setSta(sta);
    sta->makeComponents();

    Resizer *resizer = openroad->getResizer();
    resizer->copyState(sta);
    resizer->initFlute(argv[0]);

    int thread_count = parseThreadsArg(argc, argv);
    sta->setThreadCount(thread_count);

    staSetupAppInit(argc, argv, ".openroad", swigInit,
		    sta::openroad_tcl_inits);
    // Set argc to 1 so Tcl_Main doesn't source any files.
    // Tcl_Main never returns.
    Tcl_Main(1, argv, staTclAppInit);
    return 0;
  }
}
