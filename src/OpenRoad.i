%module openroad

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

%{

#include "opendb/db.h"
#include "opendb/lefin.h"
#include "opendb/defin.h"
#include "opendb/defout.h"
#include "Machine.hh"
#include "Report.hh"
#include "Network.hh"
#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "openroad/Version.hh"
#include "openroad/OpenRoad.hh"

////////////////////////////////////////////////////////////////
//
// C++ helper functions used by the interface functions.
// These are not visible in the TCL API.
//
////////////////////////////////////////////////////////////////

using ord::OpenRoad;

using sta::dbSta;
using sta::dbNetwork;
using sta::Resizer;

using odb::dbDatabase;

namespace sta {
// Defined in StaTcl.i
LibertyLibrarySeq *
tclListSeqLibertyLibrary(Tcl_Obj *const source,
			 Tcl_Interp *interp);
LibertyCellSeq *
tclListSeqLibertyCell(Tcl_Obj *const source,
		      Tcl_Interp *interp);
}

OpenRoad *
getOpenRoad()
{
  return OpenRoad::openRoad();
}

dbDatabase *
getDb()
{
  return getOpenRoad()->getDb();
}

// Copied from StaTcl.i because of ordering issues.
class CmdErrorNetworkNotLinked : public sta::StaException
{
public:
  virtual const char *what() const throw()
  { return "Error: no network has been linked."; }
};

void
ensureLinked(OpenRoad *openroad)
{
  dbNetwork *network = openroad->getDbNetwork();
  if (!network->isLinked())
    throw CmdErrorNetworkNotLinked();
}

dbNetwork *
getDbNetwork()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getDbNetwork();
}

dbSta *
getSta()
{
  return getOpenRoad()->getSta();
}

Resizer *
getResizer()
{
  OpenRoad *openroad = getOpenRoad();
  ensureLinked(openroad);
  return openroad->getResizer();
}

%}

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%include "OpenSTA/tcl/StaException.i"

%inline %{

const char *
openroad_version()
{
  return OPENROAD_VERSION;
}

const char *
openroad_git_sha1()
{
  return OPENROAD_GIT_SHA1;
}

bool
db_has_tech()
{
  return getDb()->getTech() != nullptr;
}

void
read_lef_cmd(const char *filename,
	     const char *lib_name,
	     bool make_tech,
	     bool make_library)
{
  OpenRoad *ord = getOpenRoad();
  ord->readLef(filename, lib_name, make_tech, make_library);
}

void
read_def_cmd(const char *filename)
{
  OpenRoad *ord = getOpenRoad();
  ord->readDef(filename);
}

void
write_def_cmd(const char *filename)
{
  OpenRoad *ord = getOpenRoad();
  ord->writeDef(filename);
}

void
read_db_cmd(const char *filename)
{
  OpenRoad *ord = getOpenRoad();
  ord->readDb(filename);
}

void
write_db_cmd(const char *filename)
{
  OpenRoad *ord = getOpenRoad();
  ord->writeDb(filename);
}

void
read_verilog(const char *filename)
{
  OpenRoad *ord = getOpenRoad();
  ord->readVerilog(filename);
}

void
link_design_db_cmd(const char *top_cell_name)
{
  OpenRoad *ord = getOpenRoad();
  ord->linkDesign(top_cell_name);
}

void
write_verilog_cmd(const char *filename,
		  bool sort)
{
  OpenRoad *ord = getOpenRoad();
  ord->writeVerilog(filename, sort);
}

%} // inline

// OpenROAD swig files
%include "InitFloorplan.i"
%include "resizer/Resizer.i"
