// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/Traversal.h"

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "syn/lir/Ids.h"
#include "syn/lir/Module.h"
#include "syn/lir/Op.h"

namespace syn::lir {

namespace {

// topologicalOrder(m), or, if `cycle` is not null and the DFS meets a
// fanin that is still open (a combinational cycle), stops and sets *cycle
// to that fanin, an object on the cycle.
std::vector<ObjId> dfsOrder(const Module& m, ObjId* cycle)
{
  std::vector<ObjId> order;
  if (m.isOrdered()) {
    for (const ObjId id : m.objects()) {
      if (!m.isDead(id)) {
        order.push_back(id);
      }
    }
    return order;
  }
  enum class VisitState : uint8_t
  {
    kNew,
    kOpen,
    kDone,
  };
  std::vector<VisitState> state(m.size(), VisitState::kNew);
  // Pis and state outputs read nothing: they go first, in id order, so a
  // renumbering keeps the order of ports and state.
  for (const ObjId id : m.objects()) {
    const Op op = m.op(id);
    if (!m.isDead(id) && (op == Op::kPi || isStateOut(op))) {
      state[id.value()] = VisitState::kDone;
      order.push_back(id);
    }
  }
  std::vector<std::pair<ObjIndex, int>> stack;  // object, next fanin
  for (const ObjId root : m.objects()) {
    if (m.isDead(root) || state[root.value()] != VisitState::kNew) {
      continue;
    }
    state[root.value()] = VisitState::kOpen;
    stack.emplace_back(root.value(), 0);
    while (!stack.empty()) {
      const ObjId id(stack.back().first);
      int k = stack.back().second;
      const std::span<const ObjId> fanins = m.fanins(id);
      const int n = static_cast<int>(fanins.size());
      bool descended = false;
      while (k < n) {
        const ObjId fanin = fanins[k++];
        if (!m.isValid(fanin) || m.isDead(fanin)) {
          continue;
        }
        if (state[fanin.value()] == VisitState::kOpen && cycle != nullptr) {
          *cycle = fanin;
          return order;
        }
        if (state[fanin.value()] == VisitState::kNew) {
          stack.back().second = k;
          state[fanin.value()] = VisitState::kOpen;
          stack.emplace_back(fanin.value(), 0);
          descended = true;
          break;
        }
      }
      if (!descended) {
        state[id.value()] = VisitState::kDone;
        order.push_back(id);
        stack.pop_back();
      }
    }
  }
  return order;
}

}  // namespace

std::vector<ObjId> topologicalOrder(const Module& m)
{
  // A kOpen fanin is a combinational cycle: left to checkWellFormed.
  return dfsOrder(m, nullptr);
}

absl::StatusOr<std::vector<ObjId>> acyclicTopologicalOrder(const Module& m)
{
  ObjId cycle;
  std::vector<ObjId> order = dfsOrder(m, &cycle);
  if (cycle.isValid()) {
    return absl::FailedPreconditionError(
        absl::StrCat("object ",
                     cycle.value(),
                     " (",
                     opName(m.op(cycle)),
                     ") is on a combinational cycle that no loop pair cuts"));
  }
  return order;
}

ObjId loopPairDriver(const Module& m, ObjId id)
{
  // A chain has at most one LoopOut per object, so more steps than objects
  // means it closes on itself.
  for (ObjIndex steps = 0; steps < m.size(); ++steps) {
    if (m.op(id) != Op::kLoopOut) {
      return id;
    }
    const ObjId loop_in = m.stateIn(id);
    if (!loop_in.isValid() || m.faninCount(loop_in) == 0) {
      return id;
    }
    id = m.fanin(loop_in, 0);
  }
  return id;
}

}  // namespace syn::lir
