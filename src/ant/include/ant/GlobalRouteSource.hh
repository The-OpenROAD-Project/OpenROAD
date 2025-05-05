// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

namespace ant {
class GlobalRouteSource
{
 public:
  virtual ~GlobalRouteSource() = default;

  virtual void makeNetWires() = 0;
};
}  // namespace ant
