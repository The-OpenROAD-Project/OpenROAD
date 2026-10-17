// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "object.h"
#include "odb/geom.h"
#include "packing.h"

namespace mpl {

// The packing problem an annealer is asked to solve: its first
// `sequence_pair_macros` macros (0: all) inside `outline`. The remaining
// macros are terminals for wirelength only. Macro locations are relative to
// the outline's lower-left corner, as the annealer keeps them.
// `macro_of_block`, when given, receives each block's index in `macros`;
// macros that take no room have no block.
PackingProblem toPackingProblem(const odb::Rect& outline,
                                const std::vector<SoftMacro>& macros,
                                int sequence_pair_macros,
                                std::vector<int>* macro_of_block = nullptr);
PackingProblem toPackingProblem(const odb::Rect& outline,
                                const std::vector<HardMacro>& macros,
                                int sequence_pair_macros,
                                std::vector<int>* macro_of_block = nullptr);

}  // namespace mpl
