// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace grt {

class RoutePt
{
 public:
  RoutePt() = default;
  RoutePt(int x, int y, int layer) : x_(x), y_(y), layer_(layer) {}
  int x() const { return x_; }
  int y() const { return y_; }
  int layer() const { return layer_; }

  friend bool operator<(const RoutePt& p1, const RoutePt& p2);
  friend bool operator==(const RoutePt& p1, const RoutePt& p2);

 private:
  int x_;
  int y_;
  int layer_;
};

bool operator<(const RoutePt& p1, const RoutePt& p2);
bool operator==(const RoutePt& p1, const RoutePt& p2);

}  // namespace grt
