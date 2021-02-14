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


%module fastroute

%{
#include "fastroute/GlobalRouter.h"
#include "openroad/OpenRoad.hh"
#include "sta/Liberty.hh"

namespace ord {
// Defined in OpenRoad.i
grt::GlobalRouter* getFastRoute();
}  // namespace ord

using ord::getFastRoute;
using sta::LibertyPort;
%}

%include "../../Exception.i"

%inline %{

bool
have_routes()
{
  return getFastRoute()->haveRoutes();
}

void
set_tile_size(int tileSize)
{
  getFastRoute()->setPitchesInTile(tileSize);
}

void
set_capacity_adjustment(float adjustment)
{
  getFastRoute()->setAdjustment(adjustment);
}

void
add_layer_adjustment(int layer, float reductionPercentage)
{
  getFastRoute()->addLayerAdjustment(layer, reductionPercentage);
}

void
add_region_adjustment(int minX,
                           int minY,
                           int maxX,
                           int maxY,
                           int layer,
                           float reductionPercentage)
{
  getFastRoute()->addRegionAdjustment(
      minX, minY, maxX, maxY, layer, reductionPercentage);
}

void
set_min_layer(int minLayer)
{
  getFastRoute()->setMinRoutingLayer(minLayer);
}

void
set_max_layer(int maxLayer)
{
  getFastRoute()->setMaxRoutingLayer(maxLayer);
}

void
set_unidirectional_routing(bool unidirRouting)
{
  getFastRoute()->setUnidirectionalRoute(unidirRouting);
}

void
set_alpha(float alpha)
{
  getFastRoute()->setAlpha(alpha);
}

void
set_alpha_for_net(char* netName, float alpha)
{
  getFastRoute()->addAlphaForNet(netName, alpha);
}

void
set_verbose(int v)
{
  getFastRoute()->setVerbose(v);
}

void
set_overflow_iterations(int iterations)
{
  getFastRoute()->setOverflowIterations(iterations);
}

void
set_grid_origin(long x, long y)
{
  getFastRoute()->setGridOrigin(x, y);
}

void
set_pdrev_for_high_fanout(int pdRevForHighFanout)
{
  getFastRoute()->setPDRevForHighFanout(pdRevForHighFanout);
}

void
set_allow_overflow(bool allowOverflow)
{
  getFastRoute()->setAllowOverflow(allowOverflow);
}

void
set_seed(unsigned seed)
{
  getFastRoute()->setSeed(seed);
}

void
set_layer_pitch(int layer, float pitch)
{
  getFastRoute()->setLayerPitch(layer, pitch);
}

void
set_clock_layer_range(int minLayer, int maxLayer)
{
  getFastRoute()->setMinRoutingLayer(minLayer);
  getFastRoute()->setMaxRoutingLayer(maxLayer);
}

void set_clock_cost(int cost)
{
  getFastRoute()->setClockCost(cost);
}

void
set_macro_extension(int macroExtension)
{
  getFastRoute()->setMacroExtension(macroExtension);
}

void
run_fastroute(bool onlySignal)
{
  getFastRoute()->runFastRoute(onlySignal);
}

void
estimate_rc()
{
  getFastRoute()->estimateRC();
}

void
route_clock_nets()
{
  getFastRoute()->routeClockNets();
}

void
repair_antennas(LibertyPort* diodePort)
{
  getFastRoute()->repairAntennas(diodePort);
}

void
clear_fastroute()
{
  getFastRoute()->clear();
}

void
write_guides(char* fileName)
{
  getFastRoute()->writeGuides(fileName);
}

void
report_congestion(char* congest_file)
{
  getFastRoute()->setReportCongestion(congest_file);
}

void
highlight_net_route(const odb::dbNet *net)
{
  getFastRoute()->highlightRoute(net);
}

void
report_layer_wire_lengths()
{
  getFastRoute()->reportLayerWireLengths();
}

%} // inline
