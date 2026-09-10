// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "syn/ir/Const.h"

namespace syn {

class Bundle;

class Net
{
 public:
  Net(Trit trit)
  {
    switch (trit) {
      case Trit::One:
        *this = Net::one();
        break;
      case Trit::Zero:
        *this = Net::zero();
        break;
      case Trit::Undef:
      default:
        *this = Net::undef();
        break;
    }
  }

  static constexpr Net zero() { return Net(0); }
  static constexpr Net one() { return Net(1); }
  static constexpr Net undef() { return Net(2); }
  static constexpr Net sentinel() { return Net(UINT32_MAX); }

  friend bool operator==(Net a, Net b) { return a.id_ == b.id_; }
  friend bool operator!=(Net a, Net b) { return a.id_ != b.id_; }
  friend bool operator<(Net a, Net b) { return a.id_ < b.id_; }

  bool isConst() const { return id_ <= 2; }
  bool isSentinel() const { return id_ == UINT32_MAX; }

  Trit constTrit() const
  {
    if (id_ == 0) {
      return Trit::Zero;
    }
    if (id_ == 1) {
      return Trit::One;
    }
    return Trit::Undef;
  }

  Bundle repeated(uint32_t times) const;

 protected:
  uint32_t id_ = 0;

 private:
  explicit constexpr Net(uint32_t id) : id_(id) {}

  friend class BundleView;
  friend class NetTable;
  friend class Graph;
  friend class Bundle;
  friend struct NetTestAccess;
  friend struct GraphTestAccess;
  friend struct std::hash<Net>;
};

}  // namespace syn

template <>
struct std::hash<syn::Net>
{
  std::size_t operator()(syn::Net n) const noexcept
  {
    return std::hash<uint32_t>{}(n.id_);
  }

 private:
  friend class syn::Net;
};
