/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "ord/OpenRoad.hh"

#include <iostream>
#include <thread>
#ifdef ENABLE_PYTHON3
#define PY_SSIZE_T_CLEAN
#include "Python.h"
#endif

#include "ant/MakeAntennaChecker.hh"
#include "cts/MakeTritoncts.h"
#include "db_sta/MakeDbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "dpl/MakeOpendp.h"
#include "dpo/MakeOptdp.h"
#include "dst/MakeDistributed.h"
#include "fin/MakeFinale.h"
#include "gpl/MakeReplace.h"
#include "grt/MakeGlobalRouter.h"
#include "gui/MakeGui.h"
#include "ifp//MakeInitFloorplan.hh"
#include "mpl/MakeMacroPlacer.h"
#ifdef ENABLE_MPL2
// mpl2 aborts with link error on darwin
#include "mpl2/MakeMacroPlacer.h"
#endif
#include "dft/MakeDft.hh"
#include "odb/cdl.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/lefin.h"
#include "odb/lefout.h"
#include "ord/InitOpenRoad.hh"
#include "pad/MakePad.h"
#include "par/MakePartitionMgr.h"
#include "pdn/MakePdnGen.hh"
#include "ppl/MakeIoplacer.h"
#include "psm/MakePDNSim.hh"
#include "rcx/MakeOpenRCX.h"
#include "rmp/MakeRestructure.h"
#include "rsz/MakeResizer.hh"
#include "sta/StaMain.hh"
#include "sta/VerilogWriter.hh"
#include "stt/MakeSteinerTreeBuilder.h"
#include "tap/MakeTapcell.h"
#include "triton_route/MakeTritonRoute.h"
#include "utl/Logger.h"
#include "utl/MakeLogger.h"

namespace sta {
extern const char* openroad_swig_tcl_inits[];
extern const char* upf_tcl_inits[];
}  // namespace sta

// Swig uses C linkage for init functions.
extern "C" {
extern int Openroad_swig_Init(Tcl_Interp* interp);
extern int Odbtcl_Init(Tcl_Interp* interp);
extern int Upf_Init(Tcl_Interp* interp);
}

// Main.cc set by main()
extern const char* log_filename;
extern const char* metrics_filename;

namespace ord {

using std::max;
using std::min;

using odb::dbBlock;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbLib;
using odb::dbTech;
using odb::Point;
using odb::Rect;

using sta::dbSta;
using sta::evalTclInit;
using sta::Resizer;

using utl::ORD;

OpenRoad::OpenRoad()
    : tcl_interp_(nullptr),
      logger_(nullptr),
      db_(nullptr),
      verilog_network_(nullptr),
      sta_(nullptr),
      resizer_(nullptr),
      ioPlacer_(nullptr),
      opendp_(nullptr),
      optdp_(nullptr),
      finale_(nullptr),
      macro_placer_(nullptr),
      macro_placer2_(nullptr),
      global_router_(nullptr),
      restructure_(nullptr),
      tritonCts_(nullptr),
      tapcell_(nullptr),
      extractor_(nullptr),
      detailed_router_(nullptr),
      antenna_checker_(nullptr),
      replace_(nullptr),
      pdnsim_(nullptr),
      partitionMgr_(nullptr),
      pdngen_(nullptr),
      distributer_(nullptr),
      stt_builder_(nullptr),
      dft_(nullptr),
      threads_(1)
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
  deleteOptdp(optdp_);
  deleteGlobalRouter(global_router_);
  deleteRestructure(restructure_);
  deleteTritonCts(tritonCts_);
  deleteTapcell(tapcell_);
  deleteMacroPlacer(macro_placer_);
#ifdef ENABLE_MPL2
  deleteMacroPlacer2(macro_placer2_);
#endif
  deleteOpenRCX(extractor_);
  deleteTritonRoute(detailed_router_);
  deleteReplace(replace_);
  deleteFinale(finale_);
  deleteAntennaChecker(antenna_checker_);
  odb::dbDatabase::destroy(db_);
  deletePartitionMgr(partitionMgr_);
  deletePdnGen(pdngen_);
  deleteDistributed(distributer_);
  deleteSteinerTreeBuilder(stt_builder_);
  dft::deleteDft(dft_);
  delete logger_;
}

sta::dbNetwork* OpenRoad::getDbNetwork()
{
  return sta_->getDbNetwork();
}

/* static */
OpenRoad* OpenRoad::openRoad()
{
  // This will be destroyed at application exit
  static OpenRoad o;
  return &o;
}

////////////////////////////////////////////////////////////////

void initOpenRoad(Tcl_Interp* interp)
{
  OpenRoad::openRoad()->init(interp);
}

void OpenRoad::init(Tcl_Interp* tcl_interp)
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
  optdp_ = makeOptdp();
  finale_ = makeFinale();
  global_router_ = makeGlobalRouter();
  restructure_ = makeRestructure();
  tritonCts_ = makeTritonCts();
  tapcell_ = makeTapcell();
  macro_placer_ = makeMacroPlacer();
#ifdef ENABLE_MPL2
  macro_placer2_ = makeMacroPlacer2();
#endif
  extractor_ = makeOpenRCX();
  detailed_router_ = makeTritonRoute();
  replace_ = makeReplace();
  pdnsim_ = makePDNSim();
  antenna_checker_ = makeAntennaChecker();
  partitionMgr_ = makePartitionMgr();
  pdngen_ = makePdnGen();
  distributer_ = makeDistributed();
  stt_builder_ = makeSteinerTreeBuilder();
  dft_ = dft::makeDft();

  // Init components.
  Openroad_swig_Init(tcl_interp);
  // Import TCL scripts.
  evalTclInit(tcl_interp, sta::openroad_swig_tcl_inits);

  initLogger(logger_, tcl_interp);
  initGui(this);  // first so we can register our sink with the logger
  Odbtcl_Init(tcl_interp);
  Upf_Init(tcl_interp);
  evalTclInit(tcl_interp, sta::upf_tcl_inits);
  initInitFloorplan(this);
  initDbSta(this);
  initResizer(this);
  initDbVerilogNetwork(this);
  initIoplacer(this);
  initReplace(this);
  initOpendp(this);
  initOptdp(this);
  initFinale(this);
  initGlobalRouter(this);
  initTritonCts(this);
  initTapcell(this);
  initMacroPlacer(this);
#ifdef ENABLE_MPL2
  initMacroPlacer2(this);
#endif
  initOpenRCX(this);
  initPad(this);
  initRestructure(this);
  initTritonRoute(this);
  initPDNSim(this);
  initAntennaChecker(this);
  initPartitionMgr(this);
  initPdnGen(this);
  initDistributed(this);
  initSteinerTreeBuilder(this);
  dft::initDft(this);

  // Import exported commands to global namespace.
  Tcl_Eval(tcl_interp, "sta::define_sta_cmds");
  Tcl_Eval(tcl_interp, "namespace import sta::*");

  // Initialize tcl history
  if (Tcl_Eval(tcl_interp, "history") == TCL_ERROR) {
    // There appears to be a typo in the history.tcl file in some
    // distributions, which is generating this error.
    // remove error from tcl result.
    Tcl_ResetResult(tcl_interp);
  }
}

////////////////////////////////////////////////////////////////

void OpenRoad::readLef(const char* filename,
                       const char* lib_name,
                       bool make_tech,
                       bool make_library)
{
  odb::lefin lef_reader(db_, logger_, false);
  dbLib* lib = nullptr;
  dbTech* tech = nullptr;
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

void OpenRoad::readDef(const char* filename,
                       bool continue_on_errors,
                       bool floorplan_init,
                       bool incremental)
{
  if (!floorplan_init && !incremental && db_->getChip()
      && db_->getChip()->getBlock()) {
    logger_->error(
        ORD,
        48,
        "You can't load a new DEF file as the db is already populated.");
  }

  odb::defin::MODE mode = odb::defin::DEFAULT;
  if (floorplan_init)
    mode = odb::defin::FLOORPLAN;
  else if (incremental)
    mode = odb::defin::INCREMENTAL;
  odb::defin def_reader(db_, logger_, mode);
  std::vector<odb::dbLib*> search_libs;
  for (odb::dbLib* lib : db_->getLibs())
    search_libs.push_back(lib);
  if (continue_on_errors) {
    def_reader.continueOnErrors();
  }
  dbChip* chip = def_reader.createChip(search_libs, filename);
  if (chip) {
    dbBlock* block = chip->getBlock();
    for (Observer* observer : observers_) {
      observer->postReadDef(block);
    }
  }
}

static odb::defout::Version stringToDefVersion(string version)
{
  if (version == "5.8")
    return odb::defout::Version::DEF_5_8;
  else if (version == "5.7")
    return odb::defout::Version::DEF_5_7;
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

void OpenRoad::writeDef(const char* filename, string version)
{
  odb::dbChip* chip = db_->getChip();
  if (chip) {
    odb::dbBlock* block = chip->getBlock();
    if (block) {
      odb::defout def_writer(logger_);
      def_writer.setVersion(stringToDefVersion(version));
      def_writer.writeBlock(block, filename);
    }
  }
}

void OpenRoad::writeLef(const char* filename)
{
  auto libs = db_->getLibs();
  int num_libs = libs.size();
  odb::lefout lef_writer(logger_);
  if (num_libs > 0) {
    if (num_libs > 1) {
      logger_->info(
          ORD, 34, "More than one lib exists, multiple files will be written.");
    }

    int cnt = 0;
    for (auto lib : libs) {
      std::string name(filename);
      if (cnt > 0) {
        name += "_" + std::to_string(cnt);
        lef_writer.writeLib(lib, name.c_str());
      } else {
        lef_writer.writeTechAndLib(lib, name.c_str());
      }
      ++cnt;
    }
  } else if (db_->getTech()) {
    lef_writer.writeTech(db_->getTech(), filename);
  }
}

void OpenRoad::writeCdl(const char* outFilename,
                        const std::vector<const char*>& mastersFilenames,
                        bool includeFillers)
{
  odb::dbChip* chip = db_->getChip();
  if (chip) {
    odb::dbBlock* block = chip->getBlock();
    if (block) {
      odb::cdl::writeCdl(
          getLogger(), block, outFilename, mastersFilenames, includeFillers);
    }
  }
}

void OpenRoad::readDb(const char* filename)
{
  if (db_->getChip() && db_->getChip()->getBlock()) {
    logger_->error(
        ORD, 47, "You can't load a new db file as the db is already populated");
  }

  FILE* stream = fopen(filename, "r");
  if (stream == nullptr) {
    return;
  }

  db_->read(stream);
  fclose(stream);

  for (Observer* observer : observers_) {
    observer->postReadDb(db_);
  }
}

void OpenRoad::writeDb(const char* filename)
{
  FILE* stream = fopen(filename, "w");
  if (stream) {
    db_->write(stream);
    fclose(stream);
  }
}

void OpenRoad::diffDbs(const char* filename1,
                       const char* filename2,
                       const char* diffs)
{
  FILE* stream1 = fopen(filename1, "r");
  if (stream1 == nullptr) {
    logger_->error(ORD, 103, "Can't open {}", filename1);
  }

  FILE* stream2 = fopen(filename2, "r");
  if (stream2 == nullptr) {
    logger_->error(ORD, 104, "Can't open {}", filename1);
  }

  FILE* out = fopen(diffs, "w");
  if (out == nullptr) {
    logger_->error(ORD, 105, "Can't open {}", diffs);
  }

  auto db1 = odb::dbDatabase::create();
  auto db2 = odb::dbDatabase::create();

  db1->read(stream1);
  db2->read(stream2);

  odb::dbDatabase::diff(db1, db2, out, 2);

  fclose(stream1);
  fclose(stream2);
  fclose(out);
}

void OpenRoad::readVerilog(const char* filename)
{
  verilog_network_->deleteTopInstance();
  dbReadVerilog(filename, verilog_network_);
}

void OpenRoad::linkDesign(const char* design_name)

{
  dbLinkDesign(design_name, verilog_network_, db_, logger_);
  for (Observer* observer : observers_) {
    observer->postReadDb(db_);
  }
}

void OpenRoad::designCreated()
{
  for (Observer* observer : observers_) {
    observer->postReadDb(db_);
  }
}

bool OpenRoad::unitsInitialized()
{
  // Units are set by the first liberty library read.
  return getDbNetwork()->defaultLibertyLibrary() != nullptr;
}

odb::Rect OpenRoad::getCore()
{
  return db_->getChip()->getBlock()->getCoreArea();
}

void OpenRoad::addObserver(Observer* observer)
{
  assert(observer->owner_ == nullptr);
  observer->owner_ = this;
  observers_.insert(observer);
}

void OpenRoad::removeObserver(Observer* observer)
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
void OpenRoad::setThreadCount(int threads, bool printInfo)
{
  int max_threads = std::thread::hardware_concurrency();
  if (max_threads == 0) {
    logger_->warn(ORD,
                  31,
                  "Unable to determine maximum number of threads.\n"
                  "One thread will be used.");
    max_threads = 1;
  }
  if (threads <= 0) {  // max requested
    threads = max_threads;
  } else if (threads > max_threads) {
    threads = max_threads;
  }
  threads_ = threads;

  if (printInfo)
    logger_->info(ORD, 30, "Using {} thread(s).", threads_);

  // place limits on tools with threads
  sta_->setThreadCount(threads_);
}

void OpenRoad::setThreadCount(const char* threads, bool printInfo)
{
  int max_threads = threads_;  // default, make no changes

  if (strcmp(threads, "max") == 0) {
    max_threads = -1;  // -1 is max cores
  } else {
    try {
      max_threads = std::stoi(threads);
    } catch (const std::invalid_argument&) {
      logger_->warn(
          ORD, 32, "Invalid thread number specification: {}.", threads);
    }
  }

  setThreadCount(max_threads, printInfo);
}

int OpenRoad::getThreadCount()
{
  return threads_;
}

}  // namespace ord
