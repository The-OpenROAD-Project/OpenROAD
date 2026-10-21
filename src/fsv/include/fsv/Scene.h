// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace fsv {

class SpatialIndex;

// A geometry prepared for solving: its nearest-conductor index and the
// dielectric. Build one per geometry and reuse it for every master. The
// geometry must outlive the scene and must not change while it exists.
class Scene
{
 public:
  // A uniform dielectric of relative permittivity eps_r.
  explicit Scene(const Geometry& geometry, double eps_r = 1.0);
  // The planar dielectrics of a process stack. Throws std::invalid_argument
  // if a permittivity is not positive, if the dielectrics are not in
  // strictly increasing order of their tops, or if the stack has conformal
  // dielectrics, which are not supported yet.
  Scene(const Geometry& geometry, const Stack& stack);
  ~Scene();

  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;

  const Geometry& geometry() const { return geometry_; }
  // Relative permittivity at height z; on an interface, the one above.
  double epsAt(double z) const;
  // Planar interfaces in increasing z; empty for a uniform dielectric.
  const std::vector<Stack::PlanarInterface>& interfaces() const
  {
    return interfaces_;
  }
  // Smallest box dimension, which scales the capture distance.
  double minDimension() const { return min_dimension_; }
  const SpatialIndex& index() const { return *index_; }

 private:
  const Geometry& geometry_;
  // Permittivity below the first interface (everywhere, when uniform).
  double base_eps_;
  std::vector<Stack::PlanarInterface> interfaces_;
  double min_dimension_;
  std::unique_ptr<SpatialIndex> index_;
};

}  // namespace fsv
