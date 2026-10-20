// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace fsv {

using Vec3 = std::array<double, 3>;

// int underlying type: indexing arrays needs no zero-extension.
enum class Axis : int
{
  kX = 0,
  kY = 1,
  kZ = 2
};

inline constexpr std::array<Axis, 3> kAxes{Axis::kX, Axis::kY, Axis::kZ};

// Component index for Vec3 and Box arrays (std::to_underlying is C++23).
constexpr int index(const Axis a)
{
  return static_cast<int>(a);
}

// Axis-aligned box in µm.
struct Box
{
  Vec3 lo;
  Vec3 hi;
};

// Conductors above a grounded plane at z = 0. The domain is open laterally
// and above; the ground plane is the only fixed boundary.
class Geometry
{
 public:
  int addNet(std::string_view name);
  // Adds a conductor box to a net. The box must lie above the ground plane.
  void addBox(int net, const Box& box);

  int netCount() const { return static_cast<int>(net_names_.size()); }
  const std::string& netName(int net) const { return net_names_.at(net); }
  const std::vector<Box>& boxes() const { return boxes_; }
  int boxNet(int box) const { return box_nets_[box]; }

 private:
  std::vector<std::string> net_names_;
  std::vector<Box> boxes_;
  std::vector<int> box_nets_;
};

// L-infinity distance from a point to a box (0 when inside). Inline: it is
// the innermost operation of the nearest-conductor query.
inline double linfDistance(const Vec3& p, const Box& box)
{
  double d = 0;
  for (int k = 0; k < 3; k++) {
    d = std::max({d, box.lo[k] - p[k], p[k] - box.hi[k]});
  }
  return d;
}

// L-infinity gap between two boxes (0 when they touch or overlap).
inline double linfGap(const Box& a, const Box& b)
{
  double d = 0;
  for (int k = 0; k < 3; k++) {
    d = std::max({d, b.lo[k] - a.hi[k], a.lo[k] - b.hi[k]});
  }
  return d;
}

}  // namespace fsv
