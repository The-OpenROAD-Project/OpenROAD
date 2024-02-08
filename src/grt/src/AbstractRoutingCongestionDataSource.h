// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
