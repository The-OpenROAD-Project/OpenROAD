// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Scene.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Dielectric.h"
#include "SpatialIndex.h"
#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace fsv {

namespace {

// A box's z extent matches a conductor layer's within this (µm).
constexpr double kLayerTolerance = 1e-9;

bool overlaps(const Box& a, const Box& b)
{
  for (int k = 0; k < 3; k++) {
    if (!(a.lo[k] < b.hi[k] && b.lo[k] < a.hi[k])) {
      return false;
    }
  }
  return true;
}

// Whether part of `box`, of positive volume, lies outside every hole from
// `first` on.
bool anyLeft(const Box& box,
             const std::vector<const Box*>& holes,
             const size_t first)
{
  for (size_t i = first; i < holes.size(); i++) {
    const Box& hole = *holes[i];
    if (!overlaps(box, hole)) {
      continue;
    }
    // Split off the slabs of `box` outside the hole, one axis at a time, and
    // keep the part inside it for the next axis.
    Box rest = box;
    for (int k = 0; k < 3; k++) {
      if (rest.lo[k] < hole.lo[k]) {
        Box slab = rest;
        slab.hi[k] = hole.lo[k];
        if (anyLeft(slab, holes, i + 1)) {
          return true;
        }
        rest.lo[k] = hole.lo[k];
      }
      if (rest.hi[k] > hole.hi[k]) {
        Box slab = rest;
        slab.lo[k] = hole.hi[k];
        if (anyLeft(slab, holes, i + 1)) {
          return true;
        }
        rest.hi[k] = hole.hi[k];
      }
    }
    // `rest` is inside the hole.
    return false;
  }
  return true;
}

}  // namespace

Scene::Scene(const Geometry& geometry, const double eps_r)
    : geometry_(geometry),
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
  dielectric_ = std::make_unique<Dielectric>(
      eps_r, std::vector<Stack::PlanarInterface>{}, std::vector<Shell>{});
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
  // The interface lookups need strictly increasing interfaces.
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
    for (const Stack::ConformalDielectric& cf : c.conformals) {
      check_eps(cf.eps, "conformal dielectric " + cf.name);
      if (!(cf.side_thickness >= 0 && cf.top_thickness >= 0)) {
        throw std::invalid_argument("fsv: conformal dielectric " + cf.name
                                    + " has a negative thickness");
      }
    }
  }

  // A box on a conductor layer (its z extent equals the layer's) gets the
  // layer's conformal dielectrics, each the box grown by the summed side
  // and top thicknesses so far. The boxes of one net on one layer form a
  // group (see Shell).
  std::map<std::string, int> materials;
  std::map<std::pair<int, int>, int> groups;
  std::vector<std::string> material_names;
  std::vector<Shell> shells;
  const std::vector<Box>& boxes = geometry.boxes();
  for (int b = 0; b < static_cast<int>(boxes.size()); b++) {
    const Box& box = boxes[b];
    for (int layer = 0; layer < static_cast<int>(stack.conductors.size());
         layer++) {
      const Stack::Conductor& c = stack.conductors[layer];
      if (std::abs(box.lo[2] - c.bottom) > kLayerTolerance
          || std::abs(box.hi[2] - c.top) > kLayerTolerance) {
        continue;
      }
      const int group = groups
                            .try_emplace({geometry.boxNet(b), layer},
                                         static_cast<int>(groups.size()))
                            .first->second;
      Box inner = box;
      double side = 0;
      double top = 0;
      int depth = 0;
      for (const Stack::ConformalDielectric& cf : c.conformals) {
        if (cf.side_thickness == 0 && cf.top_thickness == 0) {
          continue;
        }
        side += cf.side_thickness;
        top += cf.top_thickness;
        Box outer = box;
        for (int k = 0; k < 2; k++) {
          outer.lo[k] -= side;
          outer.hi[k] += side;
        }
        outer.hi[2] += top;
        const auto [it, added] = materials.try_emplace(
            cf.name, static_cast<int>(material_names.size()));
        if (added) {
          material_names.push_back(cf.name);
        }
        shells.push_back({outer, inner, cf.eps, it->second, b, group, ++depth});
        inner = outer;
      }
      break;
    }
  }

  // Outside a group, shells of different conformal dielectrics may not
  // overlap, except where a conductor cuts into them. A shell's region is
  // its outer box minus the shallower shells of its group.
  for (size_t i = 0; i < shells.size(); i++) {
    for (size_t j = i + 1; j < shells.size(); j++) {
      const Shell& a = shells[i];
      const Shell& s = shells[j];
      if (a.material == s.material || a.group == s.group
          || !overlaps(a.outer, s.outer)) {
        continue;
      }
      Box both;
      for (int k = 0; k < 3; k++) {
        both.lo[k] = std::max(a.outer.lo[k], s.outer.lo[k]);
        both.hi[k] = std::min(a.outer.hi[k], s.outer.hi[k]);
      }
      std::vector<const Box*> holes;
      for (const Shell& t : shells) {
        const bool shallower = (t.group == a.group && t.depth < a.depth)
                               || (t.group == s.group && t.depth < s.depth);
        if (shallower && overlaps(both, t.outer)) {
          holes.push_back(&t.outer);
        }
      }
      for (const Box& box : boxes) {
        if (overlaps(both, box)) {
          holes.push_back(&box);
        }
      }
      if (anyLeft(both, holes, 0)) {
        throw std::invalid_argument("fsv: conformal dielectrics "
                                    + material_names[a.material] + " and "
                                    + material_names[s.material] + " overlap");
      }
    }
  }
  dielectric_ = std::make_unique<Dielectric>(
      stack.dielectrics.empty() ? stack.background_eps
                                : stack.dielectrics.front().eps,
      stack.planarInterfaces(),
      std::move(shells));
}

Scene::~Scene() = default;

const std::vector<Stack::PlanarInterface>& Scene::interfaces() const
{
  return dielectric_->planar();
}

double Scene::epsAt(const double z) const
{
  return dielectric_->planarEps(z);
}

}  // namespace fsv
