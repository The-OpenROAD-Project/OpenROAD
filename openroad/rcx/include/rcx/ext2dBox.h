// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>

namespace rcx {

class ext2dBox  // assume cross-section on the z-direction
{
 public:
  // Implementation note: a default constructor is necessary because these are
  // AthPool allocated, and it requires the type to be default constructible.
  // We use placement new (with the full constructor below) for initialization
  // after pool allocation.
  ext2dBox() = default;

  ext2dBox(std::array<int, 2> ll, std::array<int, 2> ur) : _ll(ll), _ur(ur) {}

  int loX() const { return _ll[0]; }
  int loY() const { return _ll[1]; }

  int ur0() const { return _ur[0]; }
  int ur1() const { return _ur[1]; }
  int ll0() const { return _ll[0]; }
  int ll1() const { return _ll[1]; }

 private:
  std::array<int, 2> _ll;
  std::array<int, 2> _ur;
};

}  // namespace rcx
