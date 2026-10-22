// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <random>
#include <vector>

#include "Dielectric.h"
#include "fsv/Geometry.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"

namespace fsv {
namespace {

// Two wires with nested shells whose outer shells merge, over a two-layer
// planar stack whose interface the shells cross.
Dielectric twoWires()
{
  const std::vector<Stack::PlanarInterface> planar{{0.6, 2.7, 3.9},
                                                   {1.2, 3.9, 1.0}};
  std::vector<Shell> shells;
  int wire_index = 0;
  for (const double x0 : {0.0, 0.42}) {
    const Box wire{{x0, 0, 0.5}, {x0 + 0.2, 3, 0.8}};
    const Box inner{{x0 - 0.05, -0.05, 0.5}, {x0 + 0.25, 3.05, 0.84}};
    const Box outer{{x0 - 0.12, -0.12, 0.5}, {x0 + 0.32, 3.12, 0.9}};
    shells.push_back({inner, wire, 7.0, 0, wire_index, wire_index, 1});
    shells.push_back({outer, inner, 4.5, 1, wire_index, wire_index, 2});
    wire_index++;
  }
  return Dielectric(2.7, planar, shells);
}

// Short wires with sidewall-only shells (TW_T = 0, so the shell's top is
// level with the wire's), whose line ends are wrapped too, over one planar
// interface at the wires' bottom.
Dielectric sidewallOnly()
{
  const std::vector<Stack::PlanarInterface> planar{{0.5, 4.2, 3.9}};
  std::vector<Shell> shells;
  int wire_index = 0;
  for (const double x0 : {0.0, 0.5}) {
    const Box wire{{x0, 0, 0.5}, {x0 + 0.3, 0.14, 0.86}};
    const Box shell{{x0 - 0.03, -0.03, 0.5}, {x0 + 0.33, 0.17, 0.86}};
    shells.push_back({shell, wire, 3.5, 0, wire_index, wire_index, 1});
    wire_index++;
  }
  return Dielectric(4.2, planar, shells);
}

std::vector<Box> wiresOf(const Dielectric& d)
{
  std::vector<Box> wires;
  for (const Shell& s : d.shells()) {
    if (s.material == 0) {
      wires.push_back(s.inner);
    }
  }
  return wires;
}

// The contract of Dielectric::cube: inside the returned cube the
// permittivity is uniform, or takes one value on each side of the plane.
// Checked by sampling random cubes near the wires.
void checkCubes(const Dielectric& d, const Box& region)
{
  const std::vector<Box> wires = wiresOf(d);
  std::mt19937_64 rng(7);
  std::uniform_real_distribution<double> unit(-1, 1);
  std::uniform_real_distribution<double> fraction(0, 1);
  int splits = 0;
  for (int trial = 0; trial < 20000; trial++) {
    Vec3 p;
    for (int k = 0; k < 3; k++) {
      p[k] = region.lo[k] + fraction(rng) * (region.hi[k] - region.lo[k]);
    }
    double clearance = 0.5;
    for (const Box& w : wires) {
      clearance = std::min(clearance, linfDistance(p, w));
    }
    if (clearance <= 0) {
      continue;
    }
    const Dielectric::Cube c = d.cube(p, clearance);
    ASSERT_GT(c.half_size, 0);
    ASSERT_LE(c.half_size, clearance);
    ASSERT_FALSE(c.floored);
    splits += c.split ? 1 : 0;
    const double eps_p = d.eps(p);
    const auto where = [&] {
      return testing::Message()
             << "trial " << trial << " p " << p[0] << " " << p[1] << " " << p[2]
             << " h " << c.half_size << " split " << c.split << " axis "
             << index(c.axis) << " at " << c.coordinate;
    };
    for (int s = 0; s < 64; s++) {
      Vec3 q;
      for (int k = 0; k < 3; k++) {
        q[k] = p[k] + 0.999 * c.half_size * unit(rng);
      }
      if (!c.split) {
        ASSERT_EQ(d.eps(q), eps_p)
            << where() << " q " << q[0] << " " << q[1] << " " << q[2];
        continue;
      }
      const double offset = q[index(c.axis)] - c.coordinate;
      if (std::abs(offset) < 1e-12) {
        continue;
      }
      ASSERT_EQ(d.eps(q), offset < 0 ? c.eps_minus : c.eps_plus)
          << where() << " q " << q[0] << " " << q[1] << " " << q[2];
    }
  }
  // The structures have interfaces of all three orientations near the
  // wires.
  EXPECT_GT(splits, 1000);
}

TEST(Dielectric, CubeHoldsAtMostOnePlane)
{
  checkCubes(twoWires(), {{-0.3, -0.3, 0.3}, {1.0, 0.3, 1.3}});
  checkCubes(sidewallOnly(), {{-0.2, -0.2, 0.3}, {1.0, 0.35, 1.1}});
}

// Planes of any orientation: a wire's vertical shell face, the shell's top,
// and a planar interface.
TEST(Dielectric, SplitsByNearestPlane)
{
  const Dielectric d = twoWires();
  // Beside the left wire's outer shell, at mid height below the planar
  // interface: the shell's x face is nearest.
  const Dielectric::Cube side = d.cube({-0.15, 1.5, 0.55}, 0.15);
  ASSERT_TRUE(side.split);
  EXPECT_EQ(side.axis, Axis::kX);
  EXPECT_DOUBLE_EQ(side.coordinate, -0.12);
  EXPECT_EQ(side.eps_minus, 2.7);
  EXPECT_EQ(side.eps_plus, 4.5);
  // Above the left wire, just above its outer shell.
  const Dielectric::Cube top = d.cube({0.1, 1.5, 0.93}, 0.13);
  ASSERT_TRUE(top.split);
  EXPECT_EQ(top.axis, Axis::kZ);
  EXPECT_DOUBLE_EQ(top.coordinate, 0.9);
  EXPECT_EQ(top.eps_minus, 4.5);
  EXPECT_EQ(top.eps_plus, 3.9);
  // Far from the wires, only the planar interface.
  const Dielectric::Cube planar = d.cube({5, 1.5, 1.1}, 1.0);
  ASSERT_TRUE(planar.split);
  EXPECT_EQ(planar.axis, Axis::kZ);
  EXPECT_DOUBLE_EQ(planar.coordinate, 1.2);
  EXPECT_DOUBLE_EQ(planar.half_size, 0.5);  // the interface at 0.6
}

// The planar interface does not exist inside a shell.
TEST(Dielectric, ShellsReplacePlanarLayers)
{
  const Dielectric d = twoWires();
  EXPECT_EQ(d.eps({0.1, 1.5, 0.82}), 7.0);
  EXPECT_EQ(d.eps({-0.1, 1.5, 0.55}), 4.5);
  EXPECT_EQ(d.eps({-0.1, 1.5, 0.65}), 4.5);
  EXPECT_EQ(d.eps({-0.2, 1.5, 0.65}), 3.9);
  EXPECT_EQ(d.eps({-0.2, 1.5, 0.55}), 2.7);
  // Between the wires the outer shells merge.
  EXPECT_EQ(d.eps({0.31, 1.5, 0.6}), 4.5);
  // Inside the inner shell the planar interface is not a split.
  const Dielectric::Cube c = d.cube({0.225, 1.5, 0.6}, 0.025);
  EXPECT_FALSE(c.split);
}

// At an edge where two interfaces meet, the cube is floored at the minimum
// size and accounts for the nearest plane only.
TEST(Dielectric, FloorsCubesAtEdges)
{
  const Dielectric d = twoWires();
  const Vec3 edge{-0.12 - 1e-9, 1.5, 0.9 + 2e-9};
  const Dielectric::Cube exact = d.cube(edge, 0.1);
  EXPECT_LT(exact.half_size, 1e-8);
  const Dielectric::Cube floored = d.cube(edge, 0.1, 1e-5);
  EXPECT_TRUE(floored.floored);
  EXPECT_DOUBLE_EQ(floored.half_size, 1e-5);
}

}  // namespace
}  // namespace fsv
