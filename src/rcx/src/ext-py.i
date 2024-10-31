/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
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

using namespace odb;
using rcx::Ext;
%}

%include <std_string.i>
%include "../../Exception-py.i"
%import "odb/odb.h"
// Python doesn't supported nested classes so this is
// required to make them available.
%feature ("flatnested");
%include "rcx/ext.h"

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
             bool no_merge_via_res);

void write_spef(const char* file, const char* nets, int net_id,
                bool write_coordinates);

void adjust_rc(double res_factor,
               double cc_factor,
               double gndc_factor);

void diff_spef(const char* file,
               bool r_conn,
               bool r_res,
               bool r_cap,
               bool r_cc_cap);

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
                 int under_dist);

void bench_verilog(const char* file);

void write_rules(const char* file,
                 const char* dir,
                 const char* name,
                 int pattern);

void read_spef(const char* file);

%}


