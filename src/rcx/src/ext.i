/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
///////////////////////////////////////////////////////////////////////////////

%{
#include "ord/OpenRoad.hh"
#include "rcx/ext.h"

namespace ord {
// Defined in OpenRoad.i
rcx::Ext*
getOpenRCX();

OpenRoad *
getOpenRoad();

}

using ord::getOpenRCX;
using rcx::Ext;
%}

%inline %{

void
define_process_corner(int ext_model_index, const char* file)
{
  Ext* ext = getOpenRCX();
  ext->define_process_corner(ext_model_index, file);
}

void
extract(const char* ext_model_file,
        int corner_cnt,
        double max_res,
        float coupling_threshold,
        int signal_table,
        int cc_model,
        int context_depth,
        const char* debug_net_id,
        bool lef_res,
        bool no_merge_via_res)
{
  Ext* ext = getOpenRCX();
  Ext::ExtractOptions opts;

  opts.ext_model_file = ext_model_file;
  opts.corner_cnt = corner_cnt;
  opts.max_res = max_res;
  opts.coupling_threshold = coupling_threshold;
  opts.signal_table = signal_table;
  opts.cc_model = cc_model;
  opts.context_depth = context_depth;
  opts.lef_res = lef_res;
  opts.debug_net = debug_net_id;
  opts.no_merge_via_res = no_merge_via_res;
  
  ext->extract(opts);
}

void
write_spef(const char* file,
           const char* nets,
           int net_id)
{
  Ext* ext = getOpenRCX();
  Ext::SpefOptions opts;
  opts.file = file;
  opts.nets = nets;
  opts.net_id = net_id;
  
  ext->write_spef(opts);
}

void
adjust_rc(double res_factor,
          double cc_factor,
          double gndc_factor)
{
  Ext* ext = getOpenRCX();
  ext->adjust_rc(res_factor, cc_factor, gndc_factor);
}

void
diff_spef(const char* file,
          bool r_conn,
          bool r_res,
          bool r_cap,
          bool r_cc_cap)
{
  Ext* ext = getOpenRCX();
  Ext::DiffOptions opts;
  opts.file = file;
  opts.r_res = r_res;
  opts.r_cap = r_cap;
  opts.r_cc_cap = r_cc_cap;
  opts.r_conn = r_conn;
  
  ext->diff_spef(opts);
}

void
bench_wires(bool db_only,
            bool over,
            bool diag,
            bool all,
            int met_cnt,
            int cnt,
            int len,
            int under_met,
            const char* w_list,
            const char* s_list)
{
  Ext* ext = getOpenRCX();
  Ext::BenchWiresOptions opts;
  
  opts.w_list = w_list;
  opts.s_list = s_list;
  opts.Over = over;
  opts.diag = diag;
  opts.gen_def_patterns = all;
  opts.cnt = cnt;
  opts.len = len;
  opts.under_met = under_met;
  opts.met_cnt = met_cnt;
  opts.db_only = db_only;

  ext->bench_wires(opts);
}

void
bench_verilog(const char* file)
{
  Ext* ext = getOpenRCX();
  ext->bench_verilog(file);
}

void
write_rules(const char* file,
            const char* dir,
            const char* name,
            int pattern,
            bool read_from_db,
            bool read_from_solver)
{
  Ext* ext = getOpenRCX();
  Ext::BenchWiresOptions opts;
  
  ext->write_rules(name, dir, file, pattern, read_from_db,
                   read_from_solver);
}

void 
read_spef(const char* file)
{
  Ext* ext = getOpenRCX();
  Ext::ReadSpefOpts opts;
  opts.file = file;
  
  ext->read_spef(opts);
}


%} // inline


