// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ord/OpenRoad.hh"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "ord/Version.hh"
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
#include "dft/MakeDft.hh"
#include "dpl/MakeOpendp.h"
#include "dpo/MakeOptdp.h"
#include "dst/MakeDistributed.h"
#include "fin/MakeFinale.h"
#include "gpl/MakeReplace.h"
#include "grt/MakeGlobalRouter.h"
#include "gui/MakeGui.h"
#include "ifp/MakeInitFloorplan.hh"
#include "mpl/MakeMacroPlacer.h"
#include "odb/MakeOdb.h"
#include "odb/cdl.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/lefin.h"
#include "odb/lefout.h"
#include "ord/InitOpenRoad.hh"
#include "pad/MakeICeWall.h"
#include "par/MakePartitionMgr.h"
#include "pdn/MakePdnGen.hh"
#include "ppl/MakeIoplacer.h"
#include "psm/MakePDNSim.hh"
#include "rcx/MakeOpenRCX.h"
#include "rmp/MakeRestructure.h"
#include "rsz/MakeResizer.hh"
#include "sta/VerilogReader.hh"
#include "sta/VerilogWriter.hh"
#include "stt/MakeSteinerTreeBuilder.h"
#include "tap/MakeTapcell.h"
#include "triton_route/MakeTritonRoute.h"
#include "upf/MakeUpf.h"
#include "utl/Logger.h"
#include "utl/MakeLogger.h"
#include "utl/ScopedTemporaryFile.h"
#include "utl/decode.h"

namespace ord {
extern const char* ord_tcl_inits[];
}  // namespace ord

// Swig uses C linkage for init functions.
extern "C" {
extern int Ord_Init(Tcl_Interp* interp);
}

namespace ord {

using odb::dbBlock;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbLib;
using odb::dbTech;

using utl::ORD;

OpenRoad* OpenRoad::app_ = nullptr;

OpenRoad::OpenRoad()
{
  db_ = dbDatabase::create();
}

OpenRoad::~OpenRoad()
{
  deleteDbVerilogNetwork(verilog_network_);
  // Temporarily removed until a crash can be resolved
  // deleteDbSta(sta_);
  // sta::deleteAllMemory();
  deleteIoplacer(ioPlacer_);
  deleteResizer(resizer_);
  deleteOpendp(opendp_);
  deleteOptdp(optdp_);
  deleteGlobalRouter(global_router_);
  deleteRestructure(restructure_);
  deleteTritonCts(tritonCts_);
  deleteTapcell(tapcell_);
  deleteMacroPlacer(macro_placer_);
  deleteOpenRCX(extractor_);
  deleteTritonRoute(detailed_router_);
  deleteReplace(replace_);
  deletePDNSim(pdnsim_);
  deleteFinale(finale_);
  deleteAntennaChecker(antenna_checker_);
  odb::dbDatabase::destroy(db_);
  deletePartitionMgr(partitionMgr_);
  deletePdnGen(pdngen_);
  deleteICeWall(icewall_);
  deleteDistributed(distributer_);
  deleteSteinerTreeBuilder(stt_builder_);
  dft::deleteDft(dft_);
  delete logger_;
  delete verilog_reader_;
}

sta::dbNetwork* OpenRoad::getDbNetwork()
{
  return sta_->getDbNetwork();
}

/* static */
OpenRoad* OpenRoad::openRoad()
{
  return app_;
}

/* static */
void OpenRoad::setOpenRoad(OpenRoad* app, bool reinit_ok)
{
  if (!reinit_ok && app_) {
    std::cerr << "Attempt to reinitialize the application." << std::endl;
    exit(1);
  }
  app_ = app;
}

////////////////////////////////////////////////////////////////

void initOpenRoad(Tcl_Interp* interp,
                  const char* log_filename,
                  const char* metrics_filename)
{
  OpenRoad::openRoad()->init(interp, log_filename, metrics_filename);
}

void OpenRoad::init(Tcl_Interp* tcl_interp,
                    const char* log_filename,
                    const char* metrics_filename)
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
  extractor_ = makeOpenRCX();
  detailed_router_ = makeTritonRoute();
  replace_ = makeReplace();
  pdnsim_ = makePDNSim();
  antenna_checker_ = makeAntennaChecker();
  partitionMgr_ = makePartitionMgr();
  pdngen_ = makePdnGen();
  icewall_ = makeICeWall();
  distributer_ = makeDistributed();
  stt_builder_ = makeSteinerTreeBuilder();
  dft_ = dft::makeDft();

  // Init components.
  Ord_Init(tcl_interp);
  // Import TCL scripts.
  utl::evalTclInit(tcl_interp, ord::ord_tcl_inits);

  initLogger(logger_, tcl_interp);
  // GUI first so we can register our sink with the logger
  initGui(tcl_interp, db_, sta_, logger_);
  initOdb(tcl_interp);
  initUpf(this);
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
  initOpenRCX(this);
  initICeWall(this);
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
                       const char* tech_name,
                       bool make_tech,
                       bool make_library)
{
  odb::lefin lef_reader(db_, logger_, false);
  if (make_tech && make_library) {
    lef_reader.createTechAndLib(tech_name, lib_name, filename);
  } else if (make_tech) {
    lef_reader.createTech(tech_name, filename);
  } else if (make_library) {
    dbTech* tech;
    if (tech_name[0] != '\0') {
      tech = db_->findTech(tech_name);
    } else {
      tech = db_->getTech();
    }
    if (!tech) {
      logger_->error(ORD, 51, "Technology {} not found", tech_name);
    }
    lef_reader.createLib(tech, lib_name, filename);
  }
}

void OpenRoad::readDef(const char* filename,
                       dbTech* tech,
                       bool continue_on_errors,
                       bool floorplan_init,
                       bool incremental,
                       bool child)
{
  if (!floorplan_init && !incremental && !child && db_->getChip()
      && db_->getChip()->getBlock()) {
    logger_->info(ORD, 48, "Loading an additional DEF.");
  }

  odb::defin::MODE mode = odb::defin::DEFAULT;
  if (floorplan_init) {
    mode = odb::defin::FLOORPLAN;
  } else if (incremental) {
    mode = odb::defin::INCREMENTAL;
  }
  odb::defin def_reader(db_, logger_, mode);
  std::vector<odb::dbLib*> search_libs;
  for (odb::dbLib* lib : db_->getLibs()) {
    search_libs.push_back(lib);
  }
  if (continue_on_errors) {
    def_reader.continueOnErrors();
  }
  if (child) {
    auto parent = db_->getChip()->getBlock();
    def_reader.createBlock(parent, search_libs, filename, tech);
  } else {
    def_reader.createChip(search_libs, filename, tech);
  }
}

static odb::defout::Version stringToDefVersion(const string& version)
{
  if (version == "5.8") {
    return odb::defout::Version::DEF_5_8;
  }
  if (version == "5.7") {
    return odb::defout::Version::DEF_5_7;
  }
  if (version == "5.6") {
    return odb::defout::Version::DEF_5_6;
  }
  if (version == "5.5") {
    return odb::defout::Version::DEF_5_5;
  }
  if (version == "5.4") {
    return odb::defout::Version::DEF_5_4;
  }
  if (version == "5.3") {
    return odb::defout::Version::DEF_5_3;
  }
  return odb::defout::Version::DEF_5_8;
}

void OpenRoad::writeDef(const char* filename, const string& version)
{
  odb::dbChip* chip = db_->getChip();
  if (chip) {
    odb::dbBlock* block = chip->getBlock();
    if (block) {
      sta::dbSta* sta = getSta();
      // def names are flat hierachical
      bool hierarchy_set = sta->getDbNetwork()->hasHierarchy();
      if (hierarchy_set) {
        sta->getDbNetwork()->disableHierarchy();
      }
      odb::defout def_writer(logger_);
      def_writer.setVersion(stringToDefVersion(version));
      def_writer.writeBlock(block, filename);
      if (hierarchy_set) {
        sta->getDbNetwork()->setHierarchy();
      }
    }
  }
}

void OpenRoad::writeAbstractLef(const char* filename,
                                const int bloat_factor,
                                const bool bloat_occupied_layers)
{
  odb::dbBlock* block = nullptr;
  odb::dbChip* chip = db_->getChip();
  if (chip) {
    block = chip->getBlock();
  }
  if (!block) {
    logger_->error(ORD, 53, "No block is loaded.");
  }
  utl::StreamHandler stream_handler(filename);
  odb::lefout writer(logger_, stream_handler.getStream());
  writer.setBloatFactor(bloat_factor);
  writer.setBloatOccupiedLayers(bloat_occupied_layers);
  writer.writeAbstractLef(block);
}

void OpenRoad::writeLef(const char* filename)
{
  sta::dbSta* sta = getSta();
  bool hierarchy_set = sta->getDbNetwork()->hasHierarchy();
  if (hierarchy_set) {
    sta->getDbNetwork()->disableHierarchy();
  }
  auto libs = db_->getLibs();
  int num_libs = libs.size();
  if (num_libs > 0) {
    if (num_libs > 1) {
      logger_->info(
          ORD, 34, "More than one lib exists, multiple files will be written.");
    }
    int cnt = 0;
    for (auto lib : libs) {
      std::string name(filename);
      if (cnt > 0) {
        auto pos = name.rfind('.');
        if (pos != string::npos) {
          name.insert(pos, "_" + std::to_string(cnt));
        } else {
          name += "_" + std::to_string(cnt);
        }
        utl::StreamHandler stream_handler(name.c_str());
        odb::lefout lef_writer(logger_, stream_handler.getStream());
        lef_writer.writeLib(lib);
      } else {
        utl::StreamHandler stream_handler(filename);
        odb::lefout lef_writer(logger_, stream_handler.getStream());
        lef_writer.writeTechAndLib(lib);
      }
      ++cnt;
    }
  } else if (db_->getTech()) {
    utl::StreamHandler stream_handler(filename);
    odb::lefout lef_writer(logger_, stream_handler.getStream());
    lef_writer.writeTech(db_->getTech());
  }
  if (hierarchy_set) {
    sta->getDbNetwork()->setHierarchy();
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

void OpenRoad::readDb(const char* filename, bool hierarchy)
{
  std::ifstream stream;
  stream.open(filename, std::ios::binary);
  try {
    readDb(stream);
  } catch (const std::ios_base::failure& f) {
    logger_->error(ORD, 54, "odb file {} is invalid: {}", filename, f.what());
  }
  // treat this as a hierarchical network.
  if (hierarchy) {
    sta::dbSta* sta = getSta();
    // After streaming in the last thing we do is build the hashes
    // we cannot rely on orders to do this during stream in
    sta->getDbNetwork()->setHierarchy();
  }
}

void OpenRoad::readDb(std::istream& stream)
{
  if (db_->getChip() && db_->getChip()->getBlock()) {
    logger_->error(
        ORD, 47, "You can't load a new db file as the db is already populated");
  }

  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit
                    | std::ios::eofbit);

  db_->read(stream);
}

void OpenRoad::writeDb(std::ostream& stream)
{
  stream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  db_->write(stream);
}

void OpenRoad::writeDb(const char* filename)
{
  utl::StreamHandler stream_handler(filename, true);

  db_->write(stream_handler.getStream());
}

void OpenRoad::readVerilog(const char* filename)
{
  verilog_network_->deleteTopInstance();

  if (verilog_reader_ == nullptr) {
    verilog_reader_ = new sta::VerilogReader(verilog_network_);
  }
  setDbNetworkLinkFunc(this, verilog_reader_);
  verilog_reader_->read(filename);
}

void OpenRoad::linkDesign(const char* design_name, bool hierarchy)

{
  bool success
      = dbLinkDesign(design_name, verilog_network_, db_, logger_, hierarchy);

  if (success) {
    delete verilog_reader_;
    verilog_reader_ = nullptr;
  }

  if (hierarchy) {
    sta::dbSta* sta = getSta();
    sta->getDbNetwork()->setHierarchy();
  }
  db_->triggerPostReadDb();
}

void OpenRoad::designCreated()
{
  db_->triggerPostReadDb();
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

  if (printInfo) {
    logger_->info(ORD, 30, "Using {} thread(s).", threads_);
  }

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

std::string OpenRoad::getExePath() const
{
  // use tcl since it already has a cross platform implementation of this
  if (Tcl_Eval(tcl_interp_, "file normalize [info nameofexecutable]")
      == TCL_OK) {
    std::string path = Tcl_GetStringResult(tcl_interp_);
    Tcl_ResetResult(tcl_interp_);
    return path;
  }
  return "";
}

std::string OpenRoad::getDocsPath() const
{
  const std::string exe = getExePath();

  if (exe.empty()) {
    return "";
  }

  std::filesystem::path path(exe);

  // remove binary name
  path = path.parent_path();

  if (path.stem() == "src") {
    // remove build
    return path.parent_path().parent_path() / "docs";
  }

  // remove bin
  return path.parent_path() / "share" / "openroad" / "man";
}

const char* OpenRoad::getVersion()
{
  return OPENROAD_VERSION;
}

const char* OpenRoad::getGitDescribe()
{
  return OPENROAD_GIT_DESCRIBE;
}

bool OpenRoad::getGPUCompileOption()
{
  return GPU;
}

bool OpenRoad::getPythonCompileOption()
{
  return BUILD_PYTHON;
}

bool OpenRoad::getGUICompileOption()
{
  return BUILD_GUI;
}

}  // namespace ord
