/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
// All rights reserved.
//
// BSD 3-Clause License
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

#include "openroad/OpenRoad.hh"

#include <iostream>

#include "utility/MakeLogger.h"
#include "utility/Logger.h"

#include "opendb/db.h"
#include "opendb/wOrder.h"
#include "opendb/lefin.h"
#include "opendb/defin.h"
#include "opendb/defout.h"
#include "opendb/cdl.h"

#include "sta/VerilogWriter.hh"
#include "sta/StaMain.hh"

#include "db_sta/dbSta.hh"
#include "db_sta/MakeDbSta.hh"

#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbNetwork.hh"

#include "openroad/InitOpenRoad.hh"
#include "flute3/flute.h"

#include "ifp//MakeInitFloorplan.hh"
#include "ioplacer/MakeIoplacer.h"
#include "rsz/MakeResizer.hh"
#include "gui/MakeGui.h"
#include "dpl/MakeOpendp.h"
#include "finale/MakeFinale.h"
#include "mpl/MakeMacroPlacer.h"
#include "replace/MakeReplace.h"
#include "fastroute/MakeFastRoute.h"
#include "tritoncts/MakeTritoncts.h"
#include "tapcell/MakeTapcell.h"
#include "OpenRCX/MakeOpenRCX.h"
#include "triton_route/MakeTritonRoute.h"
#include "psm/MakePDNSim.hh"
#include "antennachecker/MakeAntennaChecker.hh"
#include "PartitionMgr/src/MakePartitionMgr.h"

namespace sta {
extern const char *openroad_swig_tcl_inits[];
}

// Swig uses C linkage for init functions.
extern "C" {
extern int Openroad_Init(Tcl_Interp *interp);
extern int Opendbtcl_Init(Tcl_Interp *interp);
}

// Main.cc set by main()
extern const char* log_filename;
extern const char* metrics_filename;

namespace ord {

using std::min;
using std::max;

using odb::dbLib;
using odb::dbTech;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbBlock;
using odb::Rect;
using odb::Point;

using sta::evalTclInit;
using sta::dbSta;
using sta::Resizer;

OpenRoad::OpenRoad()
  : tcl_interp_(nullptr),
    logger_(nullptr),
    db_(nullptr),
    verilog_network_(nullptr),
    sta_(nullptr),
    resizer_(nullptr),
    ioPlacer_(nullptr),
    opendp_(nullptr),
    finale_(nullptr),
    macro_placer_(nullptr),
    fastRoute_(nullptr),
    tritonCts_(nullptr),
    tapcell_(nullptr),
    extractor_(nullptr),
    detailed_router_(nullptr),
    antenna_checker_(nullptr),
    replace_(nullptr),
    pdnsim_(nullptr), 
    partitionMgr_(nullptr) 
{
  db_ = dbDatabase::create();
}

OpenRoad::~OpenRoad()
{
  deleteDbVerilogNetwork(verilog_network_);
  deleteDbSta(sta_);
  deleteIoplacer(ioPlacer_);
  deleteResizer(resizer_);
  deleteOpendp(opendp_);
  deleteFastRoute(fastRoute_);
  deleteTritonCts(tritonCts_);
  deleteTapcell(tapcell_);
  deleteMacroPlacer(macro_placer_);
  deleteOpenRCX(extractor_);
  deleteTritonRoute(detailed_router_);
  deleteReplace(replace_);
  deleteFinale(finale_);
  deleteAntennaChecker(antenna_checker_);
  odb::dbDatabase::destroy(db_);
  deletePartitionMgr(partitionMgr_);
  stt::deleteLUT();
  delete logger_;
}

void
deleteAllMemory()
{
  delete OpenRoad::openRoad();
  sta::Sta::setSta(nullptr);
  sta::deleteAllMemory();
}

sta::dbNetwork *
OpenRoad::getDbNetwork()
{
  return sta_->getDbNetwork();
}

/* static */
OpenRoad *OpenRoad::openRoad()
{
  // This will be destroyed at application exit
  static OpenRoad o;
  return &o;
}

////////////////////////////////////////////////////////////////

void
initOpenRoad(Tcl_Interp *interp)
{
  OpenRoad::openRoad()->init(interp);
}

void
OpenRoad::init(Tcl_Interp *tcl_interp)
{
  tcl_interp_ = tcl_interp;

  // Make components.
  logger_ = makeLogger(log_filename, metrics_filename);
  db_->setLogger(logger_);
  sta_ = makeDbSta();
  verilog_network_ = makeDbVerilogNetwork();
  ioPlacer_ = makeIoplacer();
  resizer_ = makeResizer();
  opendp_ = makeOpendp();
  finale_ = makeFinale();
  fastRoute_ = makeFastRoute();
  tritonCts_ = makeTritonCts();
  tapcell_ = makeTapcell();
  macro_placer_ = makeMacroPlacer();
  extractor_ = makeOpenRCX();
  detailed_router_ = makeTritonRoute();
  replace_ = makeReplace();
  pdnsim_ = makePDNSim();
  antenna_checker_ = makeAntennaChecker();
  partitionMgr_ = makePartitionMgr();

  // Init components.
  Openroad_Init(tcl_interp);
  // Import TCL scripts.
  evalTclInit(tcl_interp, sta::openroad_swig_tcl_inits);

  initLogger(logger_, tcl_interp);
  initGui(this); // first so we can register our sink with the logger
  Opendbtcl_Init(tcl_interp);
  initInitFloorplan(this);
  stt::readLUT();
  initDbSta(this);
  initResizer(this);
  initDbVerilogNetwork(this);
  initIoplacer(this);
  initReplace(this);
  initOpendp(this);
  initFinale(this);
  initFastRoute(this);
  initTritonCts(this);
  initTapcell(this);
  initMacroPlacer(this);
  initOpenRCX(this);
  initTritonRoute(this);
  initPDNSim(this);
  initAntennaChecker(this);
  initPartitionMgr(this);

  // Import exported commands to global namespace.
  Tcl_Eval(tcl_interp, "sta::define_sta_cmds");
  Tcl_Eval(tcl_interp, "namespace import sta::*");
}

////////////////////////////////////////////////////////////////

void
OpenRoad::readLef(const char *filename,
		  const char *lib_name,
		  bool make_tech,
		  bool make_library)
{
  odb::lefin lef_reader(db_, logger_, false);
  dbLib *lib = nullptr;
  dbTech *tech = nullptr;
  if (make_tech && make_library) {
    lib = lef_reader.createTechAndLib(lib_name, filename);
    tech = db_->getTech();
  } else if (make_tech) {
    tech = lef_reader.createTech(filename);
  } else if (make_library) {
    lib = lef_reader.createLib(lib_name, filename);
  }

  // both are null on parser failure
  if (lib != nullptr || tech != nullptr) {
    for (Observer* observer : observers_) {
      observer->postReadLef(tech, lib);
    }
  }
}

void
OpenRoad::readDef(const char *filename,
		  bool order_wires,
		  bool continue_on_errors,
      bool floorplan_init,
      bool incremental)
{
  odb::defin::MODE mode = odb::defin::DEFAULT;
  if(floorplan_init)
    mode = odb::defin::FLOORPLAN;
  else if(incremental)
    mode = odb::defin::INCREMENTAL;
  odb::defin def_reader(db_, logger_, mode);
  std::vector<odb::dbLib *> search_libs;
  for (odb::dbLib *lib : db_->getLibs())
    search_libs.push_back(lib);
  if (continue_on_errors) {
    def_reader.continueOnErrors();
  }
  dbChip* chip = def_reader.createChip(search_libs, filename);
  if (chip) {
    dbBlock* block = chip->getBlock();
    if (order_wires) {
      odb::orderWires(block,
                      nullptr /* net_name_or_id*/,
                      false /* force */,
                      false /* verbose */,
                      true /* quiet */);
    }

    for (Observer* observer : observers_) {
      observer->postReadDef(block);
    }
  }
}

static odb::defout::Version
stringToDefVersion(string version)
{
  if (version == "5.8")
    return odb::defout::Version::DEF_5_8;
  else if (version == "5.6")
    return odb::defout::Version::DEF_5_6;
  else if (version == "5.5")
    return odb::defout::Version::DEF_5_5;
  else if (version == "5.4")
    return odb::defout::Version::DEF_5_4;
  else if (version == "5.3")
    return odb::defout::Version::DEF_5_3;
  else 
    return odb::defout::Version::DEF_5_8;
}

void
OpenRoad::writeDef(const char *filename,
		   string version)
{
  odb::dbChip *chip = db_->getChip();
  if (chip) {
    odb::dbBlock *block = chip->getBlock();
    if (block) {
      odb::defout def_writer(logger_);
      def_writer.setVersion(stringToDefVersion(version));
      def_writer.writeBlock(block, filename);
    }
  }
}

void 
OpenRoad::writeCdl(const char* filename, bool includeFillers)
{
  odb::dbChip *chip = db_->getChip();
  if (chip) {
    odb::dbBlock *block = chip->getBlock();
    if (block) {
      odb::cdl::writeCdl(block, filename, includeFillers);
    }
  }
  
}

void
OpenRoad::readDb(const char *filename)
{
  FILE *stream = fopen(filename, "r");
  if (stream == nullptr) {
    return;
  }

  db_->read(stream);
  fclose(stream);

  for (Observer* observer : observers_) {
    observer->postReadDb(db_);
  }
}

void
OpenRoad::writeDb(const char *filename)
{
  FILE *stream = fopen(filename, "w");
  if (stream) {
    db_->write(stream);
    fclose(stream);
  }
}

void
OpenRoad::readVerilog(const char *filename)
{
  dbReadVerilog(filename, verilog_network_);
}

void
OpenRoad::linkDesign(const char *design_name)

{
  dbLinkDesign(design_name, verilog_network_, db_, logger_);
  for (Observer* observer : observers_) {
    observer->postReadDb(db_);
  }
}

void
OpenRoad::writeVerilog(const char *filename,
		       bool sort,
		       bool include_pwr_gnd,
		       std::vector<sta::LibertyCell*> *remove_cells)
{
  sta::writeVerilog(filename, sort, include_pwr_gnd,
		    remove_cells, sta_->network());
}

bool
OpenRoad::unitsInitialized()
{
  // Units are set by the first liberty library read.
  return getDbNetwork()->defaultLibertyLibrary() != nullptr;
}

odb::Rect
OpenRoad::getCore()
{
  return ord::getCore(db_->getChip()->getBlock());
}

void OpenRoad::addObserver(Observer *observer)
{
  assert(observer->owner_ == nullptr);
  observer->owner_ = this;
  observers_.insert(observer);
}

void OpenRoad::removeObserver(Observer *observer)
{
  observer->owner_ = nullptr;
  observers_.erase(observer);
}

OpenRoad::Observer::~Observer()
{
  if (owner_) {
    owner_->removeObserver(this);
  }
}


////////////////////////////////////////////////////////////////

// Need a header for these functions cherry uses in
// InitFloorplan, Resizer, OpenDP.

Rect
getCore(dbBlock *block)
{
  odb::Rect core;
  block->getCoreArea(core);
  return core;
}

// Return the point inside rect that is closest to pt.
Point
closestPtInRect(Rect rect,
		Point pt)
{
  return Point(min(max(pt.getX(), rect.xMin()), rect.xMax()),
               min(max(pt.getY(), rect.yMin()), rect.yMax()));
}

Point
closestPtInRect(Rect rect,
		int x,
		int y)
{
  return Point(min(max(x, rect.xMin()), rect.xMax()),
               min(max(y, rect.yMin()), rect.yMax()));
}

} // namespace
