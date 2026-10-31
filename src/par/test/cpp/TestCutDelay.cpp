// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include "gtest/gtest.h"
#include "src/par/src/Evaluator.h"
#include "src/par/src/Hypergraph.h"
#include "src/par/src/KWayFMRefine.h"
#include "src/par/src/Refiner.h"
#include "src/par/src/Utilities.h"
#include "utl/Logger.h"

namespace par {
namespace {

class CutDelayRefiner : public KWayFMRefine
{
 public:
  using KWayFMRefine::KWayFMRefine;
  using Refiner::CalculatePathCost;
  using Refiner::CalculateVertexGain;
  using Refiner::FindNeighbors;
};

class CutDelayTest : public ::testing::Test
{
 protected:
  void SetUp() override { hgraph_ = MakeGraph({0, 1, 2, 3}); }

  HGraphPtr MakeGraph(const std::vector<int>& path)
  {
    return std::make_shared<Hypergraph>(
        1,
        1,
        0,
        Matrix<int>{{0, 1}, {1, 2}, {2, 3}, {0, 2}, {1, 3}},
        Matrix<float>{{1}, {1}, {1}, {1}},
        Matrix<float>{{0}, {0}, {0}, {2}, {2}},
        std::vector<int>{0, -1, -1, 1},
        std::vector<int>{},
        Matrix<float>{},
        std::vector<VertexType>{},
        std::vector<float>(5, 0.2f),
        std::vector<std::set<int>>{{0}, {1}, {2}, {3}, {4}},
        std::vector<TimingPath>{{path, {0, 1, 2}, 0.2f}},
        &logger_);
  }

  EvaluatorPtr MakeEvaluator(float delay, float exponent = 2)
  {
    auto evaluator = std::make_shared<GoldenEvaluator>(2,
                                                       std::vector<float>{1},
                                                       std::vector<float>{1},
                                                       std::vector<float>{},
                                                       0,
                                                       1,
                                                       0,
                                                       exponent,
                                                       delay,
                                                       hgraph_,
                                                       &logger_);
    evaluator->InitializeTiming(hgraph_);
    return evaluator;
  }

  utl::Logger logger_;
  HGraphPtr hgraph_;
};

TEST_F(CutDelayTest, PenaltyUsesEveryBoundary)
{
  auto evaluator = MakeEvaluator(0.1f);
  EXPECT_FLOAT_EQ(evaluator->CalculatePathCost(0, hgraph_, {0, 0, 0, 0}), 0);
  EXPECT_NEAR(
      evaluator->CalculatePathCost(0, hgraph_, {0, 0, 1, 1}), 0.81, 1e-6);
  EXPECT_NEAR(evaluator->CalculatePathCost(0, hgraph_, {0, 1, 1, 0}), 2, 1e-6);
  EXPECT_NEAR(
      evaluator->CalculatePathCost(0, hgraph_, {0, 1, 0, 1}), 3.63, 1e-6);
  EXPECT_FLOAT_EQ(hgraph_->PathTimingSlack(0), 0.2f);
  EXPECT_NEAR(hgraph_->PathTimingCost(0), 0.64, 1e-6);

  evaluator = MakeEvaluator(0);
  EXPECT_NEAR(
      evaluator->CalculatePathCost(0, hgraph_, {0, 1, 0, 1}), 1.92, 1e-6);
}

TEST_F(CutDelayTest, DefaultExponentUsesCutDelay)
{
  auto evaluator = MakeEvaluator(0.1f, 1);
  EXPECT_NEAR(
      evaluator->CalculatePathCost(0, hgraph_, {0, 0, 1, 1}), 0.9, 1e-6);
  EXPECT_NEAR(
      evaluator->CalculatePathCost(0, hgraph_, {0, 1, 0, 1}), 3.3, 1e-6);
}

TEST_F(CutDelayTest, MoveGainsMatchPartitionCost)
{
  auto evaluator = MakeEvaluator(0.5f);
  CutDelayRefiner refiner(2, 2, 1, 0, 4, 25, evaluator, &logger_);
  const Partitions solution{0, 1, 0, 1};
  const auto paths_cost = evaluator->GetPathsCost(hgraph_, solution);
  const auto net_degs = evaluator->GetNetDegrees(hgraph_, solution);
  const float before = evaluator->CutEvaluator(hgraph_, solution, false).cost;
  for (int v = 0; v < hgraph_->GetNumVertices(); v++) {
    Partitions moved = solution;
    moved[v] = 1 - solution[v];
    const auto gain = refiner.CalculateVertexGain(
        v, solution[v], moved[v], hgraph_, solution, paths_cost, net_degs);
    const float after = evaluator->CutEvaluator(hgraph_, moved, false).cost;
    EXPECT_NEAR(gain->GetGain(), before - after, 1e-5);
    EXPECT_FLOAT_EQ(
        refiner.CalculatePathCost(0, hgraph_, solution, v, moved[v]),
        evaluator->CalculatePathCost(0, hgraph_, moved));
  }
}

TEST_F(CutDelayTest, RefreshesDistantPathNeighbors)
{
  auto evaluator = MakeEvaluator(0.5f);
  CutDelayRefiner refiner(2, 2, 1, 0, 4, 25, evaluator, &logger_);
  const std::vector<bool> visited{true, false, false, false};
  const auto neighbors = refiner.FindNeighbors(hgraph_, 0, visited);
  EXPECT_NE(std::ranges::find(neighbors, 3), neighbors.end());
  const auto pair_neighbors = refiner.FindNeighbors(
      hgraph_, 0, visited, Partitions{0, 1, 0, 1}, {0, 1});
  EXPECT_NE(std::ranges::find(pair_neighbors, 3), pair_neighbors.end());
}

TEST_F(CutDelayTest, CountsCoarsenedPathOncePerMove)
{
  hgraph_ = MakeGraph({0, 1, 2, 1, 3});
  auto evaluator = MakeEvaluator(0.5f);
  CutDelayRefiner refiner(2, 2, 1, 0, 4, 25, evaluator, &logger_);
  const Partitions solution{0, 1, 0, 1};
  const Partitions moved{0, 0, 0, 1};
  const auto gain = refiner.CalculateVertexGain(
      1,
      1,
      0,
      hgraph_,
      solution,
      evaluator->GetPathsCost(hgraph_, solution),
      evaluator->GetNetDegrees(hgraph_, solution));
  EXPECT_NEAR(gain->GetGain(),
              evaluator->CutEvaluator(hgraph_, solution, false).cost
                  - evaluator->CutEvaluator(hgraph_, moved, false).cost,
              1e-5);
}

TEST_F(CutDelayTest, RefinementRespondsToCutDelay)
{
  const Partitions initial{0, 1, 0, 1};
  Partitions without_delay = initial;
  auto evaluator = MakeEvaluator(0);
  CutDelayRefiner refiner_without_delay(2, 2, 1, 0, 4, 25, evaluator, &logger_);
  refiner_without_delay.Refine(
      hgraph_, Matrix<float>{{3}, {3}}, Matrix<float>{{1}, {1}}, without_delay);
  EXPECT_EQ(without_delay, initial);

  Partitions with_delay = initial;
  evaluator = MakeEvaluator(0.5f);
  const float before = evaluator->CutEvaluator(hgraph_, initial, false).cost;
  CutDelayRefiner refiner_with_delay(2, 2, 1, 0, 4, 25, evaluator, &logger_);
  refiner_with_delay.Refine(
      hgraph_, Matrix<float>{{3}, {3}}, Matrix<float>{{1}, {1}}, with_delay);
  EXPECT_NE(with_delay, initial);
  EXPECT_EQ(with_delay.front(), 0);
  EXPECT_EQ(with_delay.back(), 1);
  EXPECT_LT(evaluator->CutEvaluator(hgraph_, with_delay, false).cost, before);
}

}  // namespace
}  // namespace par
