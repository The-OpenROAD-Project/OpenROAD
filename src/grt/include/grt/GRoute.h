// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <utility>
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
  bool is_3d_route = false;
  GSegment() = default;
  GSegment(int x0, int y0, int l0, int x1, int y1, int l1, bool jumper = false);
  bool isVia() const { return (init_x == final_x && init_y == final_y); }
  bool isJumper() const { return is_jumper; }
  bool is3DRoute() const { return is_3d_route; }
  void setIs3DRoute(bool is_3d) { is_3d_route = is_3d; }
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
