// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"
#include "odb/geom.h"
#include "pex/Evaluator.h"
#include "pex/Model.h"
#include "pex/Patterns.h"

namespace pex {
namespace {

// A conductor of a hand-built stack, by its name and z extent.
fsv::Stack::Conductor conductorAt(const char* name,
                                  const double bottom,
                                  const double top)
{
  fsv::Stack::Conductor c;
  c.name = name;
  c.bottom = bottom;
  c.top = top;
  return c;
}

// Two layers, m1 at [1, 2] and m2 at [3, 4], a via between them.
fsv::Stack twoLayers()
{
  fsv::Stack stack;
  stack.addDielectric({.name = "d", .bottom = 0, .top = 10, .eps = 1});
  stack.addConductor(conductorAt("m1", 1, 2));
  stack.addConductor(conductorAt("m2", 3, 4));
  fsv::Stack::Via via;
  via.name = "v1";
  via.upper = stack.layer("m2");
  via.lower = stack.layer("m1");
  stack.addVia(via);
  return stack;
}

// Tables with distinct constants per context, so each piece's lookup is
// recognizable: ground = base, left = 1, right = 2, below = 3, above = 4,
// with base 10 for m1 alone, 20 under m2, 30 for m2 alone, 40 over m1.
Model constantModel()
{
  Model model;
  auto add = [&](const LateralKey& key, const double base) {
    LateralTable& t = model.addLateral(key, LateralTable({0.1, 1}, {0.1, 1}));
    const LateralCap c{
        .ground = base,
        .left = 1,
        .right = 2,
        .below = key.below.empty() ? 0.0 : 3.0,
        .above = key.above.empty() ? 0.0 : 4.0,
    };
    for (int w = 0; w < 2; w++) {
      for (int l = 0; l < 3; l++) {
        for (int r = 0; r < 3; r++) {
          t.set(w, l, r, c);
        }
      }
    }
    // An end adds a twentieth of the context's base, of which a fiftieth
    // goes to the plane below and a fortieth to the plane above. Neighbors
    // at 0.3 on both sides add 0.4 and take 0.6; a wire facing it at 0.2
    // adds 0.3 and takes all of it, at 0.6 0.1.
    const double below = key.below.empty() ? 0.0 : base / 50;
    const double above = key.above.empty() ? 0.0 : base / 40;
    EndTable& e = model.addEnd(key, EndTable({0.1, 1}, {0.3}, {0.2, 0.6}));
    for (int w = 0; w < 2; w++) {
      for (int s = 0; s < 2; s++) {
        for (int g = 0; g < 3; g++) {
          const double lateral = s == 0 ? 0.6 : 0.0;
          const double facing = std::array{0.3, 0.1, 0.0}[g];
          e.set(w,
                s,
                g,
                {
                    .total = (base / 20) + (lateral * 2 / 3) + facing,
                    .below = below,
                    .above = above,
                    .lateral = lateral,
                    .facing = facing,
                });
        }
      }
    }
  };
  add({.layer = "m1", .below = "", .above = ""}, 10);
  add({.layer = "m1", .below = "", .above = "m2"}, 20);
  add({.layer = "m2", .below = "", .above = ""}, 30);
  add({.layer = "m2", .below = "m1", .above = ""}, 40);
  DiagonalTable& d
      = model.addDiagonal({.layer = "m1", .other = "m2", .opposite = ""},
                          DiagonalTable({0.1, 1}, {0, 1}, {0.1, 1}, {0.1, 1}));
  for (int w = 0; w < 2; w++) {
    for (int g = 0; g < 2; g++) {
      for (int s = 0; s < 3; s++) {
        for (int f = 0; f < 3; f++) {
          d.set(w,
                g,
                s,
                f,
                {.coupling = 7, .total_delta = 8, .lateral_delta = -1});
        }
      }
    }
  }
  model.addCrossing({.layer = "m1", .cover = "m2", .opposite = ""},
                    CrossingTable({1, 3},
                                  {
                                      {.total = 0.8, .cross = 0.7},
                                      {.total = 0.4, .cross = 0.3},
                                  }));
  return model;
}

TEST(Evaluator, PiecesEndsAndCovers)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  const int b = g.addNet("b");
  const int c = g.addNet("c");
  // Wire a along x over [0, 10], width 0.2; b beside it over [2, 5] at a
  // gap of 0.3; c crosses above over [7, 8].
  g.addBox(a, {.lo = {0, 0, 1}, .hi = {10, 0.2, 2}});
  g.addBox(b, {.lo = {2, 0.5, 1}, .hi = {5, 0.7, 2}});
  g.addBox(c, {.lo = {7, -5, 3}, .hi = {8, 5, 4}});
  const NetEstimate est = estimate(g, stack, model, a);
  EXPECT_EQ(est.wires, 1);
  EXPECT_EQ(est.ends, 2);
  EXPECT_EQ(est.vias, 0);
  // Pieces: [0,2] [2,5] [5,7] [7,8] [8,10].
  EXPECT_EQ(est.pieces, 5);
  // b is on a's right (larger y): coupling 2 per µm over 3 µm.
  ASSERT_TRUE(est.couplings.contains(b));
  EXPECT_DOUBLE_EQ(est.couplings.at(b), 6);
  // Under c, the covered piece couples 4 per µm to it; the pieces beside
  // it take a share through the crossing table: over [5, 7] the distance
  // to c runs 2 down to 0, so gap × share integrates 0.8 x 1 + the
  // segment to 2: (0.8 + 1.2 x ...) — checked only for being positive and
  // below the full coupling.
  ASSERT_TRUE(est.couplings.contains(c));
  EXPECT_GT(est.couplings.at(c), 4);
  EXPECT_LT(est.couplings.at(c), 4 + 4 * 4);
  // The total counts every piece's table total plus the two ends.
  EXPECT_GT(est.total(), 10 * 10 + 1);
  EXPECT_GT(est.ground, 10 * 10 - 4 * 4);
}

// A crossing just past the wire's end takes a share of the end piece and
// of the end, which a crossing beyond it does not.
TEST(Evaluator, CrossingPastTheEnd)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  const int c = g.addNet("c");
  const int d = g.addNet("d");
  g.addBox(a, {.lo = {0, 0, 1}, .hi = {10, 0.2, 2}});
  g.addBox(c, {.lo = {10.5, -5, 3}, .hi = {11, 5, 4}});
  g.addBox(d, {.lo = {12, -5, 3}, .hi = {12.5, 5, 4}});
  const NetEstimate est = estimate(g, stack, model, a);
  EXPECT_EQ(est.pieces, 1);
  ASSERT_TRUE(est.couplings.contains(c));
  EXPECT_GT(est.couplings.at(c), 0);
  EXPECT_LT(est.couplings.at(c), 4 * 3);
  EXPECT_FALSE(est.couplings.contains(d));
}

// A cover starting within rounding of the wire's start leaves a zero-length
// first piece; the start's end excess still takes the covered context.
TEST(Evaluator, EndContextSurvivesAZeroLengthFirstPiece)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  const int c = g.addNet("c");
  g.addBox(a, {.lo = {0, 0, 1}, .hi = {10, 0.2, 2}});
  g.addBox(c, {.lo = {1e-10, -5, 3}, .hi = {10, 5, 4}});
  const NetEstimate est = estimate(g, stack, model, a);
  EXPECT_EQ(est.pieces, 1);
  EXPECT_EQ(est.ends, 2);
  // Under m2 the base is 20 per µm, the plane takes 4, and each end adds
  // 1 in that context; c reaches no further than the wire, so the ends'
  // part to the plane stays ground.
  EXPECT_NEAR(est.couplings.at(c), 40, 1e-6);
  EXPECT_NEAR(est.ground, 200 + 2 * 1.0, 1e-6);
}

// Under the net's own cover the plane's parts, the wire's and the ends',
// are no capacitance.
TEST(Evaluator, OwnCoverTakesNothing)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  g.addBox(a, {.lo = {0, 0, 1}, .hi = {10, 0.2, 2}});
  g.addBox(a, {.lo = {-1, -5, 3}, .hi = {11, 5, 4}});
  const NetEstimate est = estimate(g, stack, model, a, {.diagonal_depth = 0});
  EXPECT_TRUE(est.couplings.empty());
  // m1 under m2: a ground of 20 per µm, its plane's 4 dropped, and per
  // end 1 less the plane's 0.5. m2, 10 wide, is not covered by the narrow
  // m1: a ground of 30 per µm over 12 µm and 1.5 per end.
  EXPECT_EQ(est.ends, 4);
  EXPECT_NEAR(est.ground, 10 * 20 + 2 * 0.5 + 12 * 30 + 2 * 1.5, 1e-6);
}

// At a bend the boxes overlap in a corner square, which the longer box
// counts; the corner has no line end.
TEST(Evaluator, BendCountsTheCornerOnce)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  g.addBox(a, {.lo = {0, 0, 1}, .hi = {10, 0.2, 2}});
  g.addBox(a, {.lo = {9.8, 0, 1}, .hi = {10, 5, 2}});
  const NetEstimate est = estimate(g, stack, model, a);
  EXPECT_EQ(est.pieces, 2);
  EXPECT_EQ(est.ends, 2);
  EXPECT_EQ(est.bends, 1);
  // 10 per µm over 10 µm and 4.8 µm, and 0.5 per free end.
  EXPECT_NEAR(est.ground, (10 * 10) + (10 * 4.8) + 1, 1e-9);
}

// Each junction adds its corners once: a bend, overlapping or flush, an
// inner and an outer corner, a T two inner corners, a cross four.
TEST(Evaluator, JunctionCorners)
{
  const fsv::Stack stack = twoLayers();
  const Model plain = constantModel();
  Model model = plain;
  EndTable& e = model.addEnd(
      {.layer = "m1", .below = "", .above = ""},
      EndTable(*plain.end({.layer = "m1", .below = "", .above = ""})));
  for (int w = 0; w < 2; w++) {
    e.setCorners(w,
                 {
                     .inner = {.total = -0.3, .below = 0, .above = 0},
                     .outer = {.total = 0.2, .below = 0, .above = 0},
                 });
  }
  auto added = [&](const std::vector<fsv::Box>& boxes) {
    fsv::Geometry g;
    const int a = g.addNet("a");
    for (const fsv::Box& b : boxes) {
      g.addBox(a, b);
    }
    return estimate(g, stack, model, a).total()
           - estimate(g, stack, plain, a).total();
  };
  const fsv::Box wire{.lo = {0, 0, 1}, .hi = {10, 0.2, 2}};
  EXPECT_NEAR(added({wire, {{9.8, 0, 1}, {10, 5, 2}}}), -0.1, 1e-9);
  EXPECT_NEAR(added({wire, {{9.8, 0.2, 1}, {10, 5, 2}}}), -0.1, 1e-9);
  EXPECT_NEAR(added({wire, {{4.9, 0, 1}, {5.1, 5, 2}}}), -0.6, 1e-9);
  EXPECT_NEAR(added({wire, {{4.9, -5, 1}, {5.1, 5, 2}}}), -1.2, 1e-9);
  EXPECT_NEAR(added({wire}), 0, 1e-9);
}

// A line end facing another net's wire takes the facing table's total and
// coupling, by the share of the face the wire overlaps.
TEST(Evaluator, FacingEnd)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  for (const double overlap : {1.0, 0.5}) {
    fsv::Geometry g;
    const int a = g.addNet("a");
    const int f = g.addNet("f");
    g.addBox(a, {.lo = {0, 0, 1}, .hi = {10, 0.2, 2}});
    g.addBox(f, {.lo = {10.2, 0.2 - (0.2 * overlap), 1}, .hi = {12, 0.2, 2}});
    const NetEstimate est = estimate(g, stack, model, a);
    EXPECT_EQ(est.ends, 2);
    EXPECT_EQ(est.facing_ends, 1);
    // The free end adds 0.5 (m1 alone); the facing one blends 0.8 with
    // the free 0.5 by the overlap, 0.3 of it to f.
    const double faced = (overlap * 0.8) + ((1 - overlap) * 0.5);
    EXPECT_NEAR(est.couplings.at(f), overlap * 0.3, 1e-9);
    EXPECT_NEAR(est.ground, 100 + 0.5 + faced - (overlap * 0.3), 1e-9);
  }
  // Past the widest gap the end is free.
  fsv::Geometry g;
  const int a = g.addNet("a");
  const int f = g.addNet("f");
  g.addBox(a, {.lo = {0, 0, 1}, .hi = {10, 0.2, 2}});
  g.addBox(f, {.lo = {10.7, 0, 1}, .hi = {12, 0.2, 2}});
  const NetEstimate est = estimate(g, stack, model, a);
  EXPECT_EQ(est.facing_ends, 0);
  EXPECT_FALSE(est.couplings.contains(f));
  EXPECT_NEAR(est.ground, 100 + 1, 1e-9);
}

// A neighbor running past a line end takes part of the end, as far as it
// runs past against its spacing.
TEST(Evaluator, NeighborRunsPastTheEnd)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  // Past by 1 (all), 0.15 (half of the 0.3 spacing) and 0 (none).
  for (const auto& [past, share] :
       {std::pair{1.0, 1.0}, std::pair{0.15, 0.5}, std::pair{0.0, 0.0}}) {
    fsv::Geometry g;
    const int a = g.addNet("a");
    const int b = g.addNet("b");
    g.addBox(a, {.lo = {0, 0, 1}, .hi = {10, 0.2, 2}});
    g.addBox(b, {.lo = {-past, 0.5, 1}, .hi = {10 + past, 0.7, 2}});
    const NetEstimate est = estimate(g, stack, model, a, {.diagonal_depth = 0});
    // 2 per µm on the right, and per end half of the 0.6 the neighbors
    // take, of half of the 0.4 they add.
    EXPECT_NEAR(est.couplings.at(b), 20 + (2 * share * 0.3), 1e-9);
    EXPECT_NEAR(est.ground, 100 + (2 * (0.5 + (share * (0.2 - 0.3)))), 1e-9);
  }
}

// A crossing between the wire and a farther plane keeps its coupling: the
// plane stands for the uncovered rest of the pieces beside it.
TEST(Evaluator, CrossingInFrontOfAPlane)
{
  fsv::Stack stack;
  stack.addDielectric({.name = "d", .bottom = 0, .top = 10, .eps = 1});
  stack.addConductor(conductorAt("m1", 1, 2));
  stack.addConductor(conductorAt("m2", 3, 4));
  stack.addConductor(conductorAt("m3", 5, 6));
  Model model = constantModel();
  const Model& base = model;
  LateralTable& t
      = model.addLateral({.layer = "m1", .below = "", .above = "m3"},
                         LateralTable({0.1, 1}, {0.1, 1}));
  for (int w = 0; w < 2; w++) {
    for (int l = 0; l < 3; l++) {
      for (int r = 0; r < 3; r++) {
        t.set(w,
              l,
              r,
              {.ground = 15, .left = 1, .right = 2, .below = 0, .above = 2});
      }
    }
  }
  model.addEnd({.layer = "m1", .below = "", .above = "m3"},
               EndTable(*base.end({.layer = "m1", .below = "", .above = ""})));
  auto couplings = [&](const bool plane) {
    fsv::Geometry g;
    const int a = g.addNet("a");
    const int c = g.addNet("c");
    const int p = g.addNet("p");
    g.addBox(a, {.lo = {0, 0, 1}, .hi = {10, 0.2, 2}});
    g.addBox(c, {.lo = {4, -5, 3}, .hi = {5, 5, 4}});
    if (plane) {
      g.addBox(p, {.lo = {-20, -20, 5}, .hi = {30, 20, 6}});
    }
    const NetEstimate est = estimate(g, stack, model, a, {.diagonal_depth = 0});
    const auto at = [&](const int n) {
      return est.couplings.contains(n) ? est.couplings.at(n) : 0.0;
    };
    return std::pair{at(c), at(p)};
  };
  const auto [alone, none] = couplings(false);
  const auto [crossing, plane] = couplings(true);
  EXPECT_EQ(none, 0);
  // Under the crossing 4 per µm, beside it a share.
  EXPECT_GT(alone, 4);
  EXPECT_NEAR(crossing, alone, 1e-9);
  // The plane takes 2 per µm where the crossing does not cover, less than
  // over the whole 10 µm.
  EXPECT_GT(plane, 0);
  EXPECT_LT(plane, 2 * 10);
}

// A parallel wire takes from ground in proportion to what the piece's
// cover on that side leaves of it: under an m3 plane whose ground is half
// the open one, the m2 wire beside takes half its open part.
TEST(Evaluator, ParallelWireUnderACover)
{
  fsv::Stack stack;
  stack.addDielectric({.name = "d", .bottom = 0, .top = 10, .eps = 1});
  stack.addConductor(conductorAt("m1", 1, 2));
  stack.addConductor(conductorAt("m2", 3, 4));
  stack.addConductor(conductorAt("m3", 5, 6));
  auto ground = [&](const double covered_ground) {
    Model model = constantModel();
    LateralTable& t
        = model.addLateral({.layer = "m1", .below = "", .above = "m3"},
                           LateralTable({0.1, 1}, {0.1, 1}));
    for (int w = 0; w < 2; w++) {
      for (int l = 0; l < 3; l++) {
        for (int r = 0; r < 3; r++) {
          t.set(w,
                l,
                r,
                {
                    .ground = covered_ground,
                    .left = 0,
                    .right = 0,
                    .below = 0,
                    .above = 1,
                });
        }
      }
    }
    model.addEnd(
        {.layer = "m1", .below = "", .above = "m3"},
        EndTable(*model.end({.layer = "m1", .below = "", .above = ""})));
    fsv::Geometry g;
    const int a = g.addNet("a");
    const int d = g.addNet("d");
    const int p = g.addNet("p");
    g.addBox(a, {.lo = {0, 0, 1}, .hi = {4, 0.2, 2}});
    g.addBox(d, {.lo = {-1, 0.4, 3}, .hi = {5, 0.6, 4}});
    g.addBox(p, {.lo = {-20, -20, 5}, .hi = {30, 20, 6}});
    return estimate(g, stack, model, a, {.diagonal_depth = 1}).ground;
  };
  // The diagonal takes 8 - 7 + 1 = 2 per µm from ground in the open; with
  // the covered ground 5 against the open 10 it takes 1. The pieces' own
  // ground differs by 5 per µm; over 4 µm, 24 in all.
  EXPECT_NEAR(ground(10) - ground(5), (4 * 5) + (4 * 1), 1e-9);
}

// Between two crossings of different nets, each takes the coupling its
// own crossing keeps over the piece.
TEST(Evaluator, CrossingCouplingSplitsByNet)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  const int b = g.addNet("b");
  const int x = g.addNet("x");
  const int y = g.addNet("y");
  g.addBox(a, {.lo = {0, 0, 1}, .hi = {10, 0.2, 2}});
  g.addBox(x, {.lo = {2, -5, 3}, .hi = {3, 5, 4}});
  g.addBox(y, {.lo = {8, -5, 3}, .hi = {9, 5, 4}});
  // A neighbor over [3, 4] cuts the gap between the crossings unevenly.
  g.addBox(b, {.lo = {3, 0.5, 1}, .hi = {4, 0.7, 2}});
  const NetEstimate est = estimate(g, stack, model, a);
  // The crossing table's kept coupling, d × share, is 0.7 at 1, 0.9 at 3
  // and beyond, and linear between: 0.8 at 2. The plane couples 4 per µm.
  // Over [0, 2], x keeps 0.8 one-sided, so 1.6; over [4, 8], x (1 to 4
  // away) keeps 0.2 and y (0 to 4 away) 0.9, of the 2.2 the piece couples
  // to the crossings. The ends take the plane's 0.5 at the share kept at
  // the end itself: the kept coupling grows 0.1 per µm at 2 from x and
  // 0.7 per µm at 1 from y, half of it on one side.
  EXPECT_NEAR(
      est.couplings.at(x), 1.6 + 4 + 1.4 + 2.2 * 0.2 / 1.1 + 0.05 * 0.5, 1e-9);
  EXPECT_NEAR(
      est.couplings.at(y), 2.2 * 0.9 / 1.1 + 4 + 1.4 + 0.35 * 0.5, 1e-9);
  EXPECT_DOUBLE_EQ(est.couplings.at(b), 2);  // on the right: 2 per µm
}

TEST(Evaluator, DiagonalWires)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  const int b = g.addNet("b");
  const int d = g.addNet("d");
  // Wire a along x over [0, 4]; b beside it on its layer over [0, 4];
  // d on m2 beside it (not over it) over [1, 3], beyond b.
  g.addBox(a, {.lo = {0, 0, 1}, .hi = {4, 0.2, 2}});
  g.addBox(b, {.lo = {0, 0.5, 1}, .hi = {4, 0.7, 2}});
  g.addBox(d, {.lo = {1, 0.9, 3}, .hi = {3, 1.1, 4}});
  const NetEstimate est = estimate(g, stack, model, a);
  EXPECT_EQ(est.pieces, 3);
  // d couples 7 per µm over its 2 µm, and takes 1 per µm from b there.
  ASSERT_TRUE(est.couplings.contains(d));
  EXPECT_DOUBLE_EQ(est.couplings.at(d), 14);
  EXPECT_DOUBLE_EQ(est.couplings.at(b), 2 * 4 - 2);
  // The rest of d's total delta (8 - 7 + 1 = 2 per µm) lands on the ground.
  EXPECT_DOUBLE_EQ(est.ground, 10 * 4 + 2 * 2 + 2 * 0.5);

  // The same wire of the net's own on m2 shields like any other but adds
  // no coupling: the m1 wire's 40 + 4 of shielding + 1 of ends, plus the
  // m2 wire's own 60 + 3 of ends.
  fsv::Geometry own;
  const int n = own.addNet("n");
  own.addBox(n, {.lo = {0, 0, 1}, .hi = {4, 0.2, 2}});
  own.addBox(n, {.lo = {1, 0.9, 3}, .hi = {3, 1.1, 4}});
  const NetEstimate self = estimate(own, stack, model, n);
  EXPECT_TRUE(self.couplings.empty());
  EXPECT_DOUBLE_EQ(self.ground, 40 + 4 + 1 + 60 + 3);
}

TEST(Evaluator, ContinuationsAndVias)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  // An L: a wire along x then along y, with a via to m2 at the far end.
  g.addBox(a, {.lo = {0, 0, 1}, .hi = {5, 0.2, 2}});
  g.addBox(a, {.lo = {4.8, 0, 1}, .hi = {5, 6, 2}});
  g.addBox(a, {.lo = {4.8, 5.8, 2}, .hi = {5, 6, 3}});
  g.addBox(a, {.lo = {4.8, 5.8, 3}, .hi = {5, 6, 4}});
  const NetEstimate est = estimate(g, stack, model, a);
  EXPECT_EQ(est.wires, 3);
  EXPECT_EQ(est.vias, 1);
  // The corner joins the two wires: one line end each at the free ends,
  // and the m2 pad has two.
  EXPECT_EQ(est.ends, 4);
  EXPECT_TRUE(est.couplings.empty());
  EXPECT_GT(est.ground, 0);

  fsv::Geometry bad;
  const int z = bad.addNet("z");
  bad.addBox(z, {.lo = {0, 0, 2.5}, .hi = {1, 1, 2.7}});
  EXPECT_THROW(estimate(bad, stack, model, z), std::invalid_argument);
}

}  // namespace
}  // namespace pex

namespace pex {
namespace {

// Boxes past a layer's reaches change nothing: the result from only the
// boxes within the radius equals the one from every box, bit for bit, and
// each part names the boxes it joins.
TEST(Evaluator, RadiusAndAttribution)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  const Evaluator evaluator(stack, model, 1000);
  ASSERT_EQ(evaluator.layerCount(), 2);
  EXPECT_EQ(evaluator.layerName(0), "m1");
  // The farthest of the lateral spacing (1), the crossing gap (3), the
  // facing gap (0.6) and the diagonal gap (1), in database units.
  EXPECT_EQ(evaluator.radius(0), 3000);
  enum : NetIndex
  {
    kA,
    kB,
    kC,
    kX,
    kZ,
    kD,
    kE,
  };
  const std::vector<EvalBox> boxes{
      // The target, along x.
      {.net = kA, .layer = 0, .rect = odb::Rect(0, 0, 4000, 200)},
      // A neighbor 0.3 away.
      {.net = kB, .layer = 0, .rect = odb::Rect(0, 500, 4000, 700)},
      // Past the lateral reach.
      {.net = kC, .layer = 0, .rect = odb::Rect(0, 2200, 4000, 2400)},
      // A crossing over the wire.
      {.net = kX, .layer = 1, .rect = odb::Rect(2000, -5000, 3000, 5000)},
      // A crossing 5 past the end.
      {.net = kZ, .layer = 1, .rect = odb::Rect(9000, -5000, 10000, 5000)},
      // A parallel wire 0.5 beside.
      {.net = kD, .layer = 1, .rect = odb::Rect(0, -700, 4000, -500)},
      // One past the diagonal reach.
      {.net = kE, .layer = 1, .rect = odb::Rect(0, -2700, 4000, -2500)},
  };
  auto all = [&](int, std::vector<int>& out) {
    for (int i = 0; std::cmp_less(i, boxes.size()); i++) {
      out.push_back(i);
    }
  };
  // Within the radius of the target box in x-y.
  auto near = [&](const int box, std::vector<int>& out) {
    const odb::Rect reach
        = boxes[box].rect.bloat(evaluator.radius(boxes[box].layer));
    for (int i = 0; std::cmp_less(i, boxes.size()); i++) {
      if (boxes[i].rect.intersects(reach)) {
        out.push_back(i);
      }
    }
  };
  std::vector<Contribution> from_all;
  std::vector<Contribution> from_near;
  evaluator.evaluate(boxes, {kA}, all, from_all);
  evaluator.evaluate(boxes, {kA}, near, from_near);
  ASSERT_EQ(from_all.size(), from_near.size());
  for (size_t i = 0; i < from_all.size(); i++) {
    EXPECT_EQ(from_all[i].box, from_near[i].box);
    EXPECT_EQ(from_all[i].other, from_near[i].other);
    EXPECT_EQ(from_all[i].source, from_near[i].source);
    EXPECT_EQ(from_all[i].ground, from_near[i].ground);
    EXPECT_EQ(from_all[i].value, from_near[i].value);
    EXPECT_EQ(from_all[i].at, from_near[i].at);
  }
  std::map<int, double> by_box;
  for (const Contribution& c : from_all) {
    EXPECT_EQ(c.box, kA);
    EXPECT_GE(c.at, 0);
    EXPECT_LE(c.at, 4000);
    if (c.other >= 0) {
      by_box[c.other] += c.value;
    }
  }
  EXPECT_GT(by_box[kB], 0);
  EXPECT_GT(by_box[kX], 0);
  EXPECT_GT(by_box[kD], 0);
  EXPECT_FALSE(by_box.contains(kC));
  EXPECT_FALSE(by_box.contains(kZ));
  EXPECT_FALSE(by_box.contains(kE));
}

}  // namespace
}  // namespace pex
