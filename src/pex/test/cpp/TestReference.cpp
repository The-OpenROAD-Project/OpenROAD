// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <stdexcept>

#include "Reference.h"
#include "fsv/Geometry.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"
#include "pex/Model.h"

namespace pex {
namespace {

// Two layers, m1 at [1, 2] and m2 at [3, 4], a via between them.
fsv::Stack twoLayers()
{
  fsv::Stack stack;
  stack.dielectrics = {{"d", 0, 10, 1}};
  stack.conductors.push_back(
      {.name = "m1", .bottom = 1, .top = 2, .conformals = {}});
  stack.conductors.push_back(
      {.name = "m2", .bottom = 3, .top = 4, .conformals = {}});
  stack.vias.push_back({.name = "v1", .upper = "m2", .lower = "m1"});
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
    const LateralCap c{base,
                       1,
                       2,
                       key.below.empty() ? 0.0 : 3.0,
                       key.above.empty() ? 0.0 : 4.0};
    for (int w = 0; w < 2; w++) {
      for (int l = 0; l < 3; l++) {
        for (int r = 0; r < 3; r++) {
          t.set(w, l, r, c);
        }
      }
    }
    // An end adds a twentieth of the context's base.
    model.addEnd(key, EndTable({0.1, 1}, {base / 20, base / 20}));
  };
  add({"m1", "", ""}, 10);
  add({"m1", "", "m2"}, 20);
  add({"m2", "", ""}, 30);
  add({"m2", "m1", ""}, 40);
  DiagonalTable& d = model.addDiagonal(
      {"m1", "m2"}, DiagonalTable({0.1, 1}, {0, 1}, {0.1, 1}, {0.1, 1}));
  for (int w = 0; w < 2; w++) {
    for (int g = 0; g < 2; g++) {
      for (int s = 0; s < 3; s++) {
        for (int f = 0; f < 3; f++) {
          d.set(w, g, s, f, {7, 8, -1});
        }
      }
    }
  }
  model.addCrossing({"m1", "m2"},
                    CrossingTable({1, 3}, {{0.8, 0.7}, {0.4, 0.3}}));
  return model;
}

TEST(Reference, PiecesEndsAndCovers)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  const int b = g.addNet("b");
  const int c = g.addNet("c");
  // Wire a along x over [0, 10], width 0.2; b beside it over [2, 5] at a
  // gap of 0.3; c crosses above over [7, 8].
  g.addBox(a, {{0, 0, 1}, {10, 0.2, 2}});
  g.addBox(b, {{2, 0.5, 1}, {5, 0.7, 2}});
  g.addBox(c, {{7, -5, 3}, {8, 5, 4}});
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

// A cover starting within rounding of the wire's start leaves a zero-length
// first piece; the start's end excess still takes the covered context.
TEST(Reference, EndContextSurvivesAZeroLengthFirstPiece)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  const int c = g.addNet("c");
  g.addBox(a, {{0, 0, 1}, {10, 0.2, 2}});
  g.addBox(c, {{1e-10, -5, 3}, {10, 5, 4}});
  const NetEstimate est = estimate(g, stack, model, a);
  EXPECT_EQ(est.pieces, 1);
  EXPECT_EQ(est.ends, 2);
  // Under m2 the base is 20 per µm, the plane takes 4, and each end adds
  // 1 in that context.
  EXPECT_NEAR(est.couplings.at(c), 40, 1e-6);
  EXPECT_NEAR(est.ground, 200 + 2 * 1.0, 1e-6);
}

// Between two crossings of different nets, each takes the coupling its
// own crossing keeps over the piece.
TEST(Reference, CrossingCouplingSplitsByNet)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  const int b = g.addNet("b");
  const int x = g.addNet("x");
  const int y = g.addNet("y");
  g.addBox(a, {{0, 0, 1}, {10, 0.2, 2}});
  g.addBox(x, {{2, -5, 3}, {3, 5, 4}});
  g.addBox(y, {{8, -5, 3}, {9, 5, 4}});
  // A neighbor over [3, 4] cuts the gap between the crossings unevenly.
  g.addBox(b, {{3, 0.5, 1}, {4, 0.7, 2}});
  const NetEstimate est = estimate(g, stack, model, a);
  // The crossing table's kept coupling, d × share, is 0.7 at 1, 0.9 at 3
  // and beyond, and linear between: 0.8 at 2. The plane couples 4 per µm.
  // Over [0, 2], x keeps 0.8 one-sided, so 1.6; over [4, 8], x (1 to 4
  // away) keeps 0.2 and y (0 to 4 away) 0.9, of the 2.2 the piece couples
  // to the crossings.
  EXPECT_NEAR(est.couplings.at(x), 1.6 + 4 + 1.4 + 2.2 * 0.2 / 1.1, 1e-9);
  EXPECT_NEAR(est.couplings.at(y), 2.2 * 0.9 / 1.1 + 4 + 1.4, 1e-9);
  EXPECT_DOUBLE_EQ(est.couplings.at(b), 2);  // on the right: 2 per µm
}

TEST(Reference, DiagonalWires)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  const int b = g.addNet("b");
  const int d = g.addNet("d");
  // Wire a along x over [0, 4]; b beside it on its layer over [0, 4];
  // d on m2 beside it (not over it) over [1, 3], beyond b.
  g.addBox(a, {{0, 0, 1}, {4, 0.2, 2}});
  g.addBox(b, {{0, 0.5, 1}, {4, 0.7, 2}});
  g.addBox(d, {{1, 0.9, 3}, {3, 1.1, 4}});
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
  own.addBox(n, {{0, 0, 1}, {4, 0.2, 2}});
  own.addBox(n, {{1, 0.9, 3}, {3, 1.1, 4}});
  const NetEstimate self = estimate(own, stack, model, n);
  EXPECT_TRUE(self.couplings.empty());
  EXPECT_DOUBLE_EQ(self.ground, 40 + 4 + 1 + 60 + 3);
}

TEST(Reference, ContinuationsAndVias)
{
  const fsv::Stack stack = twoLayers();
  const Model model = constantModel();
  fsv::Geometry g;
  const int a = g.addNet("a");
  // An L: a wire along x then along y, with a via to m2 at the far end.
  g.addBox(a, {{0, 0, 1}, {5, 0.2, 2}});
  g.addBox(a, {{4.8, 0, 1}, {5, 6, 2}});
  g.addBox(a, {{4.8, 5.8, 2}, {5, 6, 3}});
  g.addBox(a, {{4.8, 5.8, 3}, {5, 6, 4}});
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
  bad.addBox(z, {{0, 0, 2.5}, {1, 1, 2.7}});
  EXPECT_THROW(estimate(bad, stack, model, z), std::invalid_argument);
}

}  // namespace
}  // namespace pex
