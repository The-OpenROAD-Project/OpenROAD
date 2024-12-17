////////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, Federal University of Rio Grande do Sul (UFRGS)
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
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "odb/db.h"

namespace grt {

using CapacitiesVec = std::vector<std::vector<std::vector<int>>>;

struct GSegment
{
  int init_x;
  int init_y;
  int init_layer;
  int final_x;
  int final_y;
  int final_layer;
  bool is_jumper;
  GSegment() = default;
  GSegment(int x0, int y0, int l0, int x1, int y1, int l1, bool jumper = false);
  bool isVia() const { return (init_x == final_x && init_y == final_y); }
  bool isJumper() const { return is_jumper; }
  int length() const
  {
    return std::abs(init_x - final_x) + std::abs(init_y - final_y);
  }
  bool operator==(const GSegment& segment) const;
};

struct GSegmentHash
{
  std::size_t operator()(const GSegment& seg) const;
};

struct TileCongestion
{
  int capacity;
  int usage;
};

struct TileInformation
{
  std::set<odb::dbNet*> nets;
  TileCongestion congestion;
};

using NetsPerCongestedArea = std::map<std::pair<int, int>, TileInformation>;

struct CongestionInformation
{
  GSegment segment;
  TileCongestion congestion;
  std::set<odb::dbNet*> sources;
};

struct CapacityReduction
{
  uint8_t capacity = 0;
  uint8_t reduction = 0;
};

using CapacityReductionData = std::vector<std::vector<CapacityReduction>>;

using SegmentIndex = uint16_t;

// class Route is defined in fastroute core.
using GRoute = std::vector<GSegment>;
using NetRouteMap = std::map<odb::dbNet*, GRoute>;
void print(GRoute& groute);

}  // namespace grt
