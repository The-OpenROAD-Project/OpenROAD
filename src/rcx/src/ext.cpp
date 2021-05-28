///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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
#include "rcx/ext.h"

#include <errno.h>

#include "opendb/wOrder.h"
#include "sta/StaMain.hh"
#include "utl/Logger.h"
#include "opendb/wOrder.h"

namespace sta {
// Tcl files encoded into strings.
extern const char* rcx_tcl_inits[];
}  // namespace sta

namespace rcx {

using utl::Logger;
using utl::RCX;

extern "C" { extern int Rcx_Init(Tcl_Interp* interp); }

Ext::Ext() : odb::ZInterface() {
  _ext = new extMain(5);
  _tree = NULL;
  logger_ = nullptr;
}

Ext::~Ext() { delete _ext; }

void Ext::init(Tcl_Interp* tcl_interp, odb::dbDatabase* db, Logger* logger) {
  _db = db;
  logger_ = logger;
  _ext->init(db, logger);

  // Define swig TCL commands.
  Rcx_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::rcx_tcl_inits);
}

void Ext::setLogger(Logger* logger) {
  if (!logger_)
    logger_ = logger;
}

bool Ext::load_model(const std::string& name, bool lef_rc,
                     const std::string& file, int setMin, int setTyp,
                     int setMax) {
  if (lef_rc) {
    if (!_ext->checkLayerResistance())
      return TCL_ERROR;
    _ext->addExtModel();  // fprintf(stdout, "Using LEF RC values to
                          // extract!\n");
    logger_->info(RCX, 9, "Using LEF RC values to extract!");
  } else if (!file.empty()) {
    _ext->readExtRules(name.c_str(), file.c_str(), setMin, setTyp, setMax);
    int numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg;
    _ext->getBlock()->getExtCount(numOfNet, numOfRSeg, numOfCapNode,
                                  numOfCCSeg);

    _ext->setupMapping(3 * numOfNet);
  } else {
    // fprintf(stdout, "\nHave to specify options:\n\t-lef_rc to read resistance
    // and capacitance values from LEF or \n\t-file to read high accuracy RC
    // models\n");
    logger_->info(
        RCX, 151,
        "Have to specify options:"
        "\n\t-lef_rc to read resistance and "
        "capacitance values from LEF or \n\t-file to read high accuracy RC "
        "models");
  }

  return 0;
}

bool Ext::read_process(const std::string& name, const std::string& file) {
  _ext->readProcess(name.c_str(), file.c_str());

  return TCL_OK;
}

bool Ext::rules_gen(const std::string& name, const std::string& dir,
                    const std::string& file, bool write_to_solver,
                    bool read_from_solver, bool run_solver, int pattern,
                    bool keep_file) {
  _ext->rulesGen(name.c_str(), dir.c_str(), file.c_str(), pattern,
                 write_to_solver, read_from_solver, run_solver, keep_file);

  return TCL_OK;
}

bool Ext::metal_rules_gen(const std::string& name, const std::string& dir,
                          const std::string& file, bool write_to_solver,
                          bool read_from_solver, bool run_solver, int pattern,
                          bool keep_file, int metal) {
  _ext->metRulesGen(name.c_str(), dir.c_str(), file.c_str(), pattern,
                    write_to_solver, read_from_solver, run_solver, keep_file,
                    metal);
  return TCL_OK;
}

bool Ext::write_rules(const std::string& name, const std::string& dir,
                      const std::string& file, int pattern, bool read_from_db,
                      bool read_from_solver) {
  _ext->setBlockFromChip();
  _ext->writeRules(name.c_str(), dir.c_str(), file.c_str(), pattern,
                   read_from_db, read_from_solver);
  return TCL_OK;
}

bool Ext::get_ext_metal_count(int& metal_count) {
  extRCModel* m = _ext->getRCModel();
  metal_count = m->getLayerCnt();
  return TCL_OK;
}

bool Ext::bench_net(const std::string& dir, int net, bool write_to_solver,
                    bool read_from_solver, bool run_solver,
                    int max_track_count) {
  extMainOptions opt;

  opt._write_to_solver = write_to_solver;
  opt._read_from_solver = read_from_solver;
  opt._run_solver = run_solver;

  int netId = net;
  int trackCnt = max_track_count;
  opt._topDir = dir.c_str();

  if (netId == 0) {
    logger_->info(RCX, 144, "Net (={}), should be a positive number", netId);
    return TCL_OK;
  }
  logger_->info(RCX, 145, "Benchmarking using 3d field solver net {}...",
                netId);
#ifdef ZUI
  ZPtr<ISdb> dbNetSdb = _ext->getBlock()->getNetSdb(_context, _ext->getTech());

  _ext->benchNets(&opt, netId, trackCnt, dbNetSdb);
#endif
  logger_->info(RCX, 146, "Finished 3D field solver benchmarking.");

  return TCL_OK;
}

bool Ext::run_solver(const std::string& dir, int net, int shape) {
  extMainOptions opt;
  opt._topDir = dir.c_str();
  uint netId = net;
  int shapeId = shape;
  _ext->runSolver(&opt, netId, shapeId);
  return TCL_OK;
}

bool Ext::bench_wires(const BenchWiresOptions& bwo) {
  extMainOptions opt;

  opt._topDir = bwo.dir;
  opt._met_cnt = bwo.met_cnt;
  opt._met = bwo.met;
  opt._overDist = bwo.over_dist;
  opt._underDist = bwo.under_dist;
  opt._overMet = bwo.over_met;
  opt._over = bwo.Over;
  opt._underMet = bwo.under_met;
  opt._overUnder = bwo.over_under;
  opt._len = 1000 * bwo.len;
  opt._wireCnt = bwo.cnt;
  opt._name = bwo.block;

  opt._default_lef_rules = bwo.default_lef_rules;
  opt._nondefault_lef_rules = bwo.nondefault_lef_rules;

  opt._3dFlag = bwo.ddd;
  opt._multiple_widths = bwo.multiple_widths;

  opt._write_to_solver = bwo.write_to_solver;
  opt._read_from_solver = bwo.read_from_solver;
  opt._run_solver = bwo.run_solver;
  opt._diag = bwo.diag;
  opt._db_only = bwo.db_only;
  opt._gen_def_patterns = bwo.gen_def_patterns;

  opt._res_patterns = bwo.resPatterns;

  if (opt._gen_def_patterns) {
    opt._diag = true;
    opt._overUnder = true;
    opt._db_only = true;
    opt._over = true;
    opt._underMet = 0;
  }

  Ath__parser parser;

  std::string th_list(bwo.th_list);
  std::string w_list(bwo.w_list);
  std::string s_list(bwo.s_list);
  std::string th(bwo.th);
  std::string w(bwo.w);
  std::string s(bwo.s);
  std::string d(bwo.d);
  std::string grid_list(bwo.grid_list);

  if (!th.empty()) {
    parser.mkWords(bwo.th_list);
    parser.getDoubleArray(&opt._thicknessTable, 0);
    opt._thListFlag = true;
  } else {
    parser.mkWords(bwo.th);
    parser.getDoubleArray(&opt._thicknessTable, 0);
    opt._thListFlag = false;
  }
  opt._listsFlag = false;
  opt._wsListFlag = false;
  if (opt._default_lef_rules) {  // minWidth, minSpacing, minThickness, pitch
                                 // multiplied by grid_list
    // user sets default_flag and grid_list multipliers
    if (grid_list.empty()) {
      opt._gridTable.add(1.0);
    } else {
      parser.mkWords(grid_list.c_str());
      parser.getDoubleArray(&opt._gridTable, 0);
    }
    opt._listsFlag = true;
  } else if (opt._nondefault_lef_rules) {
    opt._listsFlag = true;
  } else if (!w_list.empty() && !s_list.empty()) {
    opt._listsFlag = true;
    opt._wsListFlag = true;
    parser.mkWords(w_list.c_str());
    parser.getDoubleArray(&opt._widthTable, 0);
    parser.mkWords(s_list.c_str());
    parser.getDoubleArray(&opt._spaceTable, 0);
  } else {
    parser.mkWords(w.c_str());
    parser.getDoubleArray(&opt._widthTable, 0);
    parser.mkWords(s.c_str());
    parser.getDoubleArray(&opt._spaceTable, 0);
    parser.mkWords(th.c_str());
    parser.getDoubleArray(&opt._thicknessTable, 0);
    parser.mkWords(d.c_str());
    parser.getDoubleArray(&opt._densityTable, 0);
  }
  _ext->benchWires(&opt);

  return TCL_OK;
}

bool Ext::bench_verilog(const std::string& file) {
  _ext->setBlockFromChip();
  char* filename = (char*)file.c_str();
  if (!filename || !filename[0]) {
    logger_->error(RCX, 147, "bench_verilog: file is not defined!");
  }
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) {
    logger_->error(RCX, 378, "Can't open file {}", filename);
  }
  _ext->benchVerilog(fp);

  return TCL_OK;
}

bool Ext::clean(bool all_models, bool ext_only) { return TCL_OK; }

bool Ext::define_process_corner(int ext_model_index, const std::string& name) {
  _ext->setBlockFromChip();
  char* cornerName = _ext->addRCCorner(name.c_str(), ext_model_index);

  if (cornerName != NULL) {
    logger_->info(RCX, 29, "Defined extraction corner {}", cornerName);
    return TCL_OK;
  } else {
    return TCL_ERROR;
  }
}

bool Ext::define_derived_corner(const std::string& name,
                                const std::string& process_corner_name,
                                float res_factor, float cc_factor,
                                float gndc_factor) {
  if (process_corner_name.empty()) {
    logger_->warn(RCX, 30, "The original process corner name is required");
    return TCL_ERROR;
  }

  int model = _ext->getDbCornerModel(process_corner_name.c_str());

  char* cornerName = _ext->addRCCornerScaled(name.c_str(), model, res_factor,
                                             cc_factor, gndc_factor);

  if (cornerName != NULL) {
    logger_->info(RCX, 31, "Defined Derived extraction corner {}", cornerName);
    return TCL_OK;
  } else {
    return TCL_ERROR;
  }
}

bool Ext::delete_corners() {
  _ext->deleteCorners();
  return TCL_OK;
}

bool Ext::get_corners(std::list<std::string>& corner_list) {
  _ext->getCorners(corner_list);
  return TCL_OK;
}

bool Ext::read_qcap(const std::string& file_name, const std::string& cap_file,
                    bool skip_bterms, bool no_qcap, const std::string& design) {
  if (file_name.empty()) {
    logger_->warn(RCX, 32, "-file flag is required on the command!");
    return TCL_OK;
  }

  extMeasure m;
  if (!no_qcap)
    m.readQcap(_ext, file_name.c_str(), design.c_str(), cap_file.c_str(),
               skip_bterms, _db);
  else
    m.readAB(_ext, file_name.c_str(), design.c_str(), cap_file.c_str(),
             skip_bterms, _db);

  return TCL_OK;
}

bool Ext::get_ext_db_corner(int& index, const std::string& name) {
  index = _ext->getDbCornerIndex(name.c_str());

  if (index < 0)
    logger_->warn(RCX, 148, "Extraction corner {} not found!", name.c_str());

  return TCL_OK;
}

bool Ext::assembly(odb::dbBlock* block, odb::dbBlock* main_block) {
  if (main_block == NULL) {
    logger_->info(RCX, 149, "Add parasitics of block {} onto nets/wires ...{}",
                  block->getConstName());
  } else {
    logger_->info(RCX, 150, "Add parasitics of block {} onto block {}...",
                  block->getConstName(), main_block->getConstName());
  }

  extMain::assemblyExt(main_block, block, logger_);

  return TCL_OK;
}

bool Ext::flatten(odb::dbBlock* block, bool spef) {
  if (block == NULL) {
    logger_->error(RCX, 466, "No block for flatten command");
  }
  _ext->addRCtoTop(block, spef);
  return TCL_OK;
}

bool Ext::extract(ExtractOptions opts) {
  _ext->setBlockFromChip();
  odb::dbBlock* block = _ext->getBlock();
  logger_->info(RCX, 8, "extracting parasitics of {} ...",
                block->getConstName());

  odb::orderWires(block, nullptr /* net_name_or_id*/, false /* force */,
                  false /* verbose */, true /* quiet */);
  if (opts.lef_rc) {
    if (!_ext->checkLayerResistance())
      return TCL_ERROR;
    _ext->addExtModel();
    logger_->info(RCX, 375, "Using LEF RC values to extract!");
  }

  bool extract_power_grid_only = opts.power_grid;
#ifdef ZUI
  if (extract_power_grid_only) {
    dbBlock* block = _ext->getBlock();
    if (block != NULL) {
      bool skipCutVias = true;
      block->initSearchBlock(_db->getTech(), true, true, _context, skipCutVias);
    } else {
      logger_->error(RCX, 10, "There is no block to extract!");
      return TCL_ERROR;
    }
    _ext->setPowerExtOptions(skip_power_stubs, exclude_cells.c_str(),
                             in_args->skip_m1_caps(),
                             in_args->power_source_coords());
  }
#endif

  const char* extRules = opts.ext_model_file;
  const char* cmpFile = opts.cmp_file;
  bool density_model = opts.wire_density;

  uint ccUp = opts.cc_up;
  uint ccFlag = opts.cc_model;
  // uint ccFlag= 25;
  int ccBandTracks = opts.cc_band_tracks;
  uint use_signal_table = opts.signal_table;
  bool merge_via_res = opts.no_merge_via_res ? false : true;
  uint extdbg = opts.test;
  const char* nets = opts.net;
  const char* debug_nets = opts.debug_net;
  bool gs = opts.no_gs ? false : true;
  double ccThres = opts.coupling_threshold;
  int ccContextDepth = opts.context_depth;
  bool overCell = opts.over_cell;
  bool btermThresholdFlag = opts.tile;

  _ext->set_debug_nets(debug_nets);
  _ext->skip_via_wires(opts.skip_via_wires);
  _ext->skip_via_wires(true);
  _ext->_lef_res = opts.lef_res;

  uint tilingDegree = opts.tiling;

  odb::ZPtr<odb::ISdb> dbNetSdb = NULL;
  bool extSdb = false;

  // ccBandTracks = 0;  // tttt no band CC for now
  if (extdbg == 100 || extdbg == 102)  // 101: activate reuse metal fill
                                       // 102: no bandTrack
    ccBandTracks = 0;
  if (ccBandTracks)
    opts.eco = false;  // tbd
  else {
#ifdef ZUI
    dbNetSdb = _ext->getBlock()->getNetSdb();

    if (dbNetSdb == NULL) {
      extSdb = true;
      if (rlog)
        AthResourceLog("before get sdb", 0);

      dbNetSdb = _ext->getBlock()->getNetSdb(_context, _ext->getTech());

      if (rlog)
        AthResourceLog("after get sdb", 0);
    }
#endif
  }

  if (tilingDegree == 1)
    extdbg = 501;
  else if (tilingDegree == 10)
    extdbg = 603;
  else if (tilingDegree == 7)
    extdbg = 703;
  else if (tilingDegree == 77) {
    extdbg = 773;
    //_ext->rcGen(NULL, max_res, merge_via_res, 77, true, this);
  }
  /*
    else if (tilingDegree==77) {
    extdbg= 703;
    _ext->rcGen(NULL, max_res, merge_via_res, 77, true, this);
    }
  */
  else if (tilingDegree == 8)
    extdbg = 803;
  else if (tilingDegree == 9)
    extdbg = 603;
  else if (tilingDegree == 777) {
    extdbg = 777;

    uint cnt = 0;
    odb::dbSet<odb::dbNet> bnets = _ext->getBlock()->getNets();
    odb::dbSet<odb::dbNet>::iterator net_itr;
    for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
      odb::dbNet* net = *net_itr;

      cnt += _ext->rcNetGen(net);
    }
    logger_->info(RCX, 14, "777: Final rc segments = {}", cnt);
  }
  if (_ext->makeBlockRCsegs(
          btermThresholdFlag, cmpFile, density_model, opts.litho, nets,
          opts.bbox, opts.ibox, ccUp, ccFlag, ccBandTracks, use_signal_table,
          opts.max_res, merge_via_res, extdbg, opts.preserve_geom, opts.re_run,
          opts.eco, gs, opts.rlog, dbNetSdb, ccThres, ccContextDepth, overCell,
          extRules, this) == 0)
    return TCL_ERROR;

  odb::dbBlock* topBlock = _ext->getBlock();
  if (tilingDegree == 1) {
    odb::dbSet<odb::dbBlock> children = topBlock->getChildren();
    odb::dbSet<odb::dbBlock>::iterator itr;

    // Extraction

    logger_->info(RCX, 6, "List of extraction tile blocks:");
    for (itr = children.begin(); itr != children.end(); ++itr) {
      odb::dbBlock* blk = *itr;
      logger_->info(RCX, 377, "{}", blk->getConstName());
    }
  } else if (extdbg == 501) {
    odb::dbSet<odb::dbBlock> children = topBlock->getChildren();
    odb::dbSet<odb::dbBlock>::iterator itr;

    // Extraction
    for (itr = children.begin(); itr != children.end(); ++itr) {
      odb::dbBlock* blk = *itr;
      logger_->info(RCX, 12, "Extacting block {}...", blk->getConstName());
      extMain* ext = new extMain(5);

      ext->setBlock(blk);

      if (ext->makeBlockRCsegs(btermThresholdFlag, cmpFile, density_model,
                               opts.litho, nets, opts.bbox, opts.ibox, ccUp,
                               ccFlag, ccBandTracks, use_signal_table,
                               opts.max_res, merge_via_res, 0,
                               opts.preserve_geom, opts.re_run, opts.eco, gs,
                               opts.rlog, dbNetSdb, ccThres, ccContextDepth,
                               overCell, extRules, this) == 0) {
        logger_->warn(RCX, 13, "Failed to Extract block {}...",
                      blk->getConstName());
        return TCL_ERROR;
      }
    }

    for (itr = children.begin(); itr != children.end(); ++itr) {
      odb::dbBlock* blk = *itr;
      logger_->info(RCX, 46, "Assembly of block {}...", blk->getConstName());

      extMain::assemblyExt(topBlock, blk, logger_);
    }
    //_ext= new extMain(5);
    //_ext->setDB(_db);
    //_ext->setBlock(topBlock);
  }

  // report total net cap

  if (opts.write_total_caps) {
    char netcapfile[500];
    sprintf(netcapfile, "%s.totCap", _ext->getBlock()->getConstName());
    _ext->reportTotalCap(netcapfile, true, false, 1.0, NULL, NULL);
  }

  if (!ccBandTracks) {
    if (extSdb && extdbg != 99) {
      if (opts.rlog)
        odb::AthResourceLog("before remove sdb", 0);
      dbNetSdb->cleanSdb();
#ifdef ZUI
      _ext->getBlock()->resetNetSdb();
      if (rlog)
        AthResourceLog("after remove sdb", 0);
#endif
    }
  }

  //    fprintf(stdout, "Finished extracting %s.\n",
  //    _ext->getBlock()->getName().c_str());
  logger_->info(RCX, 15, "Finished extracting {}.",
                _ext->getBlock()->getName().c_str());
  return 0;
}

bool Ext::adjust_rc(float res_factor, float cc_factor, float gndc_factor) {
  _ext->adjustRC(res_factor, cc_factor, gndc_factor);
  return 0;
}

bool Ext::init_incremental_spef(const std::string& origp,
                                const std::string& newp, bool no_backslash,
                                const std::string& exclude_cells) {
  _ext->initIncrementalSpef(origp.c_str(), newp.c_str(), exclude_cells.c_str(),
                            no_backslash);
  return 0;
}

bool Ext::export_sdb(odb::ZPtr<odb::ISdb>& net_sdb,
                     odb::ZPtr<odb::ISdb>& cc_sdb) {
  cc_sdb = _ext->getCcSdb();
  net_sdb = _ext->getNetSdb();

  return 0;
}

bool Ext::write_spef_nets(odb::dbObject* block, bool flatten, bool parallel,
                          int corner) {
  _ext->setBlockFromChip();
  _ext->write_spef_nets(flatten, parallel);

  return 0;
}

bool Ext::write_spef(const SpefOptions& opts) {
  _ext->setBlockFromChip();
  if (opts.end) {
    _ext->writeSPEF(true);
    return 0;
  }
  const char* name = opts.ext_corner_name;

  uint netId = opts.net_id;
  if (netId > 0) {
    _ext->writeSPEF(netId, opts.single_pi, opts.debug, opts.corner, name);
    return 0;
  }
  bool useIds = opts.use_ids;
  bool stop = opts.stop_after_map;
  bool initOnly = opts.init;
  if (!initOnly)
    logger_->info(RCX, 16, "Writing SPEF ...");
  initOnly = opts.parallel && opts.flatten;
  _ext->writeSPEF((char*)opts.file, (char*)opts.nets, useIds, opts.no_name_map,
                  (char*)opts.N, opts.term_junction_xy, opts.exclude_cells,
                  opts.cap_units, opts.res_units, opts.gz, stop, opts.w_clock,
                  opts.w_conn, opts.w_cap, opts.w_cc_cap, opts.w_res,
                  opts.no_c_num, initOnly, opts.single_pi, opts.no_backslash,
                  opts.corner, name, opts.flatten, opts.parallel);

  logger_->info(RCX, 17, "Finished writing SPEF ...");
  return 0;
}

bool Ext::independent_spef_corner() {
  _ext->setUniqueExttreeCorner();
  return 0;
}

bool Ext::read_spef(ReadSpefOpts& opt) {
  _ext->setBlockFromChip();
  logger_->info(RCX, 1, "Reading SPEF file: {}", opt.file);

  odb::ZPtr<odb::ISdb> netSdb = NULL;
  bool stampWire = opt.stamp_wire;
#ifdef ZUI
  if (stampWire)
    netSdb = _ext->getBlock()->getSignalNetSdb(_context, _db->getTech());
#endif
  bool useIds = opt.use_ids;
  uint testParsing = opt.test_parsing;

  Ath__parser parser;
  char* filename = (char*)opt.file;
  if (!filename || !filename[0]) {
    logger_->error(RCX, 2, "Filename is not defined!");
  }
  parser.mkWords(filename);

  _ext->readSPEF(
      parser.get(0), (char*)opt.net, opt.force, useIds, opt.r_conn,
      (char*)opt.N, opt.r_cap, opt.r_cc_cap, opt.r_res, opt.cc_threshold,
      opt.cc_ground_factor, opt.length_unit, opt.m_map, opt.no_cap_num_collapse,
      (char*)opt.cap_node_map_file, opt.log, opt.corner, 0.0, 0.0, NULL, NULL,
      NULL, opt.db_corner_name, opt.calibrate_base_corner, opt.spef_corner,
      opt.fix_loop, opt.keep_loaded_corner, stampWire, netSdb, testParsing,
      opt.more_to_read, false /*diff*/, false /*calibrate*/,
      opt.app_print_limit);

  for (int ii = 1; ii < parser.getWordCnt(); ii++)
    _ext->readSPEFincr(parser.get(ii));

  return 0;
}

bool Ext::diff_spef(const DiffOptions& opt) {
  _ext->setBlockFromChip();
  std::string filename(opt.file);
  if (filename.empty()) {
    logger_->error(RCX, 380,
                   "Filename is not defined to run diff_spef command!");
  }
  logger_->info(RCX, 19, "diffing spef {}", opt.file);

  Ath__parser parser;
  parser.mkWords(opt.file);

  // char* excludeSubWord = (char*) opt.exclude_net_subword.c_str();
  // char* subWord        = (char*) opt.net_subword.c_str();
  // char* statsFile      = (char*) opt.rc_stats_file.c_str();

  _ext->readSPEF(parser.get(0), (char*)opt.net, false /*force*/, opt.use_ids,
                 opt.r_conn, NULL /*N*/, opt.r_cap, opt.r_cc_cap, opt.r_res,
                 -1.0, 0.0 /*cc_ground_factor*/, 1.0 /*length_unit*/, opt.m_map,
                 false /*noCapNumCollapse*/, NULL /*capNodeMapFile*/, opt.log,
                 opt.ext_corner, opt.low_guard, opt.upper_guard,
                 (char*)opt.exclude_net_subword, (char*)opt.net_subword,
                 (char*)opt.rc_stats_file, (char*)opt.db_corner_name, NULL,
                 opt.spef_corner, 0 /*fix_loop*/, false /*keepLoadedCorner*/,
                 false /*stampWire*/, NULL /*netSdb*/, opt.test_parsing,
                 false /*moreToRead*/, true /*diff*/, false /*calibrate*/, 0);

  // for (uint ii=1; ii<parser.getWordCnt(); ii++)
  //	_ext->readSPEFincr(parser.get(ii));

  return 0;
}

bool Ext::calibrate(const std::string& spef_file,
                    const std::string& db_corner_name, int corner,
                    int spef_corner, bool m_map, float upper_limit,
                    float lower_limit) {
  if (spef_file.empty())
    logger_->error(RCX, 381,
                   "Filename for calibration is not defined. Define the "
                   "filename using -spef_file");

  logger_->info(RCX, 21, "calibrate on spef file  {}", spef_file.c_str());
  Ath__parser parser;
  parser.mkWords((char*)spef_file.c_str());
  _ext->calibrate(parser.get(0), m_map, upper_limit, lower_limit,
                  db_corner_name.c_str(), corner, spef_corner);
  return 0;
}

bool Ext::match(const std::string& spef_file, const std::string& db_corner_name,
                int corner, int spef_corner, bool m_map) {
  if (spef_file.empty()) {
    logger_->info(RCX, 20,
                  "Filename for calibration is not defined. Define the "
                  "filename using -spef_file");
  }
  logger_->info(RCX, 18, "match on spef file  {}", spef_file.c_str());
  Ath__parser parser;
  parser.mkWords((char*)spef_file.c_str());
  _ext->match(parser.get(0), m_map, db_corner_name.c_str(), corner,
              spef_corner);
  return 0;
}

#if 0
TCL_METHOD ( Ext::setRules )
{
    ZIn_Ext_setRules * in_args = (ZIn_Ext_setRules *) in;
	_extModel= (ExtModel *) in_args->model();
	xm->getFringeCap(....)
}
#endif

bool Ext::set_block(const std::string& block_name, odb::dbBlock* block,
                    const std::string& inst_name, odb::dbInst* inst) {
  if (!inst_name.empty()) {
    odb::dbChip* chip = _db->getChip();
    odb::dbInst* ii = chip->getBlock()->findInst(inst_name.c_str());
    odb::dbMaster* m = ii->getMaster();
    logger_->info(RCX, 24, "Inst={} ==> {} {} of Master {} {}",
                  inst_name.c_str(), ii->getId(), ii->getConstName(),
                  m->getId(), m->getConstName());
  }
  if (block == NULL) {
    if (block_name.empty()) {
      logger_->warn(RCX, 22, "command requires either dbblock or block name{}");
      return TCL_ERROR;
    }
    odb::dbChip* chip = _db->getChip();
    odb::dbBlock* child = chip->getBlock()->findChild(block_name.c_str());
    if (child == NULL) {
      logger_->warn(RCX, 430, "Cannot find block with master name {}",
                    block_name.c_str());
      return TCL_ERROR;
    }
    block = child;
  }

  delete _ext;
  _ext = new extMain(5);
  _ext->init(_db, logger_);

  _ext->setBlock(block);
  return 0;
}

bool Ext::report_total_cap(const std::string& file, bool res_only,
                           bool cap_only, float ccmult, const std::string& ref,
                           const std::string& read) {
  _ext->reportTotalCap(file.c_str(), cap_only, res_only, ccmult, ref.c_str(),
                       read.c_str());
  /*
          dbChip *chip= _db->getChip();
          dbBlock * child = chip->getBlock()->findChild(block_name);
  if (child==NULL) {
  logger_->warn(RCX, 23, "Cannot find block with master name {}",
  block_name);
  return TCL_ERROR;
  }
  block= child;
  */
  return 0;
}

bool Ext::report_total_cc(const std::string& file, const std::string& ref,
                          const std::string& read) {
  _ext->reportTotalCc(file.c_str(), ref.c_str(), read.c_str());
  return 0;
}

bool Ext::dump(bool open_tree_file, bool close_tree_file, bool cc_cap_geom,
               bool cc_net_geom, bool track_cnt, bool signal, bool power,
               int layer, const std::string& file) {
  logger_->info(RCX, 25, "Printing file {}", file.c_str());

  _ext->extDump((char*)file.c_str(), open_tree_file, close_tree_file,
                cc_cap_geom, cc_net_geom, track_cnt, signal, power, layer);

  return 0;
}

bool Ext::count(bool signal_wire_seg, bool power_wire_seg) {
  _ext->extCount(signal_wire_seg, power_wire_seg);

  return 0;
}

bool Ext::rc_tree(float max_cap, uint test, int net,
                  const std::string& print_tag) {
  int netId = net;
  char* printTag = (char*)print_tag.c_str();

  odb::dbBlock* block = _ext->getBlock();

  if (_tree == NULL)
    _tree = new extRcTree(block, logger_);

  uint cnt;
  if (netId > 0)
    _tree->makeTree((uint)netId, max_cap, test, true, true, cnt, 1.0 /*mcf*/,
                    printTag, false /*for_buffering*/);
  else
    _tree->makeTree(max_cap, test);

  return TCL_OK;
}
bool Ext::net_stats(std::list<int>& net_ids, const std::string& tcap,
                    const std::string& ccap, const std::string& ratio_cap,
                    const std::string& res, const std::string& len,
                    const std::string& met_cnt, const std::string& wire_cnt,
                    const std::string& via_cnt, const std::string& seg_cnt,
                    const std::string& term_cnt, const std::string& bterm_cnt,
                    const std::string& file, const std::string& bbox,
                    const std::string& branch_len) {
  Ath__parser parser;
  extNetStats limits;
  limits.reset();

  limits.update_double(&parser, tcap.c_str(), limits._tcap);
  limits.update_double(&parser, ccap.c_str(), limits._ccap);
  limits.update_double(&parser, ratio_cap.c_str(), limits._cc2tcap);
  limits.update_double(&parser, res.c_str(), limits._res);

  limits.update_int(&parser, len.c_str(), limits._len, 1000);
  limits.update_int(&parser, met_cnt.c_str(), limits._layerCnt);
  limits.update_int(&parser, wire_cnt.c_str(), limits._wCnt);
  limits.update_int(&parser, via_cnt.c_str(), limits._vCnt);
  limits.update_int(&parser, term_cnt.c_str(), limits._termCnt);
  limits.update_int(&parser, bterm_cnt.c_str(), limits._btermCnt);
  limits.update_bbox(&parser, bbox.c_str());

  FILE* fp = stdout;
  const char* filename = file.c_str();
  if (filename != NULL) {
    fp = fopen(filename, "w");
    if (fp == NULL) {
      logger_->warn(RCX, 11, "Can't open file {}", filename);
      return TCL_OK;
    }
  }
  bool skipDb = false;
  bool skipRC = false;
  bool skipPower = true;

  odb::dbBlock* block = _ext->getBlock();
  if (block == NULL) {
    logger_->warn(RCX, 28, "There is no extracted block");
    skipRC = true;
    return TCL_OK;
  }
  std::list<int> list_of_nets;
  int n = _ext->printNetStats(fp, block, &limits, skipRC, skipDb, skipPower,
                              &list_of_nets);
  logger_->info(RCX, 26, "{} nets found", n);

  net_ids = list_of_nets;

  return TCL_OK;
}

}  // namespace rcx
