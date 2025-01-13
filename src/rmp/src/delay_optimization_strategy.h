// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "base/abc/abc.h"
#include "db_sta/dbSta.hh"
#include "logic_optimization_strategy.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rmp {

class DelayOptimizationStrategy : public LogicOptimizationStrategy
{
 public:
  ~DelayOptimizationStrategy() override = default;

  DelayOptimizationStrategy(sta::dbSta* sta) : sta_(sta) {}

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> Optimize(
      const abc::Abc_Ntk_t* ntk,
      utl::Logger* logger) override;

 private:
  sta::dbSta* sta_;
};

}  // namespace rmp
