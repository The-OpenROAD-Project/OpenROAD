// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Scene.h"

#include <algorithm>
#include <limits>
#include <memory>

#include "SpatialIndex.h"
#include "fsv/Geometry.h"

namespace fsv {

Scene::Scene(const Geometry& geometry, const double eps_r)
    : geometry_(geometry),
      eps_r_(eps_r),
      min_dimension_(std::numeric_limits<double>::max()),
      index_(std::make_unique<SpatialIndex>(geometry.boxes()))
{
  for (const Box& b : geometry.boxes()) {
    for (int k = 0; k < 3; k++) {
      min_dimension_ = std::min(min_dimension_, b.hi[k] - b.lo[k]);
    }
  }
}

Scene::~Scene() = default;

}  // namespace fsv
