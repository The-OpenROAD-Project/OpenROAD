// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace fsv {

class Dielectric;
class SpatialIndex;

// Selects a cross-section scene (see Scene).
struct CrossSection
{
};

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
  // Cross sections in the y-z plane: every box is infinitely long in x, its
  // x extent ignored, and solves give capacitance per unit length (fF/µm).
  // Conformal dielectrics grow on the sides only (there are no line ends).
  Scene(const Geometry& geometry, double eps_r, CrossSection);
  Scene(const Geometry& geometry, const Stack& stack, CrossSection);
  ~Scene();

  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;

  // The geometry as solved: for a cross section, a copy whose boxes are
  // stretched along x. Nets and boxes keep their indices.
  const Geometry& geometry() const { return geometry_; }
  bool crossSection() const { return stretched_ != nullptr; }
  // Relative permittivity of the planar stack at height z, away from
  // conformal dielectrics; on an interface, the one above.
  double epsAt(double z) const;
  // Planar interfaces in increasing z; empty for a uniform dielectric.
  const std::vector<Stack::PlanarInterface>& interfaces() const;
  const Dielectric& dielectric() const { return *dielectric_; }
  // Smallest box dimension, which scales the capture distance.
  double minDimension() const { return min_dimension_; }
  const SpatialIndex& index() const { return *index_; }

 private:
  Scene(std::unique_ptr<Geometry> stretched,
        const Geometry& geometry,
        double eps_r);
  void addStack(const Stack& stack);

  std::unique_ptr<Geometry> stretched_;  // for a cross section
  const Geometry& geometry_;
  std::unique_ptr<Dielectric> dielectric_;
  double min_dimension_;
  std::unique_ptr<SpatialIndex> index_;
};

}  // namespace fsv
