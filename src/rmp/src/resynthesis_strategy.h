// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "db_sta/dbSta.hh"
#include "utl/Logger.h"

namespace rmp {

class ResynthesisStrategy
{
 public:
  virtual ~ResynthesisStrategy() = default;
  virtual void OptimizeDesign(sta::dbSta* sta, utl::Logger* logger) = 0;
};
}  // namespace rmp