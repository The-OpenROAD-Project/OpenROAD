// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <limits>
#include <stdexcept>

#include "gtest/gtest.h"
#include "pex/Model.h"

namespace pex {
namespace {

const LateralKey kKey{"m2", "m1", ""};

// A table whose ground is linear in the width and in the logarithms of the
// spacings, and whose couplings are exponential in the logarithm of their
// own spacing, with the open column apart. Both sides follow the same
// formula, as the table's mirror symmetry requires.
Model logLinearModel()
{
  Model model;
  LateralTable& t = model.addLateral(
      kKey, LateralTable({0.1, 0.2, 0.4, 0.8}, {0.1, 0.3, 0.9}));
  const int open = static_cast<int>(t.spacings().size());
  for (int w = 0; w < 4; w++) {
    for (int l = 0; l <= open; l++) {
      for (int r = 0; r <= open; r++) {
        LateralCap c;
        const double lw = t.widths()[w];
        const double ll = l == open ? 0 : std::log(t.spacings()[l]);
        const double lr = r == open ? 0 : std::log(t.spacings()[r]);
        c.ground = 1 + 0.5 * lw + 0.1 * (ll + lr);
        c.left = l == open ? 0 : std::exp(0.2 - 0.3 * ll) + 0.05 * lw;
        c.right = r == open ? 0 : std::exp(0.2 - 0.3 * lr) + 0.05 * lw;
        t.set(w, l, r, c);
      }
    }
  }
  return model;
}

TEST(Model, TableIndexingAndValidation)
{
  LateralTable t({1, 2}, {1, 2, 3});
  EXPECT_EQ(t.spacingCount(), 4);
  EXPECT_EQ(t.values().size(), 2u * 4 * 4);
  t.set(1, 3, 2, {0, 5, 6, 0, 0});
  EXPECT_EQ(t.values()[16 + 12 + 2].left, 5);
  EXPECT_EQ(t.at(1, 3, 2).left, 5);
  // The mirror point holds the same value with the sides swapped.
  EXPECT_EQ(t.at(1, 2, 3).left, 6);
  EXPECT_EQ(t.at(1, 2, 3).right, 5);
  // Grids must be ascending and positive; values finite and on the grid.
  EXPECT_THROW(LateralTable({}, {1}), std::invalid_argument);
  EXPECT_THROW(LateralTable({2, 1}, {1}), std::invalid_argument);
  EXPECT_THROW(LateralTable({1}, {0, 1}), std::invalid_argument);
  EXPECT_THROW(t.set(2, 0, 0, {}), std::invalid_argument);
  EXPECT_THROW(t.set(0, 4, 0, {}), std::invalid_argument);
  EXPECT_THROW(t.set(0, 0, 0, {std::numeric_limits<double>::infinity()}),
               std::invalid_argument);
  EXPECT_THROW(CrossingTable({1, 1}, {{}, {}}), std::invalid_argument);
  EXPECT_THROW(CrossingTable({1}, {}), std::invalid_argument);
  EXPECT_THROW(CrossingTable({1}, {{std::nan(""), 0}}), std::invalid_argument);
  EXPECT_THROW(DiagonalTable({1}, {}, {1}, {1}), std::invalid_argument);
  EXPECT_NO_THROW(DiagonalTable({1}, {-1, 0}, {1}, {1}));
  EXPECT_THROW(EndTable({}), std::invalid_argument);
  EXPECT_THROW(EndTable({1}, {0.2, 0.1}), std::invalid_argument);
  EXPECT_THROW(EndTable({1}, {}, {-1}), std::invalid_argument);
  EndTable e({1}, {0.1}, {0.2});
  EXPECT_EQ(e.values().size(), 1u * 2 * 2);
  EXPECT_THROW(e.set(0, 2, 0, {}), std::invalid_argument);
  EXPECT_THROW(e.set(0, 0, 0, {std::nan("")}), std::invalid_argument);
}

TEST(Model, ExactAtGridPoints)
{
  const Model model = logLinearModel();
  const LateralTable& t = *model.lateral(kKey);
  for (int w = 0; w < 4; w++) {
    for (int l = 0; l < 3; l++) {
      for (int r = 0; r < 3; r++) {
        const LateralCap v = model.lateral(
            kKey, t.widths()[w], t.spacings()[l], t.spacings()[r]);
        const LateralCap& e = t.at(w, l, r);
        EXPECT_DOUBLE_EQ(v.ground, e.ground);
        EXPECT_DOUBLE_EQ(v.left, e.left);
        EXPECT_DOUBLE_EQ(v.right, e.right);
      }
    }
  }
}

TEST(Model, InterpolatesWidthLinearlyAndSpacingsInTheLogarithms)
{
  const Model model = logLinearModel();
  // At a grid width, the spacings interpolate in their logarithms: the
  // ground linearly, a coupling geometrically along its own side.
  const double w = 0.2;
  const double l = std::sqrt(0.1 * 0.3);
  const double r = std::sqrt(0.3 * 0.9);
  const LateralCap v = model.lateral(kKey, w, l, r);
  EXPECT_NEAR(v.ground, 1 + 0.5 * w + 0.1 * (std::log(l) + std::log(r)), 1e-12);
  const double left_lo = std::exp(0.2 - 0.3 * std::log(0.1)) + 0.05 * w;
  const double left_hi = std::exp(0.2 - 0.3 * std::log(0.3)) + 0.05 * w;
  EXPECT_NEAR(v.left, std::sqrt(left_lo * left_hi), 1e-12);
  const double right_lo = std::exp(0.2 - 0.3 * std::log(0.3)) + 0.05 * w;
  const double right_hi = std::exp(0.2 - 0.3 * std::log(0.9)) + 0.05 * w;
  EXPECT_NEAR(v.right, std::sqrt(right_lo * right_hi), 1e-12);
  // At grid spacings, the width interpolates linearly.
  const LateralCap u = model.lateral(kKey, 0.3, 0.3, 0.9);
  EXPECT_NEAR(
      u.ground, 1 + 0.5 * 0.3 + 0.1 * (std::log(0.3) + std::log(0.9)), 1e-12);
  EXPECT_NEAR(u.left, std::exp(0.2 - 0.3 * std::log(0.3)) + 0.05 * 0.3, 1e-12);
}

TEST(Model, OpenSidesAndClampedWidth)
{
  const Model model = logLinearModel();
  const LateralTable& t = *model.lateral(kKey);
  const LateralCap open = model.lateral(kKey, 0.2, -1, 5);
  const LateralCap& e = t.at(1, 3, 3);
  EXPECT_DOUBLE_EQ(open.ground, e.ground);
  EXPECT_EQ(open.left, 0);
  EXPECT_EQ(open.right, 0);
  // Beyond the last spacing is open; the last spacing itself is not.
  EXPECT_EQ(model.lateral(kKey, 0.2, 0.9, 0.9).left, t.at(1, 2, 2).left);
  EXPECT_EQ(model.lateral(kKey, 0.2, 0.91, 0.9).left, 0);

  const LateralCap narrow = model.lateral(kKey, 0.01, 0.1, 0.1);
  EXPECT_DOUBLE_EQ(narrow.ground, t.at(0, 0, 0).ground);
  // Wider than the grid: the ground keeps growing with the width (here by
  // 0.5 per µm), the couplings stay at the widest wire's.
  const LateralCap wide = model.lateral(kKey, 10, 0.1, 0.1);
  EXPECT_NEAR(wide.ground, t.at(3, 0, 0).ground + 0.5 * (10 - 0.8), 1e-12);
  EXPECT_DOUBLE_EQ(wide.left, t.at(3, 0, 0).left);
}

TEST(Model, CrossingShareOverGaps)
{
  Model model;
  model.addCrossing({"m2", "m3", ""},
                    CrossingTable({1, 3}, {{0.8, 0.7}, {0.4, 0.3}}));
  const CrossingKey key{"m2", "m3", ""};
  EXPECT_EQ(model.crossing(key, 0).total, 1);
  EXPECT_EQ(model.crossing(key, 0).cross, 1);
  EXPECT_DOUBLE_EQ(model.crossing(key, 1).total, 0.8);
  EXPECT_DOUBLE_EQ(model.crossing(key, 3).cross, 0.3);
  // The kept amount gap × share is 0.8 at 1 and 1.2 at 3, linear between
  // (0, 0) and the points: 0.4 at 0.5 and 1.0 at 2.
  EXPECT_DOUBLE_EQ(model.crossingKept(key, 0.5).total, 0.4);
  EXPECT_DOUBLE_EQ(model.crossing(key, 0.5).total, 0.8);
  EXPECT_DOUBLE_EQ(model.crossingKept(key, 2).total, 1.0);
  EXPECT_DOUBLE_EQ(model.crossing(key, 2).total, 0.5);
  // Past the last gap the kept amount stays.
  EXPECT_DOUBLE_EQ(model.crossingKept(key, 6).total, 1.2);
  EXPECT_DOUBLE_EQ(model.crossing(key, 6).total, 0.2);
  // The one-sided share is the kept amount's slope: 0.8 up to the first
  // gap, 0.2 to the second, 0 beyond; under the crossing it is 1.
  EXPECT_EQ(model.crossingOneSided(key, 0).total, 1);
  EXPECT_DOUBLE_EQ(model.crossingOneSided(key, 0.5).total, 0.8);
  EXPECT_DOUBLE_EQ(model.crossingOneSided(key, 1).total, 0.8);
  EXPECT_DOUBLE_EQ(model.crossingOneSided(key, 2).total, 0.2);
  EXPECT_DOUBLE_EQ(model.crossingOneSided(key, 2.9).total, 0.2);
  EXPECT_EQ(model.crossingOneSided(key, 4).total, 0);
  // cross: 0.7 at 1 and 0.9 at 3, so slopes 0.7 and 0.1.
  EXPECT_DOUBLE_EQ(model.crossingOneSided(key, 0.5).cross, 0.7);
  EXPECT_DOUBLE_EQ(model.crossingOneSided(key, 2).cross, 0.1);
  // The two-sided share over a gap is the mean of the one-sided shares.
  for (const double gap : {0.3, 1.0, 1.7, 2.9, 3.0, 5.0}) {
    double sum = 0;
    const int n = 10000;
    for (int i = 0; i < n; i++) {
      sum += model.crossingOneSided(key, (i + 0.5) * gap / n).total;
    }
    EXPECT_NEAR(model.crossing(key, gap).total, sum / n, 1e-3);
  }
  EXPECT_THROW(model.crossing({"m2", "m4", ""}, 1), std::out_of_range);
}

// With neighbors, each side takes half from its spacing's column.
TEST(Model, CrossingShareWithNeighbors)
{
  EXPECT_THROW(CrossingTable({1}, {{0.5, 0.5}}, {0.1}), std::invalid_argument);
  Model model;
  const CrossingKey key{"m2", "m3", ""};
  // At 0.1 the share over a gap of 1 is 0.4, at 0.4 0.6, with no neighbor
  // 0.8.
  model.addCrossing(
      key,
      CrossingTable({1}, {{0.4, 0.3}, {0.6, 0.5}, {0.8, 0.7}}, {0.1, 0.4}));
  EXPECT_DOUBLE_EQ(model.crossing(key, 1).total, 0.8);
  EXPECT_DOUBLE_EQ(model.crossing(key, 1, 0.1, 0.1).total, 0.4);
  EXPECT_DOUBLE_EQ(model.crossing(key, 1, 0.1, -1).total, 0.6);
  // Halfway in the spacing's logarithm; past the last, none.
  EXPECT_DOUBLE_EQ(model.crossing(key, 1, 0.2, 0.2).cross, 0.4);
  EXPECT_DOUBLE_EQ(model.crossing(key, 1, 0.5, 0.5).total, 0.8);
  EXPECT_DOUBLE_EQ(model.crossingKept(key, 0.5, 0.4, 0.4).total, 0.3);
  EXPECT_DOUBLE_EQ(model.crossingOneSided(key, 0.5, 0.1, 0.4).total, 0.5);
}

// A table whose kept amount dips between gaps (noise at a saturated gap)
// never takes capacitance away: the share is held, not reduced.
TEST(Model, CrossingShareNeverNegative)
{
  Model model;
  model.addCrossing({"m2", "m3", ""},
                    CrossingTable({1, 2}, {{0.8, 0.7}, {0.3, 0.3}}));
  const CrossingKey key{"m2", "m3", ""};
  // Kept 0.8 at 1 but 0.6 at 2: the slope would be -0.2.
  EXPECT_EQ(model.crossingOneSided(key, 1.5).total, 0);
  EXPECT_DOUBLE_EQ(model.crossingKept(key, 2).total, 0.8);
  EXPECT_DOUBLE_EQ(model.crossing(key, 2).total, 0.4);
  for (double d = 0; d < 3; d += 0.01) {
    EXPECT_GE(model.crossingOneSided(key, d).total, 0);
    EXPECT_LE(model.crossingOneSided(key, d).cross, 1);
    EXPECT_GE(model.crossingKept(key, d).cross, 0);
  }
}

// A single crossing keeps as much of its effect as it runs past the
// wire, and the wire past it.
TEST(Model, CrossingReach)
{
  Model model;
  const CrossingKey key{"m2", "m3", ""};
  CrossingTable table({1, 3}, {{0.8, 0.7}, {0.4, 0.3}});
  model.addCrossing(key, table);
  EXPECT_EQ(model.crossingReach(key, 0, 1, 0, 0).total, 1);
  CrossingReach reach;
  reach.width = 0.2;
  reach.past = {0, 1};
  reach.extents = {0, 1, 4};
  reach.values
      = {{0.5, 0.4}, {0.8, 0.7}, {1, 1}, {0.3, 0.2}, {0.6, 0.5}, {1, 1}};
  table.setReach(reach);
  model.addCrossing(key, table);
  // Under the crossing: the ratio at no past, each side of the crossing
  // half; running on past the last extent keeps all.
  EXPECT_DOUBLE_EQ(model.crossingReach(key, 0, 0, 0, 0).total, 0.5);
  EXPECT_DOUBLE_EQ(model.crossingReach(key, 0, 0, 0, 10).total, 0.75);
  EXPECT_DOUBLE_EQ(model.crossingReach(key, 0, 0, 0.5, 0.5).cross, 0.55);
  EXPECT_EQ(model.crossingReach(key, 0, 0, 4, 5).cross, 1);
  // Beside it, the difference of the kept effect: (width + kept) / 2
  // times the ratio, 0.1 x 0.5 at 0 and 0.5 x 0.3 at 1 for the total,
  // over the 0.4 a crossing running on keeps.
  EXPECT_DOUBLE_EQ(model.crossingReach(key, 0, 1, 0, 0).total, 0.25);
  EXPECT_DOUBLE_EQ(model.crossingReach(key, 0, 1, 0, 0).cross,
                   (0.45 * 0.2 - 0.1 * 0.4) / 0.35);
  // Past the last gap nothing more is kept: the ratio there, held past
  // the last past.
  EXPECT_DOUBLE_EQ(model.crossingReach(key, 3, 5, 0, 0).total, 0.3);
  // A table with a plane opposite and no reach takes the reach of the
  // table without the plane.
  const CrossingKey planed{"m2", "m3", "m1"};
  model.addCrossing(planed, CrossingTable({1, 3}, {{0.8, 0.7}, {0.4, 0.3}}));
  EXPECT_DOUBLE_EQ(model.crossingReach(planed, 0, 0, 0, 0).total, 0.5);
  CrossingReach bad = reach;
  bad.past = {0.1, 1};
  EXPECT_THROW(table.setReach(bad), std::invalid_argument);
  bad = reach;
  bad.values.pop_back();
  EXPECT_THROW(table.setReach(bad), std::invalid_argument);
  bad = reach;
  bad.width = 0;
  EXPECT_THROW(table.setReach(bad), std::invalid_argument);
  bad.width = std::numeric_limits<double>::infinity();
  EXPECT_THROW(table.setReach(bad), std::invalid_argument);
}

TEST(Model, DiagonalLookup)
{
  Model model;
  DiagonalTable& t = model.addDiagonal(
      {"m2", "m1", ""},
      DiagonalTable({0.1, 1}, {-0.05, 0, 1}, {0.1, 0.4}, {0.2}));
  const int open = 2;
  for (int w = 0; w < 2; w++) {
    for (int g = 0; g < 3; g++) {
      for (int s = 0; s <= open; s++) {
        for (int f = 0; f <= 1; f++) {
          // Coupling falls with the gap, grows with the width, is
          // shielded by a near neighbor, and more by one on the far side.
          const double c = (3 - g) * (1 + w) * (s == open ? 1.0 : 0.5 + 0.2 * s)
                           * (f == 1 ? 1.0 : 0.8);
          t.set(w, g, s, f, {c, 1.2 * c, -0.1 * c});
        }
      }
    }
  }
  const DiagonalKey key{"m2", "m1", ""};
  EXPECT_DOUBLE_EQ(model.diagonal(key, 0.1, 0, 0.1, -1).coupling, 2 * 0.5);
  EXPECT_DOUBLE_EQ(model.diagonal(key, 0.1, 0, 0.1, 0.2).coupling,
                   2 * 0.5 * 0.8);
  EXPECT_DOUBLE_EQ(model.diagonal(key, 1, 1, -1, -1).coupling, 1 * 2);
  EXPECT_DOUBLE_EQ(model.diagonal(key, 1, 1, 5, 5).total_delta, 1.2 * 2);
  // Linear in the gap, in the logarithm of the width.
  EXPECT_DOUBLE_EQ(model.diagonal(key, 0.1, 0.5, -1, -1).coupling, 1.5);
  EXPECT_DOUBLE_EQ(model.diagonal(key, std::sqrt(0.1), 0, -1, -1).coupling,
                   2 * 1.5);
  // Past the last gap the coupling fades as 1 / gap.
  EXPECT_DOUBLE_EQ(model.diagonal(key, 0.1, 4, -1, -1).coupling, 0.25);
  EXPECT_DOUBLE_EQ(model.diagonal(key, 0.1, 4, -1, -1).lateral_delta, -0.025);
  // Before the first gap the values are the first point's.
  EXPECT_DOUBLE_EQ(model.diagonal(key, 0.1, -0.5, -1, -1).coupling, 3);
  EXPECT_THROW(model.diagonal({"m2", "m3", ""}, 0.1, 0, 0.1, 0.1),
               std::out_of_range);
}

// An end table with no neighbors and no facing wire.
EndTable freeEnds(const std::vector<double>& widths,
                  const std::vector<double>& totals,
                  const std::vector<double>& below = {},
                  const std::vector<double>& above = {})
{
  EndTable t(widths);
  for (size_t w = 0; w < widths.size(); w++) {
    t.set(w,
          0,
          0,
          {totals[w],
           below.empty() ? 0 : below[w],
           above.empty() ? 0 : above[w]});
  }
  return t;
}

TEST(Model, EndExcessLinearInWidth)
{
  Model model;
  model.addEnd({"m2", "m1", ""}, freeEnds({0.1, 0.3}, {0.02, 0.06}));
  const LateralKey key{"m2", "m1", ""};
  EXPECT_DOUBLE_EQ(model.endExcess(key, 0.1), 0.02);
  EXPECT_DOUBLE_EQ(model.endExcess(key, 0.2), 0.04);
  EXPECT_DOUBLE_EQ(model.endExcess(key, 0.05), 0.02);
  EXPECT_DOUBLE_EQ(model.endExcess(key, 1), 0.06);
  EXPECT_THROW(model.endExcess({"m2", "", ""}, 0.1), std::out_of_range);
  model.addEnd({"m2", "m1", "m3"},
               freeEnds({0.1, 0.3}, {0.02, 0.06}, {0.01, 0.03}, {0, 0.02}));
  const EndCap e = model.end({"m2", "m1", "m3"}, 0.2);
  EXPECT_DOUBLE_EQ(e.total, 0.04);
  EXPECT_DOUBLE_EQ(e.below, 0.02);
  EXPECT_DOUBLE_EQ(e.above, 0.01);
}

TEST(Model, EndNeighborsAndFacing)
{
  Model model;
  const LateralKey key{"m2", "", ""};
  EndTable& t = model.addEnd(key, EndTable({0.1}, {0.1, 0.4}, {0.2}));
  // [spacing][gap], the open spacing and no facing wire last.
  t.set(0, 0, 0, {0.06, 0, 0, 0.04, 0.02});
  t.set(0, 0, 1, {0.05, 0, 0, 0.04, 0});
  t.set(0, 1, 0, {0.05, 0, 0, 0.01, 0.03});
  t.set(0, 1, 1, {0.04, 0, 0, 0.01, 0});
  t.set(0, 2, 0, {0.045, 0, 0, 0, 0.035});
  t.set(0, 2, 1, {0.03, 0, 0, 0, 0});
  EXPECT_DOUBLE_EQ(model.end(key, 0.1).total, 0.03);
  // Each side takes half of the end with its spacing on both.
  const EndCap both = model.end(key, 0.1, 0.1, 0.1);
  EXPECT_DOUBLE_EQ(both.total, 0.05);
  EXPECT_DOUBLE_EQ(both.left, 0.02);
  EXPECT_DOUBLE_EQ(both.right, 0.02);
  const EndCap one = model.end(key, 0.1, 0.1, -1);
  EXPECT_DOUBLE_EQ(one.total, 0.04);
  EXPECT_DOUBLE_EQ(one.left, 0.02);
  EXPECT_DOUBLE_EQ(one.right, 0);
  // Halfway in the spacing's logarithm, the coupling geometric.
  const EndCap mid = model.end(key, 0.1, 0.2, 0.2);
  EXPECT_DOUBLE_EQ(mid.total, 0.045);
  EXPECT_NEAR(mid.left, 0.01, 1e-12);
  // Facing a wire; past the last gap, none.
  const EndCap faced = model.end(key, 0.1, -1, -1, 0.2);
  EXPECT_DOUBLE_EQ(faced.total, 0.045);
  EXPECT_DOUBLE_EQ(faced.facing, 0.035);
  EXPECT_DOUBLE_EQ(model.end(key, 0.1, -1, -1, 0.3).total, 0.03);
  EXPECT_DOUBLE_EQ(model.end(key, 0.1, 0.1, 0.1, 0.1).facing, 0.02);
}

TEST(Model, EndDifferentSides)
{
  Model model;
  const LateralKey key{"m2", "", ""};
  EndTable& t = model.addEnd(key, EndTable({0.1}, {0.1, 0.4}, {0.2}));
  t.set(0, 0, 0, {0.06, 0, 0, 0.04, 0.02});
  t.set(0, 0, 1, {0.05, 0, 0, 0.04, 0});
  t.set(0, 1, 0, {0.05, 0, 0, 0.01, 0.03});
  t.set(0, 1, 1, {0.04, 0, 0, 0.01, 0});
  t.set(0, 2, 0, {0.045, 0, 0, 0, 0.035});
  t.set(0, 2, 1, {0.03, 0, 0, 0, 0});
  EXPECT_FALSE(t.hasSides());
  t.setSides(0, 0, 1, {0.047, 0, 0, 0.03, 0.008});
  t.setSides(0, 0, 2, {0.045, 0.001, 0.002, 0.035, 0});
  t.setSides(0, 1, 2, {0.036, 0, 0, 0.009, 0});
  EXPECT_TRUE(t.hasSides());
  EXPECT_THROW(t.setSides(0, 1, 1, {}), std::invalid_argument);
  // A lone neighbor takes the held end, mirrored on the other side.
  const EndCap one = model.end(key, 0.1, 0.1, -1);
  EXPECT_DOUBLE_EQ(one.total, 0.045);
  EXPECT_DOUBLE_EQ(one.below, 0.001);
  EXPECT_DOUBLE_EQ(one.above, 0.002);
  EXPECT_DOUBLE_EQ(one.left, 0.035);
  EXPECT_DOUBLE_EQ(one.right, 0);
  const EndCap mirrored = model.end(key, 0.1, -1, 0.1);
  EXPECT_DOUBLE_EQ(mirrored.left, 0);
  EXPECT_DOUBLE_EQ(mirrored.right, 0.035);
  EXPECT_DOUBLE_EQ(model.end(key, 0.1, 0.4, 0.1).left, 0.008);
  // Like sides read the like-sided end.
  const EndCap both = model.end(key, 0.1, 0.1, 0.1);
  EXPECT_DOUBLE_EQ(both.total, 0.05);
  EXPECT_DOUBLE_EQ(both.left, 0.02);
  // A facing wire adds what it adds to the like-sided ends: the halves at
  // the gap (total 0.0525, left 0.02) less those with none (0.04, 0.02).
  const EndCap faced = model.end(key, 0.1, 0.1, -1, 0.2);
  EXPECT_DOUBLE_EQ(faced.total, 0.045 + 0.0525 - 0.04);
  EXPECT_DOUBLE_EQ(faced.left, 0.035);
  EXPECT_DOUBLE_EQ(faced.facing, 0.0275);
}

TEST(Model, Corners)
{
  Model model;
  const LateralKey key{"m2", "", ""};
  EndTable& t = model.addEnd(key, EndTable({0.1, 0.3}));
  EXPECT_FALSE(t.hasCorners());
  EXPECT_EQ(model.corners(key, 0.1).inner.total, 0);
  t.setCorners(0, {{-0.02, -0.004, 0}, {0.01, 0.002, 0}});
  t.setCorners(1, {{-0.04, -0.008, 0}, {0.03, 0.006, 0}});
  EXPECT_TRUE(t.hasCorners());
  const Corners mid = model.corners(key, 0.2);
  EXPECT_DOUBLE_EQ(mid.inner.total, -0.03);
  EXPECT_DOUBLE_EQ(mid.inner.below, -0.006);
  EXPECT_DOUBLE_EQ(mid.outer.total, 0.02);
  EXPECT_DOUBLE_EQ(model.corners(key, 1).outer.total, 0.03);
  EXPECT_THROW(t.setCorners(2, {}), std::invalid_argument);
  EXPECT_THROW(model.corners({"m3", "", ""}, 0.1), std::out_of_range);
}

TEST(Model, MissingContextThrows)
{
  const Model model = logLinearModel();
  EXPECT_THROW(model.lateral({"m3", "", ""}, 0.2, 0.1, 0.1), std::out_of_range);
  EXPECT_EQ(model.lateral({"m3", "", ""}), nullptr);
}

}  // namespace
}  // namespace pex
