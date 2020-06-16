%module openroad

/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

%{

#include "opendb/db.h"
#include "opendb/lefin.h"
#include "opendb/defin.h"
#include "opendb/defout.h"
#include "sta/Report.hh"
#include "sta/Network.hh"
#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "openroad/Version.hh"
#include "openroad/Error.hh"
#include "openroad/OpenRoad.hh"

////////////////////////////////////////////////////////////////
//
// C++ helper functions used by the interface functions.
// These are not visible in the TCL API.
//
////////////////////////////////////////////////////////////////

namespace ord {

using sta::dbSta;
using sta::dbNetwork;
using sta::Resizer;

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

// Copied from StaTcl.i because of ordering issues.
class CmdErrorNetworkNotLinked : public sta::Exception
{
public:
  virtual const char *what() const throw()
  { return "Error: no network has been linked."; }
};

void
ensureLinked()
{
  OpenRoad *openroad = getOpenRoad();
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
  return openroad->getResizer();
}

TritonCTS::TritonCTSKernel *
getTritonCts()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getTritonCts();
}

MacroPlace::TritonMacroPlace *
getTritonMp()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getTritonMp();
}

replace::Replace*
getReplace()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getReplace();
}

OpenRCX::Ext *
getOpenRCX()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getOpenRCX();
}

pdnsim::PDNSim*
getPDNSim()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getPDNSim();
}

} // namespace

using ord::OpenRoad;
using ord::getOpenRoad;
using ord::getDb;
using ord::ensureLinked;

using odb::dbDatabase;
using odb::dbBlock;
using odb::dbTechLayer;
using odb::dbTrackGrid;
using odb::dbTech;
%}

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%include "Exception.i"

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
read_def_cmd(const char *filename, bool order_wires, bool continue_on_errors)
{
  OpenRoad *ord = getOpenRoad();
  ord->readDef(filename, order_wires, continue_on_errors);
}

void
write_def_cmd(const char *filename,
	      const char *version)
{
  OpenRoad *ord = getOpenRoad();
  ord->writeDef(filename, version);
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
read_verilog_cmd(const char *filename)
{
  OpenRoad *ord = getOpenRoad();
  ord->readVerilog(filename);
}

void
link_design_db_cmd(const char *design_name)
{
  OpenRoad *ord = getOpenRoad();
  ord->linkDesign(design_name);
}

void
ensure_linked()
{
  return ensureLinked();
}

void
write_verilog_cmd(const char *filename,
		  bool sort)
{
  OpenRoad *ord = getOpenRoad();
  ord->writeVerilog(filename, sort);
}

////////////////////////////////////////////////////////////////

odb::dbDatabase *
get_db()
{
  return getDb();
}

odb::dbTech *
get_db_tech()
{
  return getDb()->getTech();
}

bool
db_has_tech()
{
  return getDb()->getTech() != nullptr;
}

odb::dbBlock *
get_db_block()
{
  odb::dbDatabase *db = getDb();
  if (db) {
    odb::dbChip *chip = db->getChip();
    if (chip)
      return chip->getBlock();
  }
  return nullptr;
}

odb::Rect
get_db_core()
{
  OpenRoad *ord = getOpenRoad();
  return ord->getCore();
}

double
dbu_to_microns(int dbu)
{
  return static_cast<double>(dbu) / getDb()->getTech()->getLefUnits();
}

int
microns_to_dbu(double microns)
{
  return microns * getDb()->getTech()->getLefUnits();
}

// Common check for placement tools.
bool
db_has_rows()
{
  dbDatabase *db = OpenRoad::openRoad()->getDb();
  return db->getChip()
    && db->getChip()->getBlock()
    && db->getChip()->getBlock()->getRows().size() > 0;
}

bool
db_layer_has_tracks(unsigned layerId, bool hor)
{
  dbDatabase *db = OpenRoad::openRoad()->getDb();
  dbBlock *block = db->getChip()->getBlock();
  dbTech *tech = db->getTech();
  
  dbTechLayer *layer = tech->findRoutingLayer(layerId);
  if (!layer) {
    return false;
  }
    
  dbTrackGrid *trackGrid = block->findTrackGrid(layer);
  if (!trackGrid) {
    return false;
  }

  if (hor) {
    return trackGrid->getNumGridPatternsY() > 0; 
  } else {
    return trackGrid->getNumGridPatternsX() > 0; 
  }
}

bool
db_layer_has_hor_tracks(unsigned layerId)
{
  return db_layer_has_tracks(layerId, true);
}

bool
db_layer_has_ver_tracks(unsigned layerId)
{
  return db_layer_has_tracks(layerId, false);
}

sta::Sta *
get_sta()
{
  return sta::Sta::sta();
}

// For some bizzare reason this fails without the namespace qualifier for Sta.
void
set_cmd_sta(sta::Sta *sta)
{
  sta::Sta::setSta(sta);
}

// Used by test/error1.tcl
void
test_error1()
{
  ord::error("this is only a test.");
}

bool
units_initialized()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->unitsInitialized();
}

%} // inline
