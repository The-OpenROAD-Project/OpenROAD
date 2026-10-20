// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <numbers>
#include <stdexcept>

#include "fsv/Geometry.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "gtest/gtest.h"

namespace fsv {
namespace {

constexpr double kEps0 = 8.8541878128e-3;  // fF/µm
// Capacitance of an isolated unit cube in units of 4 pi eps0 * edge
// (Hwang and Mascagni, J. Appl. Phys. 95, 2004).
constexpr double kUnitCube = 0.6606785;

Box cube(const double x, const double y, const double z, const double edge)
{
  const double h = edge / 2;
  return {{x - h, y - h, z - h}, {x + h, y + h, z + h}};
}

// A cube high above the ground plane: its capacitance is the free-space value
// with a first-order image correction, C = C0 / (1 - C0 / (4 pi eps 2H)).
TEST(Solver, CubeAboveGround)
{
  const double height = 50;
  Geometry geometry;
  const int net = geometry.addNet("cube");
  geometry.addBox(net, cube(0, 0, height, 1));

  SolveOptions options;
  options.tol_total = 0.001;
  const CapRow row
      = Solver({.threads = 8}).solve(Scene(geometry), net, options);

  const double four_pi_eps = 4 * std::numbers::pi * kEps0;
  const double c0 = kUnitCube * four_pi_eps;
  const double expected = c0 / (1 - kUnitCube / (2 * height));
  EXPECT_LT(row.sigma[net], 0.001 * row.total());
  EXPECT_NEAR(row.total(), expected, 0.005 * expected);
  // All of the cube's charge couples to the ground plane.
  EXPECT_NEAR(
      row.ground, -row.total(), 4 * (row.sigma[net] + row.ground_sigma));
}

// Capacitance scales with the dielectric constant and with size.
TEST(Solver, Scaling)
{
  Geometry small;
  small.addNet("a");
  small.addBox(0, cube(0, 0, 2, 1));
  Geometry large;
  large.addNet("a");
  large.addBox(0, cube(0, 0, 6, 3));

  SolveOptions options;
  options.tol_total = 0.002;
  Solver solver({.threads = 8});
  const CapRow a = solver.solve(Scene(small), 0, options);
  const CapRow b = solver.solve(Scene(large, 3.9), 0, options);
  const double ratio = b.total() / a.total();
  const double sigma
      = ratio * std::hypot(a.sigma[0] / a.total(), b.sigma[0] / b.total());
  EXPECT_NEAR(ratio, 3.9 * 3, 4 * sigma);
}

// Maxwell matrix of two identical cubes: symmetric, couplings negative.
TEST(Solver, TwoCubesSymmetric)
{
  Geometry geometry;
  const int a = geometry.addNet("a");
  const int b = geometry.addNet("b");
  geometry.addBox(a, cube(0, 0, 3, 1));
  geometry.addBox(b, cube(1.5, 0, 3, 1));

  SolveOptions options;
  options.tol_total = 0.002;
  options.tol_coupling = 0.005;
  Solver solver({.threads = 8});
  const Scene scene(geometry);
  const CapRow ra = solver.solve(scene, a, options);
  const CapRow rb = solver.solve(scene, b, options);
  EXPECT_LT(ra.cap[b], 0);
  EXPECT_NEAR(ra.cap[b], rb.cap[a], 4 * std::hypot(ra.sigma[b], rb.sigma[a]));
  EXPECT_NEAR(ra.total(), rb.total(), 4 * std::hypot(ra.sigma[a], rb.sigma[b]));
  // Row sums vanish: raising every conductor to 1 V puts no charge on any.
  const double sum = ra.cap[a] + ra.cap[b] + ra.ground;
  EXPECT_NEAR(sum, 0, 4 * (ra.sigma[a] + ra.sigma[b] + ra.ground_sigma));
}

// Results are bit-identical for any thread count.
TEST(Solver, DeterministicAcrossThreads)
{
  Geometry geometry;
  geometry.addNet("a");
  geometry.addNet("b");
  geometry.addBox(0, {{0, 0, 0.5}, {4, 0.2, 0.7}});
  geometry.addBox(1, {{0, 0.4, 0.5}, {4, 0.6, 0.7}});

  SolveOptions options;
  options.min_walks = 65536;
  options.max_walks = 65536;
  const Scene scene(geometry);
  const CapRow one = Solver({.threads = 1}).solve(scene, 0, options);
  const CapRow many = Solver({.threads = 7}).solve(scene, 0, options);
  EXPECT_EQ(one.walks, many.walks);
  EXPECT_EQ(one.hops, many.hops);
  for (int net = 0; net < 2; net++) {
    EXPECT_EQ(one.cap[net], many.cap[net]);
    EXPECT_EQ(one.sigma[net], many.sigma[net]);
  }
  EXPECT_EQ(one.ground, many.ground);
}

// A Solver reused across scenes gives the same results as fresh Solvers: no
// state carries over between solves.
TEST(Solver, ReuseAcrossScenes)
{
  Geometry first;
  first.addNet("a");
  first.addBox(0, cube(0, 0, 2, 1));
  Geometry second;
  second.addNet("a");
  second.addNet("b");
  second.addBox(0, cube(0, 0, 2, 1));
  second.addBox(1, cube(2, 0, 2, 1));

  SolveOptions options;
  options.min_walks = 32768;
  options.max_walks = 32768;
  const Scene scene1(first);
  const Scene scene2(second);
  Solver reused({.threads = 4});
  const CapRow r1 = reused.solve(scene1, 0, options);
  const CapRow r2 = reused.solve(scene2, 1, options);
  const CapRow f1 = Solver({.threads = 4}).solve(scene1, 0, options);
  const CapRow f2 = Solver({.threads = 4}).solve(scene2, 1, options);
  EXPECT_EQ(r1.cap, f1.cap);
  EXPECT_EQ(r1.ground, f1.ground);
  EXPECT_EQ(r2.cap, f2.cap);
  EXPECT_EQ(r2.ground, f2.ground);
}

// A walk that exceeds the hop guard fails the solve rather than being
// dropped, which would bias the result; bad arguments are rejected upfront.
TEST(Solver, HopGuardFailsSolve)
{
  Geometry geometry;
  const int net = geometry.addNet("a");
  geometry.addBox(net, cube(0, 0, 2, 1));
  const Scene scene(geometry);
  Solver solver({.threads = 2});

  SolveOptions options;
  options.hop_guard = 1;
  EXPECT_THROW(solver.solve(scene, net, options), std::runtime_error);
  options.hop_guard = 0;
  EXPECT_THROW(solver.solve(scene, net, options), std::invalid_argument);
  EXPECT_THROW(solver.solve(scene, 1, SolveOptions{}), std::invalid_argument);
  // The solver stays usable after a failed solve.
  EXPECT_GT(solver.solve(scene, net, SolveOptions{}).total(), 0);
}

}  // namespace
}  // namespace fsv
