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
  return netCount() - 1;
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

}  // namespace fsv
