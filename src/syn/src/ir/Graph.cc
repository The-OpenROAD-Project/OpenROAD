// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/ir/Graph.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <new>
#include <ostream>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "sta/Liberty.hh"
#include "syn/ir/Bundle.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/ir/NetTableEntry.h"
#include "syn/ir/TritModel.h"
#include "utl/Logger.h"

namespace syn {

Graph::Graph()
{
  // Pre-allocate the three tie-off constants at indices 0, 1, 2.
  // Tie-offs fit inline (8 bytes <= kSlotSize).
  NetTableId id0 = table_.allocateSlots(1);
  new (table_.pointer(id0)) TieLow();
  static_cast<NetTableEntry*>(table_.pointer(id0))
      ->setObjectIndex(id0 & NetTableEntry::kNetTableIdxMask);

  NetTableId id1 = table_.allocateSlots(1);
  new (table_.pointer(id1)) TieHigh();
  static_cast<NetTableEntry*>(table_.pointer(id1))
      ->setObjectIndex(id1 & NetTableEntry::kNetTableIdxMask);

  NetTableId id2 = table_.allocateSlots(1);
  new (table_.pointer(id2)) TieX();
  static_cast<NetTableEntry*>(table_.pointer(id2))
      ->setObjectIndex(id2 & NetTableEntry::kNetTableIdxMask);
}

Graph::Graph(Graph&& other) noexcept
    : name_(std::move(other.name_)),
      logger_(other.logger_),
      table_(std::move(other.table_)),
      heap_instances_(std::move(other.heap_instances_)),
      normalized_(other.normalized_)
{
}

Graph& Graph::operator=(Graph&& other) noexcept
{
  if (this != &other) {
    for (auto* inst : heap_instances_) {
      inst->destroy();
      ::operator delete(inst);
    }
    name_ = std::move(other.name_);
    logger_ = other.logger_;
    table_ = std::move(other.table_);
    heap_instances_ = std::move(other.heap_instances_);
    normalized_ = other.normalized_;
  }
  return *this;
}

Graph::~Graph()
{
  for (auto* inst : heap_instances_) {
    inst->destroy();
    ::operator delete(inst);
  }
}

std::pair<const Instance*, uint32_t> Graph::resolve(Net net) const
{
  const NetTableId id = Graph::netId(net);
  const NetTableEntry& e = table_.entry(id);
  if (e.entryType() == EntryType::kPlaceholder) {
    auto& ph = static_cast<const PlaceholderEntry&>(e);
    return {ph.instance(), ph.offset()};
  }
  // Inline instance.
  return {static_cast<const Instance*>(table_.pointer(id)), 0};
}

std::pair<Instance*, uint32_t> Graph::resolve(Net net)
{
  const NetTableId id = Graph::netId(net);
  const NetTableEntry& e = table_.entry(id);
  if (e.entryType() == EntryType::kPlaceholder) {
    auto& ph = static_cast<const PlaceholderEntry&>(e);
    return {ph.instance(), ph.offset()};
  }
  // Inline instance.
  return {static_cast<Instance*>(table_.pointer(id)), 0};
}

NetTableId Graph::baseId(const Instance* inst) const
{
  if (inst->isInline()) {
    auto* entry = static_cast<const NetTableEntry*>(inst);
    return NetTableBlock::blockOf(entry)->slotId(entry);
  }
  return inst->baseIndex();
}

NetTableId Graph::resolveId(Net net) const
{
  const NetTableId id = Graph::netId(net);
  const NetTableEntry& e = table_.entry(id);
  if (e.entryType() == EntryType::kPlaceholder) {
    auto& ph = static_cast<const PlaceholderEntry&>(e);
    return id - ph.offset();
  }
  return id;
}

BundleView Graph::output(const Instance* inst) const
{
  return BundleView(Net(baseId(inst)), inst->outputWidth());
}

Buffer* Graph::bufferHeapOutput(Instance* inst)
{
  assert(!inst->isInline());

  const NetTableId old_base = inst->baseIndex();
  const uint32_t slot_count = std::max(inst->outputWidth(), 1u);

  // Relocate the heap instance to new contiguous slots.
  NetTableId new_base = table_.allocateSlots(slot_count);
  for (uint32_t i = 0; i < slot_count; i++) {
    new (table_.pointer(new_base + i)) PlaceholderEntry(inst, i);
  }
  inst->setBaseIndex(new_base);

  // Create a wide buffer at the old slots forwarding to the new location.
  Bundle input(Net(new_base), slot_count);
  void* mem = ::operator new(sizeof(BufferWide));
  new (mem) BufferWide(std::move(input));
  auto* buf = static_cast<Buffer*>(mem);
  heap_instances_.insert(buf);
  buf->setBaseIndex(old_base);
  for (uint32_t i = 0; i < slot_count; i++) {
    new (table_.pointer(old_base + i)) PlaceholderEntry(buf, i);
  }
  return buf;
}

void Graph::removeInstance(Instance* inst)
{
  assert(!inst->isInline());
  const NetTableId base = inst->baseIndex();
  const uint32_t slot_count = std::max(inst->outputWidth(), 1u);
  for (uint32_t i = 0; i < slot_count; i++) {
    new (table_.pointer(base + i)) NetTableEntry();
  }
  assert(heap_instances_.count(inst));
  heap_instances_.erase(inst);
  inst->destroy();
  ::operator delete(inst);
  normalized_ = false;
}

void Graph::forceReplace(BundleView target, BundleView replacement)
{
  assert(target.width() == replacement.width());

  for (uint32_t i = 0; i < target.width(); i++) {
    if (target[i] == replacement[i]) {
      continue;
    }

    const auto [inst, offset] = resolve(target[i]);
    if (inst->is<Buffer>()) {
      inst->as<Buffer>()->setA(offset, replacement[i]);
    } else if (inst->isInline()) {
      const NetTableId old_id = netId(target[i]);
      new (table_.pointer(old_id)) BufferFine(replacement[i]);
      static_cast<NetTableEntry*>(table_.pointer(old_id))
          ->setObjectIndex(old_id & NetTableEntry::kNetTableIdxMask);
    } else {
      Buffer* buffer = bufferHeapOutput(inst);
      buffer->setA(offset, replacement[i]);
    }

    normalized_ = false;
  }
}

void Graph::replace(BundleView target,
                    BundleView replacement,
                    Equivalence type,
                    BundleView verificationSupport)
{
  assert(target.width() == replacement.width());
  if (logger_ && logger_->debugCheck(utl::SYN, "eqsan", 1)) {
    Solver solver;
    TritModel model(solver, *this);

    // Encode only the logic cone from target/replacement back to support.
    std::vector<Net> roots;
    roots.reserve(target.width() + replacement.width());
    for (uint32_t i = 0; i < target.width(); i++) {
      roots.push_back(target[i]);
    }
    for (uint32_t i = 0; i < replacement.width(); i++) {
      roots.push_back(replacement[i]);
    }
    model.encodeCone(verificationSupport,
                     BundleView(Bundle::fromVec(std::move(roots))));

    if (type == Equivalence::TwoValued) {
      for (uint32_t i = 0; i < verificationSupport.width(); i++) {
        solver.addClause({model.defVar(verificationSupport[i])});
      }
    }

    const uint32_t width = target.width();

    // Build per-bit "differs" indicators, then assert at least one is true.
    // If UNSAT → no counterexample → substitution is valid.
    std::vector<int> differs;
    for (uint32_t i = 0; i < width; i++) {
      const int t_v = model.valVar(target[i]), t_d = model.defVar(target[i]);
      const int r_v = model.valVar(replacement[i]);
      const int r_d = model.defVar(replacement[i]);

      if (type == Equivalence::Full) {
        // Differ if definedness disagrees, or both defined but values differ.
        int def_diff = solver.encodeXor(t_d, r_d);
        int both_def = solver.encodeAnd(t_d, r_d);
        int val_diff = solver.encodeAnd(both_def, solver.encodeXor(t_v, r_v));
        differs.push_back(solver.encodeOr(val_diff, def_diff));
      } else {
        // Refinement: target defined but replacement not, or values disagree.
        int target_def_repl_undef = solver.encodeAnd(t_d, -r_d);
        int both_def = solver.encodeAnd(t_d, r_d);
        int val_diff = solver.encodeXor(t_v, r_v);
        int both_def_val_diff = solver.encodeAnd(both_def, val_diff);
        differs.push_back(
            solver.encodeOr(target_def_repl_undef, both_def_val_diff));
      }
    }

    // Assert at least one bit differs.
    int any_diff = differs[0];
    for (size_t i = 1; i < differs.size(); i++) {
      any_diff = solver.encodeOr(any_diff, differs[i]);
    }
    solver.addClause({any_diff});

    auto result = solver.solve();
    if (result != Solver::Unsat) {
      std::stringstream ss;
      ss << "replace: SAT verification failed (status "
         << (result == Solver::Sat ? "SAT" : "UNKNOWN") << ")\n";
      ss << "replacing ";
      writeValue(ss, *this, target);
      ss << " with ";
      writeValue(ss, *this, replacement);
      ss << " under support ";
      writeValue(ss, *this, verificationSupport);
      ss << " failed:\n";
      ss << "support cex ";
      writeValue(
          ss, *this, Bundle::fromConst(model.value(verificationSupport)));
      ss << "\n";
      ss << "target cex ";
      writeValue(ss, *this, Bundle::fromConst(model.value(target)));
      ss << "\n";
      ss << "replacement cex ";
      writeValue(ss, *this, Bundle::fromConst(model.value(replacement)));
      ss << "\n";
      model.dumpCone(ss);
      logger_->reportLiteral(ss.str());
      logger_->error(
          utl::SYN,
          101,
          "SAT verification of optimization failed, see details above");
    }
  }

  forceReplace(target, replacement);
}

void Graph::normalize()
{
  if (normalized_) {
    return;
  }

  checkConsistency();

  const size_t old_size = table_.size();
  std::vector<bool> live(old_size, false);
  live[0] = true;
  live[1] = true;
  live[2] = true;

  struct TentativeNameInfo
  {
    std::string name;
    Bundle value;
    uint32_t from;
    uint32_t to;
    bool is_vector;
  };
  std::vector<TentativeNameInfo> tentative_names;

  // Perform liveness analysis and collect tentative names.
  {
    std::vector<NetTableId> queue;
    auto markLive = [&](Net net) {
      if (net.isConst()) {
        return;
      }
      assert(!net.isSentinel());
      NetTableId slot = Graph::netId(net);
      if (!live[slot]) {
        live[slot] = true;
        queue.push_back(slot);
      }
    };
    forEachInstance([&](const Instance* inst) {
      if (inst->is<Name>() && inst->as<Name>()->tentative()) {
        auto* nm = inst->as<Name>();
        tentative_names.push_back({std::string(nm->nameStr()),
                                   Bundle(nm->value()),
                                   nm->from(),
                                   nm->to(),
                                   nm->isVector()});
        return;
      }
      if (inst->hasEffects() || inst->is<Input>()) {
        for (auto bit : output(inst)) {
          live[Graph::netId(bit)] = true;
        }
        inst->visit([&](Net net) { markLive(net); });
      }
    });
    while (!queue.empty()) {
      NetTableId slot = queue.back();
      queue.pop_back();
      auto [inst, bit_offset] = resolve(Net(slot));
      inst->visitSlice(bit_offset, [&](Net net) { markLive(net); });
    }

    // If a tentative Name references a net through an inverter whose
    // input is live, keep that inverter alive so the Name stays valid.
    for (auto& tn : tentative_names) {
      for (uint32_t i = 0; i < tn.value.width(); i++) {
        Net net = tn.value[i];
        if (net.isConst()) {
          continue;
        }
        NetTableId slot = Graph::netId(net);
        if (live[slot]) {
          continue;
        }

        // strip any buffers or inveters
        ControlNet cnet = ControlNet::pos(net);
        Net lastInverter = Net::sentinel();
        while (true) {
          auto [inst, offset] = resolve(cnet.net());
          if (auto* not1 = inst->try_as<Not>()) {
            lastInverter = output(not1)[offset];
            cnet = ControlNet::withPolarity(not1->a()[offset],
                                            !cnet.isPositive());
          } else if (auto* buf = inst->try_as<Buffer>()) {
            cnet
                = ControlNet::withPolarity(buf->a()[offset], cnet.isPositive());
          } else {
            break;
          }
        }

        if (live[Graph::netId(cnet.net())]) {
          if (cnet.isPositive()) {
            tn.value.mutableNet(i) = cnet.net();
          } else {
            markLive(lastInverter);
            if (lastInverter != net) {
              tn.value.mutableNet(i) = lastInverter;
            }
          }
        }
      }
    }

    // Drain the queue to propagate liveness from nets made live by
    // tentative Name processing (e.g. naming-only inverters).
    while (!queue.empty()) {
      NetTableId slot = queue.back();
      queue.pop_back();
      auto [inst, bit_offset] = resolve(Net(slot));
      inst->visitSlice(bit_offset, [&](Net net) { markLive(net); });
    }
  }

  // Emit instances into `new_table` in topological order.
  // At first, all nets in use old indices. We map from old
  // indices to new indices via `net_map` in place as a final step.
  NetTable new_table;
  std::vector<uint32_t> net_map(old_size, Graph::netId(Net::sentinel()));

  // Allocate constants (same as constructor).
  NetTableId c0 = new_table.allocateSlots(1);
  new (new_table.pointer(c0)) TieLow();
  static_cast<NetTableEntry*>(new_table.pointer(c0))
      ->setObjectIndex(c0 & NetTableEntry::kNetTableIdxMask);
  net_map[0] = c0;

  NetTableId c1 = new_table.allocateSlots(1);
  new (new_table.pointer(c1)) TieHigh();
  static_cast<NetTableEntry*>(new_table.pointer(c1))
      ->setObjectIndex(c1 & NetTableEntry::kNetTableIdxMask);
  net_map[1] = c1;

  NetTableId c2 = new_table.allocateSlots(1);
  new (new_table.pointer(c2)) TieX();
  static_cast<NetTableEntry*>(new_table.pointer(c2))
      ->setObjectIndex(c2 & NetTableEntry::kNetTableIdxMask);
  net_map[2] = c2;

  std::unordered_set<Instance*> new_heap;
  std::unordered_set<const Instance*> emitted_set;

  // Emit an instance into the new table.
  auto emit = [&](const Instance* inst) {
    NetTableId old_base = baseId(inst);
    // LoopBreaker insertion may have grown the old table beyond net_map size.
    uint32_t slots_needed = old_base + std::max(inst->outputWidth(), 1u);
    if (slots_needed > net_map.size()) {
      net_map.resize(slots_needed, Graph::netId(Net::sentinel()));
    }
    if (inst->isInline()) {
      uint32_t slot_count = std::max(inst->outputWidth(), 1u);
      NetTableId new_base = new_table.allocateSlots(slot_count);
      std::memcpy(new_table.pointer(new_base), inst, kSlotSize);
      static_cast<NetTableEntry*>(new_table.pointer(new_base))
          ->setObjectIndex(new_base & NetTableEntry::kNetTableIdxMask);
      auto* new_inst = static_cast<Instance*>(new_table.pointer(new_base));
      for (uint32_t i = 1; i < slot_count; ++i) {
        new (new_table.pointer(new_base + i)) PlaceholderEntry(new_inst, i);
      }
      for (uint32_t i = 0; i < slot_count; ++i) {
        net_map[old_base + i] = new_base + i;
      }
    } else {
      assert(emitted_set.insert(inst).second && "duplicate heap emit");
      new_heap.insert(const_cast<Instance*>(inst));
      uint32_t slot_count = std::max(inst->outputWidth(), 1u);
      NetTableId new_base = new_table.allocateSlots(slot_count);
      for (uint32_t i = 0; i < slot_count; i++) {
        new (new_table.pointer(new_base + i))
            PlaceholderEntry(const_cast<Instance*>(inst), i);
        net_map[old_base + i] = new_base + i;
      }
      // Do not update baseIndex() — the instance still points into the old
      // table so that output(inst) continues to return old-table net IDs.
      // Callers must call setBaseIndex() after they are done reading old
      // outputs.
    }
  };

  auto emitSlice = [&](const Instance* inst, uint32_t start, uint32_t width) {
    // For simplicity construct the slice in the old table
    // before passing it to emit() like any other instance
    Net lsb = Net::sentinel();
    if (inst->is<Buffer>()) {
      lsb = add<Buffer>(
          static_cast<const Buffer*>(inst)->a().slice(start, width))[0];
    } else if (inst->is<Not>()) {
      lsb = add<Not>(static_cast<const Not*>(inst)->a().slice(start, width))[0];
    } else if (inst->is<And>()) {
      auto* op = static_cast<const And*>(inst);
      lsb = add<And>(op->a().slice(start, width),
                     op->b().slice(start, width))[0];
    } else if (inst->is<Or>()) {
      auto* op = static_cast<const Or*>(inst);
      lsb = add<Or>(op->a().slice(start, width),
                    op->b().slice(start, width))[0];
    } else if (inst->is<Andnot>()) {
      auto* op = static_cast<const Andnot*>(inst);
      lsb = add<Andnot>(op->a().slice(start, width),
                        op->b().slice(start, width))[0];
    } else if (inst->is<Xor>()) {
      auto* op = static_cast<const Xor*>(inst);
      lsb = add<Xor>(op->a().slice(start, width),
                     op->b().slice(start, width))[0];
    } else if (inst->is<Mux>()) {
      auto* op = static_cast<const Mux*>(inst);
      lsb = add<Mux>(op->sel(),
                     op->a().slice(start, width),
                     op->b().slice(start, width))[0];
    } else if (inst->is<Dangling>()) {
      lsb = add<Dangling>(width)[0];
    } else {
      std::abort();
    }
    auto* new_inst = resolve(lsb).first;
    emit(new_inst);

    // `new_inst` was emitted into the new table. `inst` will not be emitted.
    // References to `inst` need to refer to `new_inst` instead, update the net
    // mapping accordingly.
    for (uint32_t i = 0; i < width; i++) {
      net_map[output(inst)[start + i].id_] = net_map[output(new_inst)[i].id_];
    }
  };

  std::unordered_set<NetTableId> topo_stack;
  std::vector<const Instance*> deferred_stateful;
  std::unordered_set<NetTableId> topo_visited;

  topo_visited.insert(0);
  topo_visited.insert(1);
  topo_visited.insert(2);

  forEachInstance([&](const Instance* inst) {
    if (inst->hasEffects() || inst->is<Input>()) {
      emit(inst);
      for (auto bit : output(inst)) {
        topo_visited.insert(bit.id_);
      }
    }
  });

  std::function<void(Net)> topo_dfs = [&](Net net) {
    if (topo_visited.contains(net.id_)) {
      return;
    }
    assert(!topo_stack.count(net.id_)
           && "topo_dfs re-entered for net on stack");
    assert(net.id_ < live.size() && live[net.id_]
           && "topo_dfs on dead or out-of-bounds net");
    topo_stack.insert(net.id_);

    auto [inst, offset] = resolve(net);
    if (inst->is<Buffer>() || inst->is<LoopBreaker>()) {
      Net fanin = inst->is<Buffer>() ? inst->as<Buffer>()->a()[offset]
                                     : inst->as<LoopBreaker>()->a()[offset];
      if (topo_stack.contains(fanin.id_)) {
        Net breaker_net = add<LoopBreaker>(fanin).asNet();
        auto* breaker = resolve(breaker_net).first;
        emit(breaker);
        topo_visited.insert(Graph::netId(breaker_net));
        net_map[net.id_] = net_map[Graph::netId(breaker_net)];
      } else {
        topo_dfs(fanin);
        net_map[net.id_] = net_map[fanin.id_];
      }
      topo_visited.insert(net.id_);
    } else if (inst->hasState()) {
      // Sequential boundary: emit now, mark outputs visited.
      // Inputs are DFS'd later (after all outputs are visited
      // so feedback paths don't create loop_breakers).
      emit(inst);
      for (auto bit : output(inst)) {
        topo_visited.insert(Graph::netId(bit));
      }
      deferred_stateful.push_back(inst);
    } else {
      if (inst->isSliceable()) {
        const_cast<Instance*>(inst)->visitSliceMut(offset, [&](Net& fanin) {
          if (topo_stack.contains(fanin.id_)) {
            // Patch in an instance to break the loop
            Net breaker_net = add<LoopBreaker>(fanin).asNet();
            auto* breaker = resolve(breaker_net).first;
            emit(breaker);
            fanin = breaker_net;
            topo_visited.insert(Graph::netId(breaker_net));
          } else {
            topo_dfs(fanin);
          }
        });

        uint32_t bottom = offset;
        uint32_t top = offset + 1;

        while (bottom > 0) {
          Net output_net = output(inst)[bottom - 1];
          if (!live[output_net.id_] || topo_visited.contains(output_net.id_)
              || topo_stack.contains(output_net.id_)) {
            break;
          }

          bool abort_expansion = false;
          inst->visitSlice(bottom - 1, [&](Net fanin) {
            if (!topo_visited.contains(fanin.id_)) {
              abort_expansion = true;
            }
          });
          if (abort_expansion) {
            break;
          }
          --bottom;
        }

        while (top < inst->outputWidth()) {
          Net output_net = output(inst)[top];
          if (!live[output_net.id_] || topo_visited.contains(output_net.id_)
              || topo_stack.contains(output_net.id_)) {
            break;
          }

          bool abort_expansion = false;
          inst->visitSlice(top, [&](Net fanin) {
            if (!topo_visited.contains(fanin.id_)) {
              abort_expansion = true;
            }
          });
          if (abort_expansion) {
            break;
          }
          ++top;
        }

        if (bottom == 0 && top == inst->outputWidth()) {
          emit(inst);
        } else {
          emitSlice(inst, bottom, top - bottom);
        }

        for (uint32_t i = bottom; i < top; i++) {
          assert(!topo_visited.count(Graph::netId(output(inst)[i]))
                 && "sliceable instance already visited before emit");
          topo_visited.insert(Graph::netId(output(inst)[i]));
        }
      } else {
        // For non-sliceable instances, all output bits share the same
        // inputs. Push all output bits onto the stack so that cycles
        // through any bit are detected.
        for (auto bit : output(inst)) {
          topo_stack.insert(Graph::netId(bit));
        }
        const_cast<Instance*>(inst)->visitMut([&](Net& fanin) {
          if (topo_stack.contains(fanin.id_)) {
            // Patch in an instance to break the loop
            Net breaker_net = add<LoopBreaker>(fanin).asNet();
            auto* breaker = resolve(breaker_net).first;
            emit(breaker);
            fanin = breaker_net;
            topo_visited.insert(Graph::netId(breaker_net));
          } else {
            topo_dfs(fanin);
          }
        });
        assert(!topo_visited.count(Graph::netId(output(inst)[0]))
               && "non-sliceable instance already visited before emit");
        emit(inst);
        for (auto bit : output(inst)) {
          topo_visited.insert(Graph::netId(bit));
          topo_stack.erase(Graph::netId(bit));
        }
      }
    }
    topo_stack.erase(net.id_);
    assert(topo_visited.count(net.id_));
  };

  // Kick off topo DFS from each input net of root instances.
  forEachInstance([&](const Instance* inst) {
    if (inst->hasEffects()) {
      inst->visit([&](Net net) { topo_dfs(net); });
    }
  });

  // DFS the inputs of stateful instances (sequential Targets, Dffs)
  // whose outputs are already visited — feedback won't create loop_breakers.
  // topo_dfs can push onto deferred_stateful, so re-check size() each
  // iteration; a range-for would cache end() and miss late entries.
  // NOLINTNEXTLINE(modernize-loop-convert)
  for (size_t si = 0; si < deferred_stateful.size(); si++) {
    deferred_stateful[si]->visit([&](Net net) { topo_dfs(net); });
  }

  // Visit any live nets not reached from effect instances (e.g. naming-only
  // inverters kept alive by tentative Name processing).
  for (size_t i = 3; i < live.size(); i++) {
    if (live[i]) {
      topo_dfs(Net(i));
    }
  }

  // Update baseIndex on heap instances by scanning the new table for
  // offset-0 placeholder entries.
  new_table.forEachId([&](NetTableId id) {
    auto& e = new_table.entry(id);
    if (e.entryType() == EntryType::kPlaceholder) {
      auto& ph = static_cast<PlaceholderEntry&>(e);
      if (ph.offset() == 0) {
        ph.instance()->setBaseIndex(id);
      }
    }
  });

  auto remap = [&](Net& net) {
    uint32_t id = Graph::netId(net);
    assert(id < net_map.size() && "net id out of range in remap");
    if (net_map[id] == Graph::netId(Net::sentinel())) {
      dumpInstance(std::cout, resolve(net).first);
      std::cout << '\n';
      printf(
          "live: out of range %d live %d\n", id < live.size(), (int) live[id]);
    }
    assert(net_map[id] != Graph::netId(Net::sentinel()) && "dead net in remap");
    net = Net(net_map[id]);
  };

  // Remap inline instances in the new table.
  new_table.forEachId([&](NetTableId id) {
    auto& e = new_table.entry(id);
    auto type = e.entryType();
    if (type == EntryType::kPlaceholder || type == EntryType::kVoid) {
      return;
    }
    auto* inst = static_cast<Instance*>(new_table.pointer(id));
    inst->visitMut(remap);
  });

  // Remap heap instances.
  for (auto* inst : new_heap) {
    inst->visitMut(remap);
  }

  // Re-add tentative names. For each one, find contiguous live slices
  // and emit a tentative Name with adjusted from/to for each slice.
  for (auto& tn : tentative_names) {
    tn.value.visitMut([&](Net& net) {
      uint32_t id = Graph::netId(net);
      if (id < net_map.size() && net_map[id] != Graph::netId(Net::sentinel())) {
        net = Net(net_map[id]);
      } else {
        net = Net::sentinel();
      }
    });
  }
  for (auto& tn : tentative_names) {
    uint32_t width = tn.value.width();
    uint32_t i = 0;
    while (i < width) {
      // Skip dead bits.
      if (tn.value[i] == Net::sentinel()) {
        i++;
        continue;
      }
      // Start of a live slice.
      uint32_t start = i;
      while (i < width && tn.value[i] != Net::sentinel()) {
        i++;
      }
      uint32_t slice_width = i - start;
      uint32_t slice_from = tn.from + start;
      uint32_t slice_to = tn.from + i;

      void* mem = ::operator new(sizeof(Name));
      new (mem) Name(std::string(tn.name),
                     tn.value.slice(start, slice_width),
                     slice_from,
                     slice_to,
                     true,
                     tn.is_vector);
      auto* inst = static_cast<Instance*>(mem);
      new_heap.insert(inst);

      NetTableId slot = new_table.allocateSlots(1);
      inst->setBaseIndex(slot);
      new (new_table.pointer(slot)) PlaceholderEntry(inst, 0);
    }
  }

  // Delete all non-surviving heap instances.
  for (auto* inst : heap_instances_) {
    if (new_heap.contains(inst) == 0) {
      inst->destroy();
      ::operator delete(inst);
    }
  }

  table_ = std::move(new_table);
  heap_instances_ = std::move(new_heap);

  checkNormalization();
  normalized_ = true;
}

void Graph::checkNormalization() const
{
  forEachInstance([&](const Instance* inst) {
    // Skip instances whose inputs are allowed to follow their outputs
    if (inst->is<Output>() || inst->is<LoopBreaker>() || inst->hasState()
        || inst->hasEffects()) {
      return;
    }

    NetTableId inst_base = baseId(inst);
    inst->visit([&](Net net) {
      if (net.isConst()) {
        return;
      }
      if (Graph::netId(net) >= inst_base) {
        std::stringstream ss;
        ss << "checkNormalization: not in topological order\n";
        ss << "  referencing:\n";
        dumpInstance(ss, inst);
        ss << '\n';
        auto [prod, off] = resolve(net);
        ss << "  referenced\n";
        dumpInstance(ss, prod);
        ss << '\n';
        if (logger_) {
          logger_->reportLiteral(ss.str());
          logger_->error(
              utl::SYN, 102, "checkNormalization failed, see details above");
        } else {
          std::cerr << ss.str() << '\n';
          std::abort();
        }
      }
    });
  });
}

void Graph::makeNamesTentative()
{
  forEachInstance([&](const Instance* inst) {
    if (inst->is<Name>()) {
      const_cast<Name*>(inst->as<Name>())->setTentative(true);
    }
  });
}

void Graph::checkConsistency() const
{
  // Collect all slots owned by heap instances.
  std::unordered_set<NetTableId> owned_slots;

  for (auto* inst : heap_instances_) {
    NetTableId base = inst->baseIndex();
    uint32_t slot_count = std::max(inst->outputWidth(), 1u);

    for (uint32_t i = 0; i < slot_count; i++) {
      NetTableId id = base + i;
      if (id >= table_.size()) {
        logger_->error(utl::SYN,
                       103,
                       "checkConsistency: heap instance type={} base={} "
                       "slot {} ({}) out of table bounds (size={})",
                       cellKeyword(inst->entryType()),
                       base,
                       i,
                       id,
                       table_.size());
      }
      auto& e = table_.entry(id);
      if (e.entryType() != EntryType::kPlaceholder) {
        logger_->error(utl::SYN,
                       104,
                       "checkConsistency: heap instance type={} base={} "
                       "slot {} ({}) is not a placeholder (got type={})\n",
                       cellKeyword(inst->entryType()),
                       base,
                       i,
                       id,
                       cellKeyword(e.entryType()));
      }
      auto& ph = static_cast<const PlaceholderEntry&>(e);
      if (ph.instance() != inst) {
        logger_->error(
            utl::SYN,
            105,
            "checkConsistency: heap instance type={} base={} "
            "slot {} ({}) placeholder points to different instance\n",
            cellKeyword(inst->entryType()),
            base,
            i,
            id);
      }
      if (ph.offset() != i) {
        logger_->error(utl::SYN,
                       106,
                       "checkConsistency: heap instance type={} base={} "
                       "slot {} ({}) placeholder offset={} (expected {})\n",
                       cellKeyword(inst->entryType()),
                       base,
                       i,
                       id,
                       ph.offset(),
                       i);
      }
      owned_slots.insert(id);
    }
  }

  // Multi-bit inline instances live in slot 0 of their output range and own
  // PlaceholderEntries at slots 1..outputWidth-1 pointing back to themselves.
  table_.forEachId([&](NetTableId id) {
    auto& e = table_.entry(id);
    auto type = e.entryType();
    if (type == EntryType::kVoid || type == EntryType::kPlaceholder) {
      return;
    }
    auto* inst = static_cast<const Instance*>(table_.pointer(id));
    uint32_t output_width = inst->outputWidth();
    for (uint32_t i = 1; i < output_width; i++) {
      NetTableId slot = id + i;
      if (slot >= table_.size()) {
        logger_->error(
            utl::SYN,
            107,
            "checkConsistency: inline instance type={} at {} slot {} "
            "({}) out of table bounds (size={})\n",
            cellKeyword(type),
            id,
            i,
            slot,
            table_.size());
      }
      auto& se = table_.entry(slot);
      if (se.entryType() != EntryType::kPlaceholder) {
        logger_->error(
            utl::SYN,
            108,
            "checkConsistency: inline instance type={} at {} slot {} "
            "({}) is not a placeholder (got type={})\n",
            cellKeyword(type),
            id,
            i,
            slot,
            cellKeyword(se.entryType()));
      }
      auto& ph = static_cast<const PlaceholderEntry&>(se);
      if (ph.instance() != inst) {
        logger_->error(
            utl::SYN,
            109,
            "checkConsistency: inline instance type={} at {} slot {} "
            "({}) placeholder points to different instance\n",
            cellKeyword(type),
            id,
            i,
            slot);
      }
      if (ph.offset() != i) {
        logger_->error(
            utl::SYN,
            110,
            "checkConsistency: inline instance type={} at {} slot {} "
            "({}) placeholder offset={} (expected {})\n",
            cellKeyword(type),
            id,
            i,
            slot,
            ph.offset(),
            i);
      }
      owned_slots.insert(slot);
    }
  });

  // Check that no placeholder exists without a corresponding heap instance.
  table_.forEachId([&](NetTableId id) {
    auto& e = table_.entry(id);
    if (e.entryType() == EntryType::kPlaceholder) {
      if (owned_slots.contains(id) == 0) {
        auto& ph = static_cast<const PlaceholderEntry&>(e);
        logger_->error(
            utl::SYN,
            111,
            "checkConsistency: orphan placeholder at {} (offset={})\n",
            id,
            ph.offset());
      }
    }
  });
}

Net ControlNet::emitNet(Graph& g)
{
  if (isPositive()) {
    return net();
  }
  return g.add<Not>(BundleView(net()))[0];
}

void Graph::stats(std::ostream& os) const
{
  struct CellStats
  {
    uint32_t cells = 0;
    uint32_t bits = 0;
    float area = 0.0f;
  };
  std::map<std::string, CellStats> counts;
  uint32_t total_cells = 0;
  uint32_t total_bits = 0;
  float total_area = 0.0f;

  forEachInstance([&](const Instance* inst) {
    if (inst->is<TieLow>() || inst->is<TieHigh>() || inst->is<TieX>()) {
      return;
    }
    std::string name = cellKeyword(inst->entryType());
    float area = 0.0f;
    if (inst->is<Other>()) {
      name = static_cast<const Other*>(inst)->cellType();
    } else if (inst->is<Target>()) {
      name = std::string("target: ") + inst->as<Target>()->cell()->name();
      area = inst->as<Target>()->cell()->area();
    } else if (inst->is<Name>() && inst->as<Name>()->tentative()) {
      name = "name (tentative)";
    }
    uint32_t w = inst->outputWidth();
    counts[name].cells++;
    counts[name].bits += w;
    counts[name].area += area;
    total_cells++;
    total_bits += w;
    total_area += area;
  });

  for (auto& [name, s] : counts) {
    os << "  " << name << ": " << s.cells << " cells, " << s.bits << " bits";
    if (s.area > 0.0f) {
      os << ", area " << s.area;
    }
    os << "\n";
  }
  os << "  total: " << total_cells << " cells, " << total_bits << " bits";
  if (total_area > 0.0f) {
    os << ", area " << total_area;
  }
  os << "\n";
}

void Graph::memoryUsage(std::ostream& os) const
{
  size_t net_table_bytes = table_.memoryBytes();
  size_t heap_inst_count = heap_instances_.size();
  size_t heap_inst_bytes = 0;
  size_t name_data_bytes = 0;
  size_t vector_data_bytes = 0;

  for (auto* inst : heap_instances_) {
    heap_inst_bytes += inst->sizeOf();
    size_t nb, vb;
    inst->heapBytes(nb, vb);
    name_data_bytes += nb;
    vector_data_bytes += vb;
  }

  // Inline instances: heapBytes is always 0 for inline types (they have
  // no Bundle/string/Const members), so we only need to account for heap
  // instances above.  The net table slots already cover inline instance
  // storage.

  size_t total
      = net_table_bytes + heap_inst_bytes + name_data_bytes + vector_data_bytes;

  os << "  Net table: " << table_.size() << " slots in "
     << (table_.memoryBytes() / sizeof(NetTableBlock)) << " blocks, "
     << net_table_bytes << " bytes\n";
  os << "  Heap instances: " << heap_inst_count << " instances, "
     << heap_inst_bytes << " bytes\n";
  os << "  Name data: " << name_data_bytes << " bytes\n";
  os << "  Vector data: " << vector_data_bytes << " bytes\n";
  os << "  Total: " << total << " bytes\n";
}

}  // namespace syn
