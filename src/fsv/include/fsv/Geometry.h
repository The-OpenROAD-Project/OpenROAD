// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace fsv {

// Points in D dimensions. In 3D the axes are x, y, z; in 2D (cross
// sections, whose wires run along y) they are x, z. Either way the last axis
// is the height above the ground plane.
template <int D>
using Vec = std::array<double, D>;
using Vec3 = Vec<3>;
using Vec2 = Vec<2>;

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
template <int D>
struct AlignedBox
{
  Vec<D> lo;
  Vec<D> hi;
};
using Box = AlignedBox<3>;
// A rectangle in the x-z plane of a cross section, in µm.
using Rect = AlignedBox<2>;

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

// A cross section: conductors in the x-z plane, infinitely long along y,
// above a grounded plane at z = 0. Solving it gives capacitance per unit
// length.
class Geometry2D
{
 public:
  int addNet(std::string_view name);
  // Adds a conductor rectangle to a net. It must lie above the ground plane.
  void addRect(int net, const Rect& rect);

  int netCount() const { return static_cast<int>(net_names_.size()); }
  const std::string& netName(int net) const { return net_names_.at(net); }
  const std::vector<Rect>& rects() const { return rects_; }
  int rectNet(int rect) const { return rect_nets_[rect]; }

 private:
  std::vector<std::string> net_names_;
  std::vector<Rect> rects_;
  std::vector<int> rect_nets_;
};

// L-infinity distance from a point to a box (0 when inside). Inline: it is
// the innermost operation of the nearest-conductor query. D is deduced from
// the box: std::array's size_t extent cannot deduce an int.
template <int D>
double linfDistance(const std::type_identity_t<Vec<D>>& p,
                    const AlignedBox<D>& box)
{
  double d = 0;
  for (int k = 0; k < D; k++) {
    d = std::max({d, box.lo[k] - p[k], p[k] - box.hi[k]});
  }
  return d;
}

// L-infinity gap between two boxes (0 when they touch or overlap).
template <int D>
double linfGap(const AlignedBox<D>& a, const AlignedBox<D>& b)
{
  double d = 0;
  for (int k = 0; k < D; k++) {
    d = std::max({d, b.lo[k] - a.hi[k], a.lo[k] - b.hi[k]});
  }
  return d;
}

}  // namespace fsv
