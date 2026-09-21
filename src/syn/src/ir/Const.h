// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstdint>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace syn {

// Three-valued logic bit state.
enum class Trit : uint8_t
{
  Zero = 0,
  One = 1,
  Undef = 2,
};

inline Trit operator!(Trit a)
{
  switch (a) {
    case Trit::Zero:
      return Trit::One;
    case Trit::One:
      return Trit::Zero;
    case Trit::Undef:
      return Trit::Undef;
  }
  throw std::logic_error("Invalid Trit value");
}

inline std::ostream& operator<<(std::ostream& os, Trit t)
{
  switch (t) {
    case Trit::Zero:
      return os << '0';
    case Trit::One:
      return os << '1';
    case Trit::Undef:
      return os << 'X';
  }
  throw std::logic_error("Invalid Trit value");
}

// AND: 0 dominates X.
inline Trit operator&(Trit a, Trit b)
{
  if (a == Trit::Zero || b == Trit::Zero) {
    return Trit::Zero;
  }
  if (a == Trit::Undef || b == Trit::Undef) {
    return Trit::Undef;
  }
  return Trit::One;
}

// OR: 1 dominates X.
inline Trit operator|(Trit a, Trit b)
{
  if (a == Trit::One || b == Trit::One) {
    return Trit::One;
  }
  if (a == Trit::Undef || b == Trit::Undef) {
    return Trit::Undef;
  }
  return Trit::Zero;
}

// XOR: X propagates.
inline Trit operator^(Trit a, Trit b)
{
  if (a == Trit::Undef || b == Trit::Undef) {
    return Trit::Undef;
  }
  return (a != b) ? Trit::One : Trit::Zero;
}

// MUX: sel ? a : b, with X-awareness.
inline Trit mux(Trit sel, Trit a, Trit b)
{
  if (sel == Trit::One) {
    return a;
  }
  if (sel == Trit::Zero) {
    return b;
  }
  return (a == b) ? a : Trit::Undef;
}

inline std::optional<Trit> refine(std::optional<Trit> a, std::optional<Trit> b)
{
  if (!a || !b) {
    return std::nullopt;
  }
  if (*a != Trit::Undef && *b != Trit::Undef && *a != *b) {
    // The values fight
    return std::nullopt;
  }
  // Otherwise, values are compatible
  if (*a != Trit::Undef) {
    return a;
  }
  return b;
}

// Multi-bit constant value (LSB at index 0).
class Const
{
 public:
  Const() = default;

  static Const zero(uint32_t width) { return Const(width, Trit::Zero); }
  static Const ones(uint32_t width) { return Const(width, Trit::One); }
  static Const undef(uint32_t width) { return Const(width, Trit::Undef); }
  static Const from(std::vector<Trit> bits)
  {
    Const c;
    c.bits_ = std::move(bits);
    return c;
  }

  uint32_t width() const { return static_cast<uint32_t>(bits_.size()); }
  bool empty() const { return bits_.empty(); }
  Trit operator[](uint32_t i) const { return bits_[i]; }
  Trit msb() const { return bits_.back(); }

  bool hasUndef() const;
  bool isZero() const;

  Const slice(uint32_t offset, uint32_t width) const;
  Const concat(const Const& other) const;

  // Bitwise operations.
  Const not_() const;
  Const and_(const Const& other) const;
  Const or_(const Const& other) const;
  Const xor_(const Const& other) const;

  // Mux: per-bit sel ? a : b.
  static Const mux(Trit sel, const Const& a, const Const& b);

  // Adder with carry. Returns width+1 bits.
  Const adc(const Const& other, Trit ci) const;

  // Comparisons. Return single Trit.
  Trit eq(const Const& other) const;
  Trit ult(const Const& other) const;
  Trit slt(const Const& other) const;

  // Shifts. amt is treated as unsigned integer, multiplied by stride.
  Const shl(const Const& amt, uint32_t stride) const;
  Const ushr(const Const& amt, uint32_t stride) const;
  Const sshr(const Const& amt, uint32_t stride) const;
  Const xshr(const Const& amt, uint32_t stride) const;

  // Multiply. Returns same width as inputs (truncated).
  Const mul(const Const& other) const;

  // Unsigned division and modulo. Returns same width as inputs.
  Const udiv(const Const& other) const;
  Const umod(const Const& other) const;

  const std::vector<Trit>& bits() const { return bits_; }

 private:
  explicit Const(uint32_t width, Trit fill) : bits_(width, fill) {}
  std::vector<Trit> bits_;
};

}  // namespace syn
