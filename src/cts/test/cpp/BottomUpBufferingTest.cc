// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "BottomUpBuffering.h"
#include "gtest/gtest.h"

namespace cts {
namespace {
BufferingNode sink(double cap, double x = 0, double y = 0)
{
  BufferingNode node;
  node.sink = true;
  node.input_cap = cap;
  node.point = {x, y};
  return node;
}

BufferingNode branch(std::vector<size_t> children, double x = 0, double y = 0)
{
  BufferingNode node;
  node.children = std::move(children);
  node.point = {x, y};
  return node;
}

std::optional<BufferingChoice> select(const BufferingLoad& load,
                                      const std::string& fixed)
{
  if (load.capacitance() <= 5 && fixed != "large") {
    return BufferingChoice{"small", 1};
  }
  if (load.capacitance() <= 20 && fixed != "small") {
    return BufferingChoice{"large", 2};
  }
  return std::nullopt;
}
}  // namespace

TEST(BottomUpBufferingTest, NonuniformSubtreesPropagateSelectedInputCap)
{
  std::vector<double> observed;
  BottomUpBuffering strategy(0, 1000, [&](const auto& load, const auto& fixed) {
    observed.push_back(load.capacitance());
    return select(load, fixed);
  });
  const auto nodes = strategy.run(
      {branch({1, 2}), branch({3}), branch({4}), sink(2), sink(12)}, 0);
  ASSERT_EQ(nodes.size(), 5u);
  EXPECT_EQ(nodes[1].master, "small");
  EXPECT_EQ(nodes[2].master, "large");
  EXPECT_EQ(nodes[0].master, "small");
  EXPECT_EQ(observed, (std::vector<double>{2, 12, 3}));
  EXPECT_DOUBLE_EQ(strategy.load(nodes, 0).pin_cap, 3);
}

TEST(BottomUpBufferingTest, WireCapacitanceIsPartOfTheLoad)
{
  BottomUpBuffering strategy(0.5, 1000, select);
  const auto nodes = strategy.run({branch({1}), sink(2, 10)}, 0);
  EXPECT_EQ(nodes[0].master, "large");
  const auto load = strategy.load(nodes, 0);
  EXPECT_DOUBLE_EQ(load.pin_cap, 2);
  EXPECT_DOUBLE_EQ(load.wire_cap, 5);
  EXPECT_DOUBLE_EQ(load.max_wire_length, 10);
}

TEST(BottomUpBufferingTest, LongBranchInsertsStagesWithoutMovingEndpoints)
{
  BottomUpBuffering strategy(0.001, 100, select);
  const auto nodes = strategy.run({branch({1}), sink(2, 500, 500)}, 0);
  ASSERT_GT(nodes.size(), 2u);
  EXPECT_DOUBLE_EQ(nodes[0].point.x, 0);
  EXPECT_DOUBLE_EQ(nodes[0].point.y, 0);
  EXPECT_DOUBLE_EQ(nodes[1].point.x, 500);
  EXPECT_DOUBLE_EQ(nodes[1].point.y, 500);
  size_t index = 0;
  size_t stages = 0;
  while (!nodes[index].sink) {
    ASSERT_EQ(nodes[index].children.size(), 1u);
    EXPECT_LE(strategy.load(nodes, index).max_wire_length, 100);
    EXPECT_LE(strategy.load(nodes, index).capacitance(), 20);
    index = nodes[index].children.front();
    ASSERT_LE(++stages, nodes.size());
  }
  EXPECT_EQ(index, 1u);
}

TEST(BottomUpBufferingTest, OverloadedBranchActuallyAddsBuffers)
{
  BottomUpBuffering strategy(0, 1000, select);
  std::vector<BufferingNode> input{branch({})};
  for (size_t i = 1; i <= 256; ++i) {
    input[0].children.push_back(i);
    input.push_back(sink(2));
  }
  const auto nodes = strategy.run(input, 0);
  ASSERT_GT(nodes.size(), input.size());
  size_t sink_count = 0;
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].sink) {
      ++sink_count;
    } else {
      EXPECT_FALSE(nodes[i].master.empty());
      EXPECT_LE(strategy.load(nodes, i).capacitance(), 20);
    }
  }
  EXPECT_EQ(sink_count, 256u);
  EXPECT_EQ(input.size(), 257u);
  EXPECT_TRUE(input[0].master.empty());
}

TEST(BottomUpBufferingTest, FixedRootMasterIsRespected)
{
  BottomUpBuffering strategy(0, 1000, select);
  auto root = branch({1});
  root.fixed_master = "large";
  const auto nodes = strategy.run({root, sink(1)}, 0);
  EXPECT_EQ(nodes[0].master, "large");
  EXPECT_DOUBLE_EQ(nodes[0].input_cap, 2);
}

TEST(BottomUpBufferingTest, SlewFailureTriggersAdditionalStage)
{
  // Model an otherwise cap-legal cell whose receiver slew fails above
  // 25 dbu. This exercises the selector's electrical feasibility decision.
  BottomUpBuffering strategy(0, 1000, [](const auto& load, const auto& fixed) {
    return load.max_wire_length <= 25 ? select(load, fixed) : std::nullopt;
  });
  const auto nodes = strategy.run({branch({1}), sink(1, 100)}, 0);
  ASSERT_GT(nodes.size(), 2u);
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (!nodes[i].sink) {
      EXPECT_LE(strategy.load(nodes, i).max_wire_length, 25);
    }
  }
}

TEST(BottomUpBufferingTest, ImpossibleSinkIsReportedInsteadOfClamped)
{
  BottomUpBuffering strategy(0, 1000, select);
  EXPECT_THROW(strategy.run({branch({1}), sink(21)}, 0), std::runtime_error);
}

TEST(BottomUpBufferingTest, LegalizerCannotCauseUnboundedInsertion)
{
  BottomUpBuffering strategy(
      0, 10, select, [](BufferingPoint) { return BufferingPoint{}; });
  EXPECT_THROW(strategy.run({branch({1}), sink(1, 100)}, 0),
               std::runtime_error);
}

TEST(BottomUpBufferingTest, RejectsInvalidTopologyAndCapacitance)
{
  BottomUpBuffering strategy(0, 1000, select);
  EXPECT_THROW(strategy.run({}, 0), std::invalid_argument);
  EXPECT_THROW(strategy.run({branch({1}), branch({0})}, 0),
               std::invalid_argument);
  EXPECT_THROW(strategy.run({branch({1, 1}), sink(1)}, 0),
               std::invalid_argument);
  EXPECT_THROW(strategy.run({branch({2}), sink(1)}, 0), std::invalid_argument);
  EXPECT_THROW(strategy.run({branch({1}), sink(-1)}, 0), std::invalid_argument);
  EXPECT_THROW(strategy.run({branch({1}), sink(1), sink(1)}, 0),
               std::invalid_argument);
}

TEST(BottomUpBufferingTest, RejectsInvalidElectricalModel)
{
  BottomUpBuffering strategy(0, 1000, [](const auto&, const auto&) {
    return BufferingChoice{"invalid", std::numeric_limits<double>::quiet_NaN()};
  });
  EXPECT_THROW(strategy.run({branch({1}), sink(1)}, 0), std::runtime_error);
  EXPECT_THROW((BottomUpBuffering(-1, 1000, select)), std::invalid_argument);
  EXPECT_THROW((BottomUpBuffering(0, 0, select)), std::invalid_argument);
}
TEST(BottomUpBufferingTest, RCEnclosesDifferentDirectionsClassesAndScenes)
{
  // Two scenes, each ordered clock-H, clock-V, signal-H, signal-V.
  const std::array<BufferingWireRC, 8> rc{
      {{1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}, {11, 12}, {13, 14}, {15, 16}}};
  const auto bound = conservativeBufferingRC(rc);
  EXPECT_DOUBLE_EQ(bound.resistance, 15);
  EXPECT_DOUBLE_EQ(bound.capacitance, 16);
}

TEST(BottomUpBufferingTest, MissingOrInvalidDirectionalRCIsNotAValidProof)
{
  EXPECT_THROW(conservativeBufferingRC({}), std::invalid_argument);
  for (const double invalid : {0.0,
                               -1.0,
                               std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::quiet_NaN()}) {
    std::array<BufferingWireRC, 4> rc{{{1, 2}, {3, 4}, {5, 6}, {7, 8}}};
    rc[3].capacitance = invalid;
    EXPECT_THROW(conservativeBufferingRC(rc), std::invalid_argument);
    rc[3] = {invalid, 8};
    EXPECT_THROW(conservativeBufferingRC(rc), std::invalid_argument);
  }
}
}  // namespace cts
