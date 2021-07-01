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
#include "tritoncts/TritonCTS.h"
#include "CtsOptions.h"
#include "TechChar.h"
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

%inline %{

void
set_simple_cts(bool enable)
{
  getTritonCts()->getParms()->setSimpleCts(enable);
}

void
set_sink_clustering(bool enable)
{
  getTritonCts()->getParms()->setSinkClustering(enable);
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
  getTritonCts()->getParms()->setRootBuffer(buffer);
}

void
set_out_path(const char* path)
{
  getTritonCts()->getParms()->setOutputPath(path);
}

void
set_cap_per_sqr(double cap)
{
  getTritonCts()->getParms()->setCapPerSqr(cap);
}

void
set_res_per_sqr(double res)
{
  getTritonCts()->getParms()->setResPerSqr(res);
}

void
set_slew_inter(double slew)
{
  getTritonCts()->getParms()->setSlewInter(slew);
}

void
set_cap_inter(double cap)
{
  getTritonCts()->getParms()->setCapInter(cap);
}

void
set_metric_output(const char* file)
{
  getTritonCts()->getParms()->setMetricsFile(file);
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
set_distance_between_buffers(double distance)
{
  getTritonCts()->getParms()->setSimpleSegmentsEnabled(true);
  getTritonCts()->getParms()->setBufferDistance(distance);
}

void
set_branching_point_buffers_distance(double distance)
{
  getTritonCts()->getParms()->setVertexBuffersEnabled(true);
  getTritonCts()->getParms()->setVertexBufferDistance(distance);
}

void
set_disable_post_cts(bool disable)
{
  getTritonCts()->getParms()->setRunPostCtsOpt(!(disable));
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
  getTritonCts()->getParms()->setSizeSinkClustering(size);
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
  getTritonCts()->getParms()->setSinkBuffer(buffer);
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

void
export_characterization(const char* file)
{
  getTritonCts()->getCharacterization()->write(file);
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
run_triton_cts()
{
  getTritonCts()->runTritonCts();
}

%} //inline
