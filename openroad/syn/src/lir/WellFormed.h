// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Invariant checker for L-IR modules and designs. Run it explicitly, or set
// DesignOptions::check_well_formed to run it on install() and Editor::commit.
// It checks the rules of Op.h and RegPinTypes.h, ports, instances and the
// absence of combinational cycles; an instance counts as connecting every
// input to every output, so feedback through a child may be reported as a
// cycle. Errors are InvalidArgument for the first violation, naming the module
// and the object.

#pragma once

#include "absl/status/status.h"
#include "syn/lir/Design.h"
#include "syn/lir/Module.h"
#include "syn/lir/OperatorSpec.h"

namespace syn::lir {

// Checks one module against the Design that owns (or will own) it.
absl::Status checkWellFormed(const Design& design, const Module& module);

// Checks every module, that the instance hierarchy has no cycles, that
// every module under a dont_touch instance or module is frozen, and that the
// stored top is not a blackbox, an operator implementation or instantiated.
absl::Status checkWellFormed(const Design& design);

// Checks that `module`'s ports follow `spec`: one input per port operand,
// in order and with its width, and one output with the result width.
absl::Status checkOperatorInterface(const Design& design,
                                    const Module& module,
                                    const OpSpec& spec);

// Checks that every port has a valid name and no two ports share one.
absl::Status checkPortNames(const Module& module);

}  // namespace syn::lir
