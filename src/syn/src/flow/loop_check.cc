// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Detect combinational loops in the graph. Latches from the elaborator
// arrive as feedback muxes, and combinational optimization does not preserve
// the state such a loop holds, so loops are rejected rather than mapped.

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/synthesis.h"
#include "utl/Logger.h"

namespace syn {

namespace {

constexpr size_t kMaxReportedLoops = 10;
constexpr size_t kMaxReportedNames = 8;

// Map each net to a user-visible name, preferring non-tentative names.
std::unordered_map<uint32_t, std::string> collectNetNames(const Graph& g)
{
  std::unordered_map<uint32_t, std::string> names;
  std::unordered_map<uint32_t, bool> tentative;
  g.forEachInstance([&](const Instance* inst) {
    const Name* name = inst->try_as<Name>();
    if (!name) {
      return;
    }
    const Bundle& value = name->value();
    for (uint32_t i = 0; i < value.width(); i++) {
      const Net net = value[i];
      if (net.isConst()) {
        continue;
      }
      const uint32_t id = Graph::netId(net);
      auto it = tentative.find(id);
      if (it != tentative.end() && (name->tentative() || !it->second)) {
        continue;
      }
      std::string str = name->nameStr();
      if (name->isVector()) {
        // Same bit mapping as Synthesis::resolveNetRef, so the name can be
        // passed back to dump_fanin_cone.
        const uint32_t index
            = name->from() <= name->to() ? name->from() + i : name->from() - i;
        str += "[" + std::to_string(index) + "]";
      }
      names[id] = std::move(str);
      tentative[id] = name->tentative();
    }
  });
  return names;
}

// For each output bit of a liberty cell, the input bits it depends on
// combinationally: those named in its function or three_state expression,
// plus those with a combinational or tristate timing arc to it. Bits are
// indexed as in a Target (see importTargets): ports in liberty order, power
// pins skipped, bidirect ports counted as both an input and an output.
using CellDeps = std::vector<std::vector<uint32_t>>;

CellDeps computeCellDeps(const sta::LibertyCell* cell)
{
  std::vector<const sta::LibertyPort*> in_bits;
  std::vector<const sta::LibertyPort*> out_bits;
  // Bit and bus ports map to the indices of the bits they cover.
  std::unordered_map<const sta::LibertyPort*, std::vector<uint32_t>> in_idx;
  std::unordered_map<const sta::LibertyPort*, std::vector<uint32_t>> out_idx;
  auto addBits
      = [](const sta::LibertyPort* port,
           std::vector<const sta::LibertyPort*>& bits,
           std::unordered_map<const sta::LibertyPort*, std::vector<uint32_t>>&
               idx) {
          for (int j = 0; j < port->size(); j++) {
            const sta::LibertyPort* bit = port;
            if (port->isBus()) {
              const int index = port->fromIndex() < port->toIndex()
                                    ? port->fromIndex() + j
                                    : port->fromIndex() - j;
              bit = port->findLibertyBusBit(index);
            }
            const auto i = static_cast<uint32_t>(bits.size());
            bits.push_back(bit);
            idx[bit].push_back(i);
            if (bit != port) {
              idx[port].push_back(i);
            }
          }
        };

  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    const sta::LibertyPort* port = port_iter.next();
    if (port->isPwrGnd()) {
      continue;
    }
    const sta::PortDirection* dir = port->direction();
    if (dir->isInput() || dir->isBidirect()) {
      addBits(port, in_bits, in_idx);
    }
    if (dir->isOutput() || dir->isBidirect()) {
      addBits(port, out_bits, out_idx);
    }
  }

  std::vector<std::vector<bool>> depends(
      out_bits.size(), std::vector<bool>(in_bits.size(), false));
  for (size_t o = 0; o < out_bits.size(); o++) {
    const sta::FuncExpr* function = out_bits[o]->function();
    const sta::FuncExpr* enable = out_bits[o]->tristateEnable();
    for (size_t i = 0; i < in_bits.size(); i++) {
      if ((function && function->hasPort(in_bits[i]))
          || (enable && enable->hasPort(in_bits[i]))) {
        depends[o][i] = true;
      }
    }
  }
  for (const sta::TimingArcSet* arc_set : cell->timingArcSets()) {
    const sta::TimingRole* role = arc_set->role();
    if (role != sta::TimingRole::combinational()
        && role != sta::TimingRole::tristateEnable()
        && role != sta::TimingRole::tristateDisable()) {
      continue;
    }
    auto from = in_idx.find(arc_set->from());
    auto to = out_idx.find(arc_set->to());
    if (from == in_idx.end() || to == out_idx.end()) {
      continue;
    }
    for (uint32_t o : to->second) {
      for (uint32_t i : from->second) {
        depends[o][i] = true;
      }
    }
  }

  CellDeps deps(out_bits.size());
  for (size_t o = 0; o < out_bits.size(); o++) {
    for (size_t i = 0; i < in_bits.size(); i++) {
      if (depends[o][i]) {
        deps[o].push_back(i);
      }
    }
  }
  return deps;
}

// Combinational fanins of a net. Dffs, Others and inputs cut the dependency;
// a Target output depends only on the inputs its liberty model connects it
// to, so an output with neither a function nor a combinational arc (e.g. a
// RAM read port) cuts it too.
class CombFanins
{
 public:
  explicit CombFanins(const Graph& g) : g_(g) {}

  template <typename F>
  void visit(Net net, F&& fn)
  {
    if (net.isConst()) {
      return;
    }
    const auto [inst, offset] = g_.resolve(net);
    auto visit = [&](Net fanin) {
      if (!fanin.isConst()) {
        fn(fanin);
      }
    };
    if (const Target* target = inst->try_as<Target>()) {
      const CellDeps& deps = cellDeps(target->cell());
      const Bundle& inputs = target->inputs();
      if (offset >= deps.size()) {
        // Layout disagrees with the liberty model; assume full dependency.
        inputs.visit(visit);
        return;
      }
      for (uint32_t i : deps[offset]) {
        if (i < inputs.width()) {
          visit(inputs[i]);
        }
      }
      return;
    }
    if (inst->hasState() || inst->is<Input>()) {
      return;
    }
    if (inst->isSliceable()) {
      inst->visitSlice(offset, visit);
    } else {
      inst->visit(visit);
    }
  }

 private:
  const CellDeps& cellDeps(const sta::LibertyCell* cell)
  {
    auto it = cell_deps_.find(cell);
    if (it == cell_deps_.end()) {
      it = cell_deps_.emplace(cell, computeCellDeps(cell)).first;
    }
    return it->second;
  }

  const Graph& g_;
  std::unordered_map<const sta::LibertyCell*, CellDeps> cell_deps_;
};

// Strongly connected components of the combinational dependency graph.
struct Sccs
{
  std::vector<uint32_t> id;  // per net
  std::vector<Net> cyclic;   // one net from each component that has a cycle
};

// Iterative Tarjan over all live nets.
Sccs computeSccs(const Graph& g, CombFanins& comb)
{
  constexpr uint32_t kUnvisited = UINT32_MAX;
  const size_t n = g.tableSize();
  Sccs sccs;
  sccs.id.assign(n, kUnvisited);
  std::vector<uint32_t> index(n, kUnvisited);
  std::vector<uint32_t> lowlink(n, 0);
  std::vector<bool> on_stack(n, false);
  std::vector<uint32_t> stack;
  uint32_t next_index = 0;
  uint32_t next_scc = 0;

  struct Frame
  {
    uint32_t id;
    std::vector<uint32_t> fanins;
    bool self_loop = false;
    size_t next = 0;
  };

  auto makeFrame = [&](uint32_t id) {
    Frame frame{id, {}};
    comb.visit(Graph::netFromId(id), [&](Net fanin) {
      frame.fanins.push_back(Graph::netId(fanin));
      frame.self_loop |= Graph::netId(fanin) == id;
    });
    index[id] = lowlink[id] = next_index++;
    stack.push_back(id);
    on_stack[id] = true;
    return frame;
  };

  g.forEachNet([&](Net root, const Instance*, uint32_t) {
    if (root.isConst() || index[Graph::netId(root)] != kUnvisited) {
      return;
    }
    std::vector<Frame> call_stack;
    call_stack.push_back(makeFrame(Graph::netId(root)));
    while (!call_stack.empty()) {
      Frame& frame = call_stack.back();
      if (frame.next < frame.fanins.size()) {
        const uint32_t fanin = frame.fanins[frame.next++];
        if (index[fanin] == kUnvisited) {
          call_stack.push_back(makeFrame(fanin));
        } else if (on_stack[fanin]) {
          lowlink[frame.id] = std::min(lowlink[frame.id], index[fanin]);
        }
        continue;
      }
      const uint32_t id = frame.id;
      if (lowlink[id] == index[id]) {
        size_t size = 0;
        uint32_t member;
        do {
          member = stack.back();
          stack.pop_back();
          on_stack[member] = false;
          sccs.id[member] = next_scc;
          size++;
        } while (member != id);
        if (size > 1 || frame.self_loop) {
          sccs.cyclic.push_back(Graph::netFromId(id));
        }
        next_scc++;
      }
      call_stack.pop_back();
      if (!call_stack.empty()) {
        Frame& parent = call_stack.back();
        lowlink[parent.id] = std::min(lowlink[parent.id], lowlink[id]);
      }
    }
  });
  return sccs;
}

// Find a combinational cycle through `net` within its strongly connected
// component. Returns the nets on the cycle, starting and ending at `net`.
std::vector<Net> findCycle(CombFanins& comb, const Sccs& sccs, Net net)
{
  const uint32_t scc = sccs.id[Graph::netId(net)];
  std::unordered_map<uint32_t, Net> parent;  // fanin -> the net it feeds
  std::vector<Net> stack;
  auto expand = [&](Net from) {
    comb.visit(from, [&](Net fanin) {
      if (sccs.id[Graph::netId(fanin)] == scc
          && parent.emplace(Graph::netId(fanin), from).second) {
        stack.push_back(fanin);
      }
    });
  };

  expand(net);
  while (!stack.empty()) {
    const Net cur = stack.back();
    stack.pop_back();
    if (cur == net) {
      std::vector<Net> cycle{net};
      for (Net n = parent.at(Graph::netId(net)); n != net;
           n = parent.at(Graph::netId(n))) {
        cycle.push_back(n);
      }
      cycle.push_back(net);
      return cycle;
    }
    expand(cur);
  }
  return {};
}

}  // namespace

void checkCombinationalLoops(Graph& g, utl::Logger* logger)
{
  // Normalize so only live logic is checked. Its loop breakers are not
  // relied on: normalization assumes every Target output depends on every
  // input and treats sequential Targets as boundaries, so cycles are found
  // here from per-pin liberty dependencies instead.
  g.normalize();

  CombFanins comb(g);
  const Sccs sccs = computeSccs(g, comb);
  if (sccs.cyclic.empty()) {
    return;
  }

  const auto names = collectNetNames(g);
  for (size_t i = 0; i < sccs.cyclic.size() && i < kMaxReportedLoops; i++) {
    std::string nets;
    size_t reported = 0;
    std::vector<Net> cycle = findCycle(comb, sccs, sccs.cyclic[i]);
    if (!cycle.empty()) {
      cycle.pop_back();  // the start net is repeated at the end
    }
    for (Net net : cycle) {
      const uint32_t id = Graph::netId(net);
      auto it = names.find(id);
      if (it == names.end() && g.resolve(net).first->is<LoopBreaker>()) {
        continue;  // an alias of its input, which is also on the cycle
      }
      if (reported == kMaxReportedNames) {
        nets += ", ...";
        break;
      }
      // Unnamed nets print as %<id>, which dump_fanin_cone also accepts.
      nets += (reported ? ", " : "")
              + (it != names.end() ? it->second : "%" + std::to_string(id));
      reported++;
    }
    logger->warn(utl::SYN, 79, "Combinational loop through: {}", nets);
  }

  logger->error(utl::SYN,
                80,
                "Design contains {} combinational loop(s). Combinational "
                "loops, including latches inferred from incomplete "
                "assignments in always blocks, are not supported.",
                sccs.cyclic.size());
}

}  // namespace syn
