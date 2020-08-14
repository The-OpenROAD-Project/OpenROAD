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

#pragma once

namespace FastRoute {

// Help cmd
void help();

// Guides interface
void set_output_file(const char* file);

// Parms interface
void set_tile_size(int tileSize);
void set_capacity_adjustment(float adjustment);
void add_layer_adjustment(int layer, float reductionPercentage);
void add_region_adjustment(int minX,
                           int minY,
                           int maxX,
                           int maxY,
                           int layer,
                           float reductionPercentage);
void set_min_layer(int minLayer);
void set_max_layer(int maxLayer);
void set_unidirectional_routing(bool unidirRouting);
void set_alpha(float alpha);
void set_alpha_for_net(char* netName, float alpha);
void set_verbose(int v);
void set_overflow_iterations(int iterations);
void set_max_routing_length(float maxLength);
void add_layer_max_length(int layer, float length);
void set_grid_origin(long x, long y);
void set_pdrev_for_high_fanout(int pdRevForHighFanout);
void set_allow_overflow(bool allowOverflow);
void set_seed(unsigned seed);
void set_layer_pitch(int layer, float pitch);
void set_clock_nets_route_flow(bool clock_flow);
void set_min_layer_for_clock(int minLayer);
void set_estimate_rc();

// Run
void estimate_rc();
void enable_antenna_avoidance_flow(char* diodeCellName, char* diodePinName);
void start_fastroute();
void run_fastroute();
void reset_fastroute();
void write_guides();
void report_congestion(char* congest_file);

}  // namespace FastRoute
