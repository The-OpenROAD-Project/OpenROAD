// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The value of a 3-state constant (0, 1, x), pooled or inline. z is not a
// constant: it comes from kTribuf and kTriJoin, and floating wires are
// kUndriven. Encoding: bit planes (aval, bval) with 0 = (0, 0), 1 = (1, 0),
// x = (0, 1); (1, 1) is normalized to x. Bit i is bit i % 64 of word i / 64;
// bits above the width are 0.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "absl/base/attributes.h"

namespace syn::lir {

// One 3-state bit; the value is aval | (bval << 1), with aval = 0 when
// bval = 1.
enum class LogicValue : uint8_t
{
  kZero = 0,
  kOne = 1,
  kX = 2,
};

// Returns '0', '1' or 'x'.
char logicValueChar(LogicValue value);

constexpr uint32_t constNumWords(uint32_t width)
{
  return (width + 63) / 64;
}

// Mask of the valid bits in the last word of a `width`-bit plane.
constexpr uint64_t constLastWordMask(uint32_t width)
{
  return width % 64 == 0 ? ~uint64_t{0} : (uint64_t{1} << (width % 64)) - 1;
}

// kConst objects of at most this width hold their value in the object record
// instead of the ConstPool (Module.h, ObjFlags::inline_const).
inline constexpr uint32_t kMaxInlineConstWidth = 32;

// A read-only view of a constant. A small (inline) constant is copied into
// the view, so keep the view alive while using its aval() and bval() spans.
class ConstView
{
 public:
  ConstView() = default;
  // `words` holds constNumWords(width) aval words followed by as many bval
  // words, normalized.
  ConstView(uint32_t width, const uint64_t* words)
      : width_(width), words_(words)
  {
  }
  // A constant of at most kMaxInlineConstWidth bits, held by value. Bits
  // above the width are ignored.
  static ConstView small(uint32_t width, uint32_t aval, uint32_t bval);

  uint32_t width() const { return width_; }
  uint32_t numWords() const { return constNumWords(width_); }
  std::span<const uint64_t> aval() const ABSL_ATTRIBUTE_LIFETIME_BOUND
  {
    return {words(), numWords()};
  }
  std::span<const uint64_t> bval() const ABSL_ATTRIBUTE_LIFETIME_BOUND
  {
    return {words() + numWords(), numWords()};
  }

  LogicValue bit(uint32_t index) const;
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

  uint32_t width_ = 0;
  const uint64_t* words_ = nullptr;         // null for a small constant
  std::array<uint64_t, 2> small_ = {0, 0};  // aval, bval of a small constant
};

}  // namespace syn::lir
