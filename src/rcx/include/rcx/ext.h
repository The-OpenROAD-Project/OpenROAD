// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <list>
#include <memory>
#include <string>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "rcx/extModelGen.h"
#include "rcx/extPattern.h"
#include "rcx/extRCap.h"
#include "rcx/ext_options.h"
#include "utl/Logger.h"

namespace rcx {

class Ext
{
 public:
  Ext(odb::dbDatabase* db, utl::Logger* logger, const char* spef_version);
  ~Ext();

  void setLogger(utl::Logger* logger);

  void bench_wires_gen(const PatternOptions& opt);

  bool gen_rcx_model(const std::string& spef_file_list,
                     const std::string& corner_list,
                     const std::string& out_file,
                     const std::string& comment,
                     const std::string& version,
                     int pattern);
  bool define_rcx_corners(const std::string& corner_list);
  static bool get_model_corners(const std::string& ext_model_file,
                                utl::Logger* logger);
  bool rc_estimate(const std::string& ext_model_file,
                   const std::string& out_file_prefix);

  // ---------------------------- dkf 092524 ---------------------------------
  bool gen_solver_patterns(const char* process_file,
                           const char* process_name,
                           int version,
                           int wire_cnt,
                           int len,
                           int over_dist,
                           int under_dist,
                           const char* w_list,
                           const char* s_list);

  // ---------------------------- dkf 092024 ---------------------------------
  bool init_rcx_model(const char* corner_names, int metal_cnt);
  bool read_rcx_tables(const char* corner,
                       const char* filename,
                       int wire,
                       bool over,
                       bool under,
                       bool over_under,
                       bool diag);
  bool write_rcx_model(const char* filename);
  void write_rules(const std::string& name, const std::string& file);
  void bench_verilog(const std::string& file);

  void bench_wires(const BenchWiresOptions& bwo);
  void write_spef_nets(odb::dbObject* block,
                       bool flatten,
                       bool parallel,
                       int corner);
  void extract(ExtractOptions options);

  void define_process_corner(int ext_model_index, const std::string& name);
  void define_derived_corner(const std::string& name,
                             const std::string& process_corner_name,
                             float res_factor,
                             float cc_factor,
                             float gndc_factor);
  void get_ext_db_corner(int& index, const std::string& name);
  void get_corners(std::list<std::string>& corner_list);
  void delete_corners();
  void adjust_rc(float res_factor, float cc_factor, float gndc_factor);

  void write_spef(const SpefOptions& options);

  void read_spef(ReadSpefOpts& opt);

  void diff_spef(const DiffOptions& opt);
  void calibrate(const std::string& spef_file,
                 const std::string& db_corner_name,
                 int corner,
                 int spef_corner,
                 bool m_map,
                 float upper_limit,
                 float lower_limit);

 private:
  odb::dbDatabase* _db = nullptr;
  // ::unique_ptr<extMain> _ext;
  extMain* _ext = nullptr;
  utl::Logger* logger_ = nullptr;
  const char* spef_version_ = nullptr;
};  // namespace rcx

}  // namespace rcx
