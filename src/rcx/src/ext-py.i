// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "rcx/ext.h"

using namespace odb;
using rcx::Ext;
%}

%include <std_string.i>
%include "../../Exception-py.i"
// Python doesn't supported nested classes so this is
// required to make them available.
%feature ("flatnested");
%include "rcx/ext.h"
%include "rcx/ext_options.h"

// Just reuse the api defined in ext.i
%inline %{

void define_process_corner(int ext_model_index, const char* file);

void extract(const char* ext_model_file,
        int corner_cnt,
        double max_res,
        float coupling_threshold,
        int cc_model,
        int context_depth,
        const char* debug_net_id,
        bool lef_res,
        bool no_merge_via_res,
        bool  lef_rc,
        bool skip_over_cell,
        float version,
        int corner,
        int dbg
        );
void
write_spef(const char* file,
           const char* nets,
           int net_id,
           bool coordinates);
           
void adjust_rc(double res_factor,
               double cc_factor,
               double gndc_factor);

void diff_spef(const char* file,
          bool r_conn,
          bool r_res,
          bool r_cap,
          bool r_cc_cap,
          int spef_corner,
          int ext_corner
          );

void bench_wires(bool db_only,
            bool over,
            bool diag,
            bool all,
            int met_cnt,
            int cnt,
            int len,
            int under_met,
            const char* w_list,
            const char* s_list,
            int over_dist,
            int under_dist,
            bool v1);

void bench_verilog(const char* file);

void write_rules(const char* file,
            const char* name);

void read_spef(const char* file);

%}


