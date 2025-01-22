// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "db_sta/dbSta.hh"
#include "utl/Logger.h"

namespace rmp {

struct OptimizationResult
{
  double initial_area;
  double final_area;
  double initial_wns;
  double final_wns;
  double initial_tns;
  double final_tns;
};

class ResynthesisStrategy
{
 public:
  virtual ~ResynthesisStrategy() = default;
  virtual OptimizationResult OptimizeDesign(sta::dbSta* sta,
                                            utl::Logger* logger)
      = 0;
};
}  // namespace rmp