// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "grt/GRoute.h"
#include "odb/geom.h"

namespace grt {

// Stage boundaries of CUGR::route(), in stage-mask bit order.
enum class CugrStage
{
  patternRoute,
  patternRouteResAware,
  patternRouteWithDetours,
  mazeRoute,
  iterativeRRR
};

inline constexpr int kCugrStageCount = 5;

inline const char* toString(const CugrStage stage)
{
  constexpr std::array<const char*, kCugrStageCount> kNames
      = {"pattern route",
         "resistance-aware re-route",
         "pattern route with detours",
         "maze route",
         "iterative RRR"};
  return kNames[static_cast<size_t>(stage)];
}

// Stage label shared by the GUI status bar and the logged slack trace.
inline std::string stageLabel(const CugrStage stage, const int iteration)
{
  std::string label = toString(stage);
  if (iteration > 0) {
    label += " iteration " + std::to_string(iteration);
  }
  return label;
}

// One stage's snapshot of the debug net, in DBU with 1-based routing levels.
struct CugrDebugFrame
{
  CugrStage stage = CugrStage::patternRoute;
  // 1-based RRR pass, 0 for the single-shot stages.
  int iteration = 0;
  // Marker size hint: the design's default gcell spacing.
  int gcell_size = 0;
  GRoute route;
  std::vector<odb::Point3D> pins;
};

class AbstractCugrRenderer
{
 public:
  virtual ~AbstractCugrRenderer() = default;

  // Draw the frame and block until the user continues.
  virtual void drawAndPause(CugrDebugFrame frame) = 0;
};

}  // namespace grt
