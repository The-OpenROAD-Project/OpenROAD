// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "src/dbSta/include/db_sta/dbSta.hh"
#include "src/rmp/src/resynthesis_strategy.h"
#include "src/rsz/include/rsz/Resizer.hh"
#include "src/sta/include/sta/Scene.hh"
#include "src/utl/include/utl/Logger.h"
#include "src/utl/include/utl/unique_name.h"

namespace rmp {

class ZeroSlackStrategy : public ResynthesisStrategy
{
 public:
  explicit ZeroSlackStrategy(sta::Scene* corner = nullptr) : corner_(corner) {}
  void OptimizeDesign(sta::dbSta* sta,
                      utl::UniqueName& name_generator,
                      rsz::Resizer* resizer,
                      utl::Logger* logger) override;

 private:
  sta::Scene* corner_;
};

}  // namespace rmp
