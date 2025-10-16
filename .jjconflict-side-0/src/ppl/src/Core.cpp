// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "Core.h"

#include <vector>

#include "odb/geom.h"

namespace ppl {

Core::Core()
{
  database_unit_ = 0;
}

int Core::getPerimeter() const
{
  int x = boundary_.xMax() - boundary_.xMin();
  int y = boundary_.yMax() - boundary_.yMin();

  return (x + y) * 2;
}

std::vector<odb::Line> Core::getDieAreaEdges()
{
  return die_area_edges_;
}

odb::Point Core::getMirroredPosition(const odb::Point& position) const
{
  odb::Point mirrored_pos = position;
  const int x_min = boundary_.xMin();
  const int x_max = boundary_.xMax();
  const int y_min = boundary_.yMin();
  const int y_max = boundary_.yMax();

  if (position.x() == x_min) {
    mirrored_pos.setX(x_max);
  } else if (position.x() == x_max) {
    mirrored_pos.setX(x_min);
  } else if (position.y() == y_min) {
    mirrored_pos.setY(y_max);
  } else {
    mirrored_pos.setY(y_min);
  }

  return mirrored_pos;
}

}  // namespace ppl
