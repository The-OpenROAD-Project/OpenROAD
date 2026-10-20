// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "BufferingSearch.h"
#include "gtest/gtest.h"

namespace cts {
namespace {
BufferingNode branch(std::vector<size_t> children, double x = 0)
{
  BufferingNode node;
  node.children = std::move(children);
  node.point.x = x;
  node.master = "fast";
  node.input_cap = node.nominal_input_cap = 2;
  return node;
}

BufferingNode sink(double cap, double x = 0)
{
  BufferingNode node;
  node.sink = true;
  node.input_cap = node.nominal_input_cap = cap;
  node.point.x = x;
  return node;
}

BufferingSearchModel model()
{
  return {0, 0, 1000, 2, 8};
}

std::vector<BufferingCellChoice> cells(const BufferingLoad& load,
                                       const std::string& fixed)
{
  std::vector<BufferingCellChoice> choices;
  for (const std::string name : {"root", "fast", "balanced", "small"}) {
    if (!fixed.empty() && name != fixed) {
      continue;
    }
    const double cap = load.capacitance();
    if (cap > (name == "small" ? 6 : 40)) {
      continue;
    }
    const double input = name == "small" ? 1 : 2;
    const double delay = name == "root"       ? 1 + cap * 0.3
                         : name == "fast"     ? 1 + cap * 0.1
                         : name == "balanced" ? 4
                                              : 2 + cap * 2;
    choices.push_back(
        {{name, input}, input, name == "small" ? 1.0 : 4.0, {delay, delay}});
  }
  return choices;
}

void fixRoot(std::vector<BufferingNode>& nodes)
{
  nodes[0].master = nodes[0].fixed_master = "root";
}
}  // namespace

TEST(BufferingSearchTest, UnnecessaryBranchStageIsActuallyBypassed)
{
  std::vector nodes{branch({1}), branch({2, 3}), sink(2), sink(2)};
  fixRoot(nodes);
  const auto reference = evaluateBuffering(nodes, 0, model(), cells);
  ASSERT_TRUE(reference);
  const auto plan = searchBuffering(nodes, 0, model(), cells, {}, reference);
  EXPECT_FALSE(plan.reference);
  EXPECT_TRUE(plan.nodes[1].master.empty());
  EXPECT_EQ(plan.depth, 1u);
  EXPECT_LT(plan.delay(), reference->delay());
}

TEST(BufferingSearchTest, WireOnlyOverloadRequiresRealBuffer)
{
  std::vector nodes{branch({1}), branch({2, 3}), sink(4), sink(4)};
  fixRoot(nodes);
  const BufferingChoices limited = [](const auto& load, const auto& fixed) {
    if (fixed == "root" && load.capacitance() > 5) {
      return std::vector<BufferingCellChoice>{};
    }
    return cells(load, fixed);
  };
  const auto plan = searchBuffering(nodes, 0, model(), limited);
  EXPECT_FALSE(plan.nodes[1].master.empty());
  EXPECT_EQ(plan.depth, 2u);
  auto bypass = plan.nodes;
  bypass[1].master.clear();
  EXPECT_FALSE(evaluateBuffering(bypass, 0, model(), limited));
}

TEST(BufferingSearchTest, CompleteDetourGeometryAndWireCapAreRetained)
{
  std::vector nodes{branch({1}), branch({2}, 10), sink(1)};
  fixRoot(nodes);
  nodes[1].master.clear();
  auto rc = model();
  rc.wire_cap = 0.2;
  BufferingLoad observed;
  const BufferingChoices inspect = [&](const auto& load, const auto& fixed) {
    observed = load;
    return cells(load, fixed);
  };
  ASSERT_TRUE(evaluateBuffering(nodes, 0, rc, inspect));
  EXPECT_DOUBLE_EQ(observed.pin_cap, 1);
  EXPECT_DOUBLE_EQ(observed.wire_cap, 4);
  EXPECT_DOUBLE_EQ(observed.max_wire_length, 20);
}

TEST(BufferingSearchTest, FewerStagesDoNotOverrideBetterDelay)
{
  std::vector nodes{branch({1}), branch({2, 3}), sink(4), sink(4)};
  fixRoot(nodes);
  const BufferingChoices slow_root = [](const auto& load, const auto& fixed) {
    auto result = cells(load, fixed);
    for (auto& cell : result) {
      if (cell.buffer.master == "root") {
        cell.delays.assign(2, 1 + 2 * load.capacitance());
      }
    }
    return result;
  };
  const auto plan = searchBuffering(nodes, 0, model(), slow_root);
  EXPECT_EQ(plan.depth, 2u);
  EXPECT_FALSE(plan.nodes[1].master.empty());
  auto bypass = plan.nodes;
  bypass[1].master.clear();
  const auto single = evaluateBuffering(bypass, 0, model(), slow_root);
  ASSERT_TRUE(single);
  EXPECT_LT(plan.delay(), single->delay());
}

TEST(BufferingSearchTest, FasterButUnbalancedPlanCannotDisplaceReference)
{
  std::vector nodes{branch({1, 2}),
                    branch({3}, -5),
                    branch({4}, 5),
                    sink(2, -10),
                    sink(6, 10)};
  fixRoot(nodes);
  nodes[1].master = nodes[2].master = "balanced";
  auto bounded = model();
  bounded.max_wire_length = 5;
  const BufferingChoices branch_cells = [](const auto& load,
                                           const auto& fixed) {
    auto result = cells(load, fixed);
    if (fixed != "root") {
      std::erase_if(result, [](const auto& cell) {
        return cell.buffer.master != "fast" && cell.buffer.master != "balanced";
      });
    }
    return result;
  };
  const auto reference = evaluateBuffering(nodes, 0, bounded, branch_cells);
  ASSERT_TRUE(reference);
  EXPECT_DOUBLE_EQ(reference->skew(), 0);
  const auto unrestricted = searchBuffering(nodes, 0, bounded, branch_cells);
  EXPECT_LT(unrestricted.delay(), reference->delay());
  EXPECT_GT(unrestricted.skew(), reference->skew());
  const auto guarded
      = searchBuffering(nodes, 0, bounded, branch_cells, {}, reference);
  EXPECT_TRUE(guarded.reference);
  EXPECT_DOUBLE_EQ(guarded.skew(), 0);
  EXPECT_EQ(guarded.nodes[1].master, "balanced");
  EXPECT_EQ(guarded.nodes[2].master, "balanced");
  // Even a frontier too small to retain a usable alternative cannot prune
  // the independently evaluated reference.
  bounded.candidate_limit = 1;
  EXPECT_TRUE(searchBuffering(nodes, 0, bounded, branch_cells, {}, reference)
                  .reference);
}

TEST(BufferingSearchTest, ExplicitRootAndInternalCellCannotBeBypassedOrResized)
{
  std::vector nodes{branch({1}), branch({2}), sink(1)};
  fixRoot(nodes);
  nodes[1].fixed_master = "fast";
  const auto plan = searchBuffering(nodes, 0, model(), cells);
  EXPECT_EQ(plan.nodes[0].master, "root");
  EXPECT_EQ(plan.nodes[1].master, "fast");
  EXPECT_EQ(plan.depth, 2u);
}

TEST(BufferingSearchTest, DummyLoadIsIncludedInElectricalFeasibility)
{
  std::vector nodes{branch({1, 2}), branch({3}), branch({4}), sink(2), sink(5)};
  fixRoot(nodes);
  const BufferingChoices limited = [](const auto& load, const auto& fixed) {
    return load.capacitance() > 5 ? std::vector<BufferingCellChoice>{}
                                  : cells(load, fixed);
  };
  ASSERT_TRUE(evaluateBuffering(nodes, 0, model(), limited));
  const BufferingDummy dummy = [](float actual, float ideal, size_t) {
    return ideal > actual
               ? std::optional<BufferingCellChoice>({{{"dummy", 5}, 5, 1, {}}})
               : std::nullopt;
  };
  EXPECT_FALSE(evaluateBuffering(nodes, 0, model(), limited, dummy));
}

TEST(BufferingSearchTest, SearchIsBoundedAndDeterministic)
{
  std::vector nodes{branch({1, 2}),
                    branch({3, 4}),
                    branch({5, 6}),
                    sink(2),
                    sink(3),
                    sink(4),
                    sink(1)};
  fixRoot(nodes);
  auto bounded = model();
  bounded.candidate_limit = 4;
  const auto first = searchBuffering(nodes, 0, bounded, cells);
  for (int repeat = 0; repeat < 100; ++repeat) {
    const auto next = searchBuffering(nodes, 0, bounded, cells);
    EXPECT_LE(next.peak_candidates, bounded.candidate_limit);
    EXPECT_DOUBLE_EQ(next.delay(), first.delay());
    EXPECT_DOUBLE_EQ(next.skew(), first.skew());
    ASSERT_EQ(next.nodes.size(), first.nodes.size());
    for (size_t i = 0; i < next.nodes.size(); ++i) {
      EXPECT_EQ(next.nodes[i].master, first.nodes[i].master);
    }
  }
}
TEST(BufferingSearchTest, WideFanoutDoesNotCopyPartialSubtrees)
{
  std::vector<BufferingNode> nodes{branch({})};
  fixRoot(nodes);
  for (size_t i = 0; i < 10000; ++i) {
    nodes.front().children.push_back(nodes.size());
    nodes.push_back(sink(1));
  }
  const BufferingChoices wide = [](const auto&, const auto& fixed) {
    return std::vector<BufferingCellChoice>{{{fixed, 1}, 1, 1, {1, 1}}};
  };
  const auto plan = searchBuffering(nodes, 0, model(), wide);
  EXPECT_EQ(plan.buffers, 1u);
  EXPECT_EQ(plan.depth, 1u);
  EXPECT_DOUBLE_EQ(plan.delay(), 1);
  EXPECT_EQ(plan.nodes.front().children.size(), 10000u);
}

TEST(BufferingSearchTest, InvalidBoundsAndMissingTimingCannotPass)
{
  std::vector nodes{branch({1}), sink(1)};
  fixRoot(nodes);
  auto invalid = model();
  invalid.candidate_limit = 0;
  EXPECT_THROW(searchBuffering(nodes, 0, invalid, cells),
               std::invalid_argument);
  invalid.candidate_limit = 65;
  EXPECT_THROW(searchBuffering(nodes, 0, invalid, cells),
               std::invalid_argument);
  invalid = model();
  invalid.wire_res = std::numeric_limits<double>::quiet_NaN();
  EXPECT_THROW(searchBuffering(nodes, 0, invalid, cells),
               std::invalid_argument);
  for (double length : {0.0,
                        -1.0,
                        -std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::quiet_NaN()}) {
    invalid = model();
    invalid.max_wire_length = length;
    EXPECT_THROW(searchBuffering(nodes, 0, invalid, cells),
                 std::invalid_argument);
  }
  EXPECT_THROW(evaluateBuffering(nodes, 0, model(), {}), std::invalid_argument);
  const BufferingChoices missing = [](const auto&, const auto& fixed) {
    return std::vector<BufferingCellChoice>{{{fixed, 1}, 1, 1, {1}}};
  };
  EXPECT_THROW(searchBuffering(nodes, 0, model(), missing),
               std::invalid_argument);
  const auto reference = evaluateBuffering(nodes, 0, model(), cells);
  ASSERT_TRUE(reference);
  auto bad_reference = reference;
  bad_reference->arrivals.clear();
  EXPECT_THROW(searchBuffering(nodes, 0, model(), cells, {}, bad_reference),
               std::invalid_argument);
}

TEST(BufferingSearchTest, BetterWorstSceneCannotHideAnotherSceneRegression)
{
  std::vector nodes{branch({1}), branch({2}), sink(1)};
  fixRoot(nodes);
  nodes[1].master = "balanced";
  auto multimode = model();
  multimode.dimensions = 4;
  const BufferingChoices scene_cells = [](const auto& load, const auto& fixed) {
    std::vector<BufferingCellChoice> result;
    if (fixed.empty() || fixed == "balanced") {
      result.push_back({{"balanced", 2}, 2, 1, {10, 10, 20, 20}});
    }
    if (fixed.empty() || fixed == "fast") {
      result.push_back({{"fast", 2}, 2, 1, {11, 11, 15, 15}});
    }
    if (fixed == "root") {
      const double delay = load.capacitance() > 1.5 ? 1 : 30;
      result.push_back({{"root", 2}, 2, 1, {delay, delay, delay, delay}});
    }
    return result;
  };
  const auto reference = evaluateBuffering(nodes, 0, multimode, scene_cells);
  ASSERT_TRUE(reference);
  const auto unguarded = searchBuffering(nodes, 0, multimode, scene_cells);
  EXPECT_LT(unguarded.delay(), reference->delay());
  EXPECT_GT(unguarded.arrivals[0].second, reference->arrivals[0].second);
  const auto guarded
      = searchBuffering(nodes, 0, multimode, scene_cells, {}, reference);
  EXPECT_TRUE(guarded.reference);
  EXPECT_EQ(guarded.nodes[1].master, "balanced");
}

TEST(BufferingSearchTest, VerifiedFeasibleSeedCannotBePruned)
{
  std::vector nodes{branch({1}), branch({2}, 5), sink(1, 10)};
  fixRoot(nodes);
  auto bounded = model();
  bounded.max_wire_length = 5;
  bounded.candidate_limit = 1;
  // One-slot pruning prefers the child's zero-delay wire state, but the
  // root cannot drive the resulting ten-unit path. The complete buffered
  // seed is independently feasible and must survive without a reference.
  ASSERT_TRUE(evaluateBuffering(nodes, 0, bounded, cells));
  const auto plan = searchBuffering(nodes, 0, bounded, cells);
  EXPECT_FALSE(plan.reference);
  EXPECT_EQ(plan.depth, 2u);
  EXPECT_EQ(plan.buffers, 2u);
  EXPECT_EQ(plan.nodes[1].master, "fast");
  EXPECT_LE(plan.peak_candidates, bounded.candidate_limit);
}

TEST(BufferingSearchTest, UnboundedSpanRetainsWireLoadAndElectricalConstraints)
{
  std::vector nodes{branch({1}), sink(1, 2000)};
  fixRoot(nodes);
  auto unbounded = model();
  unbounded.max_wire_length = std::numeric_limits<double>::infinity();
  unbounded.wire_cap = 0.01;
  const auto reference = evaluateBuffering(nodes, 0, unbounded, cells);
  ASSERT_TRUE(reference);
  const auto plan = searchBuffering(nodes, 0, unbounded, cells, {}, reference);
  EXPECT_EQ(plan.depth, 1u);
  EXPECT_EQ(plan.buffers, 1u);
  EXPECT_GT(plan.delay(), 1 + 0.3 * nodes[1].input_cap);
  unbounded.wire_cap = 0.1;
  BufferingRejection rejection;
  EXPECT_FALSE(evaluateBuffering(nodes, 0, unbounded, cells, {}, &rejection));
  EXPECT_EQ(rejection.reason, "cell electrical constraints");
  EXPECT_DOUBLE_EQ(rejection.load.capacitance(), 201);
}

TEST(BufferingSearchTest, ReferenceRejectionReportsTheActualConstraintAndLoad)
{
  std::vector nodes{branch({1}), sink(1, 20)};
  fixRoot(nodes);
  auto bounded = model();
  bounded.max_wire_length = 10;
  bounded.wire_cap = 0.1;
  BufferingRejection rejection;
  EXPECT_FALSE(evaluateBuffering(nodes, 0, bounded, cells, {}, &rejection));
  EXPECT_EQ(rejection.reason, "wire-length limit");
  EXPECT_EQ(rejection.node, 0u);
  EXPECT_DOUBLE_EQ(rejection.load.pin_cap, 1);
  EXPECT_DOUBLE_EQ(rejection.load.wire_cap, 2);
  EXPECT_DOUBLE_EQ(rejection.load.max_wire_length, 20);
  bounded.max_wire_length = 20;
  const BufferingChoices no_cell = [](const auto&, const auto&) {
    return std::vector<BufferingCellChoice>{};
  };
  EXPECT_FALSE(evaluateBuffering(nodes, 0, bounded, no_cell, {}, &rejection));
  EXPECT_EQ(rejection.reason, "cell electrical constraints");
  ASSERT_TRUE(evaluateBuffering(nodes, 0, bounded, cells, {}, &rejection));
  EXPECT_TRUE(rejection.reason.empty());
  nodes[0].fixed_master = "other";
  EXPECT_FALSE(evaluateBuffering(nodes, 0, bounded, cells, {}, &rejection));
  EXPECT_EQ(rejection.reason, "fixed master mismatch");
  nodes[0].master.clear();
  EXPECT_FALSE(evaluateBuffering(nodes, 0, bounded, cells, {}, &rejection));
  EXPECT_EQ(rejection.reason, "missing required buffer");
}

}  // namespace cts
