// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "db_sta/dbSta.hh"
#include "resynthesis_strategy.h"
#include "utl/Logger.h"

namespace rmp {

class ZeroSlackStrategy : public ResynthesisStrategy
{
 public:
  void OptimizeDesign(sta::dbSta* sta, utl::Logger* logger) override;
};

}  // namespace rmp
