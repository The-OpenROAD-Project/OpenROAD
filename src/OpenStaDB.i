%module opensta_db

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

%{

#include "opendb/db.h"
#include "opendb/lefin.h"
#include "opendb/defin.h"
#include "opendb/defout.h"
#include "Machine.hh"
#include "Report.hh"
#include "Network.hh"
#include "OpenStaDB/Version.hh"
#include "OpenStaDB/OpenStaDB.hh"
#include "OpenStaDB/OpenRoad.hh"

////////////////////////////////////////////////////////////////
//
// C++ helper functions used by the interface functions.
// These are not visible in the TCL API.
//
////////////////////////////////////////////////////////////////

using ord::OpenRoad;
using sta::OpenStaDB;
using sta::OpenDBNetwork;
using odb::dbDatabase;

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

OpenStaDB *
getSta()
{
  return getOpenRoad()->getSta();
}

OpenDBNetwork *
getDbNetwork()
{
  return getOpenRoad()->getDbNetwork();
}

%}

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%inline %{

const char *
opensta_db_version()
{
  return OPENSTA_DB_VERSION;
}

const char *
opensta_db_git_sha1()
{
  return OPENSTA_DB_GIT_SHA1;
}

void
init_sta_db()
{
  OpenRoad *ord = getOpenRoad();
  odb::dbDatabase *db = ord->getDb();
  OpenStaDB *sta = ord->getSta();
  sta->init(db);
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

%} // inline

// OpenSTA swig files
%include "StaException.i"
%include "StaTcl.i"
%include "NetworkEdit.i"
%include "Sdf.i"
%include "DelayCalc.i"
%include "Parasitics.i"

%include "Verilog2db.i"
%include "InitFloorplan.i"
