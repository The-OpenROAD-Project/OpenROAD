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

#ifndef ADS_EXT_H
#define ADS_EXT_H

#include <tcl.h>

#include "extRCap.h"
#include "exttree.h"

namespace utl {
class Logger;
}

namespace rcx {

using utl::Logger;
class Ext

#ifndef SWIG  // causes swig warnings
    : public odb::ZInterface
#endif
{
 public:
  Ext();
  ~Ext();

  void init(Tcl_Interp* tcl_interp, odb::dbDatabase* db, Logger* logger);
  void setLogger(Logger* logger);

  bool load_model(const std::string& name,
                  bool lef_rc,
                  const std::string& file,
                  int setMin,
                  int setTyp,
                  int setMax);
  bool read_process(const std::string& name, const std::string& file);
  bool rules_gen(const std::string& name,
                 const std::string& dir,
                 const std::string& file,
                 bool write_to_solver,
                 bool read_from_solver,
                 bool run_solver,
                 int pattern,
                 bool keep_file);
  bool metal_rules_gen(const std::string& name,
                       const std::string& dir,
                       const std::string& file,
                       bool write_to_solver,
                       bool read_from_solver,
                       bool run_solver,
                       int pattern,
                       bool keep_file,
                       int metal);
  bool write_rules(const std::string& name,
                   const std::string& dir,
                   const std::string& file,
                   int pattern,
                   bool read_from_db,
                   bool read_from_solver);
  bool get_ext_metal_count(int& metal_count);
  bool bench_net(const std::string& dir,
                 int net,
                 bool write_to_solver,
                 bool read_from_solver,
                 bool run_solver,
                 int max_track_count);
  bool bench_verilog(const std::string& file);
  bool run_solver(const std::string& dir, int net, int shape);

  struct BenchWiresOptions
  {
    const char* block = "blk";
    int over_dist = 100;
    int under_dist = 100;
    int met_cnt = 1000;
    int met = -1;
    int over_met = -1;
    int under_met = -1;
    int len = 200;
    int cnt = 5;
    const char* w = "1";
    const char* s = "1";
    const char* th = "1";
    const char* d = "0.0";
    const char* w_list = "1";
    const char* s_list = "1 2 2.5 3 3.5 4 4.5 5 6 8 10 12";
    const char* th_list = "0 1 2 2.5 3 3.5 4 4.5 5 6 8 10 12";
    const char* grid_list = "";
    bool default_lef_rules = false;
    bool nondefault_lef_rules = false;
    const char* dir = "./Bench";
    bool Over = false;
    bool db_only = false;
    bool ddd = false;
    bool multiple_widths = false;
    bool write_to_solver = false;
    bool read_from_solver = false;
    bool run_solver = false;
    bool diag = false;
    bool over_under = false;
    bool gen_def_patterns = false;
    bool resPatterns = false;
  };

  bool bench_wires(const BenchWiresOptions& bwo);
  bool assembly(odb::dbBlock* block, odb::dbBlock* main_block);
  bool write_spef_nets(odb::dbObject* block,
                       bool flatten,
                       bool parallel,
                       int corner);
  bool flatten(odb::dbBlock* block, bool spef);

  struct ExtractOptions
  {
    bool min = false;
    bool max = false;
    bool typ = false;
    int set_min = -1;
    int set_typ = -1;
    int set_max = -1;
    bool litho = false;
    bool wire_density = false;
    const char* debug_net = nullptr;
    const char* cmp_file = nullptr;
    const char* ext_model_file = nullptr;
    const char* net = nullptr;
    const char* bbox = nullptr;
    const char* ibox = nullptr;
    int test = 0;
    int cc_band_tracks = 1000;
    int signal_table = 3;
    int cc_up = 2;
    uint preserve_geom = 0;
    int corner_cnt = 1;
    double max_res = 50.0;
    bool no_merge_via_res = false;
    float coupling_threshold = 0.1;
    int context_depth = 5;
    int cc_model = 10;
    bool over_cell = false;
    bool remove_cc = false;
    bool remove_ext = false;
    bool unlink_ext = false;
    bool eco = false;
    bool no_gs = false;
    bool re_run = false;
    bool skip_via_wires = false;
    bool tile = false;
    int tiling = 0;
    bool skip_m1_caps = false;
    bool power_grid = false;
    bool write_total_caps = false;
    const char* exclude_cells = nullptr;
    bool skip_power_stubs = false;
    const char* power_source_coords = nullptr;
    bool lef_rc = false;
    bool lef_res = false;
    bool rlog = false;
  };

  bool extract(ExtractOptions options);

  bool define_process_corner(int ext_model_index, const std::string& name);
  bool define_derived_corner(const std::string& name,
                             const std::string& process_corner_name,
                             float res_factor,
                             float cc_factor,
                             float gndc_factor);
  bool get_ext_db_corner(int& index, const std::string& name);
  bool get_corners(std::list<std::string>& corner_list);
  bool delete_corners();
  bool clean(bool all_models, bool ext_only);
  bool adjust_rc(float res_factor, float cc_factor, float gndc_factor);

  bool init_incremental_spef(const std::string& origp,
                             const std::string& newp,
                             bool no_backslash,
                             const std::string& exclude_cells);
  struct SpefOptions
  {
    const char* nets = nullptr;
    int net_id = 0;
    const char* ext_corner_name = nullptr;
    int corner = -1;
    int debug = 0;
    bool flatten = false;
    bool parallel = false;
    bool init = false;
    bool end = false;
    bool use_ids = false;
    bool no_name_map = false;
    const char* N = nullptr;
    bool term_junction_xy = false;
    bool single_pi = false;
    const char* file = nullptr;
    bool gz = false;
    bool stop_after_map = false;
    bool w_clock = false;
    bool w_conn = false;
    bool w_cap = false;
    bool w_cc_cap = false;
    bool w_res = false;
    bool no_c_num = false;
    bool no_backslash = false;
    const char* exclude_cells = nullptr;
    const char* cap_units = "PF";
    const char* res_units = "OHM";
  };
  bool write_spef(const SpefOptions& options);

  bool independent_spef_corner();

  struct ReadSpefOpts
  {
    const char* file = nullptr;
    const char* net = nullptr;
    bool force = false;
    bool use_ids = false;
    bool keep_loaded_corner = false;
    bool stamp_wire = false;
    int test_parsing = 0;
    const char* N = nullptr;
    bool r_conn = false;
    bool r_cap = false;
    bool r_cc_cap = false;
    bool r_res = false;
    float cc_threshold = -0.5;
    float cc_ground_factor = 0;
    int app_print_limit = 0;
    int corner = -1;
    const char* db_corner_name = nullptr;
    const char* calibrate_base_corner = nullptr;
    int spef_corner = -1;
    bool m_map = false;
    bool more_to_read = false;
    float length_unit = 1;
    int fix_loop = 0;
    bool no_cap_num_collapse = false;
    const char* cap_node_map_file = nullptr;
    bool log = false;
  };

  bool read_spef(ReadSpefOpts& opt);

  struct DiffOptions
  {
    const char* net = nullptr;
    bool use_ids = false;
    bool test_parsing = 0;
    const char* file = nullptr;
    const char* db_corner_name = nullptr;
    int spef_corner = -1;
    const char* exclude_net_subword = nullptr;
    const char* net_subword = nullptr;
    const char* rc_stats_file = nullptr;
    bool r_conn = false;
    bool r_cap = false;
    bool r_cc_cap = false;
    bool r_res = false;
    int ext_corner = -1;
    float low_guard = 1;
    float upper_guard = -1;
    bool m_map = false;
    bool log = false;
  };

  bool diff_spef(const DiffOptions& opt);
  bool calibrate(const std::string& spef_file,
                 const std::string& db_corner_name,
                 int corner,
                 int spef_corner,
                 bool m_map,
                 float upper_limit,
                 float lower_limit);
  bool match(const std::string& spef_file,
             const std::string& db_corner_name,
             int corner,
             int spef_corner,
             bool m_map);
  bool set_block(const std::string& block_name,
                 odb::dbBlock* block,
                 const std::string& inst_name,
                 odb::dbInst* inst);
  bool report_total_cap(const std::string& file,
                        bool res_only,
                        bool cap_only,
                        float ccmult,
                        const std::string& ref,
                        const std::string& read);
  bool report_total_cc(const std::string& file,
                       const std::string& ref,
                       const std::string& read);

  bool export_sdb(odb::ZPtr<odb::ISdb>& net_sdb, odb::ZPtr<odb::ISdb>& cc_sdb);
  bool dump(bool open_tree_file,
            bool close_tree_file,
            bool cc_cap_geom,
            bool cc_net_geom,
            bool track_cnt,
            bool signal,
            bool power,
            int layer,
            const std::string& file);
  bool count(bool signal_wire_seg, bool power_wire_seg);
  bool read_qcap(const std::string& file_name,
                 const std::string& cap_file,
                 bool skip_bterms,
                 bool no_qcap,
                 const std::string& design);
  bool rc_tree(float max_cap, uint test, int net, const std::string& print_tag);
  bool net_stats(std::list<int>& net_ids,
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
                 const std::string& branch_len);

 private:
  odb::dbDatabase* _db;
  extMain* _ext;
  extRcTree* _tree;
  Logger* logger_;
};  // namespace rcx

}  // namespace rcx

#endif
