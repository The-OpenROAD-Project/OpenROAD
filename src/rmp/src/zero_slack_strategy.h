// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "db_sta/dbSta.hh"
#include "resynthesis_strategy.h"
#include "rsz/Resizer.hh"
#include "sta/Scene.hh"
#include "utl/Logger.h"
#include "utl/unique_name.h"

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
