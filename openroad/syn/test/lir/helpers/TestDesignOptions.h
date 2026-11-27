// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Design options for L-IR tests: kCheckWellFormed, so install() and
// Editor::commit check each edit.

#pragma once

#include "syn/lir/Design.h"

namespace syn::test {

inline constexpr lir::DesignOptions kCheckWellFormed{.check_well_formed = true};

}  // namespace syn::test
