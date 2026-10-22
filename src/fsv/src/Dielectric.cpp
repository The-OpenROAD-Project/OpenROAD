// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Dielectric.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "BoxFace.h"
#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace fsv {

namespace {

// Planes closer than this (µm) are the same plane.
constexpr double kCoplanar = 1e-9;

bool strictlyInside(const Vec3& p, const Box& b)
{
  for (int k = 0; k < 3; k++) {
    if (!(p[k] > b.lo[k] && p[k] < b.hi[k])) {
      return false;
    }
  }
  return true;
}

}  // namespace

Dielectric::Dielectric(const double base_eps,
                       std::vector<Stack::PlanarInterface> planar,
                       std::vector<Shell> shells)
    : base_eps_(base_eps),
      planar_(std::move(planar)),
      shells_(std::move(shells))
{
}

double Dielectric::planarEps(const double z) const
{
  // The first interface above z gives the permittivity below it; on an
  // interface, the one above applies.
  const auto above
      = std::ranges::upper_bound(planar_, z, {}, &Stack::PlanarInterface::z);
  if (above != planar_.end()) {
    return above->eps_below;
  }
  return planar_.empty() ? base_eps_ : planar_.back().eps_above;
}

const Shell* Dielectric::shellAt(const Vec3& p) const
{
  // The shallowest shell containing p: within a group that is the union
  // rule, and across groups only shells of one conformal dielectric may
  // overlap.
  const Shell* best = nullptr;
  for (const Shell& s : shells_) {
    if (strictlyInside(p, s.outer)
        && (best == nullptr || s.depth < best->depth)) {
      best = &s;
    }
  }
  return best;
}

double Dielectric::eps(const Vec3& p) const
{
  const Shell* s = shellAt(p);
  return s != nullptr ? s->eps : planarEps(p[2]);
}

template <typename Visit>
void Dielectric::forEachPiece(const Vec3& p,
                              const double max_half_size,
                              Visit visit) const
{
  // The nearest two planar interfaces. Inside a shell they do not apply, but
  // a cube split by a shell face reaches outside it, where they do.
  if (!planar_.empty()) {
    const auto above = std::ranges::upper_bound(
        planar_, p[2], {}, &Stack::PlanarInterface::z);
    const int count = static_cast<int>(planar_.size());
    int up = static_cast<int>(above - planar_.begin());
    int down = up - 1;
    for (int pass = 0; pass < 2 && (up < count || down >= 0); pass++) {
      const double du = up < count ? planar_[up].z - p[2] : max_half_size;
      const double dd = down >= 0 ? p[2] - planar_[down].z : max_half_size;
      const int i = du <= dd ? up++ : down--;
      const double d = std::min(du, dd);
      if (d < max_half_size) {
        visit(Piece{Axis::kZ, planar_[i].z, d});
      }
    }
  }
  // The faces of the shell boxes.
  for (const Shell& s : shells_) {
    if (linfDistance(p, s.outer) >= max_half_size) {
      continue;
    }
    for (const Axis axis : kAxes) {
      const int a = index(axis);
      const auto [t0, t1] = tangentAxes(axis);
      double excess = 0;
      for (const int t : {index(t0), index(t1)}) {
        excess = std::max({excess, s.outer.lo[t] - p[t], p[t] - s.outer.hi[t]});
      }
      for (const double c : {s.outer.lo[a], s.outer.hi[a]}) {
        const double d = std::max(std::abs(p[a] - c), excess);
        if (d < max_half_size) {
          visit(Piece{axis, c, d});
        }
      }
    }
  }
}

Dielectric::Cube Dielectric::cube(const Vec3& p,
                                  const double max_half_size,
                                  const double min_half_size) const
{
  // Interface pieces closer than max_half_size. The nearest plane may cut
  // the cube; every other piece limits its size, so each half of the cube
  // holds no interface and is uniform.
  bool any = false;
  Piece nearest{Axis::kZ, 0, max_half_size};
  forEachPiece(p, max_half_size, [&](const Piece& piece) {
    if (!any || piece.distance < nearest.distance) {
      nearest = piece;
      any = true;
    }
  });
  if (!any) {
    return {max_half_size};
  }
  double half_size = max_half_size;
  forEachPiece(p, max_half_size, [&](const Piece& piece) {
    const bool coplanar
        = piece.axis == nearest.axis
          && std::abs(piece.coordinate - nearest.coordinate) <= kCoplanar;
    if (!coplanar) {
      half_size = std::min(half_size, piece.distance);
    }
  });
  const int a = index(nearest.axis);
  const double distance = std::abs(p[a] - nearest.coordinate);
  Cube result{half_size};
  if (half_size < min_half_size) {
    half_size = min_half_size;
    result = {half_size};
    result.floored = true;
  }
  if (!(distance < half_size)) {
    return result;
  }
  // One point on each side of the plane, inside the cube.
  const double depth = 0.5 * (half_size - distance);
  Vec3 minus = p;
  Vec3 plus = p;
  minus[a] = nearest.coordinate - depth;
  plus[a] = nearest.coordinate + depth;
  result.eps_minus = eps(minus);
  result.eps_plus = eps(plus);
  if (result.eps_minus != result.eps_plus) {
    result.split = true;
    result.axis = nearest.axis;
    result.coordinate = nearest.coordinate;
    result.distance = distance;
  }
  return result;
}

}  // namespace fsv
