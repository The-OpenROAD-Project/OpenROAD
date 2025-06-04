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

namespace lext {

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> WrapUnique(abc::Abc_Ntk_t* ntk);
void AbcPrintStats(const abc::Abc_Ntk_t* ntk);
utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> BufferNetwork(
    abc::Abc_Ntk_t* ntk,
    lext::AbcLibrary& abc_sc_library);

class DelayOptimizationStrategy : public LogicOptimizationStrategy
{
 public:
  ~DelayOptimizationStrategy() override = default;

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> Optimize(
      const abc::Abc_Ntk_t* ntk,
      AbcLibrary& abc_library,
      utl::Logger* logger) override;
};

}  // namespace lext
