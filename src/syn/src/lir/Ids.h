// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Strongly typed handles for L-IR entities: dense pool indices, made distinct
// by their Tag; a default-constructed Id is invalid. Object ids are
// module-local and 32 bits (64 if built with LIR_OBJ_ID_BITS=64, at about 1.6x
// the memory); exceeding the limits gives ResourceExhausted. Pool order may
// depend on thread scheduling, so pool ids have no ordering and no value();
// rawIndex() is only for bounds checks, payload fields and pool internals.

#pragma once

#include <compare>  // IWYU pragma: keep (defaulted operator<=>)
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

#ifndef LIR_OBJ_ID_BITS
#define LIR_OBJ_ID_BITS 32
#endif

namespace syn::lir {

// Width of object ids (ObjIndex): 32 (default) or 64.
inline constexpr int kObjIdBits = LIR_OBJ_ID_BITS;
static_assert(kObjIdBits == 32 || kObjIdBits == 64,
              "LIR_OBJ_ID_BITS must be 32 or 64");

// The integer type of object ids, object counts and fanin arena slots.
using ObjIndex = std::conditional_t<kObjIdBits == 64, uint64_t, uint32_t>;

template <typename Tag, typename T = uint32_t, bool Ordered = false>
class Id
{
 public:
  using ValueType = T;
  static constexpr ValueType kInvalidValue
      = std::numeric_limits<ValueType>::max();

  constexpr Id() = default;
  constexpr explicit Id(ValueType value) : value_(value) {}

  static constexpr Id invalid() { return Id(); }

  // The index of an ordered id (ObjId, ModId).
  constexpr ValueType value() const
    requires Ordered
  {
    return value_;
  }
  // The raw index of a pool id. May depend on thread scheduling: never use
  // it for ordering, naming or output.
  constexpr ValueType rawIndex() const
    requires(!Ordered)
  {
    return value_;
  }
  constexpr bool isValid() const { return value_ != kInvalidValue; }

  friend constexpr bool operator==(const Id&, const Id&) = default;
  friend constexpr auto operator<=>(const Id&, const Id&)
    requires Ordered
  = default;

  template <typename H>
  friend H AbslHashValue(H h, Id id)
  {
    return H::combine(std::move(h), id.value_);
  }

  friend struct std::hash<Id>;

 private:
  ValueType value_ = kInvalidValue;
};

struct ObjTag;
struct ModTag;
struct NameTag;
struct ConstTag;
struct RegClassTag;
struct LibCellTag;
struct OpSpecTag;

// An object (PI, PO, operator, register half, instance, fan, ...) inside a
// module. Module-local; ObjIndex wide.
using ObjId = Id<ObjTag, ObjIndex, /*Ordered=*/true>;
// A module (definition) inside a design.
using ModId = Id<ModTag, uint32_t, /*Ordered=*/true>;
// An interned string (design-wide; dense, in insertion order).
using NameId = Id<NameTag>;
// A hash-consed 3-state constant or LUT truth table (design-wide).
using ConstId = Id<ConstTag>;
// An interned register class: present pin-type mask, flags, optional binding.
using RegClassId = Id<RegClassTag>;
// A Liberty cell known to the mapper.
using LibCellId = Id<LibCellTag>;
// An interned operator spec: the function an operator implementation module
// computes at its ports (OperatorSpec.h).
using OpSpecId = Id<OpSpecTag>;

// Access mode of a design-wide table (Design, NameInterner, ConstPool,
// OperatorPool, LibCellTable):
//   - kReadOnly (default): no additions; lock-free parallel reads.
//   - kSingleWrite: one thread may add entries; no locking.
//   - kParallelWrite: many threads may add entries; every access locks.
enum class TableMode : uint8_t
{
  kReadOnly,
  kSingleWrite,
  kParallelWrite,
};

}  // namespace syn::lir

template <typename Tag, typename T, bool Ordered>
struct std::hash<syn::lir::Id<Tag, T, Ordered>>
{
  size_t operator()(syn::lir::Id<Tag, T, Ordered> id) const noexcept
  {
    return std::hash<T>()(id.value_);
  }
};
