// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Shared view of the clock tree for the CTS watermark.
//
// The embedder and the verifier must agree exactly on what a leaf clock buffer
// is and what its sequential fanout is: the mark is the parity of that count,
// so a definition that drifts between the two turns a valid watermark into a
// failed one.  Both sides call these.

#pragma once

#include <vector>

#include "odb/db.h"

namespace wmk {

// The net an instance's single output drives, or null when the instance does
// not have exactly one output.
//
// Exactly one is the point, not an incidental bound.  It is what keeps
// multi-output cells out of the clock-buffer set: a flop presenting Q and QB
// is not a buffer, and a divider driving the tree from Q is not one either.
// Neither may lend or receive a sink, so both must answer null here.
odb::dbNet* singleOutputNet(odb::dbInst* inst);

// Is this the clock pin of a sequential instance?
//
// A CLOCK-typed pin counts directly.  Some libraries leave the pin untyped, so
// fall back to a sequential master with a conventionally named clock pin.
bool isSequentialClockSink(odb::dbITerm* iterm);

// Number of sequential sinks on a leaf clock buffer's output net.  The parity
// of this count is what the CTS watermark sets.
int seqFanout(odb::dbInst* lcb);

// Leaf clock buffers: instances driving a net that reaches at least one
// sequential clock pin.  These are the only instances the watermark touches.
std::vector<odb::dbInst*> findLeafClockBuffers(odb::dbBlock* block);

}  // namespace wmk
