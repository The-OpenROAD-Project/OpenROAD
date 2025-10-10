// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

%{
#include <cstdint>

#include "cts/TritonCTS.h"
#include "CtsOptions.h"
#include "TechChar.h"
#include "CtsGraphics.h"
#include "ord/OpenRoad.hh"

namespace ord {
// Defined in OpenRoad.i
cts::TritonCTS *
getTritonCts();
}

using namespace cts;
using ord::getTritonCts;
%}

%include "../../Exception.i"
%include "stdint.i"
%include "std_string.i"

%ignore cts::CtsOptions::setObserver;
%ignore cts::CtsOptions::getObserver;

// Enum: CtsOptions::NdrStrategy
%typemap(typecheck) CtsOptions::NdrStrategy {
  char *str = Tcl_GetStringFromObj($input, 0);
  if (strcasecmp(str, "NONE") == 0) {
    $1 = 1;
  } else if (strcasecmp(str, "ROOT_ONLY") == 0) {
    $1 = 1;
  } else if (strcasecmp(str, "HALF") == 0) {
    $1 = 1;
  } else if (strcasecmp(str, "FULL") == 0) {
    $1 = 1;
  } else {
    $1 = 0;
  }
}

%typemap(in) CtsOptions::NdrStrategy {
  char *str = Tcl_GetStringFromObj($input, 0);
  if (strcasecmp(str, "ROOT_ONLY") == 0) {
    $1 = CtsOptions::NdrStrategy::ROOT_ONLY;
  } else if (strcasecmp(str, "HALF") == 0) {
    $1 = CtsOptions::NdrStrategy::HALF;
  } else if (strcasecmp(str, "FULL") == 0) {
    $1 = CtsOptions::NdrStrategy::FULL;
  } else {
    $1 = CtsOptions::NdrStrategy::NONE;
  };
}

%inline %{

void
set_sink_clustering(bool enable)
{
  getTritonCts()->getParms()->setSinkClustering(enable);
}

void
set_plot_option(bool plot)
{
  getTritonCts()->getParms()->setPlotSolution(plot);
}

void
set_sink_clustering_levels(unsigned levels)
{
  getTritonCts()->getParms()->setSinkClusteringLevels(levels);
}

void
set_max_char_cap(double cap)
{
  getTritonCts()->getParms()->setMaxCharCap(cap);
}

void
set_max_char_slew(double slew)
{
  getTritonCts()->getParms()->setMaxCharSlew(slew);
}

void
set_wire_segment_distance_unit(unsigned unit)
{
  getTritonCts()->getParms()->setWireSegmentUnit(unit);
}

void
set_root_buffer(const char* buffer)
{
  getTritonCts()->setRootBuffer(buffer);
}

void
set_slew_steps(int steps)
{
  getTritonCts()->getParms()->setSlewSteps(steps);
}

void
set_cap_steps(int steps)
{
  getTritonCts()->getParms()->setCapSteps(steps);
}

void
set_metric_output(const char* file)
{
  getTritonCts()->getParms()->setMetricsFile(file);
}

void
set_debug_cmd()
{
  getTritonCts()->getParms()->setObserver(std::make_unique<CtsGraphics>());
}

void
report_cts_metrics()
{
  getTritonCts()->reportCtsMetrics();
}

void
set_tree_buf(const char* buffer)
{
  getTritonCts()->getParms()->setTreeBuffer(buffer);
}

void
set_distance_between_buffers(int distance)
{
  getTritonCts()->getParms()->setSimpleSegmentsEnabled(true);
  getTritonCts()->getParms()->setBufferDistance(distance);
}

void
set_branching_point_buffers_distance(int distance)
{
  getTritonCts()->getParms()->setVertexBuffersEnabled(true);
  getTritonCts()->getParms()->setVertexBufferDistance(distance);
}

void
set_clustering_exponent(unsigned power)
{
  getTritonCts()->getParms()->setClusteringPower(power);
}

void
set_clustering_unbalance_ratio(double ratio)
{
  getTritonCts()->getParms()->setClusteringCapacity(ratio);
}

void
set_sink_clustering_size(unsigned size)
{
  getTritonCts()->getParms()->setSinkClusteringSize(size);
}

void
set_clustering_diameter(double distance)
{
  getTritonCts()->getParms()->setMaxDiameter(distance);
}

void
set_macro_clustering_size(unsigned size)
{
  getTritonCts()->getParms()->setMacroClusteringSize(size);
}

void
set_macro_clustering_diameter(double distance)
{
  getTritonCts()->getParms()->setMacroMaxDiameter(distance);
}

void
set_num_static_layers(unsigned num)
{
  getTritonCts()->getParms()->setNumStaticLayers(num);
}

void
set_sink_buffer(const char* buffer)
{
  getTritonCts()->setSinkBuffer(buffer);
}

void
report_characterization()
{
  getTritonCts()->getCharacterization()->report();
}

void
report_wire_segments(unsigned length, unsigned load, unsigned outputSlew)
{
  getTritonCts()->getCharacterization()->reportSegments(length, load, outputSlew);
}

int
set_clock_nets(const char* names)
{
  return getTritonCts()->setClockNets(names);
}

void
set_skip_clock_nets(odb::dbNet* net)
{
  getTritonCts()->getParms()->setSkipNets(net);
}

void
set_buffer_list(const char* buffers)
{
  getTritonCts()->setBufferList(buffers);
}

void
set_obstruction_aware(bool obs)
{
  getTritonCts()->getParms()->setObstructionAware(obs);
}

void
set_apply_ndr(CtsOptions::NdrStrategy strategy)
{
  getTritonCts()->getParms()->setApplyNDR(strategy);
}

void
set_insertion_delay(bool insDelay)
{
  getTritonCts()->getParms()->enableInsertionDelay(insDelay);
}

void
set_sink_buffer_max_cap_derate(double derate)
{
  getTritonCts()->getParms()->setSinkBufferMaxCapDerate(derate);
}

void
set_dummy_load(bool dummyLoad)
{
  getTritonCts()->getParms()->enableDummyLoad(dummyLoad);
}

void
set_delay_buffer_derate(float derate)
{
  getTritonCts()->getParms()->setDelayBufferDerate(derate);
}

void
set_cts_library(const char* name)
{
  getTritonCts()->getParms()->setCtsLibrary(name);
}

void
set_repair_clock_nets(bool value)
{
  getTritonCts()->getParms()->setRepairClockNets(value);
}
 
void
run_triton_cts()
{
  getTritonCts()->runTritonCts();
}

const char*
get_ndr_strategy()
{
  return getTritonCts()->getParms()->getApplyNdrName();
}

int
get_branching_buffers_distance()
{
  return getTritonCts()->getParms()->getVertexBufferDistance();
}
std::string
get_buffer_list()
{
  return getTritonCts()->getParms()->getBufferListToString();
}
unsigned
get_clustering_exponent()
{
  return getTritonCts()->getParms()->getClusteringPower();
}

double
get_clustering_unbalance_ratio()
{
  return getTritonCts()->getParms()->getClusteringCapacity();
}

float
get_delay_buffer_derate()
{
  return getTritonCts()->getParms()->getDelayBufferDerate();
}

int
get_distance_between_buffers()
{
  return getTritonCts()->getParms()->getBufferDistance();
}

const char*
get_library()
{
  return getTritonCts()->getParms()->getCtsLibrary();
}

double
get_macro_clustering_max_diameter()
{
  return getTritonCts()->getParms()->getMacroMaxDiameter();
}

unsigned
get_macro_clustering_size()
{
  return getTritonCts()->getParms()->getMacroSinkClusteringSize();
}

unsigned
get_num_static_layers()
{
  return getTritonCts()->getParms()->getNumStaticLayers();
}

std::string
get_root_buffer()
{
  return getTritonCts()->getRootBufferToString();
}

double
get_sink_buffer_max_cap_derate()
{
  return getTritonCts()->getParms()->getSinkBufferMaxCapDerate();
}

unsigned
get_sink_clustering_levels()
{
  return getTritonCts()->getParms()->getSinkClusteringLevels();
}

double
get_sink_clustering_max_diameter()
{
  return getTritonCts()->getParms()->getMaxDiameter();
}

unsigned
get_sink_clustering_size()
{
  return getTritonCts()->getParms()->getSinkClusteringSize();
}

std::string
get_skip_nets()
{
  return getTritonCts()->getParms()->getSkipNetsToString();
}
std::string
get_tree_buf()
{
  return getTritonCts()->getParms()->getTreeBuffer();
}

unsigned
get_wire_unit()
{
  return getTritonCts()->getParms()->getWireSegmentUnit();
}

void
set_db_unit(bool reset)
{
  if (reset) {
    getTritonCts()->getParms()->setDbUnits(-1);
  } else {
    odb::dbBlock* block = getTritonCts()->getBlock();
    getTritonCts()->getParms()->setDbUnits(block->getDbUnitsPerMicron());
  }
}
void
reset_apply_ndr()
{
  getTritonCts()->getParms()->resetApplyNDR();
}
void
reset_buffer_list()
{
  getTritonCts()->getParms()->resetBufferList();
}
void
reset_branching_point_buffers_distance()
{
  getTritonCts()->getParms()->resetVertexBufferDistance();
}
void
reset_clustering_exponent()
{
  getTritonCts()->getParms()->resetClusteringPower();
}
void
reset_clustering_unbalance_ratio()
{
  getTritonCts()->getParms()->resetClusteringCapacity();
}
void
reset_delay_buffer_derate()
{
  getTritonCts()->getParms()->resetDelayBufferDerate();
}
void
reset_distance_between_buffers()
{
  getTritonCts()->getParms()->resetBufferDistance();
}
void
reset_cts_library()
{
  getTritonCts()->getParms()->resetCtsLibrary();
}
void
reset_macro_clustering_diameter()
{
  getTritonCts()->getParms()->resetMacroMaxDiameter();
}
void
reset_macro_clustering_size()
{
  getTritonCts()->getParms()->resetMacroClusteringSize();
}
void
reset_num_static_layers()
{
  getTritonCts()->getParms()->resetNumStaticLayers();
}
void
reset_root_buffer()
{
  getTritonCts()->resetRootBuffer();
}
void
reset_sink_buffer_max_cap_derate()
{
  getTritonCts()->getParms()->resetSinkBufferMaxCapDerate();
}
void
reset_sink_clustering_levels()
{
  getTritonCts()->getParms()->resetSinkClusteringLevels();
}
void
reset_clustering_diameter()
{
  getTritonCts()->getParms()->resetMaxDiameter();
}
void
reset_sink_clustering_size()
{
  getTritonCts()->getParms()->resetSinkClusteringSize();
}
void
reset_skip_nets()
{
  getTritonCts()->getParms()->resetSkipNets();
}
void
reset_tree_buf()
{
  getTritonCts()->getParms()->resetTreeBuffer();
}
void
reset_wire_segment_distance_unit()
{
  getTritonCts()->getParms()->resetWireSegmentUnit();
}
%} //inline
