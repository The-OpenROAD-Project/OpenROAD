// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The value of a 3-state constant (0, 1, x), pooled or inline. z is not a
// constant: it comes from kTribuf and kTriJoin, and floating wires are
// kUndriven. Encoding: bit planes (bits, x_mask) with 0 = (0, 0), 1 = (1, 0),
// x = (0, 1); (1, 1) is normalized to x. Bit i is bit i % 64 of word i / 64;
// bits above the width are 0.

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "absl/base/attributes.h"

namespace syn::lir {

// One 3-state bit; the value is bits | (x_mask << 1), with bits = 0 when
// x_mask = 1.
enum class LogicValue : uint8_t
{
  kZero = 0,
  kOne = 1,
  kX = 2,
};

// Returns '0', '1' or 'x'.
char logicValueChar(LogicValue value);

constexpr int constNumWords(int width)
{
  assert(width >= 0);
  return width == 0 ? 0 : (width - 1) / 64 + 1;
}

// Mask of the valid bits in the last word of a `width`-bit plane.
constexpr uint64_t constLastWordMask(int width)
{
  assert(width >= 0);
  const uint32_t rem = static_cast<uint32_t>(width % 64);
  return rem == 0 ? ~uint64_t{0} : (uint64_t{1} << rem) - 1;
}

// kConst objects of at most this width hold their value in the object record
// instead of the ConstPool (Module.h, ObjFlags::inline_const).
inline constexpr int kMaxInlineConstWidth = 32;

// A read-only view of a constant. A small (inline) constant is copied into
// the view, so keep the view alive while using its bits() and xMask() spans.
class ConstView
{
 public:
  ConstView() = default;
  // `words` holds constNumWords(width) `bits` words followed by as many
  // `x_mask` words, normalized.
  ConstView(int width, const uint64_t* words) : width_(width), words_(words)
  {
    assert(width >= 0);
  }
  // A constant of at most kMaxInlineConstWidth bits, held by value. Bits
  // above the width are ignored.
  static ConstView fromInline(int width, uint32_t bits, uint32_t x_mask);

  int width() const { return width_; }
  int numWords() const { return constNumWords(width_); }
  std::span<const uint64_t> bits() const ABSL_ATTRIBUTE_LIFETIME_BOUND
  {
    return {words(), static_cast<size_t>(numWords())};
  }
  std::span<const uint64_t> xMask() const ABSL_ATTRIBUTE_LIFETIME_BOUND
  {
    return {words() + numWords(), static_cast<size_t>(numWords())};
  }

  LogicValue bit(int index) const;
  // True if no bit is X.
  bool isTwoState() const;
  // True if every bit equals `value` (e.g. all-0 or all-1 reset values).
  bool isFill(LogicValue value) const;
  // The value, if the constant is two-state and every bit from 64 up is 0.
  std::optional<uint64_t> toUint64() const;
  // Verilog-style, MSB first: 4'b10x1.
  std::string toString() const;
  // Deterministic checksum of the constant's width and 3-state bits.
  uint64_t checksum() const;

 private:
  // Computed on each access so that copying a small view stays valid.
  const uint64_t* words() const
  {
    return words_ != nullptr ? words_ : small_.data();
  }

  int width_ = 0;
  const uint64_t* words_ = nullptr;         // null for a small constant
  std::array<uint64_t, 2> small_ = {0, 0};  // bits, x_mask of a small constant
};

}  // namespace syn::lir
