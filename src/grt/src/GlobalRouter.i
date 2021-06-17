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


%{
#include "grt/GlobalRouter.h"
#include "ord/OpenRoad.hh"
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

namespace grt {

bool
have_routes()
{
  return getFastRoute()->haveRoutes();
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
set_pdrev_alpha(float alpha)
{
  getFastRoute()->setPdRevAlpha(alpha);
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
set_pdrev_min_fanout(int min_fanout)
{
  getFastRoute()->setPDRevMinFanout(min_fanout);
}

void
set_allow_overflow(bool allowOverflow)
{
  getFastRoute()->setAllowOverflow(allowOverflow);
}

void
set_clock_layer_range(int minLayer, int maxLayer)
{
  getFastRoute()->setMinLayerForClock(minLayer);
  getFastRoute()->setMaxLayerForClock(maxLayer);
}

void
set_macro_extension(int macroExtension)
{
  getFastRoute()->setMacroExtension(macroExtension);
}

void
global_route()
{
  getFastRoute()->globalRoute();
}

void
global_route_clocks_separately()
{
  getFastRoute()->globalRouteClocksSeparately();
}

void
run()
{
  getFastRoute()->run();
}

void
estimate_rc()
{
  getFastRoute()->estimateRC();
}

void
repair_antennas(LibertyPort* diodePort)
{
  getFastRoute()->repairAntennas(diodePort);
}

void
clear()
{
  getFastRoute()->clear();
}

void
write_guides(char* fileName)
{
  getFastRoute()->writeGuides(fileName);
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

} // namespace

%} // inline
