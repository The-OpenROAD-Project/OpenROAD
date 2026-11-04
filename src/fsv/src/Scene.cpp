// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Scene.h"

#include <algorithm>
#include <cmath>
#include <functional>
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

double baseEps(const Stack& stack)
{
  return stack.dielectrics().empty() ? stack.backgroundEps()
                                     : stack.dielectrics().front().eps;
}

template <int D>
bool overlaps(const AlignedBox<D>& a, const AlignedBox<D>& b)
{
  for (int k = 0; k < D; k++) {
    if (!(a.lo[k] < b.hi[k] && b.lo[k] < a.hi[k])) {
      return false;
    }
  }
  return true;
}

// Whether part of `box`, of positive volume, lies outside every hole from
// `first` on.
template <int D>
bool anyLeft(const AlignedBox<D>& box,
             const std::vector<const AlignedBox<D>*>& holes,
             const size_t first)
{
  for (size_t i = first; i < holes.size(); i++) {
    const AlignedBox<D>& hole = *holes[i];
    if (!overlaps(box, hole)) {
      continue;
    }
    // Split off the slabs of `box` outside the hole, one axis at a time, and
    // keep the part inside it for the next axis.
    AlignedBox<D> rest = box;
    for (int k = 0; k < D; k++) {
      if (rest.lo[k] < hole.lo[k]) {
        AlignedBox<D> slab = rest;
        slab.hi[k] = hole.lo[k];
        if (anyLeft(slab, holes, i + 1)) {
          return true;
        }
        rest.lo[k] = hole.lo[k];
      }
      if (rest.hi[k] > hole.hi[k]) {
        AlignedBox<D> slab = rest;
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

template <int D>
double smallestDimension(const std::vector<AlignedBox<D>>& boxes)
{
  double d = std::numeric_limits<double>::max();
  for (const AlignedBox<D>& b : boxes) {
    for (int k = 0; k < D; k++) {
      d = std::min(d, b.hi[k] - b.lo[k]);
    }
  }
  return d;
}

template <int D>
std::unique_ptr<Dielectric<D>> uniformDielectric(const double eps_r)
{
  if (!(eps_r > 0) || !std::isfinite(eps_r)) {
    throw std::invalid_argument("fsv: permittivity must be positive");
  }
  return std::make_unique<Dielectric<D>>(
      eps_r, std::vector<Stack::PlanarInterface>{}, std::vector<Shell<D>>{});
}

// The dielectric of a stack around conductor boxes (rectangles, in 2D). A
// box on a conductor layer (its z extent equals the layer's) gets the
// layer's conformal dielectrics, each the box grown by the summed side
// thicknesses so far along every horizontal axis and by the summed top
// thicknesses along z. The boxes of one net on one layer form a group (see
// Shell).
template <int D>
std::unique_ptr<Dielectric<D>> stackDielectric(
    const std::vector<AlignedBox<D>>& boxes,
    const std::function<int(int)>& box_net,
    const SpatialIndex<D>& box_index,
    const Stack& stack)
{
  constexpr int kZ = D - 1;
  std::map<std::string, int> materials;
  std::map<std::pair<int, int>, int> groups;
  std::vector<std::string> material_names;
  std::vector<Shell<D>> shells;
  for (int b = 0; b < static_cast<int>(boxes.size()); b++) {
    const AlignedBox<D>& box = boxes[b];
    for (int layer = 0; layer < static_cast<int>(stack.conductors().size());
         layer++) {
      const Stack::Conductor& c = stack.conductors()[layer];
      if (std::abs(box.lo[kZ] - c.bottom) > kLayerTolerance
          || std::abs(box.hi[kZ] - c.top) > kLayerTolerance) {
        continue;
      }
      const int group = groups
                            .try_emplace({box_net(b), layer},
                                         static_cast<int>(groups.size()))
                            .first->second;
      AlignedBox<D> inner = box;
      double side = 0;
      double top = 0;
      int depth = 0;
      for (const Stack::ConformalDielectric& cf : c.conformals) {
        if (cf.side_thickness == 0 && cf.top_thickness == 0) {
          continue;
        }
        side += cf.side_thickness;
        top += cf.top_thickness;
        AlignedBox<D> outer = box;
        for (int k = 0; k < kZ; k++) {
          outer.lo[k] -= side;
          outer.hi[k] += side;
        }
        outer.hi[kZ] += top;
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

  auto dielectric = std::make_unique<Dielectric<D>>(
      baseEps(stack), stack.planarInterfaces(), std::move(shells));

  // Outside a group, shells of different conformal dielectrics may not
  // overlap, except where a conductor cuts into them. A shell's region is
  // its outer box minus the shallower shells of its group. Pairs are checked
  // in index order, so the error names the same pair however the indexes
  // visit them.
  const std::vector<Shell<D>>& all = dielectric->shells();
  const SpatialIndex<D>& shell_index = dielectric->shellIndex();
  std::vector<int> candidates;
  for (size_t i = 0; i < all.size(); i++) {
    candidates.clear();
    shell_index.forEachOverlapping(all[i].outer, [&](const int j) {
      if (j > static_cast<int>(i)) {
        candidates.push_back(j);
      }
    });
    std::ranges::sort(candidates);
    for (const int j : candidates) {
      const Shell<D>& a = all[i];
      const Shell<D>& s = all[j];
      if (a.material == s.material || a.group == s.group
          || !overlaps(a.outer, s.outer)) {
        continue;
      }
      AlignedBox<D> both;
      for (int k = 0; k < D; k++) {
        both.lo[k] = std::max(a.outer.lo[k], s.outer.lo[k]);
        both.hi[k] = std::min(a.outer.hi[k], s.outer.hi[k]);
      }
      std::vector<const AlignedBox<D>*> holes;
      shell_index.forEachOverlapping(both, [&](const int k) {
        const Shell<D>& t = all[k];
        const bool shallower = (t.group == a.group && t.depth < a.depth)
                               || (t.group == s.group && t.depth < s.depth);
        if (shallower && overlaps(both, t.outer)) {
          holes.push_back(&t.outer);
        }
      });
      box_index.forEachOverlapping(both, [&](const int k) {
        if (overlaps(both, boxes[k])) {
          holes.push_back(&boxes[k]);
        }
      });
      if (anyLeft(both, holes, 0)) {
        throw std::invalid_argument("fsv: conformal dielectrics "
                                    + material_names[a.material] + " and "
                                    + material_names[s.material] + " overlap");
      }
    }
  }
  return dielectric;
}

// Rectangles of different nets may not overlap; the first pair in index
// order is reported.
void checkNoOverlap(const Geometry2D& geometry, const SpatialIndex<2>& index)
{
  const std::vector<Rect>& rects = geometry.rects();
  std::vector<int> candidates;
  for (size_t i = 0; i < rects.size(); i++) {
    candidates.clear();
    index.forEachOverlapping(rects[i], [&](const int j) {
      if (j > static_cast<int>(i)) {
        candidates.push_back(j);
      }
    });
    std::ranges::sort(candidates);
    for (const int j : candidates) {
      const int a = geometry.rectNet(static_cast<int>(i));
      const int b = geometry.rectNet(j);
      if (a != b && overlaps(rects[i], rects[j])) {
        throw std::invalid_argument("fsv: rectangles of nets "
                                    + geometry.netName(a) + " and "
                                    + geometry.netName(b) + " overlap");
      }
    }
  }
}

}  // namespace

Scene::Scene(const Geometry& geometry, const double eps_r)
    : geometry_(geometry),
      index_(std::make_unique<SpatialIndex<3>>(geometry.boxes())),
      dielectric_(uniformDielectric<3>(eps_r)),
      min_dimension_(smallestDimension(geometry.boxes()))
{
}

Scene::Scene(const Geometry& geometry, const Stack& stack)
    : geometry_(geometry),
      index_(std::make_unique<SpatialIndex<3>>(geometry.boxes())),
      dielectric_(stackDielectric<3>(
          geometry.boxes(),
          [&](const int b) { return geometry.boxNet(b); },
          *index_,
          stack)),
      min_dimension_(smallestDimension(geometry.boxes()))
{
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

Scene2D::Scene2D(const Geometry2D& geometry, const double eps_r)
    : geometry_(geometry),
      index_(std::make_unique<SpatialIndex<2>>(geometry.rects())),
      dielectric_(uniformDielectric<2>(eps_r)),
      min_dimension_(smallestDimension(geometry.rects()))
{
  checkNoOverlap(geometry, *index_);
}

Scene2D::Scene2D(const Geometry2D& geometry, const Stack& stack)
    : geometry_(geometry),
      index_(std::make_unique<SpatialIndex<2>>(geometry.rects())),
      dielectric_(stackDielectric<2>(
          geometry.rects(),
          [&](const int r) { return geometry.rectNet(r); },
          *index_,
          stack)),
      min_dimension_(smallestDimension(geometry.rects()))
{
  checkNoOverlap(geometry, *index_);
}

Scene2D::~Scene2D() = default;

}  // namespace fsv
