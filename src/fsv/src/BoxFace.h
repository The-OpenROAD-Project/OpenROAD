// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <utility>

#include "fsv/Geometry.h"

namespace fsv {

// One of the six faces of an axis-aligned box: the face normal to `axis` at
// the box's low (lo[axis]) or high (hi[axis]) coordinate.
struct BoxFace
{
  Axis axis;
  bool high;  // true: the face at hi[axis], outward normal +axis

  static constexpr int kCount = 6;

  // Dense index 0..5 for tables: x-low, x-high, y-low, y-high, z-low, z-high.
  constexpr int index() const { return 2 * fsv::index(axis) + (high ? 1 : 0); }
  static constexpr BoxFace fromIndex(const int i)
  {
    return {static_cast<Axis>(i / 2), i % 2 == 1};
  }

  // Outward normal component along `axis`: -1 or +1.
  constexpr double normalSign() const { return high ? 1.0 : -1.0; }
};

// The two axes tangent to a face normal to `axis`, in increasing order.
constexpr std::pair<Axis, Axis> tangentAxes(const Axis axis)
{
  switch (axis) {
    case Axis::kX:
      return {Axis::kY, Axis::kZ};
    case Axis::kY:
      return {Axis::kX, Axis::kZ};
    case Axis::kZ:
      break;
  }
  return {Axis::kX, Axis::kY};
}

}  // namespace fsv
