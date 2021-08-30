/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
grt::GlobalRouter* getGlobalRouter();
}  // namespace ord

using ord::getGlobalRouter;
using sta::LibertyPort;
%}

%include "../../Exception.i"

%inline %{

namespace grt {

bool
have_routes()
{
  return getGlobalRouter()->haveRoutes();
}

void
set_capacity_adjustment(float adjustment)
{
  getGlobalRouter()->setAdjustment(adjustment);
}

void
add_layer_adjustment(int layer, float reductionPercentage)
{
  getGlobalRouter()->addLayerAdjustment(layer, reductionPercentage);
}

void
add_region_adjustment(int minX,
                           int minY,
                           int maxX,
                           int maxY,
                           int layer,
                           float reductionPercentage)
{
  getGlobalRouter()->addRegionAdjustment(
      minX, minY, maxX, maxY, layer, reductionPercentage);
}

void
set_min_layer(int minLayer)
{
  getGlobalRouter()->setMinRoutingLayer(minLayer);
}

void
set_max_layer(int maxLayer)
{
  getGlobalRouter()->setMaxRoutingLayer(maxLayer);
}

void
set_verbose(int v)
{
  getGlobalRouter()->setVerbose(v);
}

void
set_overflow_iterations(int iterations)
{
  getGlobalRouter()->setOverflowIterations(iterations);
}

void
set_grid_origin(long x, long y)
{
  getGlobalRouter()->setGridOrigin(x, y);
}

void
set_allow_congestion(bool allowCongestion)
{
  getGlobalRouter()->setAllowCongestion(allowCongestion);
}

void
set_clock_layer_range(int minLayer, int maxLayer)
{
  getGlobalRouter()->setMinLayerForClock(minLayer);
  getGlobalRouter()->setMaxLayerForClock(maxLayer);
}

void
set_macro_extension(int macroExtension)
{
  getGlobalRouter()->setMacroExtension(macroExtension);
}

void
set_seed(int seed)
{
  getGlobalRouter()->setSeed(seed);
}

void
set_capacities_perturbation_percentage(float percentage)
{
  getGlobalRouter()->setCapacitiesPerturbationPercentage(percentage);
}

void
set_perturbation_amount(int perturbation)
{
  getGlobalRouter()->setPerturbationAmount(perturbation);
}

void
run()
{
  getGlobalRouter()->globalRoute();
}

void
estimate_rc()
{
  getGlobalRouter()->estimateRC();
}

void
repair_antennas(LibertyPort* diodePort, int iterations)
{
  getGlobalRouter()->repairAntennas(diodePort, iterations);
}

void
clear()
{
  getGlobalRouter()->clear();
}

void
write_guides(char* fileName)
{
  getGlobalRouter()->writeGuides(fileName);
}

void
highlight_net_route(const odb::dbNet *net)
{
  getGlobalRouter()->highlightRoute(net);
}

void highlight_steiner_tree_builder(const odb::dbNet *net)
{
  getGlobalRouter()->highlightSteinerTreeBuilder(net);
}

void highlight_2D_tree(const odb::dbNet *net)
{
  getGlobalRouter()->highlightRectilinearSteinerTree(net, 3);
}

void highlight_3D_tree(const odb::dbNet *net)
{
  getGlobalRouter()->highlightRectilinearSteinerTree(net, 4);
}

void highlight_rectilinear_steiner_tree(const odb::dbNet *net)
{
  getGlobalRouter()->highlightRectilinearSteinerTree(net, 5);
}

void
erase_routes()
{
  getGlobalRouter()->clearRouteGui();
}

void
erase_fastroutes()
{
  getGlobalRouter()->clearFastRouteGui();
}

void 
erase_steinertrees()
{
  getGlobalRouter()->clearSteinerTreeGui();
}

void
report_layer_wire_lengths()
{
  getGlobalRouter()->reportLayerWireLengths();
}

} // namespace

%} // inline
