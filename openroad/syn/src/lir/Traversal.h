// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Read-only queries derived from a Module's public API: traversal orders
// and loop-pair resolution.

#pragma once

#include <vector>

#include "absl/status/statusor.h"
#include "syn/lir/Ids.h"
#include "syn/lir/Module.h"

namespace syn::lir {

// The live objects of `m`, every fanin before its reader. Id order if `m`
// isOrdered(), else the Pis and state outputs in id order, then a DFS
// post-order of the rest that stays close to id order. Objects on a
// combinational cycle (an Editor edit can close one) appear in DFS order;
// use acyclicTopologicalOrder() to reject them.
std::vector<ObjId> topologicalOrder(const Module& m);

// topologicalOrder(m), or FailedPreconditionError naming an object on a
// combinational cycle that no loop pair cuts. An ordered module has none.
absl::StatusOr<std::vector<ObjId>> acyclicTopologicalOrder(const Module& m);

// The real driver behind a loop pair: for a LoopOut, the object driving its
// LoopIn, following chained pairs. Returns `id` itself if it is not a linked
// LoopOut; if the chain loops back on itself, returns one of its LoopOuts.
ObjId loopPairDriver(const Module& m, ObjId id);

}  // namespace syn::lir
