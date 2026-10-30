// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "fsv/Itf.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"
#include "pex/CoverMap.h"
#include "pex/Model.h"

namespace pex {
namespace {

const fsv::Stack& sky130()
{
  static const fsv::Stack stack = fsv::readItfFile("src/fsv/test/sky130.itf");
  return stack;
}

// A model whose met2 contexts are met1 below and met3 and met5 above, met4
// left out as a builder's far factors might, with a crossing table for
// met3 only.
Model sampleModel()
{
  Model model;
  for (const char* below : {"", "met1"}) {
    for (const char* above : {"", "met3", "met5"}) {
      model.addLateral({"met2", below, above}, LateralTable({0.14}, {0.14}));
    }
  }
  model.addCrossing({"met2", "met3"}, CrossingTable({1}, {{0.5, 0.5}}));
  return model;
}

TEST(CoverMap, TabulatedCoversResolveToThemselves)
{
  const fsv::Stack& stack = sky130();
  const Model model = sampleModel();
  const CoverMap map(stack, model, stack.layer("met2"));
  const std::vector<CoverMap::Part> none = map.resolve(Layer());
  ASSERT_EQ(none.size(), 1u);
  EXPECT_EQ(none[0].key, "");
  EXPECT_EQ(none[0].weight, 1);
  const std::vector<CoverMap::Part> met3 = map.resolve(stack.layer("met3"));
  ASSERT_EQ(met3.size(), 1u);
  EXPECT_EQ(met3[0].key, "met3");
  EXPECT_EQ(met3[0].weight, 1);
  EXPECT_EQ(map.resolve(stack.layer("met1"))[0].key, "met1");
}

// A cover between two tabulated ones takes both, weighted in the
// logarithm of the distance; one past the farthest takes the farthest.
TEST(CoverMap, InterpolatesBetweenTabulatedCovers)
{
  const fsv::Stack& stack = sky130();
  const Model model = sampleModel();
  const CoverMap map(stack, model, stack.layer("met2"));
  const std::vector<CoverMap::Part> met4 = map.resolve(stack.layer("met4"));
  ASSERT_EQ(met4.size(), 2u);
  EXPECT_EQ(met4[0].key, "met3");
  EXPECT_EQ(met4[1].key, "met5");
  const double d3 = map.distance(stack.layer("met3"));
  const double d4 = map.distance(stack.layer("met4"));
  const double d5 = map.distance(stack.layer("met5"));
  const double t = std::log(d4 / d3) / std::log(d5 / d3);
  EXPECT_NEAR(met4[1].weight, t, 1e-12);
  EXPECT_NEAR(met4[0].weight + met4[1].weight, 1, 1e-12);
  EXPECT_GT(t, 0);
  EXPECT_LT(t, 1);
  // Below met2 only met1 is tabulated: li1 and fieldpoly take it alone.
  const std::vector<CoverMap::Part> li1 = map.resolve(stack.layer("li1"));
  ASSERT_EQ(li1.size(), 1u);
  EXPECT_EQ(li1[0].key, "met1");
  EXPECT_EQ(map.resolve(stack.layer("fieldpoly"))[0].key, "met1");
  // The met2 layer itself is beside met2: no cover.
  const std::vector<CoverMap::Part> beside = map.resolve(stack.layer("met2"));
  ASSERT_EQ(beside.size(), 1u);
  EXPECT_EQ(beside[0].key, "");
  EXPECT_EQ(map.crossingCover(stack.layer("met2")), "");
  EXPECT_LT(map.distance(stack.layer("met2")), 0);
  // A via is not a cover: asking for one is a caller's mistake.
  EXPECT_THROW(map.resolve(stack.layer("via3")), std::invalid_argument);
  EXPECT_THROW(map.crossingCover(stack.layer("via3")), std::invalid_argument);
}

TEST(CoverMap, CrossingTableOfTheNearestTabulatedCover)
{
  const fsv::Stack& stack = sky130();
  const Model model = sampleModel();
  const CoverMap map(stack, model, stack.layer("met2"));
  EXPECT_EQ(map.crossingCover(stack.layer("met3")), "met3");
  // met5 is tabulated but has no crossing table; met3 is the nearest that
  // does.
  EXPECT_EQ(map.crossingCover(stack.layer("met5")), "met3");
  EXPECT_EQ(map.crossingCover(stack.layer("met4")), "met3");
  // Nothing below has one.
  EXPECT_EQ(map.crossingCover(stack.layer("met1")), "");
  EXPECT_EQ(map.crossingCover(Layer()), "");
}

// A cover touching the layer, at distance 0, interpolates without a
// non-finite weight.
TEST(CoverMap, TouchingCoverStaysFinite)
{
  fsv::Stack stack;
  stack.addDielectric({"d", 0, 10, 4});
  for (const auto& [name, bottom] :
       {std::pair{"m1", 1.0}, {"m2", 1.2}, {"m3", 1.5}, {"m4", 2.5}}) {
    fsv::Stack::Conductor c;
    c.name = name;
    c.bottom = bottom;
    c.top = bottom + 0.2;  // m2 sits on m1's top face
    stack.addConductor(c);
  }
  Model model;
  for (const char* above : {"", "m2", "m4"}) {
    model.addLateral({"m1", "", above}, LateralTable({0.1}, {0.1}));
  }
  const CoverMap map(stack, model, stack.layer("m1"));
  EXPECT_EQ(map.distance(stack.layer("m2")), 0);
  const std::vector<CoverMap::Part> parts = map.resolve(stack.layer("m3"));
  ASSERT_EQ(parts.size(), 2u);
  for (const CoverMap::Part& p : parts) {
    EXPECT_TRUE(std::isfinite(p.weight));
    EXPECT_GE(p.weight, 0);
    EXPECT_LE(p.weight, 1);
  }
}

// A layer with no contexts at all resolves every cover to none.
TEST(CoverMap, NoTablesMeansNoCover)
{
  const fsv::Stack& stack = sky130();
  const Model model;
  const CoverMap map(stack, model, stack.layer("met2"));
  const std::vector<CoverMap::Part> parts = map.resolve(stack.layer("met3"));
  ASSERT_EQ(parts.size(), 1u);
  EXPECT_EQ(parts[0].key, "");
  EXPECT_THROW(CoverMap(stack, model, stack.layer("via")),
               std::invalid_argument);
}

}  // namespace
}  // namespace pex
