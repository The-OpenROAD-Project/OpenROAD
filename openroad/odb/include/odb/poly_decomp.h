// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "odb/geom.h"

namespace odb {

//
// Decompose a simple rectilinear polygon into rectangles.
// The polygon points are assumed to be in clockwise order.
//
void decompose_polygon(const std::vector<Point>& points,
                       std::vector<Rect>& rects);

// Returns true if the verticies of this polygon are clockwise orderd.
bool polygon_is_clockwise(const std::vector<Point>& points);

}  // namespace odb
