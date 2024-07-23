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

#include "odb/wOrder.h"
#include "utl/Logger.h"

namespace rcx {

using utl::Logger;
using utl::RCX;

Ext::Ext() : _ext(std::make_unique<extMain>())
{
}

void Ext::init(odb::dbDatabase* db,
               Logger* logger,
               const char* spef_version,
               const std::function<void()>& rcx_init)
{
  _db = db;
  logger_ = logger;
  spef_version_ = spef_version;
  _ext->init(db, logger);

  // Define swig TCL commands.
  rcx_init();
}

void Ext::setLogger(Logger* logger)
{
  if (!logger_) {
    logger_ = logger;
  }
}

void Ext::write_rules(const std::string& name,
                      const std::string& dir,
                      const std::string& file,
                      int pattern)
{
  _ext->setBlockFromChip();
  _ext->writeRules(name.c_str(), dir.c_str(), file.c_str(), pattern);
}

void Ext::bench_wires(const BenchWiresOptions& bwo)
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

  Ath__parser parser(logger_);

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

  _ext->makeBlockRCsegs(options.net,
                        options.cc_up,
                        options.cc_model,
                        options.max_res,
                        !options.no_merge_via_res,
                        options.coupling_threshold,
                        options.context_depth,
                        options.ext_model_file);
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

  uint netId = options.net_id;
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
  uint testParsing = opt.test_parsing;

  Ath__parser parser(logger_);
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

  Ath__parser parser(logger_);
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
  Ath__parser parser(logger_);
  parser.mkWords((char*) spef_file.c_str());
  _ext->calibrate(parser.get(0),
                  m_map,
                  upper_limit,
                  lower_limit,
                  db_corner_name.c_str(),
                  corner,
                  spef_corner);
}

}  // namespace rcx
