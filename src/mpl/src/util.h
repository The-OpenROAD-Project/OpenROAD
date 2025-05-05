// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <utility>

#include "odb/geom.h"
#include "shapes.h"

namespace mpl {

struct SACoreWeights
{
  float area{0.0f};
  float outline{0.0f};
  float wirelength{0.0f};
  float guidance{0.0f};
  float fence{0.0f};
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

// Used inside the annealer and during the orientation improvement step.
struct AvailableRegionForPins
{
  AvailableRegionForPins(const odb::Line& line, const Boundary boundary)
      : line(line), boundary(boundary)
  {
  }

  odb::Line line;
  Boundary boundary;
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
  Boundary boundary = NONE;
  if (region.dx() == 0) {
    if (region.xMin() == die.xMin()) {
      boundary = L;
    } else {
      boundary = R;
    }
  } else {
    if (region.yMin() == die.yMin()) {
      boundary = B;
    } else {
      boundary = T;
    }
  }
  return boundary;
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

  odb::Line line;
  switch (getBoundary(block, rect)) {
    case L:
      return line = {rect.ll(), rect.ul()};
    case R:
      return line = {rect.lr(), rect.ur()};
    case T:
      return line = {rect.ul(), rect.ur()};
    case B:
      return line = {rect.ll(), rect.lr()};
    case NONE:
      logger->error(utl::MPL,
                    61,
                    "Couldn't convert rect {} to line. The is region outside "
                    "of the die {} boundaries.",
                    rect,
                    block->getDieArea());
  }

  return line;
}

inline odb::Point computeNearestPointInRegion(
    const AvailableRegionForPins& region,
    const odb::Point& target)
{
  if (region.boundary == L || region.boundary == R) {
    if (target.y() >= region.line.pt1().y()) {
      return odb::Point(region.line.pt0().x(), region.line.pt1().y());
    }
    if (target.y() <= region.line.pt0().y()) {
      return odb::Point(region.line.pt0().x(), region.line.pt0().y());
    }
    return odb::Point(region.line.pt0().x(), target.y());
  }

  // Top or Bottom
  if (target.x() >= region.line.pt1().x()) {
    return odb::Point(region.line.pt1().x(), region.line.pt0().y());
  }
  if (target.x() <= region.line.pt0().x()) {
    return odb::Point(region.line.pt0().x(), region.line.pt0().y());
  }
  return odb::Point(target.x(), region.line.pt0().y());
}

// The distance in DBU from the macro bundled pin to the nearest point
// of the nearest region.
inline double computeDistToNearestRegion(
    const odb::Point& macro_location,
    const std::vector<AvailableRegionForPins>& regions,
    odb::Point* closest_point)
{
  double smallest_distance = std::numeric_limits<double>::max();
  for (const AvailableRegionForPins& region : regions) {
    odb::Point closest_point_in_region
        = computeNearestPointInRegion(region, macro_location);
    const double dist_to_closest_point_in_region = std::sqrt(
        odb::Point::squaredDistance(macro_location, closest_point_in_region));
    if (dist_to_closest_point_in_region < smallest_distance) {
      smallest_distance = dist_to_closest_point_in_region;
      if (closest_point) {
        *closest_point = closest_point_in_region;
      }
    }
  }

  return smallest_distance;
}

}  // namespace mpl
