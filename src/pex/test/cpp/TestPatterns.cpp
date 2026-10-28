// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <stdexcept>
#include <string>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Itf.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"
#include "pex/Patterns.h"

namespace pex {
namespace {

const fsv::Stack& sky130()
{
  static const fsv::Stack stack = fsv::readItfFile("src/fsv/test/sky130.itf");
  return stack;
}

TEST(Patterns, ZRangeOfConductorsAndVias)
{
  const fsv::Stack& stack = sky130();
  const fsv::Stack::Conductor* met2 = stack.findConductor("met2");
  ASSERT_NE(met2, nullptr);
  const ZRange z = zRange(stack, "met2");
  EXPECT_EQ(z.bottom, met2->bottom);
  EXPECT_EQ(z.top, met2->top);

  const ZRange via = zRange(stack, "via");
  EXPECT_EQ(via.bottom, stack.findConductor("met1")->top);
  EXPECT_EQ(via.top, met2->bottom);

  EXPECT_THROW(zRange(stack, "met9"), std::invalid_argument);

  fsv::Stack bare;
  bare.conductors.push_back(
      {.name = "m1", .bottom = 1, .top = 2, .conformals = {}});
  bare.vias.push_back({.name = "tie", .upper = "m1", .lower = "SUBSTRATE"});
  bare.vias.push_back({.name = "open", .upper = "m2", .lower = "m1"});
  EXPECT_THROW(zRange(bare, "tie"), std::invalid_argument);
  EXPECT_THROW(zRange(bare, "open"), std::invalid_argument);
}

TEST(Patterns, LayersBelowAndAboveNearestFirst)
{
  const fsv::Stack& stack = sky130();
  const std::vector<std::string> below{"met1", "li1", "fieldpoly", "DIFF"};
  EXPECT_EQ(layersBelow(stack, "met2"), below);
  const std::vector<std::string> above{"met3", "met4", "met5"};
  EXPECT_EQ(layersAbove(stack, "met2"), above);
  EXPECT_TRUE(layersAbove(stack, "met5").empty());
  EXPECT_TRUE(layersBelow(stack, "DIFF").empty());
}

TEST(Patterns, BuilderAddsBoxesWithTheLayerExtent)
{
  const fsv::Stack& stack = sky130();
  PatternBuilder builder(stack);
  const int a = builder.net("a");
  EXPECT_EQ(builder.net("a"), a);
  const int b = builder.net("b");
  EXPECT_NE(a, b);
  builder.add("met1", a, 0, 1, 2, 3);
  builder.add("via", b, 0.5, 1.5, 0.7, 1.7);
  const fsv::Geometry& g = builder.geometry();
  ASSERT_EQ(g.boxes().size(), 2u);
  const ZRange m1 = zRange(stack, "met1");
  const fsv::Box& box = g.boxes()[0];
  EXPECT_EQ(box.lo[0], 0);
  EXPECT_EQ(box.lo[1], 1);
  EXPECT_EQ(box.lo[2], m1.bottom);
  EXPECT_EQ(box.hi[0], 2);
  EXPECT_EQ(box.hi[1], 3);
  EXPECT_EQ(box.hi[2], m1.top);
  EXPECT_EQ(g.boxNet(0), a);
  EXPECT_EQ(g.boxNet(1), b);
  EXPECT_EQ(g.boxes()[1].lo[2], m1.top);

  SectionBuilder section(stack);
  const int c = section.net("c");
  section.add("met3", c, -1, 1);
  const fsv::Geometry2D& s = section.geometry();
  ASSERT_EQ(s.rects().size(), 1u);
  EXPECT_EQ(s.rects()[0].lo[0], -1);
  EXPECT_EQ(s.rects()[0].hi[0], 1);
  EXPECT_EQ(s.rects()[0].lo[1], zRange(stack, "met3").bottom);
  EXPECT_EQ(s.rects()[0].hi[1], zRange(stack, "met3").top);
}

// The golden structure three_wires of src/fsv/test/golden, built by layer.
TEST(Patterns, BuildsTheGoldenThreeWires)
{
  fsv::Stack stack;
  stack.background_eps = 1.0;
  stack.dielectrics
      = {{"a", 0, 0.3, 3.9}, {"b", 0.3, 1.0, 4.2}, {"c", 1.0, 1.05, 7.5}};
  stack.conductors.push_back(
      {.name = "m", .bottom = 0.5, .top = 0.8, .conformals = {}});
  SectionBuilder builder(stack);
  builder.add("m", builder.net("left"), 0, 0.2);
  builder.add("m", builder.net("middle"), 0.4, 0.6);
  builder.add("m", builder.net("right"), 0.8, 1.0);
  const fsv::Geometry2D& g = builder.geometry();
  ASSERT_EQ(g.rects().size(), 3u);
  for (const fsv::Rect& r : g.rects()) {
    EXPECT_EQ(r.lo[1], 0.5);
    EXPECT_EQ(r.hi[1], 0.8);
  }
  EXPECT_EQ(g.rects()[1].lo[0], 0.4);
  EXPECT_EQ(g.netName(g.rectNet(2)), "right");
}

TEST(Patterns, LateralSectionPlacesTheParts)
{
  const fsv::Stack& stack = sky130();
  LateralPattern pattern;
  pattern.layer = "met2";
  pattern.width = 0.2;
  pattern.left = 0.3;
  pattern.below = "met1";
  pattern.extent = 4;
  const LateralPattern::Section s = pattern.build(stack);
  EXPECT_EQ(s.right, kNullNetIndex);
  EXPECT_EQ(s.above, kNullNetIndex);
  ASSERT_GE(s.wire, 0);
  ASSERT_GE(s.left, 0);
  ASSERT_GE(s.below, 0);
  const auto& rects = s.geometry.rects();
  ASSERT_EQ(rects.size(), 3u);
  const double wmin = stack.findConductor("met2")->min_width;
  for (int i = 0; i < 3; i++) {
    const fsv::Rect& r = rects[i];
    const int net = s.geometry.rectNet(i);
    if (net == s.wire) {
      EXPECT_DOUBLE_EQ(r.lo[0], -0.1);
      EXPECT_DOUBLE_EQ(r.hi[0], 0.1);
    } else if (net == s.left) {
      EXPECT_DOUBLE_EQ(r.hi[0], -0.4);
      EXPECT_DOUBLE_EQ(r.lo[0], -0.4 - wmin);
    } else {
      // The plane reaches the extent past the outermost wire on each side.
      EXPECT_EQ(net, s.below);
      EXPECT_DOUBLE_EQ(r.lo[0], -0.4 - wmin - 4);
      EXPECT_DOUBLE_EQ(r.hi[0], 0.1 + 4);
      EXPECT_EQ(r.hi[1], zRange(stack, "met1").top);
    }
  }

  pattern.left = -1;
  pattern.right = 0.5;
  pattern.neighbor_width = 1;
  pattern.below.clear();
  pattern.above = "met3";
  const LateralPattern::Section t = pattern.build(stack);
  EXPECT_EQ(t.left, kNullNetIndex);
  EXPECT_EQ(t.below, kNullNetIndex);
  ASSERT_EQ(t.geometry.rects().size(), 3u);
  const fsv::Rect& right = t.geometry.rects()[1];
  EXPECT_EQ(t.geometry.rectNet(1), t.right);
  EXPECT_DOUBLE_EQ(right.lo[0], 0.6);
  EXPECT_DOUBLE_EQ(right.hi[0], 1.6);

  pattern.layer = "nope";
  EXPECT_THROW(pattern.build(stack), std::invalid_argument);
}

TEST(Patterns, EndWirePlacesThePlanes)
{
  const fsv::Stack& stack = sky130();
  EndPattern pattern;
  pattern.layer = "met2";
  pattern.width = 0.2;
  pattern.length = 3;
  pattern.below = "met1";
  pattern.extent = 4;
  pattern.margin = 1;
  const EndPattern::Wire w = pattern.build(stack);
  EXPECT_EQ(w.above, kNullNetIndex);
  ASSERT_GE(w.wire, 0);
  ASSERT_GE(w.below, 0);
  const auto& boxes = w.geometry.boxes();
  ASSERT_EQ(boxes.size(), 2u);
  for (int i = 0; i < 2; i++) {
    const fsv::Box& b = boxes[i];
    if (w.geometry.boxNet(i) == w.wire) {
      EXPECT_DOUBLE_EQ(b.lo[0], -0.1);
      EXPECT_DOUBLE_EQ(b.hi[0], 0.1);
      EXPECT_DOUBLE_EQ(b.lo[1], 0);
      EXPECT_DOUBLE_EQ(b.hi[1], 3);
      EXPECT_EQ(b.lo[2], zRange(stack, "met2").bottom);
    } else {
      // The plane reaches the extent sideways and the margin past the ends.
      EXPECT_EQ(w.geometry.boxNet(i), w.below);
      EXPECT_DOUBLE_EQ(b.lo[0], -4);
      EXPECT_DOUBLE_EQ(b.hi[0], 4);
      EXPECT_DOUBLE_EQ(b.lo[1], -1);
      EXPECT_DOUBLE_EQ(b.hi[1], 4);
      EXPECT_EQ(b.hi[2], zRange(stack, "met1").top);
    }
  }
  pattern.length = 0;
  EXPECT_THROW(pattern.build(stack), std::invalid_argument);
}

TEST(Patterns, CrossingArrayRoundsToWholePitches)
{
  const fsv::Stack& stack = sky130();
  CrossingPattern p;
  p.layer = "met2";
  p.cover = "met3";
  p.width = 0.14;
  p.crossing_width = 0.3;
  p.pitch = 1.2;
  p.length = 10;
  p.spacing = 0.14;
  p.other = "met1";
  p.extent = 5;
  p.margin = 2;
  const CrossingPattern::Array a = p.build(stack);
  EXPECT_NEAR(a.length, 8 * 1.2, 1e-9);
  ASSERT_GE(a.wire, 0);
  ASSERT_GE(a.cross, 0);
  ASSERT_GE(a.left, 0);
  ASSERT_GE(a.other, 0);
  const fsv::Geometry& g = a.geometry;
  int crossings = 0;
  for (int b = 0; b < static_cast<int>(g.boxes().size()); b++) {
    const fsv::Box& box = g.boxes()[b];
    const int net = g.boxNet(b);
    if (net == a.cross) {
      crossings++;
      EXPECT_NEAR(box.hi[1] - box.lo[1], 0.3, 1e-9);
      EXPECT_EQ(box.lo[0], -5);
      EXPECT_EQ(box.lo[2], zRange(stack, "met3").bottom);
    } else if (net == a.wire) {
      EXPECT_EQ(box.lo[1], 0);
      EXPECT_NEAR(box.hi[1], a.length, 1e-9);
    } else if (net == a.other) {
      EXPECT_EQ(box.lo[1], -2);
      EXPECT_NEAR(box.hi[1], a.length + 2, 1e-9);
    }
  }
  // 8 pitches plus 2 µm of margin each way: two more each side.
  EXPECT_EQ(crossings, 8 + 2 * 2);
  // A length under one pitch still gives one.
  p.length = 0.1;
  EXPECT_NEAR(p.build(stack).length, 1.2, 1e-9);
  p.crossing_width = 2;
  EXPECT_THROW(p.build(stack), std::invalid_argument);
}

TEST(Patterns, DiagonalSectionPlacesTheParallelWire)
{
  const fsv::Stack& stack = sky130();
  DiagonalPattern p;
  p.layer = "met2";
  p.other = "met1";
  p.width = 0.2;
  p.other_width = 0.5;
  p.gap = -0.05;
  p.spacing = 0.3;
  p.far_spacing = 0.3;
  const DiagonalPattern::Section d = p.build(stack);
  ASSERT_GE(d.diagonal, 0);
  ASSERT_GE(d.left, 0);
  const auto& rects = d.geometry.rects();
  ASSERT_EQ(rects.size(), 4u);
  const fsv::Rect& diag = rects[3];
  EXPECT_EQ(d.geometry.rectNet(3), d.diagonal);
  EXPECT_DOUBLE_EQ(diag.lo[0], 0.1 - 0.05);
  EXPECT_DOUBLE_EQ(diag.hi[0], 0.1 - 0.05 + 0.5);
  EXPECT_EQ(diag.lo[1], zRange(stack, "met1").bottom);
  EXPECT_DOUBLE_EQ(rects[2].lo[0], 0.1 + 0.3);
  p.other_width = 0;
  p.spacing = -1;
  p.far_spacing = -1;
  const DiagonalPattern::Section alone = p.build(stack);
  EXPECT_EQ(alone.diagonal, kNullNetIndex);
  EXPECT_EQ(alone.left, kNullNetIndex);
  EXPECT_EQ(alone.geometry.rects().size(), 1u);
}

}  // namespace
}  // namespace pex
