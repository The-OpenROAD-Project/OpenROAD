// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/ir/Const.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>

namespace syn {

bool Const::hasUndef() const
{
  return std::ranges::find(bits_, Trit::Undef) != bits_.end();
}

bool Const::isZero() const
{
  return std::ranges::find_if(bits_,
                              [](Trit trit) { return trit != Trit::Zero; })
         == bits_.end();
}

Const Const::slice(uint32_t offset, uint32_t width) const
{
  assert(offset + width <= bits_.size());
  Const result;
  result.bits_.assign(bits_.begin() + offset, bits_.begin() + offset + width);
  return result;
}

Const Const::concat(const Const& other) const
{
  Const result;
  result.bits_.reserve(bits_.size() + other.bits_.size());
  result.bits_.insert(result.bits_.end(), bits_.begin(), bits_.end());
  result.bits_.insert(
      result.bits_.end(), other.bits_.begin(), other.bits_.end());
  return result;
}

Const Const::not_() const
{
  Const result;
  result.bits_.reserve(width());
  for (auto t : bits_) {
    result.bits_.push_back(!t);
  }
  return result;
}

Const Const::and_(const Const& other) const
{
  assert(width() == other.width());
  Const result;
  result.bits_.reserve(width());
  for (uint32_t i = 0; i < width(); i++) {
    result.bits_.push_back(bits_[i] & other.bits_[i]);
  }
  return result;
}

Const Const::or_(const Const& other) const
{
  assert(width() == other.width());
  Const result;
  result.bits_.reserve(width());
  for (uint32_t i = 0; i < width(); i++) {
    result.bits_.push_back(bits_[i] | other.bits_[i]);
  }
  return result;
}

Const Const::xor_(const Const& other) const
{
  assert(width() == other.width());
  Const result;
  result.bits_.reserve(width());
  for (uint32_t i = 0; i < width(); i++) {
    result.bits_.push_back(bits_[i] ^ other.bits_[i]);
  }
  return result;
}

Const Const::mux(Trit sel, const Const& a, const Const& b)
{
  assert(a.width() == b.width());
  if (sel == Trit::One) {
    return a;
  }
  if (sel == Trit::Zero) {
    return b;
  }
  Const result;
  result.bits_.reserve(a.width());
  for (uint32_t i = 0; i < a.width(); i++) {
    result.bits_.push_back(syn::mux(sel, a.bits_[i], b.bits_[i]));
  }
  return result;
}

Const Const::adc(const Const& other, Trit ci) const
{
  assert(width() == other.width());
  Const result;
  result.bits_.reserve(width() + 1);
  Trit carry = ci;
  for (uint32_t i = 0; i < width(); i++) {
    Trit a = bits_[i];
    Trit b = other.bits_[i];
    Trit y;
    Trit co;
    if (a == Trit::Undef || b == Trit::Undef || carry == Trit::Undef) {
      y = Trit::Undef;
      co = Trit::Undef;
    } else if (a == Trit::Zero && b == Trit::Zero) {
      y = carry;
      co = Trit::Zero;
    } else if (a == Trit::One && b == Trit::One) {
      y = carry;
      co = Trit::One;
    } else {
      // Exactly one of a,b is 1
      y = !carry;
      co = carry;
    }
    carry = co;
    result.bits_.push_back(y);
  }
  result.bits_.push_back(carry);
  return result;
}

Trit Const::eq(const Const& other) const
{
  assert(width() == other.width());
  bool has_undef = false;
  for (uint32_t i = 0; i < width(); i++) {
    if (bits_[i] == Trit::Undef || other.bits_[i] == Trit::Undef) {
      has_undef = true;
    } else if (bits_[i] != other.bits_[i]) {
      return Trit::Zero;
    }
  }
  return has_undef ? Trit::Undef : Trit::One;
}

Trit Const::ult(const Const& other) const
{
  assert(width() == other.width());
  if (hasUndef() || other.hasUndef()) {
    return Trit::Undef;
  }
  // Compare MSB-first.
  for (int i = (int) width() - 1; i >= 0; i--) {
    if (bits_[i] != other.bits_[i]) {
      return (bits_[i] < other.bits_[i]) ? Trit::One : Trit::Zero;
    }
  }
  return Trit::Zero;  // Equal.
}

Trit Const::slt(const Const& other) const
{
  assert(width() == other.width());
  if (hasUndef() || other.hasUndef()) {
    return Trit::Undef;
  }
  // Check sign bits first (MSB). In two's complement, 1 = negative.
  if (msb() != other.msb()) {
    return (msb() > other.msb()) ? Trit::One : Trit::Zero;
  }
  // Same sign — compare remaining bits MSB-first.
  for (int i = (int) width() - 1; i >= 0; i--) {
    if (bits_[i] != other.bits_[i]) {
      return (bits_[i] < other.bits_[i]) ? Trit::One : Trit::Zero;
    }
  }
  return Trit::Zero;  // Equal.
}

// Helper: convert Const to uint64_t. Returns nullopt if too large
static std::optional<uint64_t> shiftAmountToUint(const Const& c)
{
  uint64_t val = 0;
  for (int i = (int) c.width() - 1; i >= 0; i--) {
    val <<= 1;
    if (c[i] == Trit::One) {
      if (i >= 32) {
        return std::nullopt;
      }
      val |= 1;
    }
  }
  return val;
}

Const Const::shl(const Const& amt, uint32_t stride) const
{
  if (amt.hasUndef()) {
    return Const::undef(width());
  }
  auto shift_opt = shiftAmountToUint(amt);
  if (!shift_opt) {
    if (stride == 0) {
      return *this;
    }
    return Const::zero(width());
  }
  uint64_t shift = ((uint64_t) *shift_opt) * stride;
  if (shift >= width()) {
    return Const::zero(width());
  }
  Const result = Const::zero(static_cast<uint32_t>(shift))
                     .concat(slice(0, width() - static_cast<uint32_t>(shift)));
  return result;
}

Const Const::ushr(const Const& amt, uint32_t stride) const
{
  if (amt.hasUndef()) {
    return Const::undef(width());
  }
  auto shift_opt = shiftAmountToUint(amt);
  if (!shift_opt) {
    if (stride == 0) {
      return *this;
    }
    return Const::zero(width());
  }
  uint64_t shift = ((uint64_t) *shift_opt) * stride;
  if (shift >= width()) {
    return Const::zero(width());
  }
  return slice(static_cast<uint32_t>(shift),
               width() - static_cast<uint32_t>(shift))
      .concat(Const::zero(static_cast<uint32_t>(shift)));
}

Const Const::sshr(const Const& amt, uint32_t stride) const
{
  if (amt.hasUndef()) {
    return Const::undef(width());
  }
  Trit sign = msb();
  auto shift_opt = shiftAmountToUint(amt);
  if (!shift_opt) {
    if (stride == 0) {
      return *this;
    }
    return Const(width(), sign);
  }
  uint64_t shift = ((uint64_t) *shift_opt) * stride;
  if (shift >= width()) {
    return Const(width(), sign);
  }
  Const fill(static_cast<uint32_t>(shift), sign);
  return slice(static_cast<uint32_t>(shift),
               width() - static_cast<uint32_t>(shift))
      .concat(fill);
}

Const Const::xshr(const Const& amt, uint32_t stride) const
{
  if (amt.hasUndef()) {
    return Const::undef(width());
  }
  auto shift_opt = shiftAmountToUint(amt);
  if (!shift_opt) {
    if (stride == 0) {
      return *this;
    }
    return Const::undef(width());
  }
  uint64_t shift = ((uint64_t) *shift_opt) * stride;
  if (shift >= width()) {
    return Const::undef(width());
  }
  return slice(static_cast<uint32_t>(shift),
               width() - static_cast<uint32_t>(shift))
      .concat(Const::undef(static_cast<uint32_t>(shift)));
}

Const Const::mul(const Const& other) const
{
  assert(width() == other.width());
  if (hasUndef() || other.hasUndef()) {
    return Const::undef(width());
  }
  // Schoolbook multiply, truncated to width() bits.
  Const result = Const::zero(width());
  for (uint32_t i = 0; i < other.width(); i++) {
    if (other.bits_[i] == Trit::One) {
      Const shifted = Const::zero(i).concat(*this);
      result
          = result.adc(shifted.slice(0, width()), Trit::Zero).slice(0, width());
    }
  }
  return result;
}

Const Const::udiv(const Const& other) const
{
  assert(width() == other.width());
  if (hasUndef() || other.hasUndef()) {
    return Const::undef(width());
  }
  if (other.isZero()) {
    return Const::undef(width());
  }

  uint32_t n = width();
  Const remainder = Const::zero(n);
  Const quotient = Const::zero(n);

  for (int k = (int) n - 1; k >= 0; k--) {
    // Shift remainder left, insert a[k].
    for (int i = (int) n - 1; i > 0; i--) {
      remainder.bits_[i] = remainder.bits_[i - 1];
    }
    remainder.bits_[0] = bits_[k];

    // Trial subtraction: remainder - other.
    Const trial = remainder.adc(other.not_(), Trit::One).slice(0, n);
    // Borrow = NOT carry out of the n-bit subtraction.
    Trit carry_out = remainder.adc(other.not_(), Trit::One)[n];
    // carry_out=1 means no borrow (R >= b), carry_out=0 means borrow (R < b).
    if (carry_out == Trit::One) {
      remainder = trial;
      quotient.bits_[k] = Trit::One;
    }
  }
  return quotient;
}

Const Const::umod(const Const& other) const
{
  assert(width() == other.width());
  if (hasUndef() || other.hasUndef()) {
    return Const::undef(width());
  }
  if (other.isZero()) {
    return Const::undef(width());
  }

  uint32_t n = width();
  Const remainder = Const::zero(n);

  for (int k = (int) n - 1; k >= 0; k--) {
    for (int i = (int) n - 1; i > 0; i--) {
      remainder.bits_[i] = remainder.bits_[i - 1];
    }
    remainder.bits_[0] = bits_[k];

    Const trial = remainder.adc(other.not_(), Trit::One).slice(0, n);
    Trit carry_out = remainder.adc(other.not_(), Trit::One)[n];
    if (carry_out == Trit::One) {
      remainder = trial;
    }
  }
  return remainder;
}

}  // namespace syn
