// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <optional>
#include <vector>

#include "dr/FlexDR.h"
#include "drt-global.h"
#include "drt/TritonRoute.h"
#include "frBaseTypes.h"
#include "gtest/gtest.h"

namespace drt {
namespace {

constexpr frUInt4 kShapeCost = 8;
constexpr frUInt4 kMarkerCost = 32;

void expectArgsEq(const FlexDR::SearchRepairArgs& actual,
                  const FlexDR::SearchRepairArgs& expected)
{
  EXPECT_EQ(actual.size, expected.size);
  EXPECT_EQ(actual.offset, expected.offset);
  EXPECT_EQ(actual.mazeEndIter, expected.mazeEndIter);
  EXPECT_EQ(actual.workerDRCCost, expected.workerDRCCost);
  EXPECT_EQ(actual.workerMarkerCost, expected.workerMarkerCost);
  EXPECT_EQ(actual.workerFixedShapeCost, expected.workerFixedShapeCost);
  EXPECT_FLOAT_EQ(actual.workerMarkerDecay, expected.workerMarkerDecay);
  EXPECT_EQ(actual.ripupMode, expected.ripupMode);
  EXPECT_EQ(actual.followGuide, expected.followGuide);
}

struct TraceRunner
{
  const std::vector<FlexDR::SearchRepairArgs> table
      = FlexDR::strategy(kShapeCost, kMarkerCost);
  bool has_unidir_layer{false};
  std::vector<FlexDR::SearchRepairArgs> history_args;
  std::vector<int> history_drvs;

  explicit TraceRunner(const int post_it0_drvs, const bool unidir = false)
      : has_unidir_layer(unidir),
        history_args{table[0]},
        history_drvs{post_it0_drvs}
  {
  }

  void expectStep(const FlexDR::SearchRepairArgs& expected,
                  const int post_step_drvs = 0)
  {
    const int it = static_cast<int>(history_drvs.size());
    const int drvs = history_drvs.back();
    ASSERT_EQ(history_args.size(), history_drvs.size());
    const auto actual = FlexDR::nextStepStrategy(it,
                                                 drvs,
                                                 has_unidir_layer,
                                                 history_args,
                                                 history_drvs,
                                                 table,
                                                 kShapeCost,
                                                 kMarkerCost);
    ASSERT_TRUE(actual.has_value());
    expectArgsEq(*actual, expected);
    history_args.push_back(*actual);
    history_drvs.push_back(post_step_drvs);
  }
};

TEST(AdaptiveStrategyTest, DefaultsToDisabledInParamsAndRouterConfig)
{
  const ParamStruct params;
  const RouterConfiguration router_cfg;
  EXPECT_FALSE(params.adaptiveStrategy);
  EXPECT_FALSE(router_cfg.ADAPTIVE_STRATEGY);
  EXPECT_EQ(FlexDR::kAdaptiveMinInitialDrvs, 700);
  EXPECT_EQ(FlexDR::kAdaptiveUnidirStubbornThreshold, 15);
}

TEST(AdaptiveStrategyTest, FlowStateMachineUsesCustomStubbornThreshold)
{
  const auto table = FlexDR::strategy(kShapeCost, kMarkerCost);
  FlexDRFlow default_flow;
  EXPECT_EQ(
      default_flow.determineNextFlow({.num_violations = 50, .args = table[3]}),
      FlexDRFlow::State::GUIDES);

  FlexDRFlow adaptive_unidir_flow;
  adaptive_unidir_flow.setStubbornFlowViolationThreshold(
      FlexDR::kAdaptiveUnidirStubbornThreshold);
  EXPECT_EQ(adaptive_unidir_flow.determineNextFlow(
                {.num_violations = 50, .args = table[3]}),
            FlexDRFlow::State::OPTIMIZATION);
  EXPECT_EQ(adaptive_unidir_flow.determineNextFlow(
                {.num_violations = 15, .args = table[3]}),
            FlexDRFlow::State::GUIDES);
}

TEST(AdaptiveStrategyTest, StopsOnZeroDrvsOrEndOfTable)
{
  const TraceRunner runner(1000);
  EXPECT_FALSE(FlexDR::nextStepStrategy(1,
                                        /*drvs=*/0,
                                        /*has_unidir_layer=*/false,
                                        runner.history_args,
                                        runner.history_drvs,
                                        runner.table,
                                        kShapeCost,
                                        kMarkerCost)
                   .has_value());
  EXPECT_FALSE(FlexDR::nextStepStrategy(static_cast<int>(runner.table.size()),
                                        /*drvs=*/10,
                                        /*has_unidir_layer=*/false,
                                        runner.history_args,
                                        runner.history_drvs,
                                        runner.table,
                                        kShapeCost,
                                        kMarkerCost)
                   .has_value());
}

TEST(AdaptiveStrategyTest, SmallDesignsFollowDefaultStrategyAcrossAllPdks)
{
  for (const bool unidir : {false, true}) {
    TraceRunner runner(FlexDR::kAdaptiveMinInitialDrvs - 1, unidir);
    runner.expectStep(runner.table[1], /*post_step_drvs=*/40);
    runner.expectStep(runner.table[2], /*post_step_drvs=*/5);
    runner.expectStep(runner.table[3], /*post_step_drvs=*/0);

    const auto resumed = FlexDR::nextStepStrategy(/*it=*/3,
                                                  /*drvs=*/50,
                                                  unidir,
                                                  /*history_args=*/{},
                                                  /*history_drvs=*/{},
                                                  runner.table,
                                                  kShapeCost,
                                                  kMarkerCost);
    ASSERT_TRUE(resumed.has_value());
    expectArgsEq(*resumed, runner.table[3]);
  }
}

TEST(AdaptiveStrategyTest, UnidirectionalLargeDesignSkipsSecondRipupAllPass)
{
  TraceRunner runner(4500, /*unidir=*/true);
  // Iteration 1 runs table[1] (shifted RipUpMode::ALL); iteration 2 skips
  // table[2] and steps directly into table[3] (RipUpMode::DRC).
  runner.expectStep(runner.table[1], /*post_step_drvs=*/400);
  runner.expectStep(runner.table[3], /*post_step_drvs=*/0);
}

// clang-format off
TEST(AdaptiveStrategyTest,
     BidirectionalLargeDesignUsesTargetedDrcAndEscalatesOnStall)
{
  TraceRunner runner(5000);
  runner.expectStep({5, -5, 3, kShapeCost, kShapeCost, kShapeCost, 0.950f, RipUpMode::DRC, true}, 60);
  runner.expectStep({3, -2, 3, kShapeCost, kShapeCost, kShapeCost, 0.950f, RipUpMode::DRC, true}, 20);
  runner.expectStep({3, -2, 3, kShapeCost, kMarkerCost, 2 * kShapeCost, 0.950f, RipUpMode::DRC, false}, 20);
  runner.expectStep({5, -4, 8, 4 * kShapeCost, kMarkerCost, 4 * kShapeCost, 0.950f, RipUpMode::DRC, false}, 20);
  runner.expectStep({9, -5, 8, 32 * kShapeCost, 8 * kMarkerCost, 32 * kShapeCost, 0.990f, RipUpMode::NEARDRC, false}, 0);
}

TEST(AdaptiveStrategyTest,
     HighDrvRegimeSettlePassSlowProgressAnd11x11Escalation)
{
  TraceRunner runner(5000);
  // it=1: targeted 5x5 DRC (5000 -> 1200).
  runner.expectStep({5, -5, 3, kShapeCost, kShapeCost, kShapeCost, 0.950f, RipUpMode::DRC, true}, 1200);
  // it=2: > 80 DRVs with >= 50% reduction takes 5x5 mazeEndIter=4 DRC (1200 -> 400).
  runner.expectStep({5, -2, 4, kShapeCost, kShapeCost, kShapeCost, 0.950f, RipUpMode::DRC, true}, 400);
  // it=3: drvs > 100 with healthy progress (66.7% >= 15%) uses 5x5 DRC (400 -> 380).
  runner.expectStep({5, -3, 6, kShapeCost, kShapeCost, 2 * kShapeCost, 0.950f, RipUpMode::DRC, false}, 380);
  // it=4: slow progress (400 -> 380, 5% < 15%) switches to 7x7 NEARDRC (380 -> 450 spike).
  runner.expectStep({7, -4, 8, 8 * kShapeCost, 2 * kMarkerCost, 8 * kShapeCost, 0.950f, RipUpMode::NEARDRC, false}, 450);
  // it=5: post-NEARDRC DRV spike (380 -> 450) triggers 7x7 DRC settle pass (450 -> 450).
  runner.expectStep({7, -5, 8, 4 * kShapeCost, 2 * kMarkerCost, 4 * kShapeCost, 0.950f, RipUpMode::DRC, false}, 450);
  // it=6..8: stagnant_steps == 2..4 escalates from 9x9 NEARDRC to 11x11 NEARDRC (64x/128x).
  runner.expectStep({9, -6, 8, 32 * kShapeCost, 8 * kMarkerCost, 32 * kShapeCost, 0.990f, RipUpMode::NEARDRC, false}, 450);
  runner.expectStep({11, -7, 12, 64 * kShapeCost, 16 * kMarkerCost, 64 * kShapeCost, 0.990f, RipUpMode::NEARDRC, false}, 450);
  runner.expectStep({11, -8, 12, 128 * kShapeCost, 32 * kMarkerCost, 128 * kShapeCost, 0.995f, RipUpMode::NEARDRC, false}, 0);
}

TEST(AdaptiveStrategyTest, ModeratePostIt1Regime500To599Rule)
{
  // Slow-progress branch when 500 <= history_drvs[1] < 600 and drvs > 100.
  TraceRunner slow_runner(10904);
  slow_runner.expectStep({5, -5, 3, kShapeCost, kShapeCost, kShapeCost, 0.950f, RipUpMode::DRC, true}, 572);
  slow_runner.expectStep({5, -2, 4, 2 * kShapeCost, 2 * kShapeCost, 2 * kShapeCost, 0.950f, RipUpMode::DRC, true}, 550);
  slow_runner.expectStep({7, -3, 8, 8 * kShapeCost, 2 * kMarkerCost, 8 * kShapeCost, 0.950f, RipUpMode::NEARDRC, false}, 0);

  // Normal progress through mid-DRV (85 DRVs) and endgame (5 DRVs) in moderate_post_it1.
  TraceRunner runner(10904);
  runner.expectStep({5, -5, 3, kShapeCost, kShapeCost, kShapeCost, 0.950f, RipUpMode::DRC, true}, 572);
  runner.expectStep({5, -2, 4, 2 * kShapeCost, 2 * kShapeCost, 2 * kShapeCost, 0.950f, RipUpMode::DRC, true}, 85);
  runner.expectStep({5, -3, 6, 4 * kShapeCost, kMarkerCost, 4 * kShapeCost, 0.990f, RipUpMode::DRC, false}, 5);
  runner.expectStep({5, -4, 6, 4 * kShapeCost, kMarkerCost, 4 * kShapeCost, 0.990f, RipUpMode::DRC, false}, 0);
}

TEST(AdaptiveStrategyTest, EndgameTailProgressionAndFallbackToDefaultRows30Plus)
{
  TraceRunner runner(5000);
  // it=1..2: reach 5 DRVs, then stall from it=2 onward.
  runner.expectStep({5, -5, 3, kShapeCost, kShapeCost, kShapeCost, 0.950f, RipUpMode::DRC, true}, 5);
  runner.expectStep({3, -2, 3, kShapeCost, kShapeCost, kShapeCost, 0.950f, RipUpMode::DRC, true}, 5);
  // it=3 (stagnant_steps == 1): 5x5 DRC.
  runner.expectStep({5, -3, 8, 4 * kShapeCost, kMarkerCost, 4 * kShapeCost, 0.950f, RipUpMode::DRC, false}, 5);
  // it=4..5 (stagnant_steps == 2..3): 5x5 NEARDRC -> 7x7 NEARDRC.
  runner.expectStep({5, -4, 8, 8 * kShapeCost, 2 * kMarkerCost, 8 * kShapeCost, 0.990f, RipUpMode::NEARDRC, false}, 5);
  runner.expectStep({7, -5, 8, 16 * kShapeCost, 4 * kMarkerCost, 16 * kShapeCost, 0.990f, RipUpMode::NEARDRC, false}, 5);
  // it=6..8 (stagnant_steps == 4..6): 3x3 ALL pin-unboxing with offsets 0, -1, -2.
  runner.expectStep({3, 0, 12, 8 * kShapeCost, 2 * kMarkerCost, 8 * kShapeCost, 0.990f, RipUpMode::ALL, false}, 5);
  runner.expectStep({3, -1, 12, 8 * kShapeCost, 2 * kMarkerCost, 8 * kShapeCost, 0.990f, RipUpMode::ALL, false}, 5);
  runner.expectStep({3, -2, 12, 8 * kShapeCost, 2 * kMarkerCost, 8 * kShapeCost, 0.990f, RipUpMode::ALL, false}, 5);
  // it=9 (stagnant_steps == 7): 7x7 DRC cleanup.
  runner.expectStep({7, -4, 8, 2 * kShapeCost, kMarkerCost, 3 * kShapeCost, 0.950f, RipUpMode::DRC, false}, 5);
  // it=10..11 (stagnant_steps == 8..9): fallback to default_strategy[30..31].
  runner.expectStep(runner.table[30], 5);
  runner.expectStep(runner.table[31], 0);
}
// clang-format on

}  // namespace
}  // namespace drt
