// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "base/abc/abc.h"
#include "cut/abc_library_factory.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rmp {

// The `LogicOptimizationStrategy` class defines an interface for different
// logic optimization strategies that can be applied to an ABC network.
//
// It provides a virtual `Optimize` method that takes an ABC network and a
// logger as input and returns an optimized version of the network. Concrete
// implementations of this class will implement specific optimization algorithms
// such as delay optimization, area optimization, or a combination of both.
class LogicOptimizationStrategy
{
 public:
  virtual ~LogicOptimizationStrategy() = default;

  // Optimize the given ABC network.
  //
  // This function takes an ABC network (`abc::Abc_Ntk_t`) and optimizes it
  // using a specific logic optimization strategy. The optimized network is
  // returned as a `utl::UniquePtrWithDeleter<abc::Abc_Ntk_t>`.
  //
  // **Important:** This function **does not modify the input network** (`ntk`).
  //
  // Args:
  //   ntk: A pointer to the ABC network to be optimized. The network should
  //        be a technology-mapped network (i.e., a netlist of standard cells).
  //   logger: A pointer to the logger for reporting messages.
  //
  // Returns:
  //   A unique pointer (`utl::UniquePtrWithDeleter`) to the optimized ABC
  //   network. The deleter ensures that `abc::Abc_NtkDelete` is called when
  //   the network is no longer needed.
  //
  // The function is expected to perform the following actions:
  // 1. Apply logic optimization techniques (e.g., technology mapping,
  //    buffering, area/delay optimization) to the copy.
  // 2. Return a new network that is optimized according to the strategy.
  virtual utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> Optimize(
      const abc::Abc_Ntk_t* ntk,
      cut::AbcLibrary& abc_library,
      utl::Logger* logger)
      = 0;
};

}  // namespace rmp
