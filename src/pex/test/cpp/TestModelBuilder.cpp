// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

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
std::vector<std::string> names(const std::vector<Layer>& layers)
{
  std::vector<std::string> out;
  for (const Layer layer : layers) {
    out.push_back(layer.valid() ? sky130().name(layer) : "");
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
  o.tol = 0.02;
  o.threads = threads;
  o.extent = 3;
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
        EXPECT_GT(c.ground, 0);
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
  o.tol = 0.03;
  Model model;
  // A coverage of 0 or 1 has no gap to tabulate; refused before any solve.
  for (const double bad : {0.0, 1.0}) {
    BuildOptions full = o;
    full.coverages = {bad};
    EXPECT_THROW(buildCrossing(sky130(), L("met2"), L("met3"), full, model),
                 std::invalid_argument);
  }
  buildCrossing(sky130(), L("met2"), L("met3"), o, model);
  const CrossingTable* c = model.crossing({"met2", "met3"});
  ASSERT_NE(c, nullptr);
  // Gaps are wc (1 / f - 1): 0.3, 0.9 (twice, merged) and 2.7 µm.
  ASSERT_EQ(c->gaps().size(), 3u);
  EXPECT_NEAR(c->gaps()[0], 0.3, 1e-9);
  EXPECT_NEAR(c->gaps()[1], 0.9, 1e-9);
  EXPECT_NEAR(c->gaps()[2], 2.7, 1e-9);
  // Crossings keep a share of the plane's effect, less over wider gaps.
  EXPECT_GT(c->values()[0].total, 0.5);
  EXPECT_LT(c->values()[0].total, 1.0);
  EXPECT_GT(c->values()[0].total, c->values()[1].total);
  EXPECT_GT(c->values()[1].cross, 0);
  EXPECT_THROW(buildCrossing(sky130(), L("met2"), L("met2"), o, model),
               std::invalid_argument);

  buildEnds(sky130(), L("met2"), o, model);
  EXPECT_EQ(model.ends().size(), 2u);
  const EndTable* e = model.end({"met2", "met1", ""});
  ASSERT_NE(e, nullptr);
  ASSERT_EQ(e->excess().size(), 1u);
  // One end of a minimum-width wire adds a few hundredths of a fF.
  EXPECT_GT(e->excess()[0], 0.01);
  EXPECT_LT(e->excess()[0], 0.1);
}

TEST(ModelBuilder, DiagonalTable)
{
  BuildOptions o = smallBuild(4);
  o.diagonal_width_factors = {1};
  o.diagonal_gap_factors = {0, 2};
  o.diagonal_spacing_factors = {1};
  o.diagonal_far_spacing_factors = {1};
  o.tol = 0.02;
  Model model;
  buildDiagonal(sky130(), L("met2"), L("met1"), o, model);
  const DiagonalTable* t = model.diagonal({"met2", "met1"});
  ASSERT_NE(t, nullptr);
  ASSERT_EQ(t->values().size(), 1u * 2 * 2 * 2);
  // Alone, a met1 wire beside the met2 wire couples more at the near gap
  // and adds nearly as much to the total; a neighbor at minimum spacing
  // on its side shields it and loses some of its own coupling; one on the
  // far side shields it too.
  const DiagonalCap& near_open = t->at(0, 0, 1, 1);
  const DiagonalCap& far_open = t->at(0, 1, 1, 1);
  const DiagonalCap& near_shielded = t->at(0, 0, 0, 1);
  const DiagonalCap& far_shielded = t->at(0, 0, 1, 0);
  EXPECT_GT(near_open.coupling, far_open.coupling);
  EXPECT_GT(near_open.total_delta, 0.5 * near_open.coupling);
  EXPECT_EQ(near_open.lateral_delta, 0);
  EXPECT_LT(near_shielded.coupling, near_open.coupling);
  EXPECT_LT(near_shielded.lateral_delta, 0);
  EXPECT_LT(far_shielded.coupling, near_open.coupling);
  EXPECT_EQ(far_shielded.lateral_delta, 0);
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
  o.tol = 0.03;
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
  EXPECT_NE(model.crossing({"met2", "met3"}), nullptr);
  EXPECT_NE(model.crossing({"met2", "met1"}), nullptr);
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
