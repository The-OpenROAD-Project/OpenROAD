// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Geometry.h"

#include <stdexcept>
#include <string>
#include <string_view>

namespace fsv {

int Geometry::addNet(const std::string_view name)
{
  net_names_.emplace_back(name);
  grounded_.push_back(false);
  return netCount() - 1;
}

void Geometry::setGrounded(const int net)
{
  if (net < 0 || net >= netCount()) {
    throw std::invalid_argument("fsv: unknown net " + std::to_string(net));
  }
  grounded_[net] = true;
}

void Geometry::setWindow(const AlignedBox<2>& xy)
{
  if (!(xy.lo[0] < xy.hi[0] && xy.lo[1] < xy.hi[1])) {
    throw std::invalid_argument("fsv: empty window");
  }
  window_ = xy;
}

void Geometry::addBox(const int net, const Box& box)
{
  if (net < 0 || net >= netCount()) {
    throw std::invalid_argument("fsv: unknown net " + std::to_string(net));
  }
  for (int k = 0; k < 3; k++) {
    if (!(box.lo[k] < box.hi[k])) {
      throw std::invalid_argument("fsv: empty box");
    }
  }
  if (box.lo[2] <= 0) {
    throw std::invalid_argument("fsv: box must lie above the ground plane");
  }
  boxes_.push_back(box);
  box_nets_.push_back(net);
}

int Geometry2D::addNet(const std::string_view name)
{
  net_names_.emplace_back(name);
  return netCount() - 1;
}

void Geometry2D::addRect(const int net, const Rect& rect)
{
  if (net < 0 || net >= netCount()) {
    throw std::invalid_argument("fsv: unknown net " + std::to_string(net));
  }
  for (int k = 0; k < 2; k++) {
    if (!(rect.lo[k] < rect.hi[k])) {
      throw std::invalid_argument("fsv: empty rectangle");
    }
  }
  if (rect.lo[1] <= 0) {
    throw std::invalid_argument(
        "fsv: rectangle must lie above the ground plane");
  }
  rects_.push_back(rect);
  rect_nets_.push_back(net);
}

}  // namespace fsv
