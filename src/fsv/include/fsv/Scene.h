// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace fsv {

template <int D>
class Dielectric;
template <int D>
class SpatialIndex;

// A geometry prepared for solving: its nearest-conductor index and the
// dielectric. Build one per geometry and reuse it for every master. The
// geometry must outlive the scene and must not change while it exists.
class Scene
{
 public:
  // A uniform dielectric of relative permittivity eps_r.
  explicit Scene(const Geometry& geometry, double eps_r = 1.0);
  // The dielectrics of a process stack: its planar layers, and the conformal
  // dielectrics of each conductor layer around every box whose z extent
  // equals that layer's. Throws std::invalid_argument if a permittivity is
  // not positive, if the dielectrics are not in strictly increasing order of
  // their tops, or if shells of different conformal dielectrics overlap.
  Scene(const Geometry& geometry, const Stack& stack);
  ~Scene();

  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;

  const Geometry& geometry() const { return geometry_; }
  // Relative permittivity of the planar stack at height z, away from
  // conformal dielectrics; on an interface, the one above.
  double epsAt(double z) const;
  // Planar interfaces in increasing z; empty for a uniform dielectric.
  const std::vector<Stack::PlanarInterface>& interfaces() const;
  const Dielectric<3>& dielectric() const { return *dielectric_; }
  // Smallest box dimension, which scales the capture distance.
  double minDimension() const { return min_dimension_; }
  const SpatialIndex<3>& index() const { return *index_; }

 private:
  const Geometry& geometry_;
  std::unique_ptr<Dielectric<3>> dielectric_;
  double min_dimension_;
  std::unique_ptr<SpatialIndex<3>> index_;
};

// A cross section prepared for solving, the 2D counterpart of Scene: the
// rectangles of a Geometry2D in the x-z plane, infinitely long along y.
// Conformal dielectrics grow on the sides (along x) and on top. Solving it
// gives capacitance per unit length.
class Scene2D
{
 public:
  // A uniform dielectric of relative permittivity eps_r.
  explicit Scene2D(const Geometry2D& geometry, double eps_r = 1.0);
  // The dielectrics of a process stack, as for Scene, around every
  // rectangle whose z extent equals a conductor layer's. Throws
  // std::invalid_argument for the stacks Scene rejects, and if rectangles
  // of different nets overlap.
  Scene2D(const Geometry2D& geometry, const Stack& stack);
  ~Scene2D();

  Scene2D(const Scene2D&) = delete;
  Scene2D& operator=(const Scene2D&) = delete;

  const Geometry2D& geometry() const { return geometry_; }
  const Dielectric<2>& dielectric() const { return *dielectric_; }
  // Smallest rectangle dimension, which scales the capture distance.
  double minDimension() const { return min_dimension_; }
  const SpatialIndex<2>& index() const { return *index_; }

 private:
  const Geometry2D& geometry_;
  std::unique_ptr<Dielectric<2>> dielectric_;
  double min_dimension_;
  std::unique_ptr<SpatialIndex<2>> index_;
};

}  // namespace fsv
