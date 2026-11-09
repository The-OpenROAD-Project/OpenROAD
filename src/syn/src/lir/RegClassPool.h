// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Register classes, per module: RegOut.data holds a RegClassId, comparable
// only within its module. A generic class has flags, the present input roles
// and the initial state. A mapped class is a Liberty cell: the RegIn fanins
// are its inputs in Liberty order, the RegOut is output 0 and the others are
// kFans (kept even if unread); mapping drops the initial state.

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Ids.h"
#include "syn/lir/LibCellTable.h"
#include "syn/lir/RegPinTypes.h"

namespace syn::lir {

// Initial state of a generic register's Q: per bit 0, 1 or x. All x means
// no initial state. Narrow values are inline; wider ones are a constant of
// the register width in the ConstPool.
class RegInit
{
 public:
  RegInit() = default;  // all x: no initial state

  // Bit i is known where bit i of `known` is set. `value` must be 0
  // elsewhere.
  static RegInit inlineValue(uint32_t value, uint32_t known);
  // Pooled: `id` is a constant of the register width (x = unknown). An
  // invalid `id` gives an invalid RegInit (isValid()).
  static RegInit pooled(ConstId id);
  // Inline if narrow enough, else interned in `pool`. A new wide value in a
  // read-only pool gives an invalid RegInit.
  static RegInit fromConst(ConstView value, ConstPool& pool);

  bool isNone() const { return !pooled_ && known_ == 0; }
  bool isPooled() const { return pooled_; }
  // False for a pooled value without a valid ConstId.
  bool isValid() const { return !pooled_ || constId().isValid(); }
  // Inline only.
  uint32_t value() const { return value_; }
  uint32_t known() const { return known_; }
  // Pooled only.
  ConstId constId() const { return ConstId(value_); }

  // Bit `index` (kZero, kOne or kX); `pool` is read for pooled values.
  LogicValue bit(uint32_t index, const ConstPool& pool) const;
  // Deterministic checksum; pooled values are read from `pool`.
  uint64_t checksum(const ConstPool& pool) const;

  bool operator==(const RegInit&) const = default;
  template <typename H>
  friend H AbslHashValue(H h, const RegInit& init)
  {
    return H::combine(std::move(h), init.value_, init.known_, init.pooled_);
  }

 private:
  uint32_t value_ = 0;  // inline: the value bits; pooled: the ConstId
  uint32_t known_ = 0;  // inline only
  bool pooled_ = false;
};

// Behavior of a generic register (RegClass::flags()). One byte.
struct RegFlags
{
  // Level-sensitive latch, transparent while kClock is 1; else a flip-flop.
  // A latch has only kData, kClock and the async reset/set pins.
  bool latch : 1 = false;
  // Sync reset/set act even when kEnable is inactive (s(g(D))). Otherwise
  // (the default) they are gated by the enable like the data (g(s(D))).
  bool sync_over_enable : 1 = false;
  // Set wins over reset (default: reset wins), for both the sync and the
  // async pair. Async reset/set always win over async load.
  bool set_over_reset : 1 = false;
  // Reserved. Zero.
  uint8_t reserved : 5 = 0;

  bool operator==(const RegFlags&) const = default;
  template <typename H>
  friend H AbslHashValue(H h, const RegFlags& f)
  {
    return H::combine(std::move(h),
                      f.latch,
                      f.sync_over_enable,
                      f.set_over_reset,
                      f.reserved);
  }
};
static_assert(sizeof(RegFlags) == 1, "RegFlags must stay one byte");

class RegClass
{
 public:
  enum class Mode : uint8_t
  {
    kGeneric,
    kMapped,
  };

  RegClass() = default;  // generic, no pins, no initial state
  static RegClass generic(RegFlags flags, RegPinMask pins, RegInit init = {});
  // The RegOut carries Liberty output 0; every other output is a kFan with
  // data = its output index.
  static RegClass mapped(LibCellId cell);

  Mode mode() const { return mode_; }
  bool isGeneric() const { return mode_ == Mode::kGeneric; }
  bool isMapped() const { return mode_ == Mode::kMapped; }

  // Generic mode ------------------------------------------------------------
  const RegFlags& flags() const { return flags_; }
  bool isLatch() const { return flags_.latch; }
  RegPinMask pins() const { return pins_; }
  bool has(RegPinType type) const { return (pins_ & regPinBit(type)) != 0; }
  int numPins() const { return std::popcount(pins_); }
  // Position of `type` among the RegIn fanins, or -1 if absent.
  int pinIndex(RegPinType type) const;
  const RegInit& init() const { return init_; }

  // Copies with one field changed (generic mode).
  RegClass withPins(RegPinMask pins) const;
  RegClass withInit(RegInit init) const;

  // Mapped mode -------------------------------------------------------------
  LibCellId cell() const { return cell_; }

  // Equal except for the initial state (registers that can be banked).
  bool sameKindIgnoringInit(const RegClass& other) const
  {
    return mode_ == other.mode_ && flags_ == other.flags_
           && pins_ == other.pins_ && cell_ == other.cell_;
  }

  // Deterministic checksum; initial states and cells are read from `consts`
  // and `cells`.
  uint64_t checksum(const ConstPool& consts, const LibCellTable& cells) const;

  bool operator==(const RegClass&) const = default;
  template <typename H>
  friend H AbslHashValue(H h, const RegClass& c)
  {
    return H::combine(
        std::move(h), c.mode_, c.flags_, c.pins_, c.cell_, c.init_);
  }

 private:
  Mode mode_ = Mode::kGeneric;
  RegFlags flags_;       // generic
  RegPinMask pins_ = 0;  // generic
  LibCellId cell_;       // mapped
  RegInit init_;         // generic
};

// The interned register classes of one module: equal classes have equal ids.
// Follows the module's ownership rules (one writer, no locking).
class RegClassPool
{
 public:
  // Returns the id of `reg_class`, adding it if needed. Invalid if its
  // initial state is invalid (RegInit::isValid()).
  RegClassId intern(const RegClass& reg_class);
  // The class of a valid id returned by this pool.
  RegClass get(RegClassId id) const;
  // Deterministic checksum of the class of `id` (0 if `id` is invalid).
  uint64_t checksum(RegClassId id,
                    const ConstPool& consts,
                    const LibCellTable& cells) const
  {
    return id.isValid() ? get(id).checksum(consts, cells) : 0;
  }
  // The id of `reg_class` if it is in the pool, else an invalid id.
  RegClassId find(const RegClass& reg_class) const;

  size_t size() const { return classes_.size(); }
  bool empty() const { return classes_.empty(); }
  size_t memoryBytes() const;

 private:
  std::vector<RegClass> classes_;  // by RegClassId
  absl::flat_hash_map<RegClass, uint32_t> index_;
};

// Inline definitions.

inline int RegClass::pinIndex(RegPinType type) const
{
  if (!has(type)) {
    return -1;
  }
  return std::popcount(static_cast<RegPinMask>(pins_ & (regPinBit(type) - 1u)));
}

}  // namespace syn::lir
