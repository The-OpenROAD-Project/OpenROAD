// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbNetwork;
}

namespace wmk {

// Embedding and verification share the same Liberty-based definition of the
// carrier and its sequential fanout. Unknown cells cannot establish a parity.
odb::dbNet* singleOutputNet(odb::dbInst* inst);
bool isSequentialClockSink(odb::dbITerm* iterm, sta::dbNetwork* network);
std::optional<int> seqFanout(odb::dbInst* lcb, sta::dbNetwork* network);
std::vector<odb::dbInst*> findLeafClockBuffers(odb::dbBlock* block,
                                               sta::dbNetwork* network);

struct ClockBranch
{
  odb::dbNet* source;
  bool inverted;
  bool operator==(const ClockBranch&) const = default;
};

// Trace only unconditional Liberty buffers/inverters. Gates, muxes and
// sequential cells are boundaries, even when case analysis makes them
// transparent. Equal source nets and inversion parity preserve clock logic.
// Floating nets, multiple drivers and cycles cannot establish equivalence.
std::optional<ClockBranch> clockBranch(odb::dbInst* lcb,
                                       sta::dbNetwork* network);

}  // namespace wmk
