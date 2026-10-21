// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Scene.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include "SpatialIndex.h"
#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace fsv {

Scene::Scene(const Geometry& geometry, const double eps_r)
    : geometry_(geometry),
      base_eps_(eps_r),
      min_dimension_(std::numeric_limits<double>::max()),
      index_(std::make_unique<SpatialIndex>(geometry.boxes()))
{
  if (!(eps_r > 0) || !std::isfinite(eps_r)) {
    throw std::invalid_argument("fsv: permittivity must be positive");
  }
  for (const Box& b : geometry.boxes()) {
    for (int k = 0; k < 3; k++) {
      min_dimension_ = std::min(min_dimension_, b.hi[k] - b.lo[k]);
    }
  }
}

Scene::Scene(const Geometry& geometry, const Stack& stack)
    : Scene(geometry,
            stack.dielectrics.empty() ? stack.background_eps
                                      : stack.dielectrics.front().eps)
{
  for (const Stack::Conductor& c : stack.conductors) {
    if (!c.conformals.empty()) {
      throw std::invalid_argument(
          "fsv: conformal dielectrics are not supported yet (conductor "
          + c.name + ")");
    }
  }
  interfaces_ = stack.planarInterfaces();
}

double Scene::epsAt(const double z) const
{
  // The first interface above z gives the permittivity below it.
  const auto above = std::upper_bound(
      interfaces_.begin(),
      interfaces_.end(),
      z,
      [](const double v, const Stack::PlanarInterface& i) { return v < i.z; });
  if (above != interfaces_.end()) {
    return above->eps_below;
  }
  return interfaces_.empty() ? base_eps_ : interfaces_.back().eps_above;
}

Scene::~Scene() = default;

}  // namespace fsv
