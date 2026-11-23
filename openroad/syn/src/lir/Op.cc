// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/Op.h"

#include <array>
#include <string_view>

namespace syn::lir {

namespace {

constexpr std::array<std::string_view, kNumOps> kOpNames = {
    // Structure.
    "pi",
    "po",
    "const",
    "undriven",
    "buf",
    "slice",
    "concat",
    "replicate",
    "cast",
    "inst",
    "fan",
    // State.
    "reg_out",
    "reg_in",
    "loop_out",
    "loop_in",
    "mem_out",
    "mem_in",
    "mem_read",
    "mem_write",
    // Bitwise and selection.
    "not",
    "and",
    "or",
    "xor",
    "mux",
    "pmux",
    "bmux",
    "demux",
    // Arithmetic.
    "neg",
    "add",
    "sub",
    "mul",
    "div",
    "mod",
    "pow",
    // Comparison.
    "eq",
    "lt",
    // Shifts.
    "shl",
    "shr",
    "shiftx",
    // Reductions.
    "reduce_and",
    "reduce_or",
    "reduce_xor",
    // Mapped.
    "lut",
    "cell",
    // Tri-state.
    "tribuf",
    "tri_join",
};

constexpr bool allNamed()
{
  for (const std::string_view name : kOpNames) {
    if (name.empty()) {
      return false;
    }
  }
  return true;
}
static_assert(allNamed(), "kOpNames is missing an entry");

}  // namespace

std::string_view opName(Op op)
{
  const int i = static_cast<int>(op);
  return i < kNumOps ? kOpNames[i] : "?";
}

}  // namespace syn::lir
