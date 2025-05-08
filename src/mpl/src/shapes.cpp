// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "shapes.h"

namespace mpl {

/*
  Tiling Operators

  The original implentation used std::pair to represent tilings.
  These overloads mimic the behavior w.r.t how tilings would get
  ordered in a std::set.
*/

bool Tiling::operator==(const Tiling& tiling) const
{
  return width_ == tiling.width() && height_ == tiling.height();
}

bool Tiling::operator<(const Tiling& tiling) const
{
  if (width_ != tiling.width()) {
    return width_ < tiling.width();
  }

  return height_ < tiling.height();
}

}  // namespace mpl
