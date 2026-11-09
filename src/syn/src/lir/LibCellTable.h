// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Design-wide table of the Liberty cells used by kCell objects and mapped
// registers. Each cell gets a dense LibCellId; LibCellInfo caches its input
// and output bit ports in Liberty order.
//
// Read-only by default, like NameInterner: adding a new cell requires write
// mode (Design::SingleWriteScope).

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"
#include "syn/lir/Ids.h"

namespace sta {
class LibertyCell;
class LibertyPort;
}  // namespace sta

namespace syn::lir {

// How instances of a Liberty cell are represented.
enum class LibCellKind : uint8_t
{
  // No state: a kCell.
  kCombinational,
  // Integrated clock gate: a kCell (GCLK = CLK & enable in cycle semantics).
  kClockGate,
  // Stateful, every output depends only on the state: a mapped RegOut/RegIn
  // pair. Latches and async flops qualify.
  kRegister,
  // Stateful, with an output that also depends on an input or whose function
  // is unknown: an instance of a black-box module.
  kBlackBox,
};

// Returns "combinational", "clock_gate", "register" or "black_box".
const char* libCellKindName(LibCellKind kind);

class LibCellInfo
{
 public:
  explicit LibCellInfo(sta::LibertyCell* cell);
  LibCellInfo(const LibCellInfo&) = delete;
  LibCellInfo& operator=(const LibCellInfo&) = delete;

  sta::LibertyCell* cell() const { return cell_; }
  std::string_view name() const;
  // True if the cell has an ff/latch group or a statetable (clock gates
  // included). Use kind() to choose a representation.
  bool isSequential() const;
  LibCellKind kind() const { return kind_; }

  // Input bit ports in Liberty order, buses expanded LSB-first. Power/ground,
  // internal, bidirectional and undirected pins are excluded.
  std::span<sta::LibertyPort* const> inputs() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND
  {
    return inputs_;
  }
  // Output/tristate bit ports in Liberty port order. Output 0 is the kCell
  // owner's own value; output k >= 1 is read through a kFan with data k.
  std::span<sta::LibertyPort* const> outputs() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND
  {
    return outputs_;
  }

  uint32_t numInputs() const { return static_cast<uint32_t>(inputs_.size()); }
  uint32_t numOutputs() const { return static_cast<uint32_t>(outputs_.size()); }

  sta::LibertyPort* input(uint32_t k) const
  {
    assert(k < inputs_.size());
    return inputs_[k];
  }
  sta::LibertyPort* output(uint32_t k) const
  {
    assert(k < outputs_.size());
    return outputs_[k];
  }

  // Index of `port` (or port named `name`) in inputs() / outputs(), or -1.
  int findInput(const sta::LibertyPort* port) const;
  int findInput(std::string_view name) const;
  int findOutput(const sta::LibertyPort* port) const;
  int findOutput(std::string_view name) const;

  size_t memoryBytes() const
  {
    return (inputs_.capacity() + outputs_.capacity())
           * sizeof(sta::LibertyPort*);
  }

  // Pin shapes not modeled yet. The table accepts such cells; code that
  // instantiates a cell must call checkSupported(). Bidirectional and
  // undirected pins are not listed in inputs() or outputs().
  bool hasBusPins() const { return has_bus_pins_; }
  bool hasBidirectionalPins() const { return has_bidirectional_pins_; }
  bool hasUnknownDirectionPins() const { return has_unknown_direction_pins_; }
  bool hasTristateOutputs() const { return has_tristate_outputs_; }

  // OK if instances of the cell can be represented, else Unimplemented.
  // TODO: support bus, bundle, bidirectional and kCell tristate pins.
  absl::Status checkSupported() const;

  // Deterministic checksum of the name, kind and port names.
  uint64_t checksum() const;

 private:
  sta::LibertyCell* cell_ = nullptr;
  std::vector<sta::LibertyPort*> inputs_;
  std::vector<sta::LibertyPort*> outputs_;
  LibCellKind kind_ = LibCellKind::kCombinational;
  bool has_bus_pins_ = false;
  bool has_bidirectional_pins_ = false;
  bool has_unknown_direction_pins_ = false;
  bool has_tristate_outputs_ = false;
};

class LibCellTable
{
 public:
  LibCellTable() = default;
  LibCellTable(const LibCellTable&) = delete;
  LibCellTable& operator=(const LibCellTable&) = delete;

  // Returns the id of non-null `cell`, adding it if needed. In read-only mode
  // `cell` must already be interned.
  LibCellId intern(sta::LibertyCell* cell);
  // Returns the id of `cell`, or an invalid id. Not allowed in parallel-write
  // mode.
  LibCellId find(const sta::LibertyCell* cell) const;

  // The descriptor of a valid id. The reference lives as long as the table.
  const LibCellInfo& info(LibCellId id) const;
  sta::LibertyCell* cell(LibCellId id) const { return info(id).cell(); }
  // Deterministic checksum of the cell of `id` (0 if `id` is invalid).
  uint64_t checksum(LibCellId id) const
  {
    return id.isValid() ? info(id).checksum() : 0;
  }

  size_t size() const;
  size_t memoryBytes() const;

  TableMode mode() const { return mode_.load(std::memory_order_relaxed); }
  bool isReadOnly() const { return mode() == TableMode::kReadOnly; }
  bool isWritable() const { return mode() != TableMode::kReadOnly; }
  bool isParallelWrite() const { return mode() == TableMode::kParallelWrite; }
  // Call only while no other thread uses the table.
  void setMode(TableMode mode) { mode_.store(mode, std::memory_order_relaxed); }

 private:
  absl::Mutex* lockIfParallel() const
  {
    return isParallelWrite() ? &mutex_ : nullptr;
  }
  // Unlocked: the caller holds `mutex_` or no other thread is writing.
  LibCellId lookup(const sta::LibertyCell* cell) const
      ABSL_NO_THREAD_SAFETY_ANALYSIS;
  const LibCellInfo& infoAt(LibCellId id) const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  size_t cellCount() const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  LibCellId internUnlocked(sta::LibertyCell* cell)
      ABSL_NO_THREAD_SAFETY_ANALYSIS;

  std::atomic<TableMode> mode_{TableMode::kReadOnly};
  mutable absl::Mutex mutex_;
  // unique_ptr keeps LibCellInfo addresses stable while the table grows.
  std::vector<std::unique_ptr<LibCellInfo>> cells_ ABSL_GUARDED_BY(mutex_);
  absl::flat_hash_map<const sta::LibertyCell*, LibCellId> ids_
      ABSL_GUARDED_BY(mutex_);
};

}  // namespace syn::lir
