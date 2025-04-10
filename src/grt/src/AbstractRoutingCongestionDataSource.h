// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

namespace grt {

class AbstractRoutingCongestionDataSource
{
 public:
  virtual ~AbstractRoutingCongestionDataSource() = default;

  virtual void registerHeatMap() = 0;
  virtual void update() = 0;
};

}  // namespace grt
