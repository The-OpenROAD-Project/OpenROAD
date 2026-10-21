// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>

#include "fsv/Geometry.h"

namespace fsv {

class SpatialIndex;

// A geometry prepared for solving: its nearest-conductor index and the
// dielectric. Build one per geometry and reuse it for every master. The
// geometry must outlive the scene and must not change while it exists.
class Scene
{
 public:
  explicit Scene(const Geometry& geometry, double eps_r = 1.0);
  ~Scene();

  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;

  const Geometry& geometry() const { return geometry_; }
  // Relative permittivity of the (uniform) dielectric.
  double epsR() const { return eps_r_; }
  // Smallest box dimension, which scales the capture distance.
  double minDimension() const { return min_dimension_; }
  const SpatialIndex& index() const { return *index_; }

 private:
  const Geometry& geometry_;
  double eps_r_;
  double min_dimension_;
  std::unique_ptr<SpatialIndex> index_;
};

}  // namespace fsv
