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

#pragma once

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
  bool v1 = false;
  bool ddd = false;
  bool multiple_widths = false;
  bool diag = false;
  bool over_under = false;
  bool gen_def_patterns = false;
  bool resPatterns = false;
};

struct ExtractOptions
{
  const char* debug_net = nullptr;
  const char* ext_model_file = nullptr;
  const char* net = nullptr;
  int cc_up = 2;
  int corner_cnt = 1;
  double max_res = 50.0;
  bool no_merge_via_res = false;
  int corner = -1;  // all  corners in model file
  float coupling_threshold = 0.1;
  int context_depth = 5;
  int cc_model = 10;
  int signal_table = 3;
  bool over_cell = false;

  bool skip_via_wires = false;

  bool lef_rc = false;
  bool lef_res = false;
  bool rlog = false;
  bool _v2 = false;
  float _version = 2.2;
  int _wire_extracted_progress_count = 50000;

  int _dbg = 0;
};

struct SpefOptions
{
  const char* nets = nullptr;
  int net_id = 0;

  const char* ext_corner_name = nullptr;
  int corner = -1;
  const int debug = 0;
  const bool parallel = false;
  const bool init = false;
  const bool end = false;
  const bool use_ids = false;
  const bool no_name_map = false;
  const char* N = nullptr;
  const bool term_junction_xy = false;
  const bool single_pi = false;
  const char* file = nullptr;
  const bool gz = false;
  const bool stop_after_map = false;
  const bool w_clock = false;
  const bool w_conn = false;
  const bool w_cap = false;
  const bool w_cc_cap = false;
  const bool w_res = false;
  const bool no_c_num = false;
  const bool no_backslash = false;
  const char* cap_units = "PF";
  const char* res_units = "OHM";
  bool coordinates = false;
};

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

struct DiffOptions
{
  const char* net = nullptr;
  bool use_ids = false;
  bool test_parsing = false;
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
struct PatternOptions
{
  const char* name = "blk";
  int over_dist = 1000;
  int under_dist = 1000;
  int met_cnt = 1000;
  int met = -1;
  int over_met = -1;
  int under_met = -1;

  // Target Wire
  const char* width;
  const char* spacing;
  const char* couple_width;
  const char* couple_spacing;
  const char* far_width = "1";
  const char* far_spacing = "1";
  // over context
  const char* over_width;
  const char* over_spacing;
  const char* over2_width;
  const char* over2_spacing;
  // under context
  const char* under_width;
  const char* under_spacing;
  const char* under2_width;
  const char* under2_spacing;
  int dbg;
  int wire_cnt;
  const char* mlist;
  int len;
  const char* offset_over;
  const char* offset_under;

  const char* grid_list = "";
  bool default_lef_rules = false;
  bool nondefault_lef_rules = false;
  const char* dir = "./Bench";
  bool over = false;
  bool ddd = false;

  bool diag = false;
  bool over_under = false;
  bool under = false;
};
