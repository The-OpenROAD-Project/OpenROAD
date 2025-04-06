// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

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
