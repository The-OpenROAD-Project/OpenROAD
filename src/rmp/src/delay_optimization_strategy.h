// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "abc_library_factory.h"
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

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> Optimize(
      const abc::Abc_Ntk_t* ntk,
      AbcLibrary& abc_library,
      utl::Logger* logger) override;
};

}  // namespace rmp
