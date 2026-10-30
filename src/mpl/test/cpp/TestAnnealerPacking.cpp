// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "MplTest.h"
#include "PackingOracle.h"
#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"
#include "SimulatedAnnealingCore.h"
#include "object.h"
#include "odb/geom.h"

namespace mpl {
namespace {

class AnnealerPackingTest : public MplTest
{
 protected:
  void SetUp() override
  {
    MplTest::SetUp();
    block_ = db_->getChip()->getBlock();
    db_->setDbuPerMicron(1000);
    db_->getTech()->setLefUnits(1000);
    block_->setDefUnits(1000);
  }

  odb::dbBlock* block_ = nullptr;
};

TEST_F(AnnealerPackingTest, CpSatOracleAcceptsPrimeAreaViaFloorDivision)
{
  // Prime area A = 100003 has no exact integer factors in [100, 200].
  // SoftMacro::setWidth(100) computes height = 100003 / 100 = 1000 (floor
  // division), which fits inside a 100 x 1000 outline.
  PackingProblem problem;
  problem.name = "prime_area_feasible";
  problem.stage = PackingStage::ClusterPlacement;
  problem.outline_width = 100;
  problem.outline_height = 1000;

  PackingBlock soft_block;
  soft_block.name = "prime_cluster";
  soft_block.kind = PackingBlock::Kind::SoftInterval;
  soft_block.area = 100003;
  soft_block.width_intervals = {{100, 200}};
  soft_block.soft_cluster_type = StdCellCluster;
  problem.blocks.push_back(soft_block);

  const PackingOracleResult res = solveWithCpSat(problem);
  ASSERT_EQ(res.status, PackingStatus::Feasible);
  ASSERT_EQ(res.placement.size(), 1U);
  EXPECT_EQ(res.placement[0].width, 100);
  EXPECT_EQ(res.placement[0].height, 1000);

  std::string reason;
  EXPECT_TRUE(verifyPlacement(problem, res.placement, &reason)) << reason;

  // Shrinking outline_height to 999 makes w = 100 require h = 1000 > 999,
  // while w >= 101 exceeds outline_width = 100 -> Infeasible.
  problem.outline_height = 999;
  const PackingOracleResult infeas_res = solveWithCpSat(problem);
  EXPECT_EQ(infeas_res.status, PackingStatus::Infeasible);
}

TEST_F(AnnealerPackingTest, CpSatOracleDiscreteTilingsAndFixedBlockages)
{
  PackingProblem problem;
  problem.name = "discrete_tiling_choice";
  problem.stage = PackingStage::Shaping;
  problem.outline_width = 100;
  problem.outline_height = 80;

  PackingBlock b0;
  b0.name = "c0";
  b0.kind = PackingBlock::Kind::Discrete;
  b0.shapes = {{80, 30}, {30, 80}};

  PackingBlock b1;
  b1.name = "c1";
  b1.kind = PackingBlock::Kind::Discrete;
  b1.shapes = {{70, 80}};

  problem.blocks = {b0, b1};

  const PackingOracleResult res = solveWithCpSat(problem);
  ASSERT_EQ(res.status, PackingStatus::Feasible);
  ASSERT_EQ(res.placement.size(), 2U);
  EXPECT_EQ(res.placement[0].width, 30);
  EXPECT_EQ(res.placement[0].height, 80);

  std::string reason;
  EXPECT_TRUE(verifyPlacement(problem, res.placement, &reason)) << reason;

  // Adding a fixed blockage at (0, 0, 50, 50) leaves no room for a 70x80 block
  // inside a 100x80 outline.
  PackingBlock fixed_blk;
  fixed_blk.name = "blk0";
  fixed_blk.kind = PackingBlock::Kind::Fixed;
  fixed_blk.fixed_x = 0;
  fixed_blk.fixed_y = 0;
  fixed_blk.fixed_width = 50;
  fixed_blk.fixed_height = 50;
  problem.blocks.push_back(fixed_blk);

  const PackingOracleResult blocked_res = solveWithCpSat(problem);
  EXPECT_EQ(blocked_res.status, PackingStatus::Infeasible);
}

TEST_F(AnnealerPackingTest,
       CpSatOracleRejectsAreaOverflowAndHandlesMultiIntervals)
{
  PackingProblem overflow_problem;
  overflow_problem.name = "area_overflow";
  overflow_problem.stage = PackingStage::ClusterPlacement;
  overflow_problem.outline_width = 100;
  overflow_problem.outline_height = 100;

  PackingBlock s0;
  s0.name = "s0";
  s0.kind = PackingBlock::Kind::SoftInterval;
  s0.area = 6000;
  s0.width_intervals = {{50, 80}};

  PackingBlock s1;
  s1.name = "s1";
  s1.kind = PackingBlock::Kind::SoftInterval;
  s1.area = 6000;
  s1.width_intervals = {{50, 80}};

  overflow_problem.blocks = {s0, s1};
  EXPECT_EQ(solveWithCpSat(overflow_problem).status, PackingStatus::Infeasible);

  // Disjoint width intervals: [30, 40] cannot fit height floor(3600 / w) >= 90
  // alongside a 100x20 fixed block in a 100x100 outline, whereas [60, 75] can.
  PackingProblem multi_iv_problem;
  multi_iv_problem.name = "multi_interval_feasible";
  multi_iv_problem.stage = PackingStage::ClusterPlacement;
  multi_iv_problem.outline_width = 100;
  multi_iv_problem.outline_height = 100;

  PackingBlock fixed_top;
  fixed_top.name = "top_bar";
  fixed_top.kind = PackingBlock::Kind::Fixed;
  fixed_top.fixed_x = 0;
  fixed_top.fixed_y = 80;
  fixed_top.fixed_width = 100;
  fixed_top.fixed_height = 20;

  PackingBlock multi_soft;
  multi_soft.name = "multi_soft";
  multi_soft.kind = PackingBlock::Kind::SoftInterval;
  multi_soft.area = 3605;
  multi_soft.width_intervals = {{30, 40}, {60, 75}};

  multi_iv_problem.blocks = {fixed_top, multi_soft};
  const PackingOracleResult res = solveWithCpSat(multi_iv_problem);
  ASSERT_EQ(res.status, PackingStatus::Feasible);
  ASSERT_EQ(res.placement.size(), 2U);
  EXPECT_GE(res.placement[1].width, 60);
  EXPECT_LE(res.placement[1].width, 75);
  EXPECT_EQ(res.placement[1].height, 3605 / res.placement[1].width);

  std::string reason;
  EXPECT_TRUE(verifyPlacement(multi_iv_problem, res.placement, &reason))
      << reason;
}

TEST_F(AnnealerPackingTest, SerializerDeserializerRoundTrip)
{
  PackingProblem original;
  original.name = "roundtrip_case";
  original.stage = PackingStage::ClusterPlacement;
  original.outline_width = 250;
  original.outline_height = 180;

  PackingBlock fixed_blk;
  fixed_blk.name = "f0";
  fixed_blk.kind = PackingBlock::Kind::Fixed;
  fixed_blk.fixed_x = 10;
  fixed_blk.fixed_y = 20;
  fixed_blk.fixed_width = 30;
  fixed_blk.fixed_height = 40;

  PackingBlock disc_blk;
  disc_blk.name = "d0";
  disc_blk.kind = PackingBlock::Kind::Discrete;
  disc_blk.shapes = {{50, 60}, {60, 50}, {30, 100}};

  PackingBlock soft_std;
  soft_std.name = "s_std";
  soft_std.kind = PackingBlock::Kind::SoftInterval;
  soft_std.soft_cluster_type = StdCellCluster;
  soft_std.area = 4507;
  soft_std.width_intervals = {{40, 80}, {90, 120}};

  PackingBlock soft_mix;
  soft_mix.name = "s_mix";
  soft_mix.kind = PackingBlock::Kind::SoftInterval;
  soft_mix.soft_cluster_type = MixedCluster;
  soft_mix.area = 6011;
  soft_mix.width_intervals = {{50, 100}};

  original.blocks = {fixed_blk, disc_blk, soft_std, soft_mix};

  const std::string text = serializePackingProblemToString(original);
  const PackingProblem parsed = parsePackingProblemFromString(text);
  EXPECT_EQ(parsed, original);
}

TEST_F(AnnealerPackingTest, InverseAdapterPopulatesClustersAndRunsWithoutNoOps)
{
  EXPECT_EQ(defaultMovesPerStep(PackingStage::Shaping, 4), 50);
  EXPECT_EQ(defaultMovesPerStep(PackingStage::Shaping, 80), 80);
  EXPECT_EQ(defaultMovesPerStep(PackingStage::ClusterPlacement, 4), 500);
  EXPECT_EQ(defaultMovesPerStep(PackingStage::MacroPlacement, 600), 600);

  PackingProblem problem;
  problem.name = "adapter_smoke";
  problem.stage = PackingStage::ClusterPlacement;
  problem.outline_width = 100;
  problem.outline_height = 100;

  PackingBlock disc;
  disc.name = "hm_cluster";
  disc.kind = PackingBlock::Kind::Discrete;
  disc.shapes = {{40, 50}, {50, 40}};

  PackingBlock soft_std;
  soft_std.name = "std_cluster";
  soft_std.kind = PackingBlock::Kind::SoftInterval;
  soft_std.soft_cluster_type = StdCellCluster;
  soft_std.area = 2003;
  soft_std.width_intervals = {{30, 60}};

  PackingBlock soft_mix;
  soft_mix.name = "mix_cluster";
  soft_mix.kind = PackingBlock::Kind::SoftInterval;
  soft_mix.soft_cluster_type = MixedCluster;
  soft_mix.area = 2503;
  soft_mix.width_intervals = {{40, 70}};

  problem.blocks = {disc, soft_std, soft_mix};

  auto harness = buildSoftPackingHarness(problem, block_, &logger_, /*seed=*/1);
  ASSERT_NE(harness->tree.root, nullptr);
  ASSERT_EQ(harness->macros.size(), 3U);

  // Verify non-null Cluster* with matching ClusterType and working
  // resize/setWidth/setHeight.
  EXPECT_TRUE(harness->macros[0].isMacroCluster());
  EXPECT_TRUE(harness->macros[1].isStdCellCluster());
  EXPECT_TRUE(harness->macros[2].isMixedCluster());

  harness->macros[1].setWidth(50);
  EXPECT_EQ(harness->macros[1].getWidth(), 50);
  EXPECT_EQ(harness->macros[1].getHeight(), (60 * (2003 / 60)) / 50);

  harness->macros[2].setHeight(50);
  EXPECT_GT(harness->macros[2].getWidth(), 0);

  runSA<SACoreSoftMacro>(harness->annealer.get());
  EXPECT_GE(harness->annealer->getAreaPenalty(), 0.0f);
  ASSERT_TRUE(harness->annealer->isValid());

  const std::vector<PlacedBlock> placed = extractPlacement(*harness);
  std::string reason;
  EXPECT_TRUE(verifyPlacement(problem, placed, &reason)) << reason;
}

TEST_F(AnnealerPackingTest, OverlappingIntervalsAndMultiOrientationHardMacro)
{
  // 1. Overlapping and zero-min width intervals are normalized without SIGFPE
  // or premature rejection when SoftMacro::setShapes merges [30, 50] and [45,
  // 60] into [30, 60].
  PackingProblem interval_problem;
  interval_problem.name = "overlapping_intervals_soft";
  interval_problem.stage = PackingStage::ClusterPlacement;
  interval_problem.outline_width = 50;
  interval_problem.outline_height = 40;

  PackingBlock sb;
  sb.name = "std_cluster";
  sb.kind = PackingBlock::Kind::SoftInterval;
  sb.area = 2003;
  sb.width_intervals = {{0, 10}, {30, 50}, {45, 60}};
  interval_problem.blocks = {sb};

  const PackingOracleResult cp_res = solveWithCpSat(interval_problem);
  ASSERT_EQ(cp_res.status, PackingStatus::Feasible);
  std::string reason;
  EXPECT_TRUE(verifyPlacement(interval_problem, cp_res.placement, &reason))
      << reason;
  const std::vector<PlacedBlock> sa_placed = {{"std_cluster", 0, 0, 50, 39}};
  EXPECT_TRUE(verifyPlacement(interval_problem, sa_placed, &reason)) << reason;

  // 2. Multi-orientation HardMacro in buildHardPackingHarness across seeds.
  PackingProblem hard_orient_problem;
  hard_orient_problem.name = "hard_multi_orient";
  hard_orient_problem.stage = PackingStage::MacroPlacement;
  hard_orient_problem.outline_width = 100;
  hard_orient_problem.outline_height = 80;

  PackingBlock c0;
  c0.name = "c0";
  c0.kind = PackingBlock::Kind::Discrete;
  c0.shapes = {{80, 30}, {30, 80}};

  PackingBlock c1;
  c1.name = "c1";
  c1.kind = PackingBlock::Kind::Discrete;
  c1.shapes = {{70, 80}};
  hard_orient_problem.blocks = {c0, c1};

  auto hard_harness
      = buildHardPackingHarness(hard_orient_problem, block_, &logger_, 2);
  runSA<SACoreHardMacro>(hard_harness->annealer.get());
  ASSERT_TRUE(hard_harness->annealer->isValid());
  EXPECT_TRUE(
      verifyPlacement(hard_orient_problem,
                      extractPlacement(*hard_harness, hard_orient_problem),
                      &reason))
      << reason;
}

TEST_F(AnnealerPackingTest, CorpusBenchmarkAgainstCpSatOracle)
{
  const std::string corpus_path = getFilePath(
      "_main/src/mpl/test/testcases/"
      "packing_corpus.txt");
  const std::vector<PackingProblem> corpus
      = loadPackingProblemCorpus(corpus_path);
  ASSERT_GE(corpus.size(), 9U);

  int feasible_count = 0;
  int infeasible_count = 0;
  int annealer_solved_count = 0;

  for (const PackingProblem& problem : corpus) {
    const PackingOracleResult oracle = solveWithCpSat(problem);
    ASSERT_NE(oracle.status, PackingStatus::Unknown)
        << "CP-SAT returned Unknown for " << problem.name;

    if (oracle.status == PackingStatus::Feasible) {
      ++feasible_count;
      std::string oracle_reason;
      EXPECT_TRUE(verifyPlacement(problem, oracle.placement, &oracle_reason))
          << "Oracle placement invalid on " << problem.name << ": "
          << oracle_reason;
    } else {
      ++infeasible_count;
    }

    bool annealer_found_valid = false;
    std::vector<PlacedBlock> best_placement;
    for (int seed = 1; seed <= 5 && !annealer_found_valid; ++seed) {
      if (problem.stage == PackingStage::MacroPlacement) {
        auto harness = buildHardPackingHarness(problem, block_, &logger_, seed);
        runSA<SACoreHardMacro>(harness->annealer.get());
        if (harness->annealer->isValid()) {
          annealer_found_valid = true;
          best_placement = extractPlacement(*harness, problem);
        }
      } else {
        auto harness = buildSoftPackingHarness(problem, block_, &logger_, seed);
        runSA<SACoreSoftMacro>(harness->annealer.get());
        if (harness->annealer->isValid()) {
          annealer_found_valid = true;
          best_placement = extractPlacement(*harness);
        }
      }
    }

    if (oracle.status == PackingStatus::Infeasible) {
      EXPECT_FALSE(annealer_found_valid)
          << "Annealer claimed valid placement on CP-SAT infeasible problem "
          << problem.name;
    } else if (annealer_found_valid) {
      ++annealer_solved_count;
      std::string sa_reason;
      EXPECT_TRUE(verifyPlacement(problem, best_placement, &sa_reason))
          << "Annealer placement failed verification on " << problem.name
          << ": " << sa_reason;
    }
  }

  EXPECT_GE(feasible_count, 6);
  EXPECT_GE(infeasible_count, 3);
  EXPECT_EQ(annealer_solved_count, feasible_count);
}

}  // namespace
}  // namespace mpl
