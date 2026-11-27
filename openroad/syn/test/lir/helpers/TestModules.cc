// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "helpers/TestModules.h"

#include <utility>

#include "absl/status/statusor.h"
#include "gtest/gtest.h"
#include "syn/lir/Design.h"
#include "syn/lir/Ids.h"
#include "syn/lir/Module.h"
#include "syn/lir/ModuleBuilder.h"

namespace syn::test {
namespace {

using ::syn::lir::Design;
using ::syn::lir::ModId;
using ::syn::lir::Module;
using ::syn::lir::ModuleBuilder;

}  // namespace

ModId addModuleOrFail(Design& design, ModuleBuilder&& builder)
{
  absl::StatusOr<Module> module = std::move(builder).finish();
  if (!module.ok()) {
    ADD_FAILURE() << module.status();
    return ModId();
  }
  const absl::StatusOr<ModId> id = design.addModule(*std::move(module));
  if (!id.ok()) {
    ADD_FAILURE() << id.status();
    return ModId();
  }
  return *id;
}

}  // namespace syn::test
