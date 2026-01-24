// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "base/abc/abc.h"
#include "logic_optimization_strategy.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rmp {

void AbcPrintStats(const abc::Abc_Ntk_t* ntk);
utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> BufferNetwork(
    abc::Abc_Ntk_t* ntk,
    cut::AbcLibrary& abc_sc_library);

class DelayOptimizationStrategy : public LogicOptimizationStrategy
{
 public:
  ~DelayOptimizationStrategy() override = default;

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> Optimize(
      const abc::Abc_Ntk_t* ntk,
      cut::AbcLibrary& abc_library,
      utl::Logger* logger) override;
};

}  // namespace rmp
