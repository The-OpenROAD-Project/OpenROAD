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
#include "OpenStaDB.hh"
#include "StaMain.hh"
#include "OpenStaDB/Version.hh"

using sta::stringEq;
using sta::OpenStaDB;
using sta::staMain;
using sta::showUsage;

// Swig uses C linkage for init functions.
extern "C" {
extern int Opensta_db_Init(Tcl_Interp *interp);
}

namespace sta {
extern const char *opensta_db_tcl_inits[];
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
    printf("%s\n", STA_VERSION);
    return 0;
  }
  else {
    OpenStaDB *sta = new OpenStaDB;
    staMain(sta, argc, argv, Opensta_db_Init, sta::opensta_db_tcl_inits);
    return 0;
  }
}
