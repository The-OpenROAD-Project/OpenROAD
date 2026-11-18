// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Module helpers for L-IR tests: adding a built module to a design, and a
// guard that lowers the module capacity.

#pragma once

#include "syn/lir/Design.h"
#include "syn/lir/Ids.h"
#include "syn/lir/Module.h"
#include "syn/lir/ModuleBuilder.h"

namespace syn::test {

// Finishes `builder` and adds the module to `design`. On failure, reports
// it with ADD_FAILURE() and returns an invalid id.
lir::ModId addModuleOrFail(lir::Design& design, lir::ModuleBuilder&& builder);

// Lowers the capacity of every module while in scope
// (Module::setCapacityForTesting).
class ModuleCapacity
{
 public:
  ModuleCapacity(lir::ObjIndex max_objects, lir::ObjIndex max_fanin_slots)
  {
    lir::Module::setCapacityForTesting(max_objects, max_fanin_slots);
  }
  ~ModuleCapacity() { lir::Module::setCapacityForTesting(); }

  ModuleCapacity(const ModuleCapacity&) = delete;
  ModuleCapacity& operator=(const ModuleCapacity&) = delete;
};

}  // namespace syn::test
