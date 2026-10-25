// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Dielectric.h"
#include "Structures.h"
#include "fsv/Geometry.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"
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

// Rounds are sized to the walks the tolerance still needs, so a solve stops
// just inside its target rather than up to a doubling past it: the error is
// below the tolerance but close to it, at any tolerance.
TEST(Solver, StopsNearTheTarget)
{
  Geometry geometry;
  const int net = geometry.addNet("cube");
  geometry.addBox(net, cube(0, 0, 2, 1));
  const Scene scene(geometry);
  Solver solver({.threads = 8});
  for (const double tol : {0.01, 0.005, 0.002}) {
    SolveOptions options;
    options.tol_total = tol;
    const CapRow row = solver.solve(scene, net, options);
    const double error = row.sigma[net] / row.total();
    EXPECT_LT(error, tol);
    EXPECT_GT(error, 0.8 * tol) << "tolerance " << tol;
  }
}

// Unset, tol_coupling follows tol_total, relative to the total: a looser
// total loosens the couplings and the ground too, and a tighter tol_coupling
// alone takes more walks.
TEST(Solver, CouplingToleranceFollowsTheTotal)
{
  Geometry geometry;
  const int a = geometry.addNet("a");
  const int b = geometry.addNet("b");
  geometry.addBox(a, cube(0, 0, 1, 1));
  geometry.addBox(b, cube(1.6, 0, 1, 1));
  const Scene scene(geometry);
  Solver solver({.threads = 8});
  auto worst = [&](const CapRow& row) {
    return std::max({row.sigma[a], row.sigma[b], row.ground_sigma})
           / row.total();
  };
  SolveOptions options;
  options.tol_total = 0.02;
  const CapRow loose = solver.solve(scene, a, options);
  EXPECT_LT(worst(loose), 0.02);
  EXPECT_GT(worst(loose), 0.8 * 0.02);
  options.tol_coupling = 0.005;
  const CapRow tight = solver.solve(scene, a, options);
  EXPECT_LT(tight.sigma[b], 0.005 * tight.total());
  EXPECT_LT(tight.ground_sigma, 0.005 * tight.total());
  EXPECT_GT(tight.walks, loose.walks);
}

// Around shells whose first cubes shrink, a trial picks the surface inside
// them where it is clearly cheaper: on the line ends, whose shells cross a
// layer boundary. A planar structure, and a solve capped below the trial's
// budget, run no trial.
TEST(Solver, TrialPicksTheSurface)
{
  Solver solver({.threads = 8});
  const Structure line_ends = readStructure("conformal_line_ends");
  const Scene shelled(line_ends.geometry, line_ends.stack);
  const CapRow chosen = solver.solve(shelled, 0, SolveOptions{});
  EXPECT_TRUE(chosen.inside_surface);
  EXPECT_GT(chosen.trial_walks, 0);

  // With the walk count fixed only the hops per walk count, and the inside
  // surface's walks are shorter here.
  SolveOptions fixed;
  fixed.min_walks = fixed.max_walks = 262144;
  const CapRow fixed_row = solver.solve(shelled, 0, fixed);
  EXPECT_TRUE(fixed_row.inside_surface);
  EXPECT_EQ(fixed_row.walks, 262144);

  SolveOptions capped;
  capped.min_walks = capped.max_walks = 65536;
  const CapRow quick = solver.solve(shelled, 0, capped);
  EXPECT_FALSE(quick.inside_surface);
  EXPECT_EQ(quick.trial_walks, 0);
  EXPECT_EQ(quick.walks, 65536);

  const Structure planar = readStructure("etch_stop_pair", 10);
  const CapRow clear
      = solver.solve(Scene(planar.geometry, planar.stack), 0, SolveOptions{});
  EXPECT_FALSE(clear.inside_surface);
  EXPECT_EQ(clear.trial_walks, 0);
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
  options.tol_coupling = 0.002;
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
  for (const int64_t max_walks : {0, -1}) {
    EXPECT_THROW(solver.solve(scene, net, {.max_walks = max_walks}),
                 std::invalid_argument);
  }
  // The stopping test divides by the tolerances.
  for (const double tol : {0.0,
                           -0.01,
                           std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()}) {
    EXPECT_THROW(solver.solve(scene, net, {.tol_total = tol}),
                 std::invalid_argument);
    EXPECT_THROW(solver.solve(scene, net, {.tol_coupling = tol}),
                 std::invalid_argument);
  }
  // The solver stays usable after a failed solve.
  EXPECT_GT(solver.solve(scene, net, SolveOptions{}).total(), 0);
}

// A planar stack from layer tops and permittivities, bottom-up from the
// ground plane, with the given background above.
Stack planarStack(const std::vector<std::pair<double, double>>& layers,
                  const double background)
{
  Stack stack;
  stack.technology = "test";
  stack.background_eps = background;
  double bottom = 0;
  for (const auto& [top, eps] : layers) {
    stack.dielectrics.push_back(
        {.name = "D", .bottom = bottom, .top = top, .eps = eps});
    bottom = top;
  }
  return stack;
}

// ILD (2.7) up to 0.8, an etch stop (7.0) to 1.0, then 3.9 above: one wire
// below the etch stop and one above it.
Stack etchStopStack(const double scale = 1.0)
{
  return planarStack({{0.8, 2.7 * scale}, {1.0, 7.0 * scale}}, 3.9 * scale);
}

Geometry wiresAcrossEtchStop()
{
  Geometry geometry;
  geometry.addNet("lower");
  geometry.addNet("upper");
  geometry.addBox(0, {{0, 0, 0.5}, {2, 0.2, 0.7}});
  geometry.addBox(1, {{0, 0.3, 1.1}, {2, 0.5, 1.3}});
  return geometry;
}

// A stack whose layers share one permittivity has no interfaces and matches
// the uniform scene.
TEST(Solver, EqualPermittivityStackMatchesUniform)
{
  const Geometry geometry = wiresAcrossEtchStop();
  const Stack stack = planarStack({{0.8, 3.9}, {1.0, 3.9}}, 3.9);
  const Scene layered(geometry, stack);
  EXPECT_TRUE(layered.interfaces().empty());
  SolveOptions options;
  options.min_walks = options.max_walks = 65536;
  Solver solver({.threads = 4});
  const CapRow a = solver.solve(layered, 0, options);
  const CapRow b = solver.solve(Scene(geometry, 3.9), 0, options);
  EXPECT_EQ(a.cap, b.cap);
  EXPECT_EQ(a.ground, b.ground);
}

// The Maxwell matrix stays symmetric across interfaces, and each row sums to
// zero.
TEST(Solver, ReciprocityAcrossInterfaces)
{
  const Geometry geometry = wiresAcrossEtchStop();
  const Scene scene(geometry, etchStopStack());
  ASSERT_EQ(scene.interfaces().size(), 2u);
  SolveOptions options;
  options.tol_total = 0.002;
  options.tol_coupling = 0.002;
  Solver solver({.threads = 8});
  const CapRow r0 = solver.solve(scene, 0, options);
  const CapRow r1 = solver.solve(scene, 1, options);
  EXPECT_LT(r0.cap[1], 0);
  EXPECT_NEAR(r0.cap[1], r1.cap[0], 4 * std::hypot(r0.sigma[1], r1.sigma[0]));
  for (const CapRow& r : {r0, r1}) {
    EXPECT_NEAR(r.cap[0] + r.cap[1] + r.ground,
                0,
                4 * (r.sigma[0] + r.sigma[1] + r.ground_sigma));
  }
}

// Scaling every permittivity scales the capacitances exactly: the tables
// depend only on ratios, so the walks are identical.
TEST(Solver, PermittivityScaling)
{
  const Geometry geometry = wiresAcrossEtchStop();
  SolveOptions options;
  options.min_walks = options.max_walks = 65536;
  Solver solver({.threads = 4});
  const CapRow a = solver.solve(Scene(geometry, etchStopStack()), 0, options);
  const CapRow b
      = solver.solve(Scene(geometry, etchStopStack(2.0)), 0, options);
  for (int net = 0; net < 2; net++) {
    EXPECT_NEAR(b.cap[net], 2 * a.cap[net], 1e-12 * std::abs(a.cap[net]));
  }
  EXPECT_EQ(a.hops, b.hops);
}

TEST(Solver, LayeredDeterministicAcrossThreads)
{
  const Geometry geometry = wiresAcrossEtchStop();
  const Scene scene(geometry, etchStopStack());
  SolveOptions options;
  options.min_walks = options.max_walks = 65536;
  const CapRow one = Solver({.threads = 1}).solve(scene, 0, options);
  const CapRow many = Solver({.threads = 6}).solve(scene, 0, options);
  EXPECT_EQ(one.cap, many.cap);
  EXPECT_EQ(one.hops, many.hops);
}

// Panel discretization bias of the two-dielectric tables, end to end: 32 and
// 64 panels agree within the statistical error plus 0.3%.
TEST(Solver, LayeredPanelConvergence)
{
  const Geometry geometry = wiresAcrossEtchStop();
  const Scene scene(geometry, etchStopStack());
  SolveOptions options;
  options.tol_total = 0.001;
  const CapRow coarse = Solver({.threads = 8}).solve(scene, 0, options);
  const CapRow fine
      = Solver({.threads = 8, .panels = 64}).solve(scene, 0, options);
  const double sigma = std::hypot(coarse.sigma[0], fine.sigma[0]);
  EXPECT_NEAR(coarse.total(), fine.total(), 4 * sigma + 0.003 * fine.total());
}

TEST(Solver, RejectsPanelsNotMultipleOf16)
{
  EXPECT_THROW(Solver({.panels = 24}), std::invalid_argument);
  EXPECT_THROW(Solver({.panels = 0}), std::invalid_argument);
  EXPECT_NO_THROW(Solver({.panels = 48}));
}

TEST(Solver, RejectsBadStacks)
{
  const Geometry geometry = wiresAcrossEtchStop();
  for (const double eps : {0.0, -1.0, std::nan("")}) {
    Stack layer = etchStopStack();
    layer.dielectrics.back().eps = eps;
    EXPECT_THROW(Scene(geometry, layer), std::invalid_argument) << eps;
    Stack background = etchStopStack();
    background.background_eps = eps;
    EXPECT_THROW(Scene(geometry, background), std::invalid_argument) << eps;
  }
  Stack unordered = etchStopStack();
  unordered.dielectrics.back().top = unordered.dielectrics.front().top;
  EXPECT_THROW(Scene(geometry, unordered), std::invalid_argument);
}

// The scene's interface lookup agrees with the stack's, on the interfaces
// (where both take the permittivity above) and between them.
TEST(Solver, SceneEpsMatchesStack)
{
  const Stack stack = etchStopStack();
  const Geometry geometry = wiresAcrossEtchStop();
  const Scene scene(geometry, stack);
  for (const Stack::Dielectric& d : stack.dielectrics) {
    for (const double z : {d.bottom, 0.5 * (d.bottom + d.top), d.top}) {
      EXPECT_EQ(scene.epsAt(z), stack.epsAt(z)) << z;
    }
  }
  EXPECT_EQ(scene.epsAt(10.0), stack.epsAt(10.0));
}

// The etch-stop stack with conductor layers for both wires: M1 (the lower
// wire, in the 2.7 ILD) wrapped by the given conformal dielectrics, and M2
// (the upper wire, in the 3.9 background) by a 4.5 shell.
Stack conformalStack(
    const std::vector<Stack::ConformalDielectric>& m1_conformals)
{
  Stack stack = etchStopStack();
  stack.conductors.push_back(
      {.name = "M1", .bottom = 0.5, .top = 0.7, .conformals = m1_conformals});
  stack.conductors.push_back({.name = "M2",
                              .bottom = 1.1,
                              .top = 1.3,
                              .conformals = {{"C2", 0.03, 0.02, 4.5}}});
  return stack;
}

// A conformal dielectric with the permittivity of the layer around it
// changes nothing but the cubes, so the result agrees statistically.
TEST(Solver, EqualPermittivityConformalMatchesPlanar)
{
  const Geometry geometry = wiresAcrossEtchStop();
  Stack stack = etchStopStack();
  stack.conductors.push_back({.name = "M1",
                              .bottom = 0.5,
                              .top = 0.7,
                              .conformals = {{"C", 0.05, 0.05, 2.7}}});
  SolveOptions options;
  options.tol_total = 0.002;
  Solver solver({.threads = 8});
  const CapRow planar
      = solver.solve(Scene(geometry, etchStopStack()), 0, options);
  const CapRow conformal = solver.solve(Scene(geometry, stack), 0, options);
  EXPECT_NEAR(conformal.total(),
              planar.total(),
              4 * std::hypot(conformal.sigma[0], planar.sigma[0]));
}

// A conformal dielectric that fills the rest of its layer, far beyond the
// wire, matches the planar stack with that layer split at the wire's
// bottom.
TEST(Solver, ConformalFillingLayerMatchesPlanar)
{
  const Geometry geometry = wiresAcrossEtchStop();
  Stack stack = etchStopStack();
  stack.conductors.push_back({.name = "M1",
                              .bottom = 0.5,
                              .top = 0.7,
                              .conformals = {{"C", 30, 0.1, 5.0}}});
  const Stack split = planarStack({{0.5, 2.7}, {0.8, 5.0}, {1.0, 7.0}}, 3.9);
  SolveOptions options;
  options.tol_total = 0.002;
  Solver solver({.threads = 8});
  const CapRow conformal = solver.solve(Scene(geometry, stack), 0, options);
  const CapRow planar = solver.solve(Scene(geometry, split), 0, options);
  EXPECT_NEAR(conformal.total(),
              planar.total(),
              0.003 * planar.total()
                  + 4 * std::hypot(conformal.sigma[0], planar.sigma[0]));
}

// Shells raise or lower the capacitance with their permittivity, keep the
// Maxwell matrix symmetric, and give the same result for any thread count.
TEST(Solver, ConformalReciprocityAndDeterminism)
{
  const Geometry geometry = wiresAcrossEtchStop();
  const Scene scene(
      geometry,
      conformalStack({{"C1a", 0.02, 0.0, 7.0}, {"C1b", 0.03, 0.03, 3.0}}));
  SolveOptions options;
  options.tol_total = 0.004;
  options.tol_coupling = 0.004;
  Solver solver({.threads = 8});
  const CapRow r0 = solver.solve(scene, 0, options);
  const CapRow r1 = solver.solve(scene, 1, options);
  EXPECT_NEAR(r0.cap[1], r1.cap[0], 4 * std::hypot(r0.sigma[1], r1.sigma[0]));

  SolveOptions fixed;
  fixed.min_walks = fixed.max_walks = 32768;
  const CapRow one = Solver({.threads = 1}).solve(scene, 0, fixed);
  const CapRow many = Solver({.threads = 6}).solve(scene, 0, fixed);
  EXPECT_EQ(one.cap, many.cap);
  EXPECT_EQ(one.hops, many.hops);

  const Scene high_k(geometry, conformalStack({{"C", 0.05, 0.05, 7.0}}));
  const Scene low_k(geometry, conformalStack({{"C", 0.05, 0.05, 1.5}}));
  const CapRow high = solver.solve(high_k, 0, options);
  const CapRow low = solver.solve(low_k, 0, options);
  EXPECT_GT(high.total(),
            low.total() + 4 * std::hypot(high.sigma[0], low.sigma[0]));
}

// A wire drawn as two abutting boxes of one net gets the shells of the
// merged wire (the union of its boxes), and matches the single box.
TEST(Solver, SplitWireMatchesSingleBox)
{
  const std::vector<Stack::ConformalDielectric> nested{
      {"C1a", 0.02, 0.0, 7.0}, {"C1b", 0.03, 0.03, 3.0}};
  Geometry whole;
  whole.addNet("lower");
  whole.addNet("upper");
  whole.addBox(0, {{0, 0, 0.5}, {2, 0.2, 0.7}});
  whole.addBox(1, {{0, 0.3, 1.1}, {2, 0.5, 1.3}});
  Geometry split;
  split.addNet("lower");
  split.addNet("upper");
  split.addBox(0, {{0, 0, 0.5}, {0.8, 0.2, 0.7}});
  split.addBox(0, {{0.8, 0, 0.5}, {2, 0.2, 0.7}});
  split.addBox(1, {{0, 0.3, 1.1}, {2, 0.5, 1.3}});
  const Stack stack = conformalStack(nested);
  const Scene whole_scene(whole, stack);
  const Scene split_scene(split, stack);
  // Along the seam the union rule gives the inner shell, as for one box.
  EXPECT_EQ(split_scene.dielectric().eps({0.8, -0.01, 0.6}), 7.0);
  EXPECT_EQ(split_scene.dielectric().eps({0.8, -0.04, 0.6}), 3.0);
  SolveOptions options;
  options.tol_total = 0.003;
  Solver solver({.threads = 8});
  const CapRow a = solver.solve(whole_scene, 0, options);
  const CapRow b = solver.solve(split_scene, 0, options);
  EXPECT_NEAR(a.total(), b.total(), 4 * std::hypot(a.sigma[0], b.sigma[0]));
}

// Expects C_ij from master i to equal C_ji from master j within four
// standard errors.
void expectReciprocal(const CapRow& ri, const CapRow& rj)
{
  const int i = ri.master;
  const int j = rj.master;
  EXPECT_NEAR(ri.cap[j], rj.cap[i], 4 * std::hypot(ri.sigma[j], rj.sigma[i]))
      << "between nets " << i << " and " << j;
}

// An L-shaped net around the end of a wire, which lies inside the L's
// bounding box. The L's surface is the union of its two grown boxes; the
// wire's is a single box, so reciprocity checks the union against it.
Geometry wrappedWire(const bool redundant)
{
  Geometry geometry;
  geometry.addNet("l");
  geometry.addNet("wire");
  geometry.addBox(0, {{0, 0, 0.5}, {2, 0.2, 0.7}});
  geometry.addBox(0, {{1.8, 0.2, 0.5}, {2, 1.5, 0.7}});
  if (redundant) {
    // The corner again, and a box inside the first: no new conductor.
    geometry.addBox(0, {{1.7, 0, 0.5}, {2, 0.4, 0.7}});
    geometry.addBox(0, {{0.5, 0.05, 0.55}, {1, 0.15, 0.65}});
  }
  geometry.addBox(1, {{0.5, 0.5, 0.5}, {1.5, 0.7, 0.7}});
  return geometry;
}

TEST(Solver, MultiBoxReciprocity)
{
  const Geometry geometry = wrappedWire(false);
  const Scene scene(geometry, 3.9);
  SolveOptions options;
  options.tol_total = 0.002;
  options.tol_coupling = 0.002;
  Solver solver({.threads = 8});
  expectReciprocal(solver.solve(scene, 0, options),
                   solver.solve(scene, 1, options));
}

// Boxes that add no conductor (overlapping or inside others) change the
// surface but not the capacitance.
TEST(Solver, RedundantBoxesChangeNothing)
{
  const Geometry plain = wrappedWire(false);
  const Geometry redundant = wrappedWire(true);
  SolveOptions options;
  options.tol_total = 0.002;
  options.tol_coupling = 0.002;
  Solver solver({.threads = 8});
  const CapRow a = solver.solve(Scene(plain, 3.9), 0, options);
  const CapRow b = solver.solve(Scene(redundant, 3.9), 0, options);
  EXPECT_NEAR(a.total(), b.total(), 4 * std::hypot(a.sigma[0], b.sigma[0]));
  EXPECT_NEAR(a.cap[1], b.cap[1], 4 * std::hypot(a.sigma[1], b.sigma[1]));
}

// A wire drawn in pieces, abutting and overlapping, merges back into one
// box before its surface is built, so its row is the single box's exactly,
// not just within the statistics. (A small box inside the wire would also
// merge away, but would shrink the scene's capture distance.)
TEST(Solver, FragmentedWireMatchesSingleBox)
{
  Geometry whole;
  whole.addNet("a");
  whole.addNet("b");
  whole.addBox(0, {{0, 0, 0.5}, {2, 0.25, 0.75}});
  whole.addBox(1, {{0, 0.5, 0.5}, {2, 0.75, 0.75}});
  Geometry pieces;
  pieces.addNet("a");
  pieces.addNet("b");
  pieces.addBox(0, {{0, 0, 0.5}, {0.5, 0.25, 0.75}});
  pieces.addBox(0, {{1.25, 0, 0.5}, {2, 0.25, 0.75}});
  pieces.addBox(0, {{0.5, 0, 0.5}, {1.25, 0.25, 0.75}});
  pieces.addBox(0, {{0.375, 0, 0.5}, {0.875, 0.25, 0.75}});
  pieces.addBox(1, {{0, 0.5, 0.5}, {2, 0.75, 0.75}});
  SolveOptions options;
  options.min_walks = options.max_walks = 65536;
  Solver solver({.threads = 4});
  const CapRow a = solver.solve(Scene(whole, 3.9), 0, options);
  const CapRow b = solver.solve(Scene(pieces, 3.9), 0, options);
  EXPECT_EQ(a.cap, b.cap);
  EXPECT_EQ(a.sigma, b.sigma);
  EXPECT_EQ(a.hops, b.hops);
}

// A via stack (M1 wire, via, M2 wire) with conformal dielectrics on both
// layers, between an M1 and an M2 neighbour.
TEST(Solver, ViaStackReciprocity)
{
  Geometry geometry;
  geometry.addNet("stack");
  geometry.addNet("m1");
  geometry.addNet("m2");
  geometry.addBox(0, {{0, 0, 0.5}, {1.5, 0.2, 0.7}});
  geometry.addBox(0, {{1.3, 0, 0.7}, {1.5, 0.2, 1.1}});
  geometry.addBox(0, {{1.3, 0, 1.1}, {3, 0.2, 1.3}});
  geometry.addBox(1, {{0, 0.4, 0.5}, {3, 0.6, 0.7}});
  geometry.addBox(2, {{0, -0.6, 1.1}, {3, -0.4, 1.3}});
  const Scene scene(geometry, conformalStack({{"C", 0.03, 0.02, 7.0}}));
  SolveOptions options;
  options.tol_total = 0.005;
  options.tol_coupling = 0.005;
  Solver solver({.threads = 8});
  const CapRow stack = solver.solve(scene, 0, options);
  expectReciprocal(stack, solver.solve(scene, 1, options));
  expectReciprocal(stack, solver.solve(scene, 2, options));
}

// A ring of four rectangles closed around another net: the ring's surface
// has an inner boundary, and every walk from the inner net ends on the ring.
TEST(Solver, RingAroundNet)
{
  Geometry2D section;
  section.addNet("ring");
  section.addNet("inner");
  section.addRect(0, {{0, 0.5}, {2, 0.7}});
  section.addRect(0, {{0, 2.3}, {2, 2.5}});
  section.addRect(0, {{0, 0.7}, {0.2, 2.3}});
  section.addRect(0, {{1.8, 0.7}, {2, 2.3}});
  section.addRect(1, {{0.8, 1.4}, {1.2, 1.6}});
  const Scene2D scene(section, 3.9);
  SolveOptions options;
  options.tol_total = 0.002;
  options.tol_coupling = 0.002;
  Solver solver({.threads = 8});
  const CapRow ring = solver.solve(scene, 0, options);
  const CapRow inner = solver.solve(scene, 1, options);
  EXPECT_EQ(inner.ground, 0);
  expectReciprocal(ring, inner);
}

TEST(Solver, RejectsConductorTouchingMaster)
{
  Geometry geometry;
  geometry.addNet("a");
  geometry.addNet("b");
  geometry.addBox(0, {{0, 0, 0.5}, {1, 0.2, 0.7}});
  geometry.addBox(0, {{0.8, 0.2, 0.5}, {1, 1, 0.7}});
  geometry.addBox(1, {{0, 0.2, 0.5}, {0.5, 0.4, 0.7}});
  Solver solver({.threads = 2});
  EXPECT_THROW(solver.solve(Scene(geometry, 3.9), 0), std::invalid_argument);
}

// A Gaussian surface on the conductors themselves would divide by a zero
// cube size.
TEST(Solver, RejectsNonPositiveGaussOffset)
{
  Geometry geometry;
  geometry.addNet("a");
  geometry.addBox(0, {{0, 0, 0.5}, {1, 1, 1.5}});
  const Scene scene(geometry);
  Solver solver({.threads = 2});
  for (const double offset : {0.0, -0.5, std::nan("")}) {
    SolveOptions options;
    options.gauss_offset = offset;
    EXPECT_THROW(solver.solve(scene, 0, options), std::invalid_argument)
        << offset;
  }
}

// Three parallel wires; `with_far` adds a fourth just outside the window
// WindowLeakageBoundsOutsideConductors uses, as a conductor it leaves out.
Geometry threeWires(const bool with_far)
{
  Geometry geometry;
  geometry.addNet("a");
  geometry.addNet("b");
  geometry.addNet("c");
  geometry.addBox(0, {{0, 0, 0.5}, {2, 0.2, 0.7}});
  geometry.addBox(1, {{0, 0.4, 0.5}, {2, 0.6, 0.7}});
  geometry.addBox(2, {{0, -0.4, 0.5}, {2, -0.2, 0.7}});
  if (with_far) {
    geometry.addNet("far");
    geometry.addBox(3, {{0, 1.65, 0.5}, {2, 1.85, 0.7}});
  }
  return geometry;
}

// A grounded net's coupling joins the ground entry; nothing else changes.
TEST(Solver, GroundedNetJoinsGround)
{
  const Geometry plain = threeWires(false);
  Geometry tied = threeWires(false);
  tied.setGrounded(1);
  SolveOptions options;
  options.min_walks = options.max_walks = 65536;
  Solver solver({.threads = 4});
  const CapRow a = solver.solve(Scene(plain, 3.9), 0, options);
  const CapRow b = solver.solve(Scene(tied, 3.9), 0, options);
  EXPECT_EQ(b.cap[0], a.cap[0]);
  EXPECT_EQ(b.cap[2], a.cap[2]);
  EXPECT_EQ(b.cap[1], 0);
  EXPECT_NEAR(b.ground, a.ground + a.cap[1], 1e-12);
  EXPECT_THROW(solver.solve(Scene(tied, 3.9), 1, options),
               std::invalid_argument);
}

// Adding a conductor just outside the window changes the total and the
// couplings to nets inside it by less than the leakage estimate.
TEST(Solver, WindowLeakageBoundsOutsideConductors)
{
  Geometry inside = threeWires(false);
  Geometry all = threeWires(true);
  const AlignedBox<2> window{{-1, -1}, {3, 1.6}};
  inside.setWindow(window);
  all.setWindow(window);
  SolveOptions options;
  options.min_walks = options.max_walks = 65536;
  Solver solver({.threads = 4});
  const CapRow a = solver.solve(Scene(inside, 3.9), 0, options);
  const CapRow b = solver.solve(Scene(all, 3.9), 0, options);
  EXPECT_GT(a.window_leakage, 0);
  EXPECT_LT(a.window_leakage, 0.5 * a.total());
  for (int net = 0; net < 3; net++) {
    EXPECT_LE(std::abs(b.cap[net] - a.cap[net]), a.window_leakage) << net;
  }
  // Without a window there is nothing to leak.
  EXPECT_EQ(
      solver.solve(Scene(threeWires(false), 3.9), 0, options).window_leakage,
      0);
}

// Conformal dielectrics in a stack without planar interfaces: the shells
// meet only the background, whose permittivity the tables must still know.
// A planar interface far above, with a negligible contrast, does not change
// the result.
TEST(Solver, ConformalWithoutPlanarInterfaces)
{
  const Geometry geometry = wiresAcrossEtchStop();
  Stack bare = planarStack({{1.0, 3.9}}, 3.9);
  bare.conductors.push_back({.name = "M1",
                             .bottom = 0.5,
                             .top = 0.7,
                             .conformals = {{"C", 0.05, 0.05, 7.0}}});
  Stack layered = bare;
  layered.dielectrics = {{.name = "D", .bottom = 0, .top = 20, .eps = 3.9}};
  layered.background_eps = 3.9 * (1 + 1e-9);
  const Scene bare_scene(geometry, bare);
  ASSERT_TRUE(bare_scene.interfaces().empty());
  SolveOptions options;
  options.tol_total = 0.003;
  Solver solver({.threads = 8});
  const CapRow a = solver.solve(bare_scene, 0, options);
  const CapRow b = solver.solve(Scene(geometry, layered), 0, options);
  EXPECT_NEAR(a.total(), b.total(), 4 * std::hypot(a.sigma[0], b.sigma[0]));
  const CapRow uniform = solver.solve(Scene(geometry, 3.9), 0, options);
  EXPECT_GT(a.total(),
            uniform.total() + 4 * std::hypot(a.sigma[0], uniform.sigma[0]));
}

// The cross section of the etch-stop wires in the x-z plane.
Geometry2D etchStopSection()
{
  Geometry2D section;
  section.addNet("lower");
  section.addNet("upper");
  section.addRect(0, {{0, 0.5}, {0.2, 0.7}});
  section.addRect(1, {{0.3, 1.1}, {0.5, 1.3}});
  return section;
}

// The same wires in 3D, running along y for the given length.
Geometry wiresAlongY(const double length)
{
  Geometry geometry;
  geometry.addNet("lower");
  geometry.addNet("upper");
  geometry.addBox(0, {{0, 0, 0.5}, {0.2, length, 0.7}});
  geometry.addBox(1, {{0.3, 0, 1.1}, {0.5, length, 1.3}});
  return geometry;
}

TEST(Solver, Geometry2DRejectsBadRects)
{
  Geometry2D section;
  section.addNet("a");
  EXPECT_THROW(section.addRect(1, {{0, 0.5}, {1, 0.7}}), std::invalid_argument);
  EXPECT_THROW(section.addRect(0, {{1, 0.5}, {1, 0.7}}), std::invalid_argument);
  EXPECT_THROW(section.addRect(0, {{0, 0}, {1, 0.7}}), std::invalid_argument);
}

// Rectangles of different nets may not overlap; those of one net may.
TEST(Solver, CrossSectionRejectsOverlap)
{
  Geometry2D section;
  section.addNet("a");
  section.addNet("b");
  section.addRect(0, {{0, 0.5}, {0.2, 0.7}});
  section.addRect(0, {{0.1, 0.5}, {0.3, 0.7}});
  EXPECT_NO_THROW(Scene2D(section, 3.9));
  section.addRect(1, {{0.25, 0.6}, {0.5, 0.8}});
  EXPECT_THROW(Scene2D(section, 3.9), std::invalid_argument);
}

// A cross section's Maxwell matrix is symmetric, and its results do not
// depend on the thread count.
TEST(Solver, CrossSectionReciprocityAndDeterminism)
{
  const Geometry2D section = etchStopSection();
  const Scene2D scene(
      section,
      conformalStack({{"C1a", 0.02, 0.0, 7.0}, {"C1b", 0.03, 0.03, 3.0}}));
  SolveOptions options;
  options.tol_total = 0.002;
  options.tol_coupling = 0.002;
  Solver solver({.threads = 8});
  const CapRow r0 = solver.solve(scene, 0, options);
  const CapRow r1 = solver.solve(scene, 1, options);
  EXPECT_NEAR(r0.cap[1], r1.cap[0], 4 * std::hypot(r0.sigma[1], r1.sigma[0]));

  SolveOptions fixed;
  fixed.min_walks = fixed.max_walks = 32768;
  const CapRow one = Solver({.threads = 1}).solve(scene, 0, fixed);
  const CapRow many = Solver({.threads = 6}).solve(scene, 0, fixed);
  EXPECT_EQ(one.cap, many.cap);
  EXPECT_EQ(one.hops, many.hops);
}

// The 2D mode agrees with long-line 3D per unit length: the difference of
// 3D solves on 10 and 30 um wires cancels their end effects. Within 0.5%
// plus three standard errors.
TEST(Solver, CrossSectionMatchesLongLine3D)
{
  const Stack stack = conformalStack({{"C", 0.05, 0.05, 7.0}});
  const Geometry2D section = etchStopSection();
  const Geometry ten = wiresAlongY(10.0);
  const Geometry thirty = wiresAlongY(30.0);
  SolveOptions options;
  options.tol_total = 0.002;
  options.tol_coupling = 0.002;
  Solver solver({.threads = 8});
  const CapRow cross = solver.solve(Scene2D(section, stack), 0, options);
  const CapRow a = solver.solve(Scene(ten, stack), 0, options);
  const CapRow b = solver.solve(Scene(thirty, stack), 0, options);
  const auto per_length
      = [&](const double va, const double vb) { return (vb - va) / 20; };
  const auto sigma = [&](const double sa, const double sb, const double s2) {
    return std::hypot(std::hypot(sa, sb) / 20, s2);
  };
  EXPECT_NEAR(cross.total(),
              per_length(a.total(), b.total()),
              0.005 * cross.total()
                  + 3 * sigma(a.sigma[0], b.sigma[0], cross.sigma[0]));
  EXPECT_NEAR(cross.cap[1],
              per_length(a.cap[1], b.cap[1]),
              0.005 * std::abs(cross.cap[1])
                  + 3 * sigma(a.sigma[1], b.sigma[1], cross.sigma[1]));
  EXPECT_NEAR(
      cross.ground,
      per_length(a.ground, b.ground),
      0.005 * std::abs(cross.ground)
          + 3 * sigma(a.ground_sigma, b.ground_sigma, cross.ground_sigma));
}

// Shells of the same conformal dielectric merge; shells of different ones
// may not overlap.
TEST(Solver, RejectsOverlappingConformals)
{
  Geometry geometry;
  geometry.addNet("a");
  geometry.addNet("b");
  geometry.addBox(0, {{0, 0, 0.5}, {2, 0.2, 0.7}});
  geometry.addBox(1, {{0, 0.3, 0.5}, {2, 0.5, 0.7}});
  Stack stack = etchStopStack();
  stack.conductors.push_back({.name = "M1",
                              .bottom = 0.5,
                              .top = 0.7,
                              .conformals = {{"C", 0.06, 0.02, 4.5}}});
  EXPECT_NO_THROW(Scene(geometry, stack));
  // Each wire's outer shell D reaches into the other's inner shell C.
  stack.conductors.back().conformals
      = {{"C", 0.04, 0.02, 4.5}, {"D", 0.03, 0.02, 3.5}};
  EXPECT_THROW(Scene(geometry, stack), std::invalid_argument);
}

}  // namespace
}  // namespace fsv
