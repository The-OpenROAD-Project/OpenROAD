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
#include "db_sta/dbReadVerilog.hh"
#include "ord/Version.hh"
#include "utl/Logger.h"
#include "ord/OpenRoad.hh"

////////////////////////////////////////////////////////////////
//
// C++ helper functions used by the interface functions.
// These are not visible in the TCL API.
//
////////////////////////////////////////////////////////////////

namespace ord {

using sta::dbSta;
using sta::dbNetwork;
using rsz::Resizer;

using odb::dbDatabase;

OpenRoad *
getOpenRoad()
{
  return OpenRoad::openRoad();
}

utl::Logger *
getLogger()
{
  return OpenRoad::openRoad()->getLogger();
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

rmp::Restructure *
getRestructure()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getRestructure();
}

cts::TritonCTS *
getTritonCts()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getTritonCts();
}

mpl::MacroPlacer *
getMacroPlacer()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getMacroPlacer();
}

mpl::MacroPlacer2 *
getMacroPlacer2()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getMacroPlacer2();
}

gpl::Replace*
getReplace()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getReplace();
}

rcx::Ext *
getOpenRCX()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getOpenRCX();
}

triton_route::TritonRoute *
getTritonRoute()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getTritonRoute();
}

psm::PDNSim*
getPDNSim()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getPDNSim();
}

grt::GlobalRouter*
getGlobalRouter()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getGlobalRouter();
}

ppl::IOPlacer*
getIOPlacer()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getIOPlacer();
}

par::PartitionMgr*
getPartitionMgr()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getPartitionMgr();
}

pdn::PdnGen*
getPdnGen()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getPdnGen();
}

stt::SteinerTreeBuilder*
getSteinerTreeBuilder()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->getSteinerTreeBuilder();
}

} // namespace ord

namespace sta {
std::vector<LibertyCell*> *
tclListSeqLibertyCell(Tcl_Obj *const source,
		      Tcl_Interp *interp);
} // namespace sta


using std::vector;

using sta::LibertyCell;

using ord::OpenRoad;
using ord::getOpenRoad;
using ord::getLogger;
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
// C++ functions visible as SWIG functions.
//
////////////////////////////////////////////////////////////////

#ifdef SWIGTCL
%include "Exception.i"

%typemap(in) vector<LibertyCell*> * {
  $1 = sta::tclListSeqLibertyCell($input, interp);
}

%typemap(in) utl::ToolId {
  int length;
  const char *arg = Tcl_GetStringFromObj($input, &length);
  $1 = utl::Logger::findToolId(arg);
}
#endif

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
read_def_cmd(const char *filename,
             bool continue_on_errors,
             bool floorplan_init,
             bool incremental)
{
  OpenRoad *ord = getOpenRoad();
  ord->readDef(filename, continue_on_errors, floorplan_init, incremental);
}

void
write_def_cmd(const char *filename,
	      const char *version)
{
  OpenRoad *ord = getOpenRoad();
  ord->writeDef(filename, version);
}


void 
write_cdl_cmd(const char *filename,
              bool includeFillers)
{
  OpenRoad *ord = getOpenRoad();
  ord->writeCdl(filename, includeFillers);
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
		  bool sort,
		  bool include_pwr_gnd,
		  vector<LibertyCell*> *remove_cells)
{
  OpenRoad *ord = getOpenRoad();
  ord->writeVerilog(filename, sort, include_pwr_gnd, remove_cells);
}

void
set_debug_level(const char* tool_name,
                const char* group,
                int level)
{
  using namespace ord;
  auto logger = getLogger();

  auto id = utl::Logger::findToolId(tool_name);
  if (id == utl::UKN) {
    logger->error(utl::ORD, 15, "Unknown tool name {}", tool_name);
  }
  logger->setDebugLevel(id, group, level);
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

bool
units_initialized()
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->unitsInitialized();
}

#ifdef ENABLE_PYTHON3
void
python_cmd(const char* py_command)
{
  OpenRoad *openroad = getOpenRoad();
  return openroad->pythonCommand(py_command);
}
#endif

namespace ord {

void
set_thread_count(int threads)
{
  OpenRoad *ord = getOpenRoad();
  ord->setThreadCount(threads);
}

void
set_thread_count(const char* threads)
{
  OpenRoad *ord = getOpenRoad();
  ord->setThreadCount(threads);
}

int
thread_count()
{
  OpenRoad *ord = getOpenRoad();
  return ord->getThreadCount();
}

void
delete_all_memory()
{
  ord::deleteAllMemory();
}

}

%} // inline
