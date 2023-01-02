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

#include "odb/wOrder.h"
#include "sta/StaMain.hh"
#include "utl/Logger.h"

namespace sta {
// Tcl files encoded into strings.
extern const char* rcx_tcl_inits[];
}  // namespace sta

namespace rcx {

using utl::Logger;
using utl::RCX;

extern "C" {
extern int Rcx_Init(Tcl_Interp* interp);
}

Ext::Ext() : odb::ZInterface()
{
  _ext = new extMain(5);
  _tree = NULL;
  logger_ = nullptr;
}

Ext::~Ext()
{
  delete _ext;
}

void Ext::init(Tcl_Interp* tcl_interp, odb::dbDatabase* db, Logger* logger)
{
  _db = db;
  logger_ = logger;
  _ext->init(db, logger);

  // Define swig TCL commands.
  Rcx_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::rcx_tcl_inits);
}

void Ext::setLogger(Logger* logger)
{
  if (!logger_)
    logger_ = logger;
}

bool Ext::load_model(const std::string& name,
                     bool lef_rc,
                     const std::string& file,
                     int setMin,
                     int setTyp,
                     int setMax)
{
  if (lef_rc) {
    if (!_ext->checkLayerResistance())
      return TCL_ERROR;
    _ext->addExtModel();  // fprintf(stdout, "Using LEF RC values to
                          // extract!\n");
    logger_->info(RCX, 9, "Using LEF RC values to extract!");
  } else if (!file.empty()) {
    _ext->readExtRules(name.c_str(), file.c_str(), setMin, setTyp, setMax);
    int numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg;
    _ext->getBlock()->getExtCount(
        numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg);

    _ext->setupMapping(3 * numOfNet);
  } else {
    logger_->info(
        RCX,
        151,
        "Have to specify options:"
        "\n\t-lef_rc to read resistance and "
        "capacitance values from LEF or \n\t-file to read high accuracy RC "
        "models");
  }

  return 0;
}

bool Ext::read_process(const std::string& name, const std::string& file)
{
  _ext->readProcess(name.c_str(), file.c_str());

  return TCL_OK;
}

bool Ext::rules_gen(const std::string& name,
                    const std::string& dir,
                    const std::string& file,
                    bool write_to_solver,
                    bool read_from_solver,
                    bool run_solver,
                    int pattern,
                    bool keep_file)
{
  _ext->rulesGen(name.c_str(),
                 dir.c_str(),
                 file.c_str(),
                 pattern,
                 write_to_solver,
                 read_from_solver,
                 run_solver,
                 keep_file);

  return TCL_OK;
}

bool Ext::metal_rules_gen(const std::string& name,
                          const std::string& dir,
                          const std::string& file,
                          bool write_to_solver,
                          bool read_from_solver,
                          bool run_solver,
                          int pattern,
                          bool keep_file,
                          int metal)
{
  _ext->metRulesGen(name.c_str(),
                    dir.c_str(),
                    file.c_str(),
                    pattern,
                    write_to_solver,
                    read_from_solver,
                    run_solver,
                    keep_file,
                    metal);
  return TCL_OK;
}

bool Ext::write_rules(const std::string& name,
                      const std::string& dir,
                      const std::string& file,
                      int pattern,
                      bool read_from_db,
                      bool read_from_solver)
{
  _ext->setBlockFromChip();
  _ext->writeRules(name.c_str(),
                   dir.c_str(),
                   file.c_str(),
                   pattern,
                   read_from_db,
                   read_from_solver);
  return TCL_OK;
}

bool Ext::get_ext_metal_count(int& metal_count)
{
  extRCModel* m = _ext->getRCModel();
  metal_count = m->getLayerCnt();
  return TCL_OK;
}

bool Ext::bench_net(const std::string& dir,
                    int net,
                    bool write_to_solver,
                    bool read_from_solver,
                    bool run_solver,
                    int max_track_count)
{
  extMainOptions opt;

  opt._write_to_solver = write_to_solver;
  opt._read_from_solver = read_from_solver;
  opt._run_solver = run_solver;

  int netId = net;
  opt._topDir = dir.c_str();

  if (netId == 0) {
    logger_->info(RCX, 144, "Net (={}), should be a positive number", netId);
    return TCL_OK;
  }
  logger_->info(
      RCX, 145, "Benchmarking using 3d field solver net {}...", netId);
  logger_->info(RCX, 146, "Finished 3D field solver benchmarking.");

  return TCL_OK;
}

bool Ext::run_solver(const std::string& dir, int net, int shape)
{
  extMainOptions opt;
  opt._topDir = dir.c_str();
  uint netId = net;
  int shapeId = shape;
  _ext->runSolver(&opt, netId, shapeId);
  return TCL_OK;
}

bool Ext::bench_wires(const BenchWiresOptions& bwo)
{
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

bool Ext::bench_verilog(const std::string& file)
{
  _ext->setBlockFromChip();
  char* filename = (char*) file.c_str();
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

bool Ext::clean(bool all_models, bool ext_only)
{
  return TCL_OK;
}

bool Ext::define_process_corner(int ext_model_index, const std::string& name)
{
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
                                float res_factor,
                                float cc_factor,
                                float gndc_factor)
{
  if (process_corner_name.empty()) {
    logger_->warn(RCX, 30, "The original process corner name is required");
    return TCL_ERROR;
  }

  int model = _ext->getDbCornerModel(process_corner_name.c_str());

  char* cornerName = _ext->addRCCornerScaled(
      name.c_str(), model, res_factor, cc_factor, gndc_factor);

  if (cornerName != NULL) {
    logger_->info(RCX, 31, "Defined Derived extraction corner {}", cornerName);
    return TCL_OK;
  } else {
    return TCL_ERROR;
  }
}

bool Ext::delete_corners()
{
  _ext->deleteCorners();
  return TCL_OK;
}

bool Ext::get_corners(std::list<std::string>& corner_list)
{
  _ext->getCorners(corner_list);
  return TCL_OK;
}

bool Ext::get_ext_db_corner(int& index, const std::string& name)
{
  index = _ext->getDbCornerIndex(name.c_str());

  if (index < 0)
    logger_->warn(RCX, 148, "Extraction corner {} not found!", name.c_str());

  return TCL_OK;
}

bool Ext::flatten(odb::dbBlock* block, bool spef)
{
  if (block == NULL) {
    logger_->error(RCX, 486, "No block for flatten command");
  }
  _ext->addRCtoTop(block, spef);
  return TCL_OK;
}

bool Ext::extract(ExtractOptions opts)
{
  _ext->setBlockFromChip();
  odb::dbBlock* block = _ext->getBlock();
  logger_->info(
      RCX, 8, "extracting parasitics of {} ...", block->getConstName());

  odb::orderWires(block, false /* force */);
  if (opts.lef_rc) {
    if (!_ext->checkLayerResistance())
      return TCL_ERROR;
    _ext->addExtModel();
    logger_->info(RCX, 375, "Using LEF RC values to extract!");
  }

  _ext->set_debug_nets(opts.debug_net);
  _ext->skip_via_wires(opts.skip_via_wires);
  _ext->skip_via_wires(true);
  _ext->_lef_res = opts.lef_res;

  if (_ext->makeBlockRCsegs(opts.cmp_file,
                            opts.wire_density,
                            opts.net,
                            opts.cc_up,
                            opts.cc_model,
                            opts.max_res,
                            !opts.no_merge_via_res,
                            !opts.no_gs,
                            opts.coupling_threshold,
                            opts.context_depth,
                            opts.ext_model_file,
                            this)
      == 0)
    return TCL_ERROR;

  if (opts.write_total_caps) {
    char netcapfile[500];
    sprintf(netcapfile, "%s.totCap", _ext->getBlock()->getConstName());
    _ext->reportTotalCap(netcapfile, true, false, 1.0, NULL, NULL);
  }

  logger_->info(
      RCX, 15, "Finished extracting {}.", _ext->getBlock()->getName().c_str());
  return 0;
}

bool Ext::adjust_rc(float res_factor, float cc_factor, float gndc_factor)
{
  _ext->adjustRC(res_factor, cc_factor, gndc_factor);
  return 0;
}

bool Ext::init_incremental_spef(const std::string& origp,
                                const std::string& newp,
                                bool no_backslash,
                                const std::string& exclude_cells)
{
  _ext->initIncrementalSpef(
      origp.c_str(), newp.c_str(), exclude_cells.c_str(), no_backslash);
  return 0;
}

bool Ext::write_spef_nets(odb::dbObject* block,
                          bool flatten,
                          bool parallel,
                          int corner)
{
  _ext->setBlockFromChip();
  _ext->write_spef_nets(flatten, parallel);

  return 0;
}

bool Ext::write_spef(const SpefOptions& opts)
{
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
  _ext->writeSPEF((char*) opts.file,
                  (char*) opts.nets,
                  useIds,
                  opts.no_name_map,
                  (char*) opts.N,
                  opts.term_junction_xy,
                  opts.exclude_cells,
                  opts.cap_units,
                  opts.res_units,
                  opts.gz,
                  stop,
                  opts.w_clock,
                  opts.w_conn,
                  opts.w_cap,
                  opts.w_cc_cap,
                  opts.w_res,
                  opts.no_c_num,
                  initOnly,
                  opts.single_pi,
                  opts.no_backslash,
                  opts.corner,
                  name,
                  opts.flatten,
                  opts.parallel);

  logger_->info(RCX, 17, "Finished writing SPEF ...");
  return 0;
}

bool Ext::independent_spef_corner()
{
  _ext->setUniqueExttreeCorner();
  return 0;
}

bool Ext::read_spef(ReadSpefOpts& opt)
{
  _ext->setBlockFromChip();
  logger_->info(RCX, 1, "Reading SPEF file: {}", opt.file);

  bool stampWire = opt.stamp_wire;
  bool useIds = opt.use_ids;
  uint testParsing = opt.test_parsing;

  Ath__parser parser;
  char* filename = (char*) opt.file;
  if (!filename || !filename[0]) {
    logger_->error(RCX, 2, "Filename is not defined!");
  }
  parser.mkWords(filename);

  _ext->readSPEF(parser.get(0),
                 (char*) opt.net,
                 opt.force,
                 useIds,
                 opt.r_conn,
                 (char*) opt.N,
                 opt.r_cap,
                 opt.r_cc_cap,
                 opt.r_res,
                 opt.cc_threshold,
                 opt.cc_ground_factor,
                 opt.length_unit,
                 opt.m_map,
                 opt.no_cap_num_collapse,
                 (char*) opt.cap_node_map_file,
                 opt.log,
                 opt.corner,
                 0.0,
                 0.0,
                 NULL,
                 NULL,
                 NULL,
                 opt.db_corner_name,
                 opt.calibrate_base_corner,
                 opt.spef_corner,
                 opt.fix_loop,
                 opt.keep_loaded_corner,
                 stampWire,
                 testParsing,
                 opt.more_to_read,
                 false /*diff*/,
                 false /*calibrate*/,
                 opt.app_print_limit);

  for (int ii = 1; ii < parser.getWordCnt(); ii++)
    _ext->readSPEFincr(parser.get(ii));

  return 0;
}

bool Ext::diff_spef(const DiffOptions& opt)
{
  _ext->setBlockFromChip();
  std::string filename(opt.file);
  if (filename.empty()) {
    logger_->error(
        RCX, 380, "Filename is not defined to run diff_spef command!");
  }
  logger_->info(RCX, 19, "diffing spef {}", opt.file);

  Ath__parser parser;
  parser.mkWords(opt.file);

  _ext->readSPEF(parser.get(0),
                 (char*) opt.net,
                 false /*force*/,
                 opt.use_ids,
                 opt.r_conn,
                 NULL /*N*/,
                 opt.r_cap,
                 opt.r_cc_cap,
                 opt.r_res,
                 -1.0,
                 0.0 /*cc_ground_factor*/,
                 1.0 /*length_unit*/,
                 opt.m_map,
                 false /*noCapNumCollapse*/,
                 NULL /*capNodeMapFile*/,
                 opt.log,
                 opt.ext_corner,
                 opt.low_guard,
                 opt.upper_guard,
                 (char*) opt.exclude_net_subword,
                 (char*) opt.net_subword,
                 (char*) opt.rc_stats_file,
                 (char*) opt.db_corner_name,
                 NULL,
                 opt.spef_corner,
                 0 /*fix_loop*/,
                 false /*keepLoadedCorner*/,
                 false /*stampWire*/,
                 opt.test_parsing,
                 false /*moreToRead*/,
                 true /*diff*/,
                 false /*calibrate*/,
                 0);

  return 0;
}

bool Ext::calibrate(const std::string& spef_file,
                    const std::string& db_corner_name,
                    int corner,
                    int spef_corner,
                    bool m_map,
                    float upper_limit,
                    float lower_limit)
{
  if (spef_file.empty())
    logger_->error(RCX,
                   381,
                   "Filename for calibration is not defined. Define the "
                   "filename using -spef_file");

  logger_->info(RCX, 21, "calibrate on spef file  {}", spef_file.c_str());
  Ath__parser parser;
  parser.mkWords((char*) spef_file.c_str());
  _ext->calibrate(parser.get(0),
                  m_map,
                  upper_limit,
                  lower_limit,
                  db_corner_name.c_str(),
                  corner,
                  spef_corner);
  return 0;
}

bool Ext::count(bool signal_wire_seg, bool power_wire_seg)
{
  _ext->extCount(signal_wire_seg, power_wire_seg);

  return 0;
}

bool Ext::rc_tree(float max_cap,
                  uint test,
                  int net,
                  const std::string& print_tag)
{
  int netId = net;
  char* printTag = (char*) print_tag.c_str();

  odb::dbBlock* block = _ext->getBlock();

  if (_tree == NULL)
    _tree = new extRcTree(block, logger_);

  uint cnt;
  if (netId > 0)
    _tree->makeTree((uint) netId,
                    max_cap,
                    test,
                    true,
                    true,
                    cnt,
                    1.0 /*mcf*/,
                    printTag,
                    false /*for_buffering*/);
  else
    _tree->makeTree(max_cap, test);

  return TCL_OK;
}
bool Ext::net_stats(std::list<int>& net_ids,
                    const std::string& tcap,
                    const std::string& ccap,
                    const std::string& ratio_cap,
                    const std::string& res,
                    const std::string& len,
                    const std::string& met_cnt,
                    const std::string& wire_cnt,
                    const std::string& via_cnt,
                    const std::string& seg_cnt,
                    const std::string& term_cnt,
                    const std::string& bterm_cnt,
                    const std::string& file,
                    const std::string& bbox,
                    const std::string& branch_len)
{
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
  int n = _ext->printNetStats(
      fp, block, &limits, skipRC, skipDb, skipPower, &list_of_nets);
  logger_->info(RCX, 26, "{} nets found", n);

  net_ids = list_of_nets;

  return TCL_OK;
}

}  // namespace rcx
