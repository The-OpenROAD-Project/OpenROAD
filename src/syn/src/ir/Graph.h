// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <map>
#include <memory>
#include <new>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "syn/ir/Bundle.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/ir/NetTable.h"
#include "syn/ir/NetTableEntry.h"

namespace sta {
class Network;
}

namespace utl {
class Logger;
}

namespace syn {

class Graph
{
 public:
  Graph();
  ~Graph();

  Graph(Graph&& other) noexcept;
  Graph& operator=(Graph&& other) noexcept;

  Graph(const Graph&) = delete;
  Graph& operator=(const Graph&) = delete;

  // Add an instance to the graph.
  // Returns the output Bundle.
  template <typename T, typename... Args>
  Bundle add(Args&&... args);

  // Resolve a Net to its owning Instance and bit offset.
  std::pair<const Instance*, uint32_t> resolve(Net net) const;
  std::pair<Instance*, uint32_t> resolve(Net net);

  // Doo a topological sort of the combinational network and eliminate
  // unused instances, or unused slices of word-level operations
  void normalize();

  // Iterate all instances in net-number order. Callback receives the Instance
  // pointer. The iteration range is captured before the loop starts: newly
  // added instances (at higher indices) are not visited. Modifications to the
  // current instance's slots or lower-indexed slots are safe.
  template <typename F>
  void forEachInstance(F&& fn) const
  {
    const size_t sz = table_.size();
    for (NetTableId id = 0; id < sz; id++) {
      const NetTableEntry& e = table_.entry(id);
      const EntryType type = e.entryType();
      if (type == EntryType::kVoid) {
        continue;
      }
      if (type == EntryType::kPlaceholder) {
        auto& ph = static_cast<const PlaceholderEntry&>(e);
        if (ph.offset() == 0) {
          const Instance* inst = ph.instance();
          const uint32_t skip = std::max(inst->outputWidth(), 1u);
          fn(inst);
          id += skip - 1;  // -1 because the for loop increments
        }
        continue;
      }
      fn(static_cast<const Instance*>(table_.pointer(id)));
    }
  }

  // Same as forEachInstance but iterates in reverse order.
  template <typename F>
  void forEachInstanceReversed(F&& fn) const
  {
    const size_t sz = table_.size();
    if (sz == 0) {
      return;
    }
    for (NetTableId id = sz - 1; id < sz; id--) {
      const NetTableEntry& e = table_.entry(id);
      const EntryType type = e.entryType();
      if (type == EntryType::kVoid) {
        continue;
      }
      if (type == EntryType::kPlaceholder) {
        auto& ph = static_cast<const PlaceholderEntry&>(e);
        if (ph.offset() == 0) {
          fn(ph.instance());
        }
        continue;
      }
      fn(static_cast<const Instance*>(table_.pointer(id)));
    }
  }

  // Iterate over all live nets in topological order.
  // Calls fn(Net, const Instance*, uint32_t offset) for each output bit.
  template <typename F>
  void forEachNet(F&& fn) const
  {
    const size_t sz = table_.size();
    for (NetTableId id = 0; id < sz; id++) {
      const NetTableEntry& e = table_.entry(id);
      const EntryType type = e.entryType();
      if (type == EntryType::kVoid) {
        continue;
      }
      if (type == EntryType::kPlaceholder) {
        auto& ph = static_cast<const PlaceholderEntry&>(e);
        fn(Net(id), ph.instance(), ph.offset());
        continue;
      }
      fn(Net(id), static_cast<const Instance*>(table_.pointer(id)), 0u);
    }
  }

  // Parse text IR from an input stream and reconstruct a Graph.
  // When network is provided, 'target' blocks are parsed directly
  // as Target instances by looking up liberty cells.
  static std::unique_ptr<Graph> parse(std::istream& is,
                                      sta::Network* network = nullptr);

  // Verify normalize() invariants: topological order and no unbroken
  // combinational loops. Aborts if a violation is found.
  void checkNormalization() const;

  // Verify that every heap instance has consistent PlaceholderEntries
  // for all of its output slots. Aborts if a violation is found.
  void checkConsistency() const;

  // Mark all Name instances as tentative.
  void makeNamesTentative();

  // Remove a heap instance: void out its table slots, remove from
  // heap_instances_, destroy and free.
  void removeInstance(Instance* inst);

  // Replace target nets with buffers pointing to replacement nets
  // without verification.
  void forceReplace(BundleView target, BundleView replacement);

  enum class Equivalence
  {
    Full,
    Refinement,
    TwoValued
  };

  // Replace target nets after optional SAT-based verification against a
  // TritModel. verificationSupport: nets forming the structural support shared
  // by both target and replacement, delineating the SAT model boundary.
  //
  // Verification is controlled by the `SYN eqsan` debug level being 1 or
  // higher.
  void replace(BundleView target,
               BundleView replacement,
               Equivalence type,
               BundleView verificationSupport);

  // Dump the graph in text IR format (prjunnamed-compatible).
  void dump(std::ostream& os) const;

  // Dump the input cone of a net up to `max_depth` levels.
  // If `stop_at_stateful` is true, stop at Dff/Target(seq)/Other boundaries.
  void dumpFaninCone(std::ostream& os,
                     Net root,
                     int max_depth,
                     bool stop_at_stateful) const;

  // Dump the output (fanout) cone of a net up to `max_depth` levels.
  void dumpFanoutCone(std::ostream& os,
                      Net root,
                      int max_depth,
                      bool stop_at_stateful) const;

  // Dump a single instance in text IR format.
  void dumpInstance(std::ostream& os, const Instance* inst) const;

  // Print cell statistics.
  void stats(std::ostream& os) const;

  // Print memory usage breakdown.
  void memoryUsage(std::ostream& os) const;

  // Assert that no instance of type T exists in the graph. Used in tests.
  template <typename T>
  void assertNone() const
  {
    forEachInstance([&](const Instance* inst) {
      if (inst->is<T>()) {
        std::fprintf(stderr,
                     "assertNone: found instance of prohibited type at %%%u\n",
                     baseId(inst));
        std::abort();
      }
    });
  }

  // Count instances of type T. Used in tests.
  template <typename T>
  int count() const
  {
    int n = 0;
    forEachInstance([&](const Instance* inst) {
      if (inst->is<T>()) {
        n++;
      }
    });
    return n;
  }

  // Find the single instance of type T. Aborts if zero or more than one found.
  // Used in tests.
  template <typename T>
  const T* findOne() const
  {
    const T* result = nullptr;
    forEachInstance([&](const Instance* inst) {
      if (inst->is<T>()) {
        if (result) {
          std::fprintf(stderr,
                       "findOne: found more than one instance of type\n");
          std::abort();
        }
        result = inst->as<T>();
      }
    });
    if (!result) {
      std::fprintf(stderr, "findOne: no instance of type found\n");
      std::abort();
    }
    return result;
  }

  // Get the output of an instance as an iterable BundleView.
  BundleView output(const Instance* inst) const;

  // Collect all top-level Input / Output ports indexed by name. The returned
  // string_views and BundleViews reference data owned by this Graph, so the
  // map must not outlive it (or any graph mutation that could move/resize
  // the table).
  std::map<std::string_view, BundleView> collectInputs() const
  {
    std::map<std::string_view, BundleView> result;
    forEachInstance([&](const Instance* inst) {
      if (auto* in = inst->try_as<Input>()) {
        result.emplace(in->name(), output(inst));
      }
    });
    return result;
  }

  std::map<std::string_view, BundleView> collectOutputs() const
  {
    std::map<std::string_view, BundleView> result;
    forEachInstance([&](const Instance* inst) {
      if (auto* out = inst->try_as<Output>()) {
        result.emplace(out->name(), BundleView(out->value()));
      }
    });
    return result;
  }

  // Get the raw id of a Net (for printing).
  static uint32_t netId(Net n) { return n.id_; }
  static Net netFromId(uint32_t id) { return Net(id); }

  // Create a Net from a raw id (for parsing).
  static Net makeNet(uint32_t id) { return Net(id); }

  size_t tableSize() const { return table_.size(); }

  // Pad the table with kVoid slots until size() == target. Used by the
  // parser to honor declared %N IDs when dumps have gaps in the net-ID
  // space (e.g. from prior DCE leaving unused slots). Safe no-op if
  // size() is already >= target.
  void padTableTo(NetTableId target)
  {
    if (table_.size() < target) {
      table_.allocateSlots(target - table_.size());
    }
  }

  const std::string& name() const { return name_; }
  void setName(const std::string& name) { name_ = name; }

  utl::Logger* logger() const { return logger_; }
  void setLogger(utl::Logger* logger) { logger_ = logger; }

 private:
  friend struct GraphTestAccess;
  friend class TritModel;

  // Get the base NetTableId of an instance (inline or heap).
  NetTableId baseId(const Instance* inst) const;

  // Resolve a Net to the NetTableId of the owning instance's first slot.
  NetTableId resolveId(Net net) const;

  // Relocate a heap instance to new contiguous slots, leaving BufferFines
  // at the old slot locations that forward to the new ones.
  Buffer* bufferHeapOutput(Instance* inst);

  NetTableEntry& entry(NetTableId id) { return table_.entry(id); }
  const NetTableEntry& entry(NetTableId id) const { return table_.entry(id); }

  template <typename T>
  T& entryAs(NetTableId id)
  {
    return *static_cast<T*>(table_.pointer(id));
  }

  template <typename T>
  const T& entryAs(NetTableId id) const
  {
    return *static_cast<const T*>(table_.pointer(id));
  }

  std::string name_;
  utl::Logger* logger_ = nullptr;
  NetTable table_;
  std::unordered_set<Instance*> heap_instances_;
  bool normalized_ = false;
};

template <typename T, typename... Args>
Bundle Graph::add(Args&&... args)
{
  size_t inst_size;
  if constexpr (requires { T::plan(args...); }) {
    inst_size = T::plan(args...);
  } else {
    inst_size = sizeof(T);
  }

  normalized_ = false;

  if (inst_size <= sizeof(PlaceholderEntry)) {
    // Inline path: instance lives in slot 0 of its output range. Slots
    // 1..outputWidth-1 hold PlaceholderEntries pointing back to it.
    const NetTableId id = table_.allocateSlots(1);
    if constexpr (requires { T::construct(table_.pointer(id), args...); }) {
      T::construct(table_.pointer(id), std::forward<Args>(args)...);
    } else {
      new (table_.pointer(id)) T(std::forward<Args>(args)...);
    }
    static_cast<NetTableEntry*>(table_.pointer(id))
        ->setObjectIndex(id & NetTableEntry::kNetTableIdxMask);
    auto* inst = static_cast<Instance*>(table_.pointer(id));
    const uint32_t output_width = inst->outputWidth();
    if (output_width > 1) {
      NetTableId rest = table_.allocateSlots(output_width - 1);
      assert(rest == id + 1);
      (void) rest;
      for (uint32_t i = 1; i < output_width; ++i) {
        new (table_.pointer(id + i)) PlaceholderEntry(inst, i);
      }
      return Bundle(Net(id), output_width);
    }
    return Bundle(Net(id));
  }

  // Heap path: allocate and construct on heap.
  void* mem = ::operator new(inst_size);
  if constexpr (requires { T::construct(mem, args...); }) {
    T::construct(mem, std::forward<Args>(args)...);
  } else {
    new (mem) T(std::forward<Args>(args)...);
  }
  auto* inst = static_cast<Instance*>(mem);
  heap_instances_.insert(inst);

  const uint32_t output_width = inst->outputWidth();
  const uint32_t slot_count = std::max(output_width, 1u);

  NetTableId first = table_.allocateSlots(slot_count);
  inst->setBaseIndex(first);
  for (uint32_t i = 0; i < slot_count; ++i) {
    new (table_.pointer(first + i)) PlaceholderEntry(inst, i);
  }

  if (output_width > 1) {
    return Bundle(Net(first), output_width);
  }
  return Bundle(Net(first));
}

// Dump.cc
void writeNet(std::ostream& os, const Graph& g, Net net);
void writeValue(std::ostream& os, const Graph& g, BundleView bv);
const char* cellKeyword(EntryType type);

}  // namespace syn
