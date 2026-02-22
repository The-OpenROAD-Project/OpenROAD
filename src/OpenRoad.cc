// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ord/OpenRoad.hh"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "ord/Version.hh"
#include "tcl.h"
#ifdef ENABLE_PYTHON3
#define PY_SSIZE_T_CLEAN
#include "Python.h"
#endif

#include "ant/AntennaChecker.hh"
#include "ant/MakeAntennaChecker.hh"
#include "cgt/ClockGating.h"
#include "cgt/MakeClockGating.h"
#include "cts/MakeTritoncts.h"
#include "cts/TritonCTS.h"
#include "db_sta/MakeDbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "dft/Dft.hh"
#include "dft/MakeDft.hh"
#include "dpl/MakeOpendp.h"
#include "dpl/Opendp.h"
#include "drt/MakeTritonRoute.h"
#include "drt/TritonRoute.h"
#include "dst/Distributed.h"
#include "dst/MakeDistributed.h"
#include "est/EstimateParasitics.h"
#include "est/MakeEstimateParasitics.h"
#include "exa/MakeExample.h"
#include "exa/example.h"
#include "fin/Finale.h"
#include "fin/MakeFinale.h"
#include "gpl/MakeReplace.h"
#include "gpl/Replace.h"
#include "grt/GlobalRouter.h"
#include "grt/MakeGlobalRouter.h"
#include "gui/MakeGui.h"
#include "ifp/MakeInitFloorplan.hh"
#include "mpl/MakeMacroPlacer.h"
#include "mpl/rtl_mp.h"
#include "odb/3dblox.h"
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
#include "par/PartitionMgr.h"
#include "pdn/MakePdnGen.hh"
#include "pdn/PdnGen.hh"
#include "ppl/IOPlacer.h"
#include "ppl/MakeIoplacer.h"
#include "psm/MakePDNSim.hh"
#include "psm/pdnsim.h"
#include "ram/MakeRam.h"
#include "ram/ram.h"
#include "rcx/MakeOpenRCX.h"
#include "rcx/ext.h"
#include "rmp/MakeRestructure.h"
#include "rmp/Restructure.h"
#include "rsz/MakeResizer.hh"
#include "rsz/Resizer.hh"
#include "sta/VerilogReader.hh"
#include "sta/VerilogWriter.hh"
#include "stt/MakeSteinerTreeBuilder.h"
#include "tap/MakeTapcell.h"
#include "tap/tapcell.h"
#include "upf/MakeUpf.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"
#include "utl/MakeLogger.h"
#include "utl/Progress.h"
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

using odb::dbChip;
using odb::dbDatabase;
using odb::dbTech;

using utl::ORD;

OpenRoad* OpenRoad::app_ = nullptr;

OpenRoad::OpenRoad()
{
  db_ = dbDatabase::create();
}

OpenRoad::~OpenRoad()
{
  delete verilog_network_;
  // Temporarily removed until a crash can be resolved
  // deleteDbSta(sta_);
  // sta::deleteAllMemory();
  delete ioPlacer_;
  delete resizer_;
  delete opendp_;
  delete global_router_;
  delete restructure_;
  delete clock_gating_;
  delete tritonCts_;
  delete tapcell_;
  delete macro_placer_;
  delete example_;
  delete extractor_;
  delete detailed_router_;
  delete replace_;
  delete pdnsim_;
  delete finale_;
  delete ram_gen_;
  delete antenna_checker_;
  odb::dbDatabase::destroy(db_);
  delete partitionMgr_;
  delete pdngen_;
  delete icewall_;
  delete distributer_;
  delete stt_builder_;
  delete dft_;
  delete estimate_parasitics_;
  delete logger_;
  delete verilog_reader_;
  delete callback_handler_;
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
    std::cerr << "Attempt to reinitialize the application.\n";
    exit(1);
  }
  app_ = app;
}

////////////////////////////////////////////////////////////////

void initOpenRoad(Tcl_Interp* interp,
                  const char* log_filename,
                  const char* metrics_filename,
                  const bool batch_mode)
{
  OpenRoad::openRoad()->init(
      interp, log_filename, metrics_filename, batch_mode);
}

void OpenRoad::init(Tcl_Interp* tcl_interp,
                    const char* log_filename,
                    const char* metrics_filename,
                    const bool batch_mode)
{
  tcl_interp_ = tcl_interp;

  // Make components.
  utl::Progress::setBatchMode(batch_mode);
  logger_ = new utl::Logger(log_filename, metrics_filename);
  callback_handler_ = new utl::CallBackHandler(logger_);
  db_->setLogger(logger_);
  sta_ = new sta::dbSta(tcl_interp, db_, logger_);
  verilog_network_ = new dbVerilogNetwork(sta_);
  ioPlacer_ = new ppl::IOPlacer(db_, logger_);
  stt_builder_ = new stt::SteinerTreeBuilder(db_, logger_);
  antenna_checker_ = new ant::AntennaChecker(db_, logger_);
  opendp_ = new dpl::Opendp(db_, logger_);
  global_router_ = new grt::GlobalRouter(logger_,
                                         callback_handler_,
                                         stt_builder_,
                                         db_,
                                         sta_,
                                         antenna_checker_,
                                         opendp_);
  grt::initGui(global_router_, db_, logger_);

  estimate_parasitics_ = new est::EstimateParasitics(
      logger_, callback_handler_, db_, sta_, stt_builder_, global_router_);
  est::initGui(estimate_parasitics_);

  resizer_ = new rsz::Resizer(logger_,
                              db_,
                              sta_,
                              stt_builder_,
                              global_router_,
                              opendp_,
                              estimate_parasitics_);
  finale_ = new fin::Finale(db_, logger_);
  restructure_ = new rmp::Restructure(
      logger_, sta_, db_, resizer_, estimate_parasitics_);
  clock_gating_ = new cgt::ClockGating(logger_, sta_);
  tritonCts_ = new cts::TritonCTS(logger_,
                                  db_,
                                  getDbNetwork(),
                                  sta_,
                                  stt_builder_,
                                  resizer_,
                                  estimate_parasitics_);
  tapcell_ = new tap::Tapcell(db_, logger_);
  partitionMgr_ = new par::PartitionMgr(db_, getDbNetwork(), sta_, logger_);
  macro_placer_
      = new mpl::MacroPlacer(getDbNetwork(), db_, sta_, logger_, partitionMgr_);
  extractor_ = new rcx::Ext(db_, logger_, getVersion());
  distributer_ = new dst::Distributed(logger_);
  detailed_router_ = new drt::TritonRoute(
      db_, logger_, callback_handler_, distributer_, stt_builder_);
  drt::initGui(detailed_router_);

  replace_ = new gpl::Replace(db_, sta_, resizer_, global_router_, logger_);
  pdnsim_ = new psm::PDNSim(logger_, db_, sta_, estimate_parasitics_, opendp_);
  pdngen_ = new pdn::PdnGen(db_, logger_);
  ram_gen_ = new ram::RamGen(getDbNetwork(),
                             db_,
                             logger_,
                             pdngen_,
                             ioPlacer_,
                             opendp_,
                             global_router_,
                             detailed_router_);
  icewall_ = new pad::ICeWall(db_, logger_);
  dft_ = new dft::Dft(db_, sta_, logger_);
  example_ = new exa::Example(db_, logger_);

  // Init components.
  Ord_Init(tcl_interp);
  // Import TCL scripts.
  utl::evalTclInit(tcl_interp, ord::ord_tcl_inits);

  utl::initLogger(tcl_interp);

  // GUI first so we can register our sink with the logger
  gui::initGui(tcl_interp, db_, sta_, logger_);
  odb::initOdb(tcl_interp);
  upf::initUpf(tcl_interp);
  ifp::initInitFloorplan(tcl_interp);
  sta::initDbSta(tcl_interp);
  rsz::initResizer(tcl_interp);
  ppl::initIoplacer(tcl_interp);
  gpl::initReplace(tcl_interp);
  gpl::initReplaceGraphics(replace_, logger_);
  dpl::initOpendp(tcl_interp);
  fin::initFinale(tcl_interp);
  ram::initRamGen(tcl_interp);
  grt::initTcl(tcl_interp);
  cts::initTritonCts(tcl_interp);
  tap::initTapcell(tcl_interp);
  mpl::initMacroPlacer(tcl_interp);
  exa::initExample(tcl_interp);
  rcx::initOpenRCX(tcl_interp);
  pad::initICeWall(tcl_interp);
  rmp::initRestructure(tcl_interp);
  cgt::initClockGating(tcl_interp);
  drt::initTcl(tcl_interp);
  psm::initPDNSim(tcl_interp);
  ant::initAntennaChecker(tcl_interp);
  par::initPartitionMgr(tcl_interp);
  pdn::initPdnGen(tcl_interp);
  dst::initDistributed(tcl_interp);
  stt::initSteinerTreeBuilder(tcl_interp);
  dft::initDft(tcl_interp);
  est::initTcl(tcl_interp);

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
                       dbChip* chip,
                       bool continue_on_errors,
                       bool floorplan_init,
                       bool incremental)
{
  if (!floorplan_init && !incremental && chip && chip->getBlock()) {
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
  def_reader.readChip(search_libs, filename, chip);
}

static odb::DefOut::Version stringToDefVersion(const std::string& version)
{
  if (version == "5.8") {
    return odb::DefOut::Version::DEF_5_8;
  }
  if (version == "5.7") {
    return odb::DefOut::Version::DEF_5_7;
  }
  if (version == "5.6") {
    return odb::DefOut::Version::DEF_5_6;
  }
  if (version == "5.5") {
    return odb::DefOut::Version::DEF_5_5;
  }
  if (version == "5.4") {
    return odb::DefOut::Version::DEF_5_4;
  }
  if (version == "5.3") {
    return odb::DefOut::Version::DEF_5_3;
  }
  return odb::DefOut::Version::DEF_5_8;
}

void OpenRoad::writeDef(const char* filename, const char* version)
{
  writeDef(filename, std::string(version));
}

void OpenRoad::writeDef(const char* filename, const std::string& version)
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
      odb::DefOut def_writer(logger_);
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
  utl::OutStreamHandler stream_handler(filename);
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
        if (pos != std::string::npos) {
          name.insert(pos, "_" + std::to_string(cnt));
        } else {
          name += "_" + std::to_string(cnt);
        }
        utl::OutStreamHandler stream_handler(name.c_str());
        odb::lefout lef_writer(logger_, stream_handler.getStream());
        lef_writer.writeLib(lib);
      } else {
        utl::OutStreamHandler stream_handler(filename);
        odb::lefout lef_writer(logger_, stream_handler.getStream());
        lef_writer.writeTechAndLib(lib);
      }
      ++cnt;
    }
  } else if (db_->getTech()) {
    utl::OutStreamHandler stream_handler(filename);
    odb::lefout lef_writer(logger_, stream_handler.getStream());
    lef_writer.writeTech(db_->getTech());
  }
  if (hierarchy_set) {
    sta->getDbNetwork()->setHierarchy();
  }
}

void OpenRoad::writeCdl(const char* out_filename,
                        const std::vector<const char*>& masters_filenames,
                        bool include_fillers)
{
  odb::dbChip* chip = db_->getChip();
  if (chip) {
    odb::dbBlock* block = chip->getBlock();
    if (block) {
      odb::cdl::writeCdl(
          getLogger(), block, out_filename, masters_filenames, include_fillers);
    }
  }
}

void OpenRoad::read3Dbv(const std::string& filename)
{
  odb::ThreeDBlox parser(logger_, db_, sta_);
  parser.readDbv(filename);
}

void OpenRoad::read3Dbx(const std::string& filename)
{
  odb::ThreeDBlox parser(logger_, db_, sta_);
  parser.readDbx(filename);
  check3DBlox();
  db_->triggerPostRead3Dbx(db_->getChip());
}

void OpenRoad::read3DBloxBMap(const std::string& filename)
{
  odb::ThreeDBlox parser(logger_, db_);
  parser.readBMap(filename);
}

void OpenRoad::check3DBlox()
{
  if (db_->getChip() == nullptr) {
    logger_->error(utl::ORD, 76, "No design loaded.");
    return;
  }
  odb::ThreeDBlox checker(logger_, db_, sta_);
  checker.check();
}

void OpenRoad::write3Dbv(const std::string& filename)
{
  odb::ThreeDBlox writer(logger_, db_, sta_);
  writer.writeDbv(filename, db_->getChip());
}

void OpenRoad::write3Dbx(const std::string& filename)
{
  odb::ThreeDBlox writer(logger_, db_, sta_);
  writer.writeDbx(filename, db_->getChip());
}

// TODO: bool hierarchy should be removed in the future.
// It is retained for a while for backward compatibility.
void OpenRoad::readDb(const char* filename, bool hierarchy)
{
  try {
    utl::InStreamHandler handler(filename, true);
    readDb(handler.getStream());
  } catch (const std::ios_base::failure& f) {
    logger_->error(ORD, 54, "odb file {} is invalid: {}", filename, f.what());
  }
  // treat this as a hierarchical network.
  if (hierarchy || db_->hasHierarchy()) {
    logger_->warn(
        ORD,
        12,
        "Hierarchical flow (-hier) is currently in development and may cause "
        "multiple issues. Do not use in production environments.");

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
  utl::OutStreamHandler stream_handler(filename, true);
  writeDb(stream_handler.getStream());
}

void OpenRoad::readVerilog(const char* filename)
{
  verilog_network_->deleteTopInstance();

  if (verilog_reader_ == nullptr) {
    verilog_reader_ = new sta::VerilogReader(verilog_network_);
  }
  setDbNetworkLinkFunc(getVerilogNetwork(), verilog_reader_);
  verilog_reader_->read(filename);
}

void OpenRoad::linkDesign(const char* design_name,
                          bool hierarchy,
                          bool omit_filename_prop)

{
  bool success = dbLinkDesign(design_name,
                              verilog_network_,
                              db_,
                              logger_,
                              hierarchy,
                              omit_filename_prop);

  if (success) {
    delete verilog_reader_;
    verilog_reader_ = nullptr;
  }

  if (hierarchy) {
    logger_->warn(
        ORD,
        11,
        "Hierarchical flow (-hier) is currently in development and may cause "
        "multiple issues. Do not use in production environments.");

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

void OpenRoad::setThreadCount(int threads, bool print_info)
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

  if (print_info) {
    logger_->info(ORD, 30, "Using {} thread(s).", threads_);
  }

  // place limits on tools with threads
  // sta_->setThreadCount(threads_);

  // Temporary until some issues can be resolved related to multi-mode.
  sta_->setThreadCount(1);
}

void OpenRoad::setThreadCount(const char* threads, bool print_info)
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

  setThreadCount(max_threads, print_info);
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
