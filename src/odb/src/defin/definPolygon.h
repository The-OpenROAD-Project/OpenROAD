// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "odb/geom.h"

namespace odb {

class definPolygon
{
 public:
  definPolygon() = default;
  definPolygon(const std::vector<Point>& points);
  void decompose(std::vector<Rect>& rect);

 private:
  std::vector<Point> _points;
};

}  // namespace odb
