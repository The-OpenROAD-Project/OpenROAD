// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//
// IR abstraction layer: types matching the surface of the Yosys ir.h but
// backed by syn::Net and syn::Bundle. The frontend code includes this
// header (via the include path) instead of the Yosys version.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <vector>

#include "syn/ir/Bundle.h"
#include "syn/ir/Net.h"

namespace ir {

// Four-valued bit constant
struct Trit
{
  enum State : uint8_t
  {
    kS0 = 0,
    kS1 = 1,
    kSx = 2
  };
  State raw_;

  constexpr Trit() : raw_(kSx) {}
  constexpr Trit(State s) : raw_(s) {}

  constexpr bool operator==(Trit o) const { return raw_ == o.raw_; }
  constexpr bool operator!=(Trit o) const { return raw_ != o.raw_; }

  // Map to/from syn::Net constants
  syn::Net to_net() const
  {
    switch (raw_) {
      case kS0:
        return syn::Net::zero();
      case kS1:
        return syn::Net::one();
      case kSx:
        return syn::Net::undef();
    }
    return syn::Net::undef();
  }

  static Trit from_net(syn::Net n)
  {
    if (n == syn::Net::zero()) {
      return Trit(kS0);
    }
    if (n == syn::Net::one()) {
      return Trit(kS1);
    }
    if (n == syn::Net::undef()) {
      return Trit(kSx);
    }
    // Non-constant net — shouldn't call trit() on it, but return Sx
    return Trit(kSx);
  }
};

inline constexpr Trit S0{Trit::kS0};
inline constexpr Trit S1{Trit::kS1};
inline constexpr Trit Sx{Trit::kSx};

// Multi-bit constant value
class Const
{
  std::vector<Trit> bits_;

 public:
  Const() = default;
  explicit Const(int val, uint64_t width)
  {
    bits_.reserve(width);
    for (uint64_t i = 0; i < width; i++) {
      bits_.push_back(((uint64_t) val >> i) & 1 ? S1 : S0);
    }
  }
  explicit Const(Trit t, uint64_t width) : bits_(width, t) {}
  Const(const std::vector<Trit>& bits) : bits_(bits) {}

  int size() const { return (int) bits_.size(); }
  bool empty() const { return bits_.empty(); }

  Const extract(int offset, int length) const
  {
    Const result;
    result.bits_.assign(bits_.begin() + offset,
                        bits_.begin() + offset + length);
    return result;
  }

  void append(Const other)
  {
    bits_.insert(bits_.end(), other.bits_.begin(), other.bits_.end());
  }
  void append(Trit t) { bits_.push_back(t); }

  int as_int(bool is_signed = false) const
  {
    int result = 0;
    for (int i = 0; i < size() && i < 32; i++) {
      if (bits_[i] == S1) {
        result |= (1 << i);
      }
    }
    if (is_signed && size() > 0 && size() <= 32 && bits_[size() - 1] == S1) {
      for (int i = size(); i < 32; i++) {
        result |= (1 << i);
      }
    }
    return result;
  }

  bool is_fully_undef() const
  {
    for (auto& b : bits_) {
      if (b != Sx) {
        return false;
      }
    }
    return true;
  }

  void set(int i, Trit t) { bits_[i] = t; }

  Trit operator[](int i) const { return bits_[i]; }

  using const_iterator = std::vector<Trit>::const_iterator;
  const_iterator begin() const { return bits_.begin(); }
  const_iterator end() const { return bits_.end(); }

  const std::vector<Trit>& bits() const { return bits_; }
};

struct Value;

// Single-bit signal reference — always a syn::Net
struct Net
{
  syn::Net raw_;

  Net() : raw_(syn::Net::undef()) {}
  Net(syn::Net n) : raw_(n) {}
  Net(Trit t) : raw_(t.to_net()) {}

  syn::Net raw() const { return raw_; }

  Value repeat(uint64_t width);

  Trit trit() const { return Trit::from_net(raw_); }

  bool operator==(Net o) const { return raw_ == o.raw_; }
  bool operator!=(Net o) const { return raw_ != o.raw_; }
  bool operator==(Trit t) const { return raw_ == t.to_net(); }
  bool operator!=(Trit t) const { return raw_ != t.to_net(); }

  bool is_const() const
  {
    return raw_ == syn::Net::zero() || raw_ == syn::Net::one()
           || raw_ == syn::Net::undef();
  }
  bool is_def() const
  {
    return raw_ == syn::Net::zero() || raw_ == syn::Net::one();
  }
};

// Proxy returned by Value::operator[] for read and write access
class NetProxy
{
  std::vector<syn::Net>& nets_;
  int idx_;

 public:
  NetProxy(std::vector<syn::Net>& nets, int i) : nets_(nets), idx_(i) {}
  operator Net() const { return nets_[idx_]; }
  NetProxy& operator=(Net n)
  {
    nets_[idx_] = n.raw_;
    return *this;
  }
  NetProxy& operator=(const NetProxy& o)
  {
    nets_[idx_] = o.nets_[o.idx_];
    return *this;
  }
  bool operator==(Net n) const { return Net(nets_[idx_]) == n; }
  bool operator!=(Net n) const { return Net(nets_[idx_]) != n; }
  bool operator==(Trit t) const { return Net(nets_[idx_]) == t; }
  bool operator!=(Trit t) const { return Net(nets_[idx_]) != t; }
  Trit trit() const { return Net(nets_[idx_]).trit(); }
};

// Multi-bit signal — stores a vector of syn::Net, convertible to/from
// syn::Bundle
struct Value
{
  std::vector<syn::Net> nets_;

  // Construction
  Value() = default;
  Value(Net bit) : nets_{bit.raw_} {}
  Value(Trit t) : nets_{t.to_net()} {}
  Value(Trit t, uint64_t width) : nets_(width, t.to_net()) {}
  Value(const Const& c)
  {
    nets_.reserve(c.size());
    for (auto i : c) {
      nets_.push_back(i.to_net());
    }
  }
  explicit Value(int val, int width = 32)
  {
    nets_.reserve(width);
    for (int i = 0; i < width; i++) {
      nets_.push_back(((uint32_t) val >> i) & 1 ? syn::Net::one()
                                                : syn::Net::zero());
    }
  }

  // From syn::Bundle
  Value(const syn::Bundle& b)
  {
    nets_.reserve(b.width());
    for (uint32_t i = 0; i < b.width(); i++) {
      nets_.push_back(b[i]);
    }
  }

  // Concatenation (MSB-first, matching Verilog semantics)
  Value(std::initializer_list<Value> parts)
  {
    for (auto it = parts.end(); it != parts.begin();) {
      --it;
      nets_.insert(nets_.end(), it->nets_.begin(), it->nets_.end());
    }
  }

  // Convert to syn::Bundle
  syn::Bundle to_bundle() const
  {
    if (nets_.empty()) {
      return syn::Bundle();
    }
    if (nets_.size() == 1) {
      return syn::Bundle(nets_[0]);
    }
    return syn::Bundle::fromVec(std::vector<syn::Net>(nets_));
  }

  // Sizing
  uint64_t size() const { return nets_.size(); }
  uint64_t width() const { return nets_.size(); }
  bool empty() const { return nets_.empty(); }

  // Element access
  Net operator[](uint64_t i) const { return nets_[i]; }
  NetProxy operator[](uint64_t i) { return NetProxy(nets_, i); }

  // Extraction / manipulation
  Value extract(uint64_t offset, uint64_t width) const
  {
    Value result;
    result.nets_.assign(nets_.begin() + offset, nets_.begin() + offset + width);
    return result;
  }
  Value extract_end(uint64_t offset) const
  {
    return extract(offset, nets_.size() - offset);
  }
  void append(Net bit) { nets_.push_back(bit.raw_); }
  void append(const NetProxy& p) { nets_.push_back(Net(p).raw_); }
  void append(Trit t) { nets_.push_back(t.to_net()); }
  void append(Value other)
  {
    nets_.insert(nets_.end(), other.nets_.begin(), other.nets_.end());
  }
  void remove(uint64_t index, uint64_t width = 1)
  {
    nets_.erase(nets_.begin() + index, nets_.begin() + index + width);
  }
  Value repeat(uint64_t count) const
  {
    Value result;
    result.nets_.reserve(nets_.size() * count);
    for (uint64_t i = 0; i < count; i++) {
      result.nets_.insert(result.nets_.end(), nets_.begin(), nets_.end());
    }
    return result;
  }
  void extend_u0(uint64_t width, bool is_signed = false)
  {
    if (width < nets_.size()) {
      nets_.erase(nets_.begin() + width, nets_.end());
      return;
    }
    syn::Net fill
        = is_signed && !nets_.empty() ? nets_.back() : syn::Net::zero();
    nets_.resize(width, fill);
  }
  Net msb() const { return nets_.back(); }
  void reserve(uint64_t n) { nets_.reserve(n); }

  // Queries
  bool is_fully_const() const
  {
    for (auto& n : nets_) {
      if (n != syn::Net::zero() && n != syn::Net::one()
          && n != syn::Net::undef()) {
        return false;
      }
    }
    return true;
  }
  bool is_fully_def() const
  {
    for (auto& n : nets_) {
      if (n != syn::Net::zero() && n != syn::Net::one()) {
        return false;
      }
    }
    return true;
  }
  bool is_fully_zero() const
  {
    for (auto& n : nets_) {
      if (n != syn::Net::zero()) {
        return false;
      }
    }
    return true;
  }
  bool is_fully_ones() const
  {
    for (auto& n : nets_) {
      if (n != syn::Net::one()) {
        return false;
      }
    }
    return true;
  }

  // Conversion
  Net as_net() const
  {
    assert(nets_.size() == 1);
    return nets_[0];
  }
  Const as_const() const
  {
    std::vector<Trit> bits;
    bits.reserve(nets_.size());
    for (auto& n : nets_) {
      bits.push_back(Trit::from_net(n));
    }
    return Const(bits);
  }
  bool as_bool() const
  {
    for (auto& n : nets_) {
      if (n == syn::Net::one()) {
        return true;
      }
    }
    return false;
  }
  int as_int(bool is_signed = false) const
  {
    return as_const().as_int(is_signed);
  }

  // Iteration
  struct const_iterator
  {
    using iterator_category = std::input_iterator_tag;
    using value_type = Net;
    using difference_type = std::ptrdiff_t;
    using pointer = const Net*;
    using reference = Net;

    std::vector<syn::Net>::const_iterator it;
    Net operator*() const { return Net(*it); }
    const_iterator& operator++()
    {
      ++it;
      return *this;
    }
    const_iterator operator++(int)
    {
      auto tmp = *this;
      ++it;
      return tmp;
    }
    bool operator!=(const const_iterator& o) const { return it != o.it; }
    bool operator==(const const_iterator& o) const { return it == o.it; }
  };

  const_iterator begin() const { return {nets_.begin()}; }
  const_iterator end() const { return {nets_.end()}; }

  // Comparison
  bool operator==(const Value& o) const { return nets_ == o.nets_; }
  bool operator!=(const Value& o) const { return nets_ != o.nets_; }
};

}  // namespace ir
