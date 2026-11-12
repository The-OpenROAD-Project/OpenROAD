// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Itf.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"
#include "pex/Model.h"
#include "pex/ModelBuilder.h"
#include "pex/ModelJson.h"
#include "pex/Patterns.h"

namespace pex {
namespace {

const fsv::Stack& sky130()
{
  static const fsv::Stack stack = fsv::readItfFile("src/fsv/test/sky130.itf");
  return stack;
}

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

// A sky130 layer by name; no layer for a name the stack lacks.
Layer L(const std::string& name)
{
  return sky130().layer(name);
}

// The names of layers, "" for none, for comparisons.
std::vector<std::string> names(const std::vector<Layer>& layers,
                               const fsv::Stack& stack = sky130())
{
  std::vector<std::string> out;
  for (const Layer layer : layers) {
    out.push_back(layer.valid() ? stack.name(layer) : "");
  }
  return out;
}

BuildOptions smallBuild(const int threads)
{
  BuildOptions o;
  o.width_factors = {1, 3};
  o.spacing_factors = {1, 2};
  o.below_depth = 1;
  o.above_depth = 0;
  o.far_cover_factors = {};  // the far ladder has its own test
  o.cover_tolerance = 0;     // and so do the probes
  o.tol = 0.04;
  o.threads = threads;
  o.extent = 3;
  o.crossing_spacing_factors = {};  // the crossing test builds them
  o.reach_factors = {0, 2};         // with a coarse reach
  o.end_spacing_factors = {};       // and the end test
  o.facing_factors = {};
  o.lateral_reach = 0;  // the grids' reach has its own test too
  o.lateral_reach_one_sided = 0;
  o.diagonal_reach = 0;
  return o;
}

TEST(ModelBuilder, TablesAreSymmetricAndThreadIndependent)
{
  Model one;
  int calls = 0;
  BuildOptions options = smallBuild(1);
  options.progress = [&](int done, int total) {
    calls++;
    EXPECT_EQ(done, calls);
    EXPECT_EQ(total, 2 * 2 * 6);
  };
  buildLateral(sky130(), L("met2"), options, one);
  EXPECT_EQ(calls, 24);
  ASSERT_EQ(one.laterals().size(), 2u);
  EXPECT_NE(one.lateral({"met2", "", ""}), nullptr);
  const LateralTable* table = one.lateral({"met2", "met1", ""});
  ASSERT_NE(table, nullptr);
  ASSERT_EQ(table->widths().size(), 2u);
  EXPECT_DOUBLE_EQ(table->widths()[1], 0.42);
  ASSERT_EQ(table->spacings().size(), 2u);
  EXPECT_DOUBLE_EQ(table->spacings()[1], 0.28);
  for (int w = 0; w < 2; w++) {
    for (int l = 0; l < 3; l++) {
      for (int r = 0; r < 3; r++) {
        const LateralCap& c = table->at(w, l, r);
        const LateralCap& m = table->at(w, r, l);
        // The plane below shields nearly all of the ground, so what is
        // left of it is within the noise.
        EXPECT_GT(c.ground, -0.01 * c.total());
        EXPECT_GT(c.below, 0);
        EXPECT_EQ(c.above, 0);
        EXPECT_EQ(c.left, m.right);
        EXPECT_EQ(c.ground, m.ground);
        EXPECT_EQ(c.below, m.below);
        EXPECT_EQ(c.left == 0, l == 2);
        EXPECT_EQ(c.right == 0, r == 2);
      }
    }
  }
  // Closer neighbors couple more and shield the ground.
  EXPECT_GT(table->at(0, 0, 0).left, table->at(0, 1, 0).left);
  EXPECT_LT(table->at(0, 0, 0).rest(), table->at(0, 2, 2).rest());

  Model three;
  BuildOptions threaded = smallBuild(3);
  int last = 0;
  threaded.progress = [&](const int done, const int total) {
    // Monotonic even with several threads.
    EXPECT_EQ(done, last + 1);
    EXPECT_EQ(total, 24);
    last = done;
  };
  buildLateral(sky130(), L("met2"), threaded, three);
  EXPECT_EQ(last, 24);
  const LateralTable* again = three.lateral({"met2", "met1", ""});
  ASSERT_NE(again, nullptr);
  for (size_t i = 0; i < table->values().size(); i++) {
    EXPECT_EQ(table->values()[i].ground, again->values()[i].ground);
    EXPECT_EQ(table->values()[i].left, again->values()[i].left);
  }
}

// The kept form: a share of 1 at no gap, L / g far out; a fit recovers the
// length from shares 5% off either way.
TEST(ModelBuilder, KeptFit)
{
  EXPECT_EQ(keptShare(0.9, 0), 1);
  EXPECT_NEAR(keptShare(0.9, 90), 0.01, 1e-6);
  const std::vector<double> gaps{0.05, 0.1, 0.2, 0.3, 0.5, 0.9, 1.5, 3, 8};
  std::vector<double> shares;
  for (size_t i = 0; i < gaps.size(); i++) {
    const double noise = (i % 2 == 0 ? 1 : -1) * 0.05;
    shares.push_back(keptShare(0.9, gaps[i]) * (1 + noise));
  }
  const KeptFit fit = fitKept(gaps, shares);
  EXPECT_NEAR(fit.length, 0.9, 0.05);
  EXPECT_LT(fit.residual, kKeptFitTolerance * fit.length);
  EXPECT_THROW(fitKept({1, 2}, {0.5}), std::invalid_argument);
}

TEST(ModelBuilder, CrossingAndEndTables)
{
  BuildOptions o = smallBuild(4);
  // Two width factors with coverages whose gaps coincide (1 × 3 and
  // 3 × 1 minimum widths) merge into one point.
  o.crossing_width_factors = {1, 3};
  o.coverages = {0.25, 0.5};
  o.crossing_length = 2;
  o.crossing_length_long = 4;
  o.end_length = 1;
  o.width_factors = {1};
  o.tol = 0.06;
  Model model;
  // A coverage of 0 or 1 has no gap to tabulate; refused before any solve.
  for (const double bad : {0.0, 1.0}) {
    BuildOptions full = o;
    full.coverages = {bad};
    EXPECT_THROW(buildCrossing(sky130(), L("met2"), L("met3"), full, model),
                 std::invalid_argument);
  }
  o.crossing_spacing_factors = {1};
  buildCrossing(sky130(), L("met2"), L("met3"), o, model);
  const CrossingTable* c = model.crossing({"met2", "met3", ""});
  ASSERT_NE(c, nullptr);
  ASSERT_EQ(c->spacings().size(), 1u);
  // A table built on another spacing grid is refused.
  BuildOptions other = o;
  other.crossing_spacing_factors = {2};
  EXPECT_THROW(buildCrossing(sky130(), L("met2"), L("met3"), other, model),
               std::invalid_argument);
  // Gaps are wc (1 / f - 1): 0.3, 0.9 (twice, merged) and 2.7 µm.
  ASSERT_EQ(c->gaps().size(), 3u);
  EXPECT_NEAR(c->gaps()[0], 0.3, 1e-9);
  EXPECT_NEAR(c->gaps()[1], 0.9, 1e-9);
  EXPECT_NEAR(c->gaps()[2], 2.7, 1e-9);
  // Crossings keep a share of the plane's effect, less over wider gaps.
  EXPECT_GT(c->at(1, 0).total, 0.5);
  EXPECT_LT(c->at(1, 0).total, 1.0);
  EXPECT_GT(c->at(1, 0).total, c->at(1, 1).total);
  EXPECT_GT(c->at(1, 1).cross, 0);
  EXPECT_THROW(buildCrossing(sky130(), L("met2"), L("met2"), o, model),
               std::invalid_argument);
  // One crossing ending at the wire's sides keeps less of the coupling
  // than one running past them; one running on keeps all of it.
  ASSERT_TRUE(c->hasReach());
  const CrossingReach& reach = c->reach();
  EXPECT_EQ(reach.past, (std::vector<double>{0, 0.6}));
  ASSERT_EQ(reach.extents.size(), 3u);
  EXPECT_DOUBLE_EQ(reach.extents[2], 3);
  EXPECT_DOUBLE_EQ(reach.width, 0.3);
  EXPECT_LT(reach.at(1, 0).cross, reach.at(1, 1).cross);
  EXPECT_GT(reach.at(1, 0).cross, 0.2);
  EXPECT_LT(reach.at(1, 1).cross, 1);
  EXPECT_EQ(reach.at(1, 2).cross, 1);
  EXPECT_EQ(reach.at(0, 2).total, 1);
  // With a plane below the wire, the crossings above have a table of
  // their own on the same gaps, with no reach of its own.
  buildCrossings(
      sky130(), L("met2"), {L("met3")}, o, model, L("met1"), Layer());
  const CrossingTable* planed = model.crossing({"met2", "met3", "met1"});
  ASSERT_NE(planed, nullptr);
  EXPECT_EQ(planed->gaps(), c->gaps());
  EXPECT_FALSE(planed->hasReach());
  // A table held without a reach gains one and keeps its shares.
  const double share = c->at(1, 0).total;
  model.addCrossing({"met2", "met3", ""},
                    CrossingTable(c->gaps(), c->values(), c->spacings()));
  ASSERT_FALSE(model.crossing({"met2", "met3", ""})->hasReach());
  buildCrossing(sky130(), L("met2"), L("met3"), o, model);
  c = model.crossing({"met2", "met3", ""});
  EXPECT_TRUE(c->hasReach());
  EXPECT_EQ(c->at(1, 0).total, share);
  // A reach held on another grid is refused, and reach factors that do
  // not ascend from 0 are refused before any solve.
  BuildOptions other_reach = o;
  other_reach.reach_factors = {0, 3};
  EXPECT_THROW(
      buildCrossing(sky130(), L("met2"), L("met3"), other_reach, model),
      std::invalid_argument);
  for (const std::vector<double>& bad :
       {std::vector<double>{}, {1, 2}, {0, 0, 2}, {0, 3, 2}}) {
    BuildOptions refused = o;
    refused.reach_factors = bad;
    Model fresh;
    EXPECT_THROW(buildCrossing(sky130(), L("met2"), L("met3"), refused, fresh),
                 std::invalid_argument);
  }

  o.end_spacing_factors = {1};
  o.facing_factors = {1, 4};
  buildEnds(sky130(), L("met2"), o, model);
  EXPECT_EQ(model.ends().size(), 2u);
  const EndTable* e = model.end({"met2", "met1", ""});
  ASSERT_NE(e, nullptr);
  ASSERT_EQ(e->values().size(), 1u * 2 * 3);
  EXPECT_DOUBLE_EQ(e->spacings()[0], 0.14);
  EXPECT_DOUBLE_EQ(e->gaps()[1], 0.56);
  // A free end of a minimum-width wire adds a few hundredths of a fF,
  // part of it to the plane below, none above.
  const EndValue& free = e->at(0, 1, 2);
  EXPECT_GT(free.total, 0.01);
  EXPECT_LT(free.total, 0.1);
  EXPECT_GT(free.below, 0);
  EXPECT_LT(free.below, free.total);
  EXPECT_EQ(free.above, 0);
  EXPECT_EQ(free.lateral, 0);
  EXPECT_EQ(free.facing, 0);
  // Neighbors running past the end take part of it.
  EXPECT_GT(e->at(0, 0, 2).lateral, 0);
  // A facing wire takes more across a narrower gap, and less with
  // neighbors beside the end.
  EXPECT_GT(e->at(0, 1, 0).facing, e->at(0, 1, 1).facing);
  EXPECT_GT(e->at(0, 1, 1).facing, 0);
  EXPECT_LT(e->at(0, 1, 0).facing, e->at(0, 1, 0).total);
  EXPECT_GT(e->at(0, 1, 0).total, free.total);
  EXPECT_LT(e->at(0, 0, 0).facing, e->at(0, 1, 0).facing);
  // A neighbor on one side alone takes part of the end and leaves less
  // to the plane.
  ASSERT_TRUE(e->hasSides());
  const EndSides one = e->sides(0, 0, 1);
  EXPECT_GT(one.left, 0);
  EXPECT_EQ(one.right, 0);
  EXPECT_LT(one.below, (e->at(0, 0, 2).below + free.below) / 2);
  EXPECT_EQ(e->sides(0, 1, 0).right, one.left);
  // Where two wires meet, each shields the other's side.
  ASSERT_TRUE(e->hasCorners());
  EXPECT_LT(e->corners(0).inner.total, 0);
  // Built tables stay; one on another grid is refused.
  buildEnds(sky130(), L("met2"), o, model);
  EXPECT_EQ(model.end({"met2", "met1", ""}), e);
  // A table held without the different-sided ends gains them and keeps
  // its own values.
  EndTable bare(e->widths(), e->spacings(), e->gaps());
  for (int si = 0; si < bare.spacingCount(); si++) {
    for (int gi = 0; gi < bare.gapCount(); gi++) {
      bare.set(0, si, gi, e->at(0, si, gi));
    }
  }
  const EndValue kept = e->at(0, 0, 2);
  model.addEnd({"met2", "met1", ""}, std::move(bare));
  ASSERT_FALSE(model.end({"met2", "met1", ""})->hasSides());
  buildEnds(sky130(), L("met2"), o, model);
  e = model.end({"met2", "met1", ""});
  EXPECT_TRUE(e->hasSides());
  EXPECT_TRUE(e->hasCorners());
  EXPECT_EQ(e->at(0, 0, 2).total, kept.total);
  o.facing_factors = {1};
  EXPECT_THROW(buildEnds(sky130(), L("met2"), o, model), std::invalid_argument);
}

// Past the last spacing the lateral grid reaches a multiple of the
// distance to the nearest plane, further with a cover on one side only; the
// diagonal gaps reach a multiple of the distance between the layers.
TEST(ModelBuilder, GridsReachByHeight)
{
  BuildOptions o = smallBuild(4);
  o.width_factors = {1};
  o.spacing_factors = {1};
  o.tol = 0.05;
  o.lateral_reach = 6;
  o.lateral_reach_one_sided = 10;
  o.lateral_reach_step = 1.6;
  Model model;
  buildLateral(sky130(), L("met2"), o, model);
  const fsv::Stack::Conductor& met1 = sky130().conductor(L("met1"));
  const fsv::Stack::Conductor& met2 = sky130().conductor(L("met2"));
  auto check = [&](const LateralKey& key, const double reach) {
    const LateralTable* t = model.lateral(key);
    ASSERT_NE(t, nullptr);
    const std::vector<double>& s = t->spacings();
    ASSERT_GT(s.size(), 2u);
    EXPECT_DOUBLE_EQ(s.front(), met2.min_spacing);
    EXPECT_NEAR(s.back(), reach, 1e-9);
    for (size_t i = 1; i < s.size(); i++) {
      EXPECT_LE(s[i] / s[i - 1], 1.6 + 1e-9);
    }
  };
  check({"met2", "", ""}, 6 * met2.bottom);
  check({"met2", "met1", ""}, 10 * (met2.bottom - met1.top));
  // A step of 1 or less could not reach; refused.
  for (const double step : {1.0, 0.5}) {
    BuildOptions flat = o;
    flat.lateral_reach_step = step;
    Model fresh;
    EXPECT_THROW(buildLateral(sky130(), L("met2"), flat, fresh),
                 std::invalid_argument);
  }

  o.diagonal_width_factors = {1};
  o.diagonal_gap_factors = {0, 2};
  o.diagonal_spacing_factors = {1};
  o.diagonal_far_spacing_factors = {1};
  o.diagonal_reach = 4;
  buildDiagonal(sky130(), L("met2"), L("met1"), o, model);
  const DiagonalTable* d = model.diagonal({"met2", "met1", ""});
  ASSERT_NE(d, nullptr);
  ASSERT_GT(d->gaps().size(), 2u);
  EXPECT_NEAR(d->gaps().back(), 4 * (met2.bottom - met1.top), 1e-9);
  for (size_t i = 2; i < d->gaps().size(); i++) {
    EXPECT_LE(d->gaps()[i] / d->gaps()[i - 1], 2 + 1e-9);
  }
}

TEST(ModelBuilder, DiagonalTable)
{
  BuildOptions o = smallBuild(4);
  o.diagonal_width_factors = {1};
  o.diagonal_gap_factors = {0, 2};
  o.diagonal_spacing_factors = {1};
  o.diagonal_far_spacing_factors = {1};
  o.tol = 0.04;
  Model model;
  buildDiagonal(sky130(), L("met2"), L("met1"), o, model);
  const DiagonalTable* t = model.diagonal({"met2", "met1", ""});
  ASSERT_NE(t, nullptr);
  ASSERT_EQ(t->values().size(), 1u * 2 * 2 * 2);
  // Alone, a met1 wire beside the met2 wire couples more at the near gap
  // and adds a good part of it to the total; a neighbor at minimum spacing
  // on its side shields it and loses some of its own coupling; one on the
  // far side shields it too.
  const DiagonalCap& near_open = t->at(0, 0, 1, 1);
  const DiagonalCap& far_open = t->at(0, 1, 1, 1);
  const DiagonalCap& near_shielded = t->at(0, 0, 0, 1);
  const DiagonalCap& far_shielded = t->at(0, 0, 1, 0);
  EXPECT_GT(near_open.coupling, far_open.coupling);
  EXPECT_GT(near_open.total_delta, 0.25 * near_open.coupling);
  EXPECT_EQ(near_open.lateral_delta, 0);
  EXPECT_LT(near_shielded.coupling, near_open.coupling);
  EXPECT_LT(near_shielded.lateral_delta, 0);
  EXPECT_LT(far_shielded.coupling, near_open.coupling);
  EXPECT_EQ(far_shielded.lateral_delta, 0);
  // A plane above the met2 wire, on its other side from the met1 wire,
  // takes part of the coupling; a plane on the met1 wire's side is
  // refused.
  buildDiagonal(sky130(), L("met2"), L("met1"), o, model, L("met3"));
  const DiagonalTable* planed = model.diagonal({"met2", "met1", "met3"});
  ASSERT_NE(planed, nullptr);
  EXPECT_LT(planed->at(0, 0, 1, 1).coupling, 0.95 * near_open.coupling);
  EXPECT_GT(planed->at(0, 0, 1, 1).coupling, 0);
  // The parallel wire takes part of the plane's coupling; the tables with
  // no plane have none to take.
  EXPECT_LT(planed->at(0, 0, 1, 1).plane_delta, 0);
  EXPECT_EQ(near_open.plane_delta, 0);
  EXPECT_THROW(
      buildDiagonal(sky130(), L("met2"), L("met1"), o, model, L("li1")),
      std::invalid_argument);
  EXPECT_EQ(names(diagonalLayers(sky130(), L("met2"), o)),
            (std::vector<std::string>{"met1", "li1", "met3", "met4"}));
  // Only the technology's routing layers take part when they are given.
  BuildOptions routing = o;
  routing.layers = {"met1", "met2", "met3"};
  routing.above_depth = 1;
  EXPECT_EQ(names(diagonalLayers(sky130(), L("met2"), routing)),
            (std::vector<std::string>{"met1", "met3"}));
  EXPECT_EQ(names(coversBelow(sky130(), L("met1"), routing)),
            (std::vector<std::string>{""}));
  EXPECT_EQ(names(coversAbove(sky130(), L("met2"), routing)),
            (std::vector<std::string>{"", "met3"}));
  EXPECT_NE(buildFingerprint(sky130(), routing), buildFingerprint(sky130(), o));
  // Past the depth, the far factors pick layers by distance, and the
  // farthest always joins: met4 is the first past it and met5 the
  // farthest, so both are tabulated whatever the factors, each once.
  routing.layers = {"met1", "met2", "met3", "met4", "met5"};
  routing.far_cover_factors = {1, 2, 4};
  EXPECT_EQ(names(coversAbove(sky130(), L("met2"), routing)),
            (std::vector<std::string>{"", "met3", "met4", "met5"}));
  routing.far_cover_factors = {1};
  EXPECT_EQ(names(coversAbove(sky130(), L("met2"), routing)),
            (std::vector<std::string>{"", "met3", "met4", "met5"}));
  routing.far_cover_factors = {4};
  EXPECT_EQ(names(coversAbove(sky130(), L("met2"), routing)),
            (std::vector<std::string>{"", "met3", "met5"}));
  // Below met2 with met1 in the depth there is nothing far.
  EXPECT_EQ(names(coversBelow(sky130(), L("met2"), routing)),
            (std::vector<std::string>{"", "met1"}));
  EXPECT_EQ(names(coversAbove(sky130(), L("met5"), routing)),
            (std::vector<std::string>{""}));
  routing.far_cover_factors = {};
  EXPECT_EQ(names(coversAbove(sky130(), L("met2"), routing)),
            (std::vector<std::string>{"", "met3"}));
  routing.far_cover_factors = {1, 0};
  EXPECT_THROW(coversAbove(sky130(), L("met2"), routing),
               std::invalid_argument);
  routing.far_cover_factors = {};
  EXPECT_NE(buildFingerprint(sky130(), routing), buildFingerprint(sky130(), o));
}

// A table the model holds is not built again, so a build resumes from a
// saved model; a far layer gets its lateral, end and crossing tables like
// one within the depth.
TEST(ModelBuilder, ResumesAndBuildsFarLayers)
{
  BuildOptions o = smallBuild(4);
  o.above_depth = 0;
  o.below_depth = 0;
  o.far_cover_factors = {1};
  o.layers = {"met1", "met2", "met3"};
  o.crossing_width_factors = {1};
  o.coverages = {0.5};
  o.crossing_length = 2;
  o.crossing_length_long = 4;
  o.end_length = 1;
  o.width_factors = {1};
  o.diagonal_depth = 0;
  o.tol = 0.06;
  int solves = 0;
  o.progress = [&](int, int) { solves++; };
  Model model;
  buildLayer(sky130(), L("met2"), o, model);
  EXPECT_GT(solves, 0);
  // The depth is 0 each way, so met1 and met3 are far layers, the only
  // covers each side.
  EXPECT_NE(model.lateral({"met2", "met1", "met3"}), nullptr);
  EXPECT_NE(model.lateral({"met2", "", "met3"}), nullptr);
  EXPECT_NE(model.end({"met2", "met1", ""}), nullptr);
  EXPECT_NE(model.crossing({"met2", "met3", ""}), nullptr);
  EXPECT_NE(model.crossing({"met2", "met1", ""}), nullptr);
  solves = 0;
  buildLayer(sky130(), L("met2"), o, model);
  EXPECT_EQ(solves, 0);
  // A model saved and read back resumes too, though the file rounds its
  // grids (1.5 × 0.14 is 0.21000000000000002 in memory, 0.21 in the file).
  BuildOptions grid = o;
  grid.width_factors = {1, 1.5};
  Model fresh;
  buildLateral(sky130(), L("met2"), grid, fresh);
  buildEnds(sky130(), L("met2"), grid, fresh);
  std::stringstream file;
  writeModel(fresh, {}, file);
  Model saved = readModel(file);
  solves = 0;
  EXPECT_NO_THROW(buildLateral(sky130(), L("met2"), grid, saved));
  EXPECT_NO_THROW(buildEnds(sky130(), L("met2"), grid, saved));
  EXPECT_EQ(solves, 0);
  // A model built on another grid is not resumed; its tables came from
  // another build.
  BuildOptions wider = o;
  wider.width_factors = {1, 2};
  EXPECT_THROW(buildLateral(sky130(), L("met2"), wider, model),
               std::invalid_argument);
  EXPECT_THROW(buildEnds(sky130(), L("met2"), wider, model),
               std::invalid_argument);
  EXPECT_EQ(solves, 0);
}

// A curve in candidate order, with every candidate's error measured
// against 1.
ProbeCurve curveOf(const std::vector<double>& values)
{
  return {values, std::vector<double>(values.size(), 1)};
}

TEST(ModelBuilder, PickCoversKeepsOnlyWhatInterpolationNeeds)
{
  // Distances 1, 2, 4, 8, 16: equal steps in the logarithm.
  const std::vector<double> d{1, 2, 4, 8, 16};
  // Linear in the logarithm: the ends predict everything.
  EXPECT_EQ(pickCovers(d, {curveOf({5, 4, 3, 2, 1})}, 0.001),
            (std::vector<int>{0, 4}));
  // A kink at 4 needs it as a pick; a second curve straight there adds
  // nothing.
  EXPECT_EQ(pickCovers(
                d, {curveOf({5, 4, 3, 3, 3}), curveOf({1, 1, 1, 1, 1})}, 0.001),
            (std::vector<int>{0, 2, 4}));
  // The error is relative to the scale: a bend of 0.5 passes at a
  // tolerance of 0.01 when the scale is 100.
  EXPECT_EQ(pickCovers(d, {{{5, 4, 3, 3, 3}, {100, 100, 100, 100, 100}}}, 0.01),
            (std::vector<int>{0, 4}));
  // Two candidates or fewer are all picked.
  EXPECT_EQ(pickCovers({1, 3}, {curveOf({1, 9})}, 0.5),
            (std::vector<int>{0, 1}));
  // A tighter tolerance never picks fewer.
  const ProbeCurve wavy = curveOf({5, 4.1, 3.3, 2.2, 1.9});
  size_t last = 0;
  for (const double tol : {1.0, 0.3, 0.1, 0.03, 0.01, 0.001}) {
    const size_t picks = pickCovers(d, {wavy}, tol).size();
    EXPECT_GE(picks, last) << tol;
    last = picks;
  }
  EXPECT_EQ(last, d.size());
}

// The plan from probes on a uniform-dielectric stack, routing layers only:
// below m3 are m2 and m1, both kept without probing; above are m4, m5 and
// m6 at geometric distances (0.4, 1.2 and 3.6 µm), where a tight tolerance
// keeps m5 and a loose one keeps only the nearest and farthest. The plan
// does not depend on the thread count, and omits non-routing layers.
TEST(ModelBuilder, PlanCoversFromProbes)
{
  using Strings = std::vector<std::string>;
  fsv::Stack stack;
  auto addConductor = [&](const char* name, const double bottom) {
    fsv::Stack::Conductor c = conductorAt(name, bottom, bottom + 0.2);
    c.min_width = c.min_spacing = 0.2;
    return stack.addConductor(c);
  };
  const Layer diff = addConductor("diff", 0.0);
  addConductor("poly", 0.3);
  addConductor("m1", 0.8);
  addConductor("m2", 1.4);
  const Layer m3 = addConductor("m3", 2.0);
  addConductor("m4", 2.6);
  addConductor("m5", 3.4);
  const Layer m6 = addConductor("m6", 5.8);

  BuildOptions o = smallBuild(4);
  o.layers = {"m1", "m2", "m3", "m4", "m5", "m6"};
  o.cover_tolerance = 0.02;
  o.tol = 0.03;
  int probes = 0;
  o.progress = [&](int, int) { probes++; };
  const CoverPlan plan = planCovers(stack, m3, o);
  EXPECT_EQ(probes, 3 * 8);  // the below side has nothing to decide
  EXPECT_EQ(names(plan.below, stack), (Strings{"", "m2", "m1"}));
  EXPECT_EQ(names(plan.above, stack), (Strings{"", "m4", "m5", "m6"}));

  BuildOptions one = o;
  one.threads = 1;
  EXPECT_EQ(coversAbove(stack, m3, one), plan.above);

  BuildOptions loose = o;
  loose.cover_tolerance = 0.5;
  EXPECT_EQ(names(coversAbove(stack, m3, loose), stack),
            (Strings{"", "m4", "m6"}));
  EXPECT_NE(buildFingerprint(stack, o), buildFingerprint(stack, loose));

  BuildOptions bad_tolerance = o;
  bad_tolerance.cover_tolerance = -1;
  EXPECT_THROW(planCovers(stack, m3, bad_tolerance), std::invalid_argument);
  bad_tolerance.cover_tolerance = std::numeric_limits<double>::infinity();
  EXPECT_THROW(planCovers(stack, m3, bad_tolerance), std::invalid_argument);
  // The far factors are checked whichever way the covers are chosen.
  BuildOptions bad_factor = o;
  bad_factor.far_cover_factors = {1, 0};
  EXPECT_THROW(planCovers(stack, m3, bad_factor), std::invalid_argument);

  // With nothing above m6, the probes below skip the variants with the
  // other side's cover: 5 candidates × 4 variants; one width probes 5 × 2.
  BuildOptions count_only = o;
  count_only.tol = 0.1;
  probes = 0;
  planCovers(stack, m6, count_only);
  EXPECT_EQ(probes, 5 * 4);
  count_only.width_factors = {1};
  probes = 0;
  planCovers(stack, m6, count_only);
  EXPECT_EQ(probes, 5 * 2);

  // Choosing by depth needs no probes, and no layer fit to probe.
  BuildOptions fixed = o;
  fixed.cover_tolerance = 0;
  EXPECT_EQ(names(coversAbove(stack, diff, fixed), stack), (Strings{""}));
}

// A resumed build with probe-chosen covers takes the plan from the model
// it resumes, so it runs no probe and no solve.
TEST(ModelBuilder, ResumeWithProbedCoversRunsNothing)
{
  BuildOptions o = smallBuild(4);
  o.layers = {"li1", "met1", "met2", "met3", "met4", "met5"};
  o.cover_tolerance = 0.02;
  o.width_factors = {1};
  o.spacing_factors = {1};
  o.crossing_width_factors = {1};
  o.coverages = {0.5};
  o.crossing_length = 2;
  o.crossing_length_long = 4;
  o.end_length = 1;
  o.diagonal_depth = 0;
  o.tol = 0.06;
  Model model;
  buildLayer(sky130(), L("met4"), o, model);
  int solves = 0;
  o.progress = [&](int, int) { solves++; };
  std::stringstream file;
  writeModel(model, {}, file);
  Model saved = readModel(file);
  const std::optional<CoverPlan> held = heldCovers(sky130(), saved, L("met4"));
  ASSERT_TRUE(held.has_value());
  EXPECT_EQ(names(held->above),
            names(planCovers(sky130(), L("met4"), o).above));
  solves = 0;
  buildLayer(sky130(), L("met4"), *held, o, saved);
  EXPECT_EQ(solves, 0);
  EXPECT_FALSE(heldCovers(sky130(), saved, L("met1")).has_value());
  EXPECT_THROW(heldCovers(sky130(), saved, L("via")), std::invalid_argument);
}

// A plan that grows adds what the model lacks: a layer built at depth 0,
// then built again at depth 1 on the same model, gains the nearer covers.
TEST(ModelBuilder, ChangedPlanAddsMissingContexts)
{
  BuildOptions o = smallBuild(4);
  o.layers = {"met1", "met2", "met3"};
  o.width_factors = {1};
  o.spacing_factors = {1};
  o.crossing_width_factors = {1};
  o.coverages = {0.5};
  o.crossing_length = 2;
  o.crossing_length_long = 4;
  o.end_length = 1;
  o.diagonal_depth = 0;
  o.tol = 0.06;
  o.below_depth = o.above_depth = 0;
  Model model;
  buildLayer(sky130(), L("met2"), o, model);
  EXPECT_EQ(model.lateral({"met2", "met1", ""}), nullptr);
  o.below_depth = o.above_depth = 1;
  buildLayer(sky130(), L("met2"), o, model);
  EXPECT_NE(model.lateral({"met2", "met1", "met3"}), nullptr);
  EXPECT_NE(model.crossing({"met2", "met1", ""}), nullptr);
}

TEST(ModelBuilder, RefusesBadLayersBeforeSolving)
{
  BuildOptions o = smallBuild(2);
  Model model;
  // A conductor on the ground plane cannot be a wire or a parallel wire.
  EXPECT_THROW(buildLateral(sky130(), L("DIFF"), o, model),
               std::invalid_argument);
  EXPECT_THROW(buildDiagonal(sky130(), L("met1"), L("DIFF"), o, model),
               std::invalid_argument);
  // A layer the stack lacks, among the options' layers or as the layer.
  o.layers = {"met1", "metX"};
  EXPECT_THROW(coversBelow(sky130(), L("met2"), o), std::invalid_argument);
  EXPECT_THROW(diagonalLayers(sky130(), L("met2"), o), std::invalid_argument);
  EXPECT_THROW(buildLateral(sky130(), L("metX"), o, model),
               std::invalid_argument);
  EXPECT_TRUE(model.laterals().empty());
  // A reach that is not finite, or negative, is refused.
  for (const double bad : {std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(),
                           -1.0}) {
    BuildOptions r = smallBuild(2);
    r.lateral_reach = bad;
    EXPECT_THROW(buildLateral(sky130(), L("met2"), r, model),
                 std::invalid_argument);
    r = smallBuild(2);
    r.lateral_reach_one_sided = bad;
    EXPECT_THROW(buildLateral(sky130(), L("met2"), r, model),
                 std::invalid_argument);
    r = smallBuild(2);
    r.diagonal_reach = bad;
    EXPECT_THROW(buildDiagonal(sky130(), L("met2"), L("met1"), r, model),
                 std::invalid_argument);
  }
  EXPECT_TRUE(model.laterals().empty());
  EXPECT_TRUE(model.diagonals().empty());
}

// A failure inside a solver job reaches the caller as the exception it
// was, whatever thread ran the job.
TEST(ModelBuilder, JobExceptionsPropagate)
{
  BuildOptions o = smallBuild(3);
  o.above_depth = 1;
  // A negative extent inverts the planes' rectangles, which fsv rejects
  // when the pattern is built inside the job.
  o.extent = -100;
  Model model;
  EXPECT_THROW(buildLateral(sky130(), L("met2"), o, model),
               std::invalid_argument);
}

TEST(ModelBuilder, FingerprintFollowsTheStackAndTheOptions)
{
  const BuildOptions o = smallBuild(1);
  const std::string base = buildFingerprint(sky130(), o);
  EXPECT_EQ(buildFingerprint(sky130(), smallBuild(8)), base)
      << "the thread count does not shape the tables";
  BuildOptions tighter = o;
  tighter.tol /= 2;
  EXPECT_NE(buildFingerprint(sky130(), tighter), base);
  BuildOptions deeper = o;
  deeper.diagonal_depth++;
  EXPECT_NE(buildFingerprint(sky130(), deeper), base);
  BuildOptions reach = o;
  reach.reach_factors.push_back(30);
  EXPECT_NE(buildFingerprint(sky130(), reach), base);
  // A stack like sky130's with one conductor thicker, and one renamed.
  auto rebuilt = [&](const char* technology, const double extra_top) {
    fsv::Stack stack;
    stack.setTechnology(technology);
    stack.setBackgroundEps(sky130().backgroundEps());
    for (const fsv::Stack::Dielectric& d : sky130().dielectrics()) {
      stack.addDielectric(d);
    }
    for (fsv::Stack::Conductor c : sky130().conductors()) {
      if (c.name == "met1") {
        c.top += extra_top;
      }
      stack.addConductor(c);
    }
    for (const fsv::Stack::Via& v : sky130().vias()) {
      stack.addVia(v);
    }
    return stack;
  };
  EXPECT_EQ(buildFingerprint(rebuilt(sky130().technology().c_str(), 0), o),
            base);
  EXPECT_NE(buildFingerprint(rebuilt(sky130().technology().c_str(), 0.01), o),
            base);
  EXPECT_NE(buildFingerprint(rebuilt("other", 0), o), base);
}

TEST(ModelBuilder, RejectsUnknownLayer)
{
  Model model;
  EXPECT_THROW(buildLateral(sky130(), L("met9"), smallBuild(1), model),
               std::invalid_argument);
}

// The golden cross section three_wires of src/fsv/test/golden, built by
// layer, solves to its reference (cross_section.txt, from fv2d_golden.py).
TEST(ModelBuilder, SectionMatchesTheGolden)
{
  fsv::Stack stack;
  stack.addDielectric({"a", 0, 0.3, 3.9});
  stack.addDielectric({"b", 0.3, 1.0, 4.2});
  stack.addDielectric({"c", 1.0, 1.05, 7.5});
  stack.addConductor(conductorAt("m", 0.5, 0.8));
  SectionBuilder builder(stack);
  const int left = builder.net("left");
  const int middle = builder.net("middle");
  const int right = builder.net("right");
  builder.add(stack.layer("m"), left, 0, 0.2);
  builder.add(stack.layer("m"), middle, 0.4, 0.6);
  builder.add(stack.layer("m"), right, 0.8, 1.0);
  const fsv::Scene2D scene(builder.geometry(), stack);
  fsv::Solver solver({.threads = 2});
  fsv::SolveOptions options;
  options.tol_total = 0.005;
  options.tol_coupling = 0.005;
  const fsv::CapRow row = solver.solve(scene, middle, options);
  EXPECT_NEAR(row.total(), 0.211432, 0.211432 * 0.005 + 3 * row.sigma[middle]);
  EXPECT_NEAR(
      row.coupling(left), 0.0892539, 0.211432 * 0.005 + 3 * row.sigma[left]);
}

}  // namespace
}  // namespace pex
