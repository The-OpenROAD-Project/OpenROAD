// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rcx/ext.h"

#include <cstdint>
#include <cstdio>
#include <list>
#include <string>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/wOrder.h"
#include "parse.h"
#include "rcx/extMeasureRC.h"
#include "rcx/extModelGen.h"
#include "rcx/extPattern.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

using utl::Logger;
using utl::RCX;

Ext::Ext(odb::dbDatabase* db, Logger* logger, const char* spef_version)
    : _db(db), _ext(new extMain()), logger_(logger), spef_version_(spef_version)
{
  _ext->init(db, logger);
}

Ext::~Ext()
{
  delete _ext;
}

void Ext::setLogger(Logger* logger)
{
  if (!logger_) {
    logger_ = logger;
  }
}
void Ext::write_rules(const std::string& name, const std::string& file)
{
  _ext->setBlockFromChip();
  _ext->writeRules(name.c_str(), file.c_str());
}
void Ext::bench_wires(const BenchWiresOptions& bwo)
{
  extMainOptions opt;

  opt._v1 = bwo.v1;
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

  opt._multiple_widths = bwo.multiple_widths;

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

  Parser parser(logger_);

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
  // _ext->benchWires(&opt);
  if (opt._gen_def_patterns && opt._v1) {  // New patterns v1=true
    _ext->DefWires(&opt);
  } else {
    _ext->benchWires(&opt);
  }
}
void Ext::bench_wires_gen(const PatternOptions& opt)
{
  // printf("%s\n", opt.name);
  _ext->benchPatternsGen(opt);
}

void Ext::bench_verilog(const std::string& file)
{
  _ext->setBlockFromChip();
  char* filename = (char*) file.c_str();
  if (!filename || !filename[0]) {
    logger_->error(RCX, 147, "bench_verilog: file is not defined!");
  }
  FILE* fp = fopen(filename, "w");
  if (fp == nullptr) {
    logger_->error(RCX, 378, "Can't open file {}", filename);
  }
  _ext->benchVerilog(fp);
}

void Ext::define_process_corner(int ext_model_index, const std::string& name)
{
  _ext->setBlockFromChip();
  char* cornerName = _ext->addRCCorner(name.c_str(), ext_model_index);

  if (cornerName != nullptr) {
    logger_->info(RCX, 29, "Defined extraction corner {}", cornerName);
  }
}

void Ext::define_derived_corner(const std::string& name,
                                const std::string& process_corner_name,
                                float res_factor,
                                float cc_factor,
                                float gndc_factor)
{
  if (process_corner_name.empty()) {
    logger_->error(RCX, 30, "The original process corner name is required");
  }

  int model = _ext->getDbCornerModel(process_corner_name.c_str());

  char* cornerName = _ext->addRCCornerScaled(
      name.c_str(), model, res_factor, cc_factor, gndc_factor);

  if (cornerName != nullptr) {
    logger_->info(RCX, 31, "Defined Derived extraction corner {}", cornerName);
  }
}

void Ext::delete_corners()
{
  _ext->deleteCorners();
}

void Ext::get_corners(std::list<std::string>& corner_list)
{
  _ext->getCorners(corner_list);
}

void Ext::get_ext_db_corner(int& index, const std::string& name)
{
  index = _ext->getDbCornerIndex(name.c_str());

  if (index < 0) {
    logger_->warn(RCX, 148, "Extraction corner {} not found!", name.c_str());
  }
}

void Ext::extract(ExtractOptions options)
{
  _ext->setBlockFromChip();
  odb::dbBlock* block = _ext->getBlock();

  odb::orderWires(logger_, block);

  _ext->set_debug_nets(options.debug_net);

  _ext->_lef_res = options.lef_res;
  if (options.lef_rc) {
    if (!_ext->checkLayerResistance()) {
      return;
    }
    logger_->info(RCX, 375, "Using LEF RC values to extract!");
  }
  _ext->setExtractionOptions_v2(options);

  if (_ext->_v2) {
    _ext->makeBlockRCsegs_v2(options.net, options.ext_model_file);
  } else {
    _ext->makeBlockRCsegs(options.net,
                          options.cc_up,
                          options.cc_model,
                          options.max_res,
                          !options.no_merge_via_res,
                          options.coupling_threshold,
                          options.context_depth,
                          options.ext_model_file);
  }
}

void Ext::adjust_rc(float res_factor, float cc_factor, float gndc_factor)
{
  _ext->adjustRC(res_factor, cc_factor, gndc_factor);
}

void Ext::write_spef_nets(odb::dbObject* block,
                          bool flatten,
                          bool parallel,
                          int corner)
{
  _ext->setBlockFromChip();
  _ext->write_spef_nets(flatten, parallel);
}

void Ext::write_spef(const SpefOptions& options)
{
  _ext->setBlockFromChip();
  if (options.end) {
    _ext->writeSPEF(true);
    return;
  }
  const char* name = options.ext_corner_name;

  uint32_t netId = options.net_id;
  if (netId > 0) {
    _ext->writeSPEF(netId,
                    options.single_pi,
                    options.debug,
                    options.corner,
                    name,
                    spef_version_);
    return;
  }
  _ext->writeSPEF((char*) options.file,
                  (char*) options.nets,
                  options.no_name_map,
                  (char*) options.N,
                  options.term_junction_xy,
                  options.cap_units,
                  options.res_units,
                  options.gz,
                  options.stop_after_map,
                  options.w_clock,
                  options.w_conn,
                  options.w_cap,
                  options.w_cc_cap,
                  options.w_res,
                  options.no_c_num,
                  false,
                  options.single_pi,
                  options.no_backslash,
                  options.corner,
                  name,
                  spef_version_,
                  options.parallel);
}

void Ext::read_spef(ReadSpefOpts& opt)
{
  _ext->setBlockFromChip();
  logger_->info(RCX, 1, "Reading SPEF file: {}", opt.file);

  bool stampWire = opt.stamp_wire;
  uint32_t testParsing = opt.test_parsing;

  Parser parser(logger_);
  char* filename = (char*) opt.file;
  if (!filename || !filename[0]) {
    logger_->error(RCX, 2, "Filename is not defined!");
  }
  parser.mkWords(filename);

  _ext->readSPEF(parser.get(0),
                 (char*) opt.net,
                 opt.force,
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
                 nullptr,
                 nullptr,
                 nullptr,
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

  for (int ii = 1; ii < parser.getWordCnt(); ii++) {
    _ext->readSPEFincr(parser.get(ii));
  }
}

void Ext::diff_spef(const DiffOptions& opt)
{
  _ext->setBlockFromChip();
  std::string filename(opt.file);
  if (filename.empty()) {
    logger_->error(
        RCX, 380, "Filename is not defined to run diff_spef command!");
  }
  logger_->info(RCX, 19, "diffing spef {}", opt.file);

  Parser parser(logger_);
  parser.mkWords(opt.file);

  _ext->readSPEF(parser.get(0),
                 (char*) opt.net,
                 false /*force*/,
                 opt.r_conn,
                 nullptr /*N*/,
                 opt.r_cap,
                 opt.r_cc_cap,
                 opt.r_res,
                 -1.0,
                 0.0 /*cc_ground_factor*/,
                 1.0 /*length_unit*/,
                 opt.m_map,
                 false /*noCapNumCollapse*/,
                 nullptr /*capNodeMapFile*/,
                 opt.log,
                 opt.ext_corner,
                 opt.low_guard,
                 opt.upper_guard,
                 (char*) opt.exclude_net_subword,
                 (char*) opt.net_subword,
                 (char*) opt.rc_stats_file,
                 (char*) opt.db_corner_name,
                 nullptr,
                 opt.spef_corner,
                 0 /*fix_loop*/,
                 false /*keepLoadedCorner*/,
                 false /*stampWire*/,
                 opt.test_parsing,
                 false /*moreToRead*/,
                 true /*diff*/,
                 false /*calibrate*/,
                 0);
}

void Ext::calibrate(const std::string& spef_file,
                    const std::string& db_corner_name,
                    int corner,
                    int spef_corner,
                    bool m_map,
                    float upper_limit,
                    float lower_limit)
{
  if (spef_file.empty()) {
    logger_->error(RCX,
                   381,
                   "Filename for calibration is not defined. Define the "
                   "filename using -spef_file");
  }
  logger_->info(RCX, 21, "calibrate on spef file  {}", spef_file.c_str());
  Parser parser(logger_);
  parser.mkWords((char*) spef_file.c_str());
  _ext->calibrate(parser.get(0),
                  m_map,
                  upper_limit,
                  lower_limit,
                  db_corner_name.c_str(),
                  corner,
                  spef_corner);
}

bool Ext::gen_rcx_model(const std::string& spef_file_list,
                        const std::string& corner_list,
                        const std::string& out_file,
                        const std::string& comment,
                        const std::string& version,
                        int pattern)
{
  _ext->setBlockFromChip();

  if (spef_file_list.empty()) {
    logger_->error(
        RCX, 144, "\nSpef List option -spef_file_list is required\n");
  }
  if (corner_list.empty()) {
    logger_->error(
        RCX, 145, "\nCorner List option -corner_list  is required\n");
  }

  Parser parser(logger_);
  int n = parser.mkWords(spef_file_list.c_str());

  std::list<std::string> file_list;
  for (int ii = 0; ii < n; ii++) {
    std::string name(parser.get(ii));
    file_list.push_back(name);
  }
  int n1 = parser.mkWords(corner_list.c_str());
  if (n != n1) {
    logger_->error(RCX, 150, "\nMismatch of number Corners and Spef Files\n");
  }

  std::list<std::string> corners_list;
  for (int ii = 0; ii < n; ii++) {
    std::string name(parser.get(ii));
    corners_list.push_back(name);
  }
  _ext->GenExtModel(file_list,
                    corners_list,
                    out_file.c_str(),
                    comment.c_str(),
                    version.c_str(),
                    pattern);
  return true;
}
bool Ext::define_rcx_corners(const std::string& corner_list)
{
  if (corner_list.empty()) {
    logger_->error(
        RCX, 146, "\nCorner List option -corner_list  is required\n");
  }

  _ext->setBlockFromChip();

  Parser parser(logger_);
  int n1 = parser.mkWords(corner_list.c_str());
  for (int ii = 0; ii < n1; ii++) {
    const char* name = parser.get(ii);
    // char *cornerName = _ext->addRCCorner(name, ii);
    _ext->addRCCorner(name, ii);
  }
  return true;
}
bool Ext::rc_estimate(const std::string& ext_model_file,
                      const std::string& out_file_prefix)
{
  extRCModel* m = new extRCModel("MINTYPMAX", nullptr);

  double version = 0.0;
  std::list<std::string> corner_list
      = extModelGen::GetCornerNames(ext_model_file.c_str(), version, logger_);
  const auto extDbCnt = corner_list.size();

  uint32_t cornerTable[10];
  for (uint32_t ii = 0; ii < extDbCnt; ii++) {
    cornerTable[ii] = ii;
  }

  odb::dbTech* tech = _db->getTech();

  int dbunit = tech->getDbUnitsPerMicron();
  double dbFactor = 1;
  if (dbunit > 1000) {
    dbFactor = dbunit * 0.001;
  }

  if (!(m->readRules((char*) ext_model_file.c_str(),
                     false,
                     true,
                     true,
                     true,
                     true,
                     extDbCnt,
                     cornerTable,
                     dbFactor))) {
    fprintf(stderr, "Failed to parse %s\n", ext_model_file.c_str());
  }
  char buff[1000];
  sprintf(buff, "%s.estimate.wire.rc", out_file_prefix.c_str());
  m->calcMinMaxRC(tech, buff);

  delete m;

  return true;
}
bool Ext::get_model_corners(const std::string& ext_model_file, Logger* logger)
{
  double version = 0.0;
  std::list<std::string> corner_list
      = extModelGen::GetCornerNames(ext_model_file.c_str(), version, logger);
  // out_args->corner_list(corner_list);

  std::list<std::string>::iterator it;
  uint32_t cnt = 0;
  // notice(0, "List of Corners (%d) -- Model Version %g\n", corner_list.size(),
  // version);

  fprintf(stdout,
          "List of Corners (%d) -- Model Version %g\n",
          (int) corner_list.size(),
          version);
  for (it = corner_list.begin(); it != corner_list.end(); ++it) {
    const std::string& str = *it;
    // notice(0, "\t%d %s\n", cnt++, str.c_str())
    fprintf(stdout, "\t%d %s\n", cnt++, str.c_str());
  }
  return true;
}
}  // namespace rcx
