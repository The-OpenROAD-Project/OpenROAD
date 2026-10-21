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
  auto check_eps = [](const double eps, const std::string& what) {
    if (!(eps > 0) || !std::isfinite(eps)) {
      throw std::invalid_argument("fsv: permittivity of " + what
                                  + " must be positive");
    }
  };
  // The interface lookups (epsAt, the solver's cube sizing) need strictly
  // increasing interfaces.
  double bottom = 0;
  for (const Stack::Dielectric& d : stack.dielectrics) {
    check_eps(d.eps, "dielectric " + d.name);
    if (!(d.top > bottom)) {
      throw std::invalid_argument("fsv: dielectric " + d.name
                                  + " must lie above the one below it");
    }
    bottom = d.top;
  }
  check_eps(stack.background_eps, "the background");
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
  // The first interface above z gives the permittivity below it. A height on
  // an interface takes the permittivity above, as Stack::epsAt does.
  const auto above = std::ranges::upper_bound(
      interfaces_, z, {}, &Stack::PlanarInterface::z);
  if (above != interfaces_.end()) {
    return above->eps_below;
  }
  return interfaces_.empty() ? base_eps_ : interfaces_.back().eps_above;
}

Scene::~Scene() = default;

}  // namespace fsv
