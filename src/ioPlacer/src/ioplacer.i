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
#include "ioplacer/IOPlacer.h"
#include "Parameters.h"
#include "openroad/OpenRoad.hh"

namespace ord {
// Defined in OpenRoad.i
ioPlacer::IOPlacer* getIOPlacer();
} // namespace ord

using ord::getIOPlacer;
%}

%include "../../Exception.i"

%inline %{

void
set_hor_metal_layer(int layer)
{
  getIOPlacer()->getParameters()->setHorizontalMetalLayer(layer);
}

int
get_hor_metal_layer()
{
  return getIOPlacer()->getParameters()->getHorizontalMetalLayer();
}

void
set_ver_metal_layer(int layer)
{
  getIOPlacer()->getParameters()->setVerticalMetalLayer(layer);
}

int
get_ver_metal_layer()
{
  return getIOPlacer()->getParameters()->getVerticalMetalLayer();
}

void
set_num_slots(int numSlots)
{
  getIOPlacer()->getParameters()->setNumSlots(numSlots);
}

int
get_num_slots()
{
  return getIOPlacer()->getParameters()->getNumSlots();
}

void
set_random_mode(int mode)
{
  getIOPlacer()->getParameters()->setRandomMode(mode);
}

int
get_random_mode()
{
  return getIOPlacer()->getParameters()->getRandomMode();
}

void
set_slots_factor(float factor)
{
  getIOPlacer()->getParameters()->setSlotsFactor(factor);
}

float
get_slots_factor()
{
  return getIOPlacer()->getParameters()->getSlotsFactor();
}

void
set_force_spread(bool force)
{
  getIOPlacer()->getParameters()->setForceSpread(force);
}

bool
get_force_spread()
{
  return getIOPlacer()->getParameters()->getForceSpread();
}

void
set_usage(float usage)
{
  getIOPlacer()->getParameters()->setUsage(usage);
}

float
get_usage()
{
  return getIOPlacer()->getParameters()->getUsage();
}

void
set_usage_factor(float usage)
{
  getIOPlacer()->getParameters()->setUsageFactor(usage);
}

float
get_usage_factor()
{
  return getIOPlacer()->getParameters()->getUsageFactor();
}

void
set_blockages_file(const char* file)
{
  getIOPlacer()->getParameters()->setBlockagesFile(file);
}

const char*
get_blockages_file()
{
  return getIOPlacer()->getParameters()->getBlockagesFile().c_str();
}

void
add_blocked_area(int llx, int lly,
                 int urx, int ury)
{
  getIOPlacer()->addBlockedArea(llx, lly, urx, ury);
}

void
set_hor_length(float length)
{
  getIOPlacer()->getParameters()->setHorizontalLength(length);
}

float
get_hor_length()
{
  return getIOPlacer()->getParameters()->getHorizontalLength();
}

void
set_ver_length_extend(float length)
{
  getIOPlacer()->getParameters()->setVerticalLengthExtend(length);
}

void
set_hor_length_extend(float length)
{
  getIOPlacer()->getParameters()->setHorizontalLengthExtend(length);
}

void
set_ver_length(float length)
{
  getIOPlacer()->getParameters()->setVerticalLength(length);
}

float
get_ver_length()
{
  return getIOPlacer()->getParameters()->getVerticalLength();
}

void
set_interactive_mode(bool enable)
{
  getIOPlacer()->getParameters()->setInteractiveMode(enable);
}

bool
is_interactive_mode()
{
  return getIOPlacer()->getParameters()->isInteractiveMode();
}

void
print_all_parms()
{
  getIOPlacer()->getParameters()->printAll();
}

void
run_io_placement()
{
  getIOPlacer()->run();
}

void
set_report_hpwl(bool report)
{
  getIOPlacer()->getParameters()->setReportHPWL(report);
}

bool
get_report_hpwl()
{
  return getIOPlacer()->getParameters()->getReportHPWL();
}

int
compute_io_nets_hpwl()
{
  return getIOPlacer()->returnIONetsHPWL();
}

void
set_num_threads(int numThreads)
{
  ;
  getIOPlacer()->getParameters()->setNumThreads(numThreads);
}

int
get_num_threads()
{
  return getIOPlacer()->getParameters()->getNumThreads();
}

void
set_rand_seed(double seed)
{
  getIOPlacer()->getParameters()->setRandSeed(seed);
}

double
get_rand_seed()
{
  return getIOPlacer()->getParameters()->getRandSeed();
}

void
set_hor_thick_multiplier(float length)
{
  getIOPlacer()->getParameters()->setHorizontalThicknessMultiplier(length);
}

void
set_ver_thick_multiplier(float length)
{
  getIOPlacer()->getParameters()->setVerticalThicknessMultiplier(length);
}

float
get_ver_thick_multiplier()
{
  return getIOPlacer()->getParameters()->getVerticalThicknessMultiplier();
}

float
get_hor_thick_multiplier()
{
  return getIOPlacer()->getParameters()->getHorizontalThicknessMultiplier();
}

float
get_hor_length_extend()
{
  return getIOPlacer()->getParameters()->getHorizontalLengthExtend();
}

float
get_ver_length_extend()
{
  return getIOPlacer()->getParameters()->getVerticalLengthExtend();
}

void
set_boundaries_offset(int offset)
{
  getIOPlacer()->getParameters()->setBoundariesOffset(offset);
}

void
set_min_distance(int minDist)
{
  getIOPlacer()->getParameters()->setMinDistance(minDist);
}

%} // inline
