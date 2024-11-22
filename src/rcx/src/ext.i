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
utl::Logger *
getLogger();

rcx::Ext*
getOpenRCX();

OpenRoad *
getOpenRoad();

}
using namespace rcx;
using ord::getLogger;
using ord::getOpenRCX;
using rcx::Ext;
%}

%include "../../Exception.i"
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
        )
{
  Ext* ext = getOpenRCX();
  ExtractOptions opts;

  opts.ext_model_file = ext_model_file;
  opts.corner_cnt = corner_cnt;
  opts.corner = corner;
  opts.max_res = max_res;
  opts.coupling_threshold = coupling_threshold;
  opts.signal_table = 3;
  opts.cc_model = cc_model;
  opts.context_depth = context_depth;
  opts.lef_res = lef_res;
  opts.lef_rc = lef_rc;
  opts.debug_net = debug_net_id;
  opts.no_merge_via_res = no_merge_via_res;
  opts.over_cell = !skip_over_cell;

  if (version>=2.0)
    opts._v2= true;
  opts._version= version;

  opts._dbg= dbg;
  
  ext->extract(opts);
}

void
write_spef(const char* file,
           const char* nets,
           int net_id,
           bool coordinates)
{
  Ext* ext = getOpenRCX();

  SpefOptions opts;

  opts.file = file;
  // opts.corner= corner;
  opts.nets = nets;
  opts.net_id = net_id;
  opts.coordinates= coordinates;
  
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
          bool r_cc_cap,
          int spef_corner,
          int ext_corner
          )
{
  Ext* ext = getOpenRCX();
  
  DiffOptions opts;
  opts.file = file;
  opts.r_res = r_res;
  opts.r_cap = r_cap;
  opts.r_cc_cap = r_cc_cap;
  opts.r_conn = r_conn;
  opts.spef_corner = spef_corner;
  opts.ext_corner = ext_corner;
  
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
            const char* s_list,
            int over_dist,
            int under_dist,
            bool v1)
{
  Ext* ext = getOpenRCX();
  
  BenchWiresOptions opts;
  
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
  opts.over_dist = over_dist;
  opts.under_dist = under_dist;
  opts.v1 = v1;

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
            const char* name)
{
  Ext* ext = getOpenRCX();
  
  ext->write_rules(name, file);
}

void
gen_rcx_model( const char *spef_file_list,
                    const char *corner_list,
                    const char *out_file,
                    const char *const comment,
                    const char *const version,
                    int pattern)
{
  Ext* ext = getOpenRCX();
  
  ext->gen_rcx_model(spef_file_list, corner_list, out_file, comment, version, pattern);
}
void
define_rcx_corners(const char *corner_list)
{
  Ext* ext = getOpenRCX();
  
  ext->define_rcx_corners(corner_list);
}
void
get_model_corners(const char *ext_model_file)
{
  Ext* ext = getOpenRCX();
  
  ext->get_model_corners(ext_model_file, getLogger());
}
void
rc_estimate(const char *ext_model_file, const char *out_file_prefix)
{
  Ext* ext = getOpenRCX();
  
  ext->rc_estimate(ext_model_file, out_file_prefix);
}
void 
read_spef(const char* file)
{
  Ext* ext = getOpenRCX();
  
  ReadSpefOpts opts;
  opts.file = file;
  
  ext->read_spef(opts);
}


void
bench_wires_gen(
const char* width,
const char* spacing,
const char* couple_width,
const char* couple_spacing,
const char* over_width,
const char* over_spacing,
const char* under_width,
const char* under_spacing,
const char* over2_width,
const char* over2_spacing,
const char* under2_width,
const char* under2_spacing,
int dbg,
int wire_cnt,
const char* mlist,
int len,
const char* offset_over,
const char* offset_under,
int under_dist,
int over_dist,
bool over,
bool under,
bool over_under,
int met
)
{
  Ext* ext = getOpenRCX();
  
  PatternOptions opts;

 opts.width= width;
opts.spacing= spacing;
opts.couple_width= couple_width;
opts.couple_spacing= couple_spacing;
opts.over_width= over_width;
opts.over_spacing= over_spacing;
opts.under_width= under_width;
opts.under_spacing= under_spacing;
opts.over2_width= over2_width;
opts.over2_spacing= over2_spacing;
opts.under2_width= under2_width;
opts.under2_spacing= under2_spacing;
opts.dbg= dbg;
opts.wire_cnt= wire_cnt;
opts.mlist= mlist;
opts.len= len;
opts.offset_over= offset_over;
opts.offset_under= offset_under;
opts.over_dist= over_dist;
opts.under_dist= under_dist;
opts.over= over;
opts.under= under;
opts.over_under= over_under;
opts.met= met;

ext->bench_wires_gen(opts);
}

// ------------------------------- dkf 09192024 -------------------------------
void
init_rcx_model(const char* corner_names, int metal_cnt)
{
  Ext* ext = getOpenRCX();
  ext->init_rcx_model(corner_names, metal_cnt);
}
void
read_rcx_tables(const char* corner, const char* filename, int wire, bool over, bool under, bool over_under, bool diag)
{
  Ext* ext = getOpenRCX();
  ext->read_rcx_tables(corner, filename, wire, over, under, over_under, diag);
}
void
write_rcx_model(const char* filename)
{
  Ext* ext = getOpenRCX();
  ext->write_rcx_model(filename);
}
// ------------------------------- dkf 09252024 -------------------------------
void gen_solver_patterns(const char *process_file, const char * process_name, int version, int wire_cnt, int len, int over_dist, int under_dist, const char* w_list, const char* s_list)
{
  Ext* ext = getOpenRCX();

  ext->gen_solver_patterns(process_file, process_name, version, wire_cnt, len, over_dist, under_dist, w_list, s_list);

}
%} // inline


