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

%ignore cts::CtsOptions::setObserver;
%ignore cts::CtsOptions::getObserver;

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
set_balance_levels(bool balance)
{
  getTritonCts()->getParms()->setBalanceLevels(balance);
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
set_apply_ndr(bool ndr)
{
  getTritonCts()->getParms()->setApplyNDR(ndr);
}

void
set_insertion_delay(bool insDelay)
{
  getTritonCts()->getParms()->enableInsertionDelay(insDelay);
}

void
set_sink_buffer_max_cap_derate(float derate)
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
run_triton_cts()
{
  getTritonCts()->runTritonCts();
}

%} //inline
