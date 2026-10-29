// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <random>
#include <type_traits>
#include <vector>

#include "Dielectric.h"
#include "fsv/Geometry.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"

namespace fsv {
namespace {

// Two wires with nested shells whose outer shells merge, over a two-layer
// planar stack whose interface the shells cross.
Dielectric<3> twoWires()
{
  const std::vector<Stack::PlanarInterface> planar{{0.6, 2.7, 3.9},
                                                   {1.2, 3.9, 1.0}};
  std::vector<Shell<3>> shells;
  int wire_index = 0;
  for (const double x0 : {0.0, 0.42}) {
    const Box wire{{x0, 0, 0.5}, {x0 + 0.2, 3, 0.8}};
    const Box inner{{x0 - 0.05, -0.05, 0.5}, {x0 + 0.25, 3.05, 0.84}};
    const Box outer{{x0 - 0.12, -0.12, 0.5}, {x0 + 0.32, 3.12, 0.9}};
    shells.push_back({inner, wire, 7.0, 0, wire_index, wire_index, 1});
    shells.push_back({outer, inner, 4.5, 1, wire_index, wire_index, 2});
    wire_index++;
  }
  return Dielectric<3>(2.7, planar, shells);
}

// Short wires with sidewall-only shells (TW_T = 0, so the shell's top is
// level with the wire's), whose line ends are wrapped too, over one planar
// interface at the wires' bottom.
Dielectric<3> sidewallOnly()
{
  const std::vector<Stack::PlanarInterface> planar{{0.5, 4.2, 3.9}};
  std::vector<Shell<3>> shells;
  int wire_index = 0;
  for (const double x0 : {0.0, 0.5}) {
    const Box wire{{x0, 0, 0.5}, {x0 + 0.3, 0.14, 0.86}};
    const Box shell{{x0 - 0.03, -0.03, 0.5}, {x0 + 0.33, 0.17, 0.86}};
    shells.push_back({shell, wire, 3.5, 0, wire_index, wire_index, 1});
    wire_index++;
  }
  return Dielectric<3>(4.2, planar, shells);
}

// The cross section of twoWires in the x-z plane.
Dielectric<2> twoWires2D()
{
  const std::vector<Stack::PlanarInterface> planar{{0.6, 2.7, 3.9},
                                                   {1.2, 3.9, 1.0}};
  std::vector<Shell<2>> shells;
  int wire_index = 0;
  for (const double x0 : {0.0, 0.42}) {
    const Rect wire{{x0, 0.5}, {x0 + 0.2, 0.8}};
    const Rect inner{{x0 - 0.05, 0.5}, {x0 + 0.25, 0.84}};
    const Rect outer{{x0 - 0.12, 0.5}, {x0 + 0.32, 0.9}};
    shells.push_back({inner, wire, 7.0, 0, wire_index, wire_index, 1});
    shells.push_back({outer, inner, 4.5, 1, wire_index, wire_index, 2});
    wire_index++;
  }
  return Dielectric<2>(2.7, planar, shells);
}

template <int D>
std::vector<AlignedBox<D>> wiresOf(const Dielectric<D>& d)
{
  std::vector<AlignedBox<D>> wires;
  for (const Shell<D>& s : d.shells()) {
    if (s.material == 0) {
      wires.push_back(s.inner);
    }
  }
  return wires;
}

// The contract of Dielectric::cube: inside the returned cube the
// permittivity is uniform, or takes one value on each side of the plane.
// Checked by sampling random cubes near the wires.
template <int D>
void checkCubes(const Dielectric<D>& d, const AlignedBox<D>& region)
{
  const std::vector<AlignedBox<D>> wires = wiresOf(d);
  std::mt19937_64 rng(7);
  std::uniform_real_distribution<double> unit(-1, 1);
  std::uniform_real_distribution<double> fraction(0, 1);
  int splits = 0;
  for (int trial = 0; trial < 20000; trial++) {
    Vec<D> p;
    for (int k = 0; k < D; k++) {
      p[k] = region.lo[k] + fraction(rng) * (region.hi[k] - region.lo[k]);
    }
    double clearance = 0.5;
    for (const AlignedBox<D>& w : wires) {
      clearance = std::min(clearance, linfDistance(p, w));
    }
    if (clearance <= 0) {
      continue;
    }
    const typename Dielectric<D>::Cube c = d.cube(p, clearance);
    ASSERT_GT(c.half_size, 0);
    ASSERT_LE(c.half_size, clearance);
    ASSERT_FALSE(c.floored);
    splits += c.split ? 1 : 0;
    const double eps_p = d.eps(p);
    const auto point = [](const Vec<D>& v) {
      testing::Message m;
      for (int k = 0; k < D; k++) {
        m << " " << v[k];
      }
      return m;
    };
    const auto where = [&] {
      return testing::Message() << "trial " << trial << " p" << point(p)
                                << " h " << c.half_size << " split " << c.split
                                << " axis " << c.axis << " at " << c.coordinate;
    };
    for (int s = 0; s < 64; s++) {
      Vec<D> q;
      for (int k = 0; k < D; k++) {
        q[k] = p[k] + 0.999 * c.half_size * unit(rng);
      }
      if (!c.split) {
        ASSERT_EQ(d.eps(q), eps_p) << where() << " q" << point(q);
        continue;
      }
      const double offset = q[c.axis] - c.coordinate;
      if (std::abs(offset) < 1e-12) {
        continue;
      }
      ASSERT_EQ(d.eps(q), offset < 0 ? c.eps_minus : c.eps_plus)
          << where() << " q" << point(q);
    }
  }
  // The structures have interfaces of every orientation near the wires.
  EXPECT_GT(splits, 1000);
}

// A routed patch: crossing wires on two layers, each wire with two nested
// shells. The outer shells of neighbours on a layer merge (same material);
// the layers' shells are different materials and do not overlap.
Dielectric<3> wireGrid()
{
  const std::vector<Stack::PlanarInterface> planar{{0.6, 2.7, 3.9},
                                                   {1.6, 3.9, 1.0}};
  std::vector<Shell<3>> shells;
  int box = 0;
  for (int layer = 0; layer < 2; layer++) {
    const double z0 = layer == 0 ? 0.5 : 1.1;
    for (int i = 0; i < 30; i++) {
      const double c = 0.3 * i;
      Box wire{{-0.5, c, z0}, {9, c + 0.14, z0 + 0.2}};
      if (layer == 1) {
        wire = {{c, -0.5, z0}, {c + 0.14, 9, z0 + 0.2}};
      }
      Box inner = wire;
      Box outer = wire;
      for (int k = 0; k < 2; k++) {
        inner.lo[k] -= 0.02;
        inner.hi[k] += 0.02;
        outer.lo[k] -= 0.09;
        outer.hi[k] += 0.09;
      }
      inner.hi[2] += 0.02;
      outer.hi[2] += 0.05;
      shells.push_back({inner, wire, 7.0, 2 * layer, box, box, 1});
      shells.push_back({outer, inner, 4.5, 2 * layer + 1, box, box, 2});
      box++;
    }
  }
  return Dielectric<3>(2.7, planar, shells);
}

// A cross section like wireGrid's: rectangles in the x-z plane on two
// layers, with two nested shells each.
Dielectric<2> wireGrid2D()
{
  const std::vector<Stack::PlanarInterface> planar{{0.6, 2.7, 3.9},
                                                   {1.6, 3.9, 1.0}};
  std::vector<Shell<2>> shells;
  int box = 0;
  for (int layer = 0; layer < 2; layer++) {
    const double z0 = layer == 0 ? 0.5 : 1.1;
    for (int i = 0; i < 30; i++) {
      const double x = 0.3 * i + 0.15 * layer;
      const Rect wire{{x, z0}, {x + 0.14, z0 + 0.2}};
      const Rect inner{{x - 0.02, z0}, {x + 0.16, z0 + 0.22}};
      const Rect outer{{x - 0.09, z0}, {x + 0.23, z0 + 0.25}};
      shells.push_back({inner, wire, 7.0, 2 * layer, box, box, 1});
      shells.push_back({outer, inner, 4.5, 2 * layer + 1, box, box, 2});
      box++;
    }
  }
  return Dielectric<2>(2.7, planar, shells);
}

// The permittivity by a scan over every shell: the shallowest containing
// the point, else the planar stack.
template <int D>
double bruteEps(const Dielectric<D>& d, const std::type_identity_t<Vec<D>>& p)
{
  const Shell<D>* best = nullptr;
  for (const Shell<D>& s : d.shells()) {
    bool inside = true;
    for (int k = 0; k < D; k++) {
      inside = inside && p[k] > s.outer.lo[k] && p[k] < s.outer.hi[k];
    }
    if (inside && (best == nullptr || s.depth < best->depth)) {
      best = &s;
    }
  }
  return best != nullptr ? best->eps : d.planarEps(p[D - 1]);
}

// The cube's size and plane by a scan over every interface piece: the
// nearest piece may cut the cube, every other piece limits it. Among
// equals the nearest is the first of: the planar interface above, the one
// below, then shells and their faces in index order.
template <int D>
void bruteCube(const Dielectric<D>& d,
               const std::type_identity_t<Vec<D>>& p,
               const double max_half_size,
               double& half_size,
               int& axis,
               double& coordinate)
{
  constexpr int kZ = D - 1;
  struct Piece
  {
    int axis;
    double coordinate;
    double distance;
  };
  std::vector<Piece> pieces;
  // The two nearest planar interfaces, the upper first among equals.
  const std::vector<Stack::PlanarInterface>& planar = d.planar();
  const int count = static_cast<int>(planar.size());
  int up = 0;
  while (up < count && planar[up].z <= p[kZ]) {
    up++;
  }
  int down = up - 1;
  for (int pass = 0; pass < 2 && (up < count || down >= 0); pass++) {
    const double du = up < count ? planar[up].z - p[kZ] : max_half_size;
    const double dd = down >= 0 ? p[kZ] - planar[down].z : max_half_size;
    const int i = du <= dd ? up++ : down--;
    const double dist = std::min(du, dd);
    if (dist < max_half_size) {
      pieces.push_back({kZ, planar[i].z, dist});
    }
  }
  for (const Shell<D>& s : d.shells()) {
    for (int a = 0; a < D; a++) {
      double excess = 0;
      for (int t = 0; t < D; t++) {
        if (t != a) {
          excess
              = std::max({excess, s.outer.lo[t] - p[t], p[t] - s.outer.hi[t]});
        }
      }
      for (const double c : {s.outer.lo[a], s.outer.hi[a]}) {
        pieces.push_back({a, c, std::max(std::abs(p[a] - c), excess)});
      }
    }
  }
  const Piece* nearest = nullptr;
  for (const Piece& piece : pieces) {
    if (piece.distance < max_half_size
        && (nearest == nullptr || piece.distance < nearest->distance)) {
      nearest = &piece;
    }
  }
  half_size = max_half_size;
  axis = -1;
  coordinate = 0;
  if (nearest == nullptr) {
    return;
  }
  for (const Piece& piece : pieces) {
    const bool coplanar
        = piece.axis == nearest->axis
          && std::abs(piece.coordinate - nearest->coordinate) <= 1e-9;
    if (piece.distance < max_half_size && !coplanar) {
      half_size = std::min(half_size, piece.distance);
    }
  }
  axis = nearest->axis;
  coordinate = nearest->coordinate;
}

// More shells around one spot than the dielectric lists: 100 stacked short
// wires whose shells of one conformal dielectric all overlap there.
Dielectric<3> shellPile()
{
  std::vector<Shell<3>> shells;
  for (int i = 0; i < 100; i++) {
    const double c = 0.01 * i;
    const Box wire{{c, c, 0.5}, {c + 0.2, c + 0.2, 0.7}};
    const Box outer{{c - 0.5, c - 0.5, 0.5}, {c + 0.7, c + 0.7, 1.0}};
    shells.push_back({outer, wire, 4.5, 0, i, i, 1});
  }
  return Dielectric<3>(2.7, {{0.8, 2.7, 3.9}}, shells);
}

// Permittivities and cubes match a scan exactly at random points in
// `region`, of which at least min_splits have split cubes.
template <int D>
void checkAgainstScan(const Dielectric<D>& d,
                      const AlignedBox<D>& region,
                      const int min_splits)
{
  std::mt19937_64 rng(11);
  std::uniform_real_distribution<double> unit(0, 1);
  int splits = 0;
  for (int trial = 0; trial < 20000; trial++) {
    Vec<D> p;
    for (int k = 0; k < D; k++) {
      p[k] = region.lo[k] + unit(rng) * (region.hi[k] - region.lo[k]);
    }
    ASSERT_EQ(d.eps(p), bruteEps(d, p)) << trial;
    const double max_half_size = 0.4;
    double half_size = 0;
    int axis = -1;
    double coordinate = 0;
    bruteCube(d, p, max_half_size, half_size, axis, coordinate);
    const typename Dielectric<D>::Cube c = d.cube(p, max_half_size);
    ASSERT_EQ(c.half_size, half_size) << trial;
    if (c.split) {
      ASSERT_EQ(c.axis, axis) << trial;
      ASSERT_EQ(c.coordinate, coordinate) << trial;
      splits++;
    }
  }
  EXPECT_GE(splits, min_splits);
}

// The shell index finds every shell a scan finds, on patches with many
// shells in 3D and 2D, and where more shells are near a point than fit in a
// list.
TEST(Dielectric, IndexMatchesScan)
{
  checkAgainstScan(wireGrid(), {{-1, -1, 0.3}, {10, 10, 1.8}}, 1000);
  checkAgainstScan(shellPile(), {{-1, -1, 0.3}, {2, 2, 1.3}}, 100);
  checkAgainstScan(wireGrid2D(), {{-1, 0.3}, {10, 1.8}}, 1000);
}

TEST(Dielectric, CubeHoldsAtMostOnePlane)
{
  checkCubes(twoWires(), {{-0.3, -0.3, 0.3}, {1.0, 0.3, 1.3}});
  checkCubes(sidewallOnly(), {{-0.2, -0.2, 0.3}, {1.0, 0.35, 1.1}});
}

TEST(Dielectric, SquareHoldsAtMostOneLine)
{
  checkCubes(twoWires2D(), {{-0.3, 0.3}, {1.0, 1.3}});
}

// In 2D the interfaces are lines normal to x (shell sides) or z.
TEST(Dielectric, SplitsByNearestLine)
{
  const Dielectric<2> d = twoWires2D();
  const Dielectric<2>::Cube side = d.cube({-0.15, 0.55}, 0.15);
  ASSERT_TRUE(side.split);
  EXPECT_EQ(side.axis, 0);
  EXPECT_DOUBLE_EQ(side.coordinate, -0.12);
  EXPECT_EQ(side.eps_minus, 2.7);
  EXPECT_EQ(side.eps_plus, 4.5);
  const Dielectric<2>::Cube top = d.cube({0.1, 0.93}, 0.13);
  ASSERT_TRUE(top.split);
  EXPECT_EQ(top.axis, 1);
  EXPECT_DOUBLE_EQ(top.coordinate, 0.9);
  EXPECT_EQ(d.eps({0.1, 0.82}), 7.0);
  EXPECT_EQ(d.eps({-0.2, 0.55}), 2.7);
  EXPECT_EQ(d.eps({0.31, 0.6}), 4.5);
}

// Planes of any orientation: a wire's vertical shell face, the shell's top,
// and a planar interface.
TEST(Dielectric, SplitsByNearestPlane)
{
  const Dielectric<3> d = twoWires();
  // Beside the left wire's outer shell, at mid height below the planar
  // interface: the shell's x face is nearest.
  const Dielectric<3>::Cube side = d.cube({-0.15, 1.5, 0.55}, 0.15);
  ASSERT_TRUE(side.split);
  EXPECT_EQ(side.axis, 0);
  EXPECT_DOUBLE_EQ(side.coordinate, -0.12);
  EXPECT_EQ(side.eps_minus, 2.7);
  EXPECT_EQ(side.eps_plus, 4.5);
  // Above the left wire, just above its outer shell.
  const Dielectric<3>::Cube top = d.cube({0.1, 1.5, 0.93}, 0.13);
  ASSERT_TRUE(top.split);
  EXPECT_EQ(top.axis, 2);
  EXPECT_DOUBLE_EQ(top.coordinate, 0.9);
  EXPECT_EQ(top.eps_minus, 4.5);
  EXPECT_EQ(top.eps_plus, 3.9);
  // Far from the wires, only the planar interface.
  const Dielectric<3>::Cube planar = d.cube({5, 1.5, 1.1}, 1.0);
  ASSERT_TRUE(planar.split);
  EXPECT_EQ(planar.axis, 2);
  EXPECT_DOUBLE_EQ(planar.coordinate, 1.2);
  EXPECT_DOUBLE_EQ(planar.half_size, 0.5);  // the interface at 0.6
}

// The planar interface does not exist inside a shell.
TEST(Dielectric, ShellsReplacePlanarLayers)
{
  const Dielectric<3> d = twoWires();
  EXPECT_EQ(d.eps({0.1, 1.5, 0.82}), 7.0);
  EXPECT_EQ(d.eps({-0.1, 1.5, 0.55}), 4.5);
  EXPECT_EQ(d.eps({-0.1, 1.5, 0.65}), 4.5);
  EXPECT_EQ(d.eps({-0.2, 1.5, 0.65}), 3.9);
  EXPECT_EQ(d.eps({-0.2, 1.5, 0.55}), 2.7);
  // Between the wires the outer shells merge.
  EXPECT_EQ(d.eps({0.31, 1.5, 0.6}), 4.5);
  // Inside the inner shell the planar interface is not a split.
  const Dielectric<3>::Cube c = d.cube({0.225, 1.5, 0.6}, 0.025);
  EXPECT_FALSE(c.split);
}

// At an edge where two interfaces meet, the cube is floored at the minimum
// size and accounts for the nearest plane only.
TEST(Dielectric, FloorsCubesAtEdges)
{
  const Dielectric<3> d = twoWires();
  const Vec3 edge{-0.12 - 1e-9, 1.5, 0.9 + 2e-9};
  const Dielectric<3>::Cube exact = d.cube(edge, 0.1);
  EXPECT_LT(exact.half_size, 1e-8);
  const Dielectric<3>::Cube floored = d.cube(edge, 0.1, 1e-5);
  EXPECT_TRUE(floored.floored);
  EXPECT_DOUBLE_EQ(floored.half_size, 1e-5);
}

}  // namespace
}  // namespace fsv
