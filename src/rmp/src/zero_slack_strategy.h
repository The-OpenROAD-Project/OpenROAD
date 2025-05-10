// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "db_sta/dbSta.hh"
#include "resynthesis_strategy.h"
#include "rmp/unique_name.h"
#include "sta/Corner.hh"
#include "utl/Logger.h"

namespace rmp {

class ZeroSlackStrategy : public ResynthesisStrategy
{
 public:
  explicit ZeroSlackStrategy(sta::Corner* corner = nullptr) : corner_(corner) {}
  void OptimizeDesign(sta::dbSta* sta,
                      UniqueName& name_generator,
                      utl::Logger* logger) override;

 private:
  sta::Corner* corner_;
};

}  // namespace rmp
