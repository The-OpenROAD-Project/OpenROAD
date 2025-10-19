// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {

class definPolygon
{
  std::vector<Point> _points;

 public:
  definPolygon() {}
  definPolygon(const std::vector<Point>& points);
  void decompose(std::vector<Rect>& rect);
};

}  // namespace odb
