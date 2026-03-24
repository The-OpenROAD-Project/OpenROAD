// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "shapes.h"
#include "utl/Logger.h"

namespace mpl {
struct BoundaryRegion;
class Cluster;

using ClusterToBoundaryRegionMap = std::map<Cluster*, BoundaryRegion>;
using BoundaryRegionList = std::vector<BoundaryRegion>;

// One of the edges of the die area.
enum class Boundary
{
  B,
  L,
  T,
  R
};

inline std::string toString(const Boundary& boundary)
{
  std::string string;

  switch (boundary) {
    case (Boundary::L): {
      string = 'L';
      break;
    }
    case (Boundary::T): {
      string = 'T';
      break;
    }
    case (Boundary::R): {
      string = 'R';
      break;
    }
    case (Boundary::B): {
      string = 'B';
      break;
    }
  }

  return string;
}

inline bool isVertical(Boundary boundary)
{
  return boundary == Boundary::L || boundary == Boundary::R;
}

struct SACoreWeights
{
  float area{0.0f};
  float outline{0.0f};
  float wirelength{0.0f};
  float guidance{0.0f};
  float fence{0.0f};
};

struct SASoftWeights
{
  float boundary{0.0f};
  float notch{0.0f};
  float macro_blockage{0.0f};
};

// The cost of a certain penalty is:
//   cost = weight * normalized_penalty
//
// Where the normalized_penalty is:
//   normalized_penalty = value / normalization_factor
//
// Note: the normalization factor is generated during the
// annealer core initialization.
struct PenaltyData
{
  std::string name;
  float weight{0.0f};
  float value{0.0f};
  float normalization_factor{0.0f};
};

// Object to help handling available regions and constraint regions for pins.
struct BoundaryRegion
{
  BoundaryRegion() = default;
  BoundaryRegion(const odb::Line& line, const Boundary boundary)
      : line(line), boundary(boundary)
  {
  }

  odb::Line line;
  Boundary boundary = Boundary::L;
};

// Utility to help sorting width intervals.
inline bool isMinWidthSmaller(const Interval& width_interval_a,
                              const Interval& width_interval_b)
{
  return width_interval_a.min < width_interval_b.min;
}

// Utility to help sorting tilings.
inline bool isAreaSmaller(const Tiling& tiling_a, const Tiling& tiling_b)
{
  if (tiling_a.area() != tiling_b.area()) {
    return tiling_a.area() < tiling_b.area();
  }

  return tiling_a.width() < tiling_b.width();
}

inline Boundary getBoundary(odb::dbBlock* block, const odb::Rect& region)
{
  const odb::Rect& die = block->getDieArea();

  if (region.dx() == 0) {
    if (region.xMin() == die.xMin()) {
      return Boundary::L;
    }

    return Boundary::R;
  }

  if (region.yMin() == die.yMin()) {
    return Boundary::B;
  }

  return Boundary::T;
}

inline odb::Rect lineToRect(const odb::Line line)
{
  return odb::Rect(line.pt0(), line.pt1());
}

inline odb::Line rectToLine(odb::dbBlock* block,
                            const odb::Rect& rect,
                            utl::Logger* logger)
{
  if (rect.dx() != 0 && rect.dy() != 0) {
    logger->error(utl::MPL,
                  60,
                  "Coundn't convert rect {} to line. The region is not a line.",
                  rect);
  }

  return {rect.ll(), rect.ur()};
}

inline odb::Point computeNearestPointInRegion(const BoundaryRegion& region,
                                              const odb::Point& target)
{
  const odb::Line& line = region.line;
  if (region.boundary == Boundary::L || region.boundary == Boundary::R) {
    if (target.y() >= line.pt1().y()) {
      return line.pt1();
    }
    if (target.y() <= line.pt0().y()) {
      return line.pt0();
    }
    return {line.pt0().x(), target.y()};
  }

  // Top or Bottom
  if (target.x() >= line.pt1().x()) {
    return line.pt1();
  }
  if (target.x() <= line.pt0().x()) {
    return line.pt0();
  }
  return {target.x(), line.pt0().y()};
}

// The distance in DBU from the source to the nearest point of the nearest
// region.
inline int64_t computeDistToNearestRegion(
    const odb::Point& source,
    const std::vector<BoundaryRegion>& regions,
    odb::Point* nearest_point)
{
  int64_t smallest_distance = std::numeric_limits<int64_t>::max();

  for (const BoundaryRegion& region : regions) {
    const odb::Point nearest_point_in_region
        = computeNearestPointInRegion(region, source);
    const int64_t dist_to_nearest_point
        = odb::Point::squaredDistance(source, nearest_point_in_region);

    if (dist_to_nearest_point < smallest_distance) {
      smallest_distance = dist_to_nearest_point;
      if (nearest_point) {
        *nearest_point = nearest_point_in_region;
      }
    }
  }

  return std::sqrt(smallest_distance);
}

}  // namespace mpl
