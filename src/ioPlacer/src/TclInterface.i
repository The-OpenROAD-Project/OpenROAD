/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
//
///////////////////////////////////////////////////////////////////////////////

%module ioplacer
%{
#include "TclInterface.h"
%}

namespace ioPlacer {

void import_lef(const char* file);
void import_def(const char* file);
extern void  set_hor_metal_layer(int layer);
extern int   get_hor_metal_layer();
extern void  set_ver_metal_layer(int layer);
extern int   get_ver_metal_layer();
extern void  set_num_slots(int numSlots);
extern int   get_num_slots();
extern void  set_random_mode(int mode);
extern int   get_random_mode();
extern void  set_slots_factor(float factor);
extern float get_slots_factor();
extern void  set_force_spread(bool force);
extern bool  get_force_spread();
extern void  set_usage(float usage);
extern float get_usage();
extern void  set_usage_factor(float factor);
extern float get_usage_factor();
extern void  set_blockages_file(const char* file);
extern const char* get_blockages_file();
extern void add_blocked_area(long long int llx, long long int lly, long long int urx, long long int ury);
extern float get_ver_length_extend();
extern float get_hor_length_extend();
extern void  set_hor_length_extend(float length);
extern void  set_hor_length(float length);
extern void  set_hor_thick_multiplier(float length);
extern void  set_ver_thick_multiplier(float length);
extern float get_hor_thick_multiplier();
extern float get_ver_thick_multiplier();
extern float get_hor_length();
extern void  set_ver_length_extend(float length);
extern void  set_ver_length(float length);
extern float get_ver_length();
extern void  set_report_hpwl(bool report);
extern bool  get_report_hpwl();
extern void  set_interactive_mode(bool enable);
extern bool  is_interactive_mode();
extern void print_all_parms();
extern void run_io_placement();
extern int compute_io_nets_hpwl();
extern void export_def(const char*);
extern void  set_num_threads(int numThreads);
extern int   get_num_threads();
extern void   set_rand_seed(double seed);
extern double get_rand_seed();
extern void set_boundaries_offset(int offset);
extern void set_min_distance(int minDist);

}
