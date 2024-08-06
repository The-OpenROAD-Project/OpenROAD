// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "odb/db.h"

namespace grt {

class AbstractGrouteRenderer
{
 public:
  virtual ~AbstractGrouteRenderer() = default;

  virtual void highlightRoute(odb::dbNet* net,
                              bool show_segments,
                              bool show_pin_locations)
      = 0;

  virtual void clearRoute() = 0;
};

}  // namespace grt
