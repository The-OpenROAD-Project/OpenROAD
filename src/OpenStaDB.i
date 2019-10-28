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
#include "OpenStaDB/Version.hh"
#include "Machine.hh"
#include "Report.hh"
#include "Network.hh"
#include "OpenStaDB/OpenStaDB.hh"

namespace sta {

////////////////////////////////////////////////////////////////
//
// C++ helper functions used by the interface functions.
// These are not visible in the TCL API.
//
////////////////////////////////////////////////////////////////

odb::dbDatabase *db_ = nullptr;

odb::dbDatabase *
getDb()
{
  return db_;
}

odb::dbDatabase *
ensureDb()
{
  if (db_ == nullptr)
    db_ = odb::dbDatabase::create();
  return db_;
}

OpenStaDB *
openStaDB()
{
  return dynamic_cast<OpenStaDB *>(Sta::sta());
}

} // namespace

using namespace sta;

%}

////////////////////////////////////////////////////////////////
//
// SWIG type definitions.
//
////////////////////////////////////////////////////////////////

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

bool
have_db()
{  
  odb::dbDatabase *db = getDb();
  if (db) {
    odb::dbChip *chip = db->getChip();
    if (chip) {
      odb::dbBlock *block = chip->getBlock();
      if (block)
	return true;
    }
  }
  return false;
}

bool
have_db_tech()
{  
  odb::dbDatabase *db = getDb();
  if (db)
    return db->getTech() != nullptr;
  else
    return false;
}

void
init_sta_db()
{
  odb::dbDatabase *db = getDb();
  if (db)
    openStaDB()->init(db);
}

void
read_lef_cmd(const char *filename,
	     const char *lib_name,
	     bool make_tech,
	     bool make_library)
{
  odb::dbDatabase *db = ensureDb();
  odb::lefin lef_reader(db, false);
  if (make_tech && make_library)
    lef_reader.createTechAndLib(lib_name, filename);
  else if (make_tech)
    lef_reader.createTech(filename);
  else if (make_library)
    lef_reader.createLib(lib_name, filename);
}

void
read_def_cmd(const char *filename)
{
  odb::dbDatabase *db = getDb();
  if (db) {
    odb::defin def_reader(db);
    std::vector<odb::dbLib *> search_libs;
    for (odb::dbLib *lib : db->getLibs())
      search_libs.push_back(lib);
    def_reader.createChip(search_libs, filename);
  }
}

void
write_def_cmd(const char *filename)
{
  odb::dbDatabase *db = getDb();
  if (db) {
    odb::dbChip *chip = db->getChip();
    if (chip) {
      odb::dbBlock *block = chip->getBlock();
      if (block) {
	odb::defout def_writer;
	def_writer.writeBlock(block, filename);
      }
    }
  }
}

void
read_db_cmd(const char *filename)
{
  odb::dbDatabase *db = ensureDb();
  FILE *stream = fopen(filename, "r");
  db->read(stream);
  fclose(stream);
}

void
write_db_cmd(const char *filename)
{
  odb::dbDatabase *db = getDb();
  if (db) {
    FILE *stream = fopen(filename, "w");
    db->write(stream);
    fclose(stream);
  }
}

%} // inline

%include "StaException.i"
%include "StaTcl.i"
%include "NetworkEdit.i"
%include "Sdf.i"
%include "DelayCalc.i"
%include "Parasitics.i"
