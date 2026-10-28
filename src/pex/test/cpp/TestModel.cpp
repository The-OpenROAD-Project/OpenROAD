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
  EXPECT_THROW(EndTable({1, 2}, {0.1}), std::invalid_argument);
  EXPECT_THROW(EndTable({1}, {std::nan("")}), std::invalid_argument);
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
  model.addCrossing({"m2", "m3"},
                    CrossingTable({1, 3}, {{0.8, 0.7}, {0.4, 0.3}}));
  const CrossingKey key{"m2", "m3"};
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
  EXPECT_THROW(model.crossing({"m2", "m4"}, 1), std::out_of_range);
}

// A table whose kept amount dips between gaps (noise at a saturated gap)
// never takes capacitance away: the share is held, not reduced.
TEST(Model, CrossingShareNeverNegative)
{
  Model model;
  model.addCrossing({"m2", "m3"},
                    CrossingTable({1, 2}, {{0.8, 0.7}, {0.3, 0.3}}));
  const CrossingKey key{"m2", "m3"};
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

TEST(Model, DiagonalLookup)
{
  Model model;
  DiagonalTable& t = model.addDiagonal(
      {"m2", "m1"}, DiagonalTable({0.1, 1}, {-0.05, 0, 1}, {0.1, 0.4}, {0.2}));
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
  const DiagonalKey key{"m2", "m1"};
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
  EXPECT_THROW(model.diagonal({"m2", "m3"}, 0.1, 0, 0.1, 0.1),
               std::out_of_range);
}

TEST(Model, EndExcessLinearInWidth)
{
  Model model;
  model.addEnd({"m2", "m1", ""}, EndTable({0.1, 0.3}, {0.02, 0.06}));
  const LateralKey key{"m2", "m1", ""};
  EXPECT_DOUBLE_EQ(model.endExcess(key, 0.1), 0.02);
  EXPECT_DOUBLE_EQ(model.endExcess(key, 0.2), 0.04);
  EXPECT_DOUBLE_EQ(model.endExcess(key, 0.05), 0.02);
  EXPECT_DOUBLE_EQ(model.endExcess(key, 1), 0.06);
  EXPECT_THROW(model.endExcess({"m2", "", ""}, 0.1), std::out_of_range);
}

TEST(Model, MissingContextThrows)
{
  const Model model = logLinearModel();
  EXPECT_THROW(model.lateral({"m3", "", ""}, 0.2, 0.1, 0.1), std::out_of_range);
  EXPECT_EQ(model.lateral({"m3", "", ""}), nullptr);
}

}  // namespace
}  // namespace pex
