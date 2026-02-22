// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors


%{
#include "grt/GlobalRouter.h"
#include "GrouteRenderer.h"
#include "FastRouteRenderer.h"
#include "ord/OpenRoad.hh"
#include "sta/Liberty.hh"

namespace ord {
// Defined in OpenRoad.i
grt::GlobalRouter* getGlobalRouter();
}

using ord::getGlobalRouter;
using sta::LibertyPort;
%}

%include "../../Exception.i"

%ignore grt::GlobalRouter::init;
%ignore grt::GlobalRouter::initDebugFastRoute;
%ignore grt::GlobalRouter::getDebugFastRoute;
%ignore grt::GlobalRouter::setRenderer;

%import <stl.i>
%import <std_vector.i>
%template(vector_int) std::vector<int>;

%inline %{

namespace grt {

bool
have_routes()
{
  return getGlobalRouter()->haveRoutes();
}

bool
have_detailed_routes()
{
  return getGlobalRouter()->haveDetailedRoutes();
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
set_verbose(bool v)
{
  getGlobalRouter()->setVerbose(v);
}

void
set_congestion_iterations(int iterations)
{
  getGlobalRouter()->setCongestionIterations(iterations);
}

void
set_congestion_report_iter_step(int congestion_report_iter_step)
{
  getGlobalRouter()->setCongestionReportIterStep(congestion_report_iter_step);
}

void set_congestion_report_file (const char * file_name)
{
  getGlobalRouter()->setCongestionReportFile(file_name);
}

void
set_grid_origin(int x, int y)
{
  getGlobalRouter()->setGridOrigin(x, y);
}

void
set_allow_congestion(bool allowCongestion)
{
  getGlobalRouter()->setAllowCongestion(allowCongestion);
}

void
set_resistance_aware(bool resistance_aware)
{
  getGlobalRouter()->setResistanceAware(resistance_aware);
}

void
set_critical_nets_percentage(float criticalNetsPercentage)
{
  getGlobalRouter()->setCriticalNetsPercentage(criticalNetsPercentage);
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
set_use_cugr(bool use_cugr)
{
  getGlobalRouter()->setUseCUGR(use_cugr);
}

void
set_skip_large_fanout(int skip_large_fanout)
{
  getGlobalRouter()->setSkipLargeFanoutNets(skip_large_fanout);
}

void
set_infinite_cap(bool infinite_capacity)
{
  getGlobalRouter()->setInfiniteCapacity(infinite_capacity);
}

void start_incremental()
{
  getGlobalRouter()->startIncremental();
}

void end_incremental()
{
  // Save guides by default when ending incremental routing from Tcl interface.
  getGlobalRouter()->endIncremental(true);
}

void
global_route()
{
  getGlobalRouter()->globalRoute(true);
}

std::vector<int>
route_layer_lengths(odb::dbNet* db_net)
{
  return getGlobalRouter()->routeLayerLengths(db_net);
}

int
repair_antennas(odb::dbMTerm* diode_mterm, int iterations, float ratio_margin, bool jumper_only, bool diode_only)
{
  const int num_threads = ord::OpenRoad::openRoad()->getThreadCount();
  return getGlobalRouter()->repairAntennas(diode_mterm, iterations, ratio_margin, jumper_only, diode_only, num_threads);
}

void
add_net_to_route(odb::dbNet* net)
{
  getGlobalRouter()->addNetToRoute(net);
}

void
highlight_net_route(odb::dbNet *net, bool show_pin_locations)
{
  if (!gui::Gui::enabled()) {
    return;
  }

  GlobalRouter* router = getGlobalRouter();
  if (router->getRenderer() == nullptr) {
    router->setRenderer(std::make_unique<GrouteRenderer>(router, router->db()->getTech()));
  }

  router->getRenderer()->highlightRoute(net, show_pin_locations);
}

void
read_guides(const char* fileName)
{
  getGlobalRouter()->readGuides(fileName);
}

void set_global_route_debug_cmd(const odb::dbNet *net,
                                bool steinerTree,
                                bool rectilinearSTree,
                                bool tree2D,
                                bool tree3D)
{
  if (!gui::Gui::enabled()) {
    return;
  }

  GlobalRouter* global_router = getGlobalRouter();
  if (global_router->getDebugFastRoute() == nullptr) {
    global_router->initDebugFastRoute(std::make_unique<FastRouteRenderer>(
      global_router->db()->getTech()));
  }
  getGlobalRouter()->setDebugNet(net);
  getGlobalRouter()->setDebugSteinerTree(steinerTree);
  getGlobalRouter()->setDebugRectilinearSTree(rectilinearSTree);
  getGlobalRouter()->setDebugTree2D(tree2D);
  getGlobalRouter()->setDebugTree3D(tree3D);
}

void set_global_route_debug_stt_input_filename(const char* file_name)
{
  getGlobalRouter()->setSttInputFilename(file_name);
}

void create_wl_report_file(const char* file_name, bool verbose)
{
  getGlobalRouter()->createWLReportFile(file_name, verbose);
}

void report_net_wire_length(odb::dbNet* net,
                            bool global_route,
                            bool detailed_route,
                            bool verbose,
                            const char* file_name)
{
  getGlobalRouter()->reportNetWireLength(
      net, global_route, detailed_route, verbose, file_name);
}

void
clear_route_guides()
{
  if (auto* renderer = getGlobalRouter()->getRenderer()) {
    renderer->clearRoute();
  }
}

void
report_layer_wire_lengths(bool global_route, bool detailed_route)
{
  getGlobalRouter()->reportLayerWireLengths(global_route, detailed_route);
}

void write_segments(const char* file_name)
{
  getGlobalRouter()->writeSegments(file_name);
}

void read_segments(const char* file_name)
{
  getGlobalRouter()->readSegments(file_name);
}

void write_pin_locations(const char* file_name)
{
  getGlobalRouter()->writePinLocations(file_name);
}

odb::dbObject* iterm_to_object(odb::dbITerm* iterm)
{
  return (odb::dbObject*) iterm;
}

odb::dbObject* bterm_to_object(odb::dbBTerm* bterm)
{
  return (odb::dbObject*) bterm;
}

float estimate_path_resistance(odb::dbObject* pin1,
                                 odb::dbObject* pin2,
                                 bool verbose = false)
{
  return getGlobalRouter()->estimatePathResistance(pin1, pin2, verbose);
}

float estimate_path_resistance(odb::dbObject* pin1,
				 odb::dbObject* pin2,
				 odb::dbTechLayer* layer1,
				 odb::dbTechLayer* layer2,
				 bool verbose = false)
{
  return getGlobalRouter()->estimatePathResistance(
    pin1, pin2, layer1, layer2, verbose);
}

} // namespace

%} // inline
