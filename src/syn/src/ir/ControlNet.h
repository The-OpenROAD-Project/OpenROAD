// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "syn/ir/Net.h"

namespace syn {

class Graph;

// A control net with embedded polarity.
class ControlNet
{
 public:
  ControlNet() : net_(Net::zero()), positive_(true) {}
  static ControlNet pos(Net n) { return ControlNet(n, true); }
  static ControlNet neg(Net n) { return ControlNet(n, false); }
  static ControlNet zero() { return ControlNet(Net::zero(), true); }
  static ControlNet one() { return ControlNet(Net::one(), true); }
  static ControlNet withPolarity(Net net, bool polarity)
  {
    return polarity ? ControlNet::pos(net) : ControlNet::neg(net);
  };

  Net net() const { return net_; }
  bool isPositive() const { return positive_; }
  bool isNegative() const { return !positive_; }

  // True if the control is always active/inactive.
  bool isAlways(bool active) const
  {
    if (positive_) {
      return net_ == (active ? Net::one() : Net::zero());
    }
    return net_ == (active ? Net::zero() : Net::one());
  }

  bool operator==(const ControlNet& o) const
  {
    return net_ == o.net_ && positive_ == o.positive_;
  }
  bool operator!=(const ControlNet& o) const { return !(*this == o); }

  bool operator<(const ControlNet& o) const
  {
    if (net_ != o.net_) {
      return net_ < o.net_;
    }
    return positive_ < o.positive_;
  }

  ControlNet operator!() { return withPolarity(net(), !isPositive()); }

  Net emitNet(Graph& g);

 private:
  friend class Dff;
  ControlNet(Net n, bool pos) : net_(n), positive_(pos) {}
  Net net_;
  bool positive_;
};

}  // namespace syn
