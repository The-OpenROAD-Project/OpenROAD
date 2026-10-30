// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "MplTest.h"
#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"
#include "SimulatedAnnealingCore.h"
#include "clusterEngine.h"
#include "gtest/gtest.h"
#include "hier_rtlmp.h"
#include "mpl-util.h"
#include "mpl/rtl_mp.h"
#include "object.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tst/logger_checks.h"

namespace mpl {
namespace {

// Reference O(n^2) sequence-pair floorplan packing for bit-exact verification.
template <typename T>
std::pair<int, int> packFloorplanReference(const std::vector<int>& pos_seq,
                                           const std::vector<int>& neg_seq,
                                           std::vector<T>& macros)
{
  std::vector<std::pair<int, int>> sequence_pair_pos(pos_seq.size());
  for (size_t i = 0; i < pos_seq.size(); i++) {
    sequence_pair_pos[pos_seq[i]].first = static_cast<int>(i);
    sequence_pair_pos[neg_seq[i]].second = static_cast<int>(i);
  }

  std::vector<int> accumulated_length(pos_seq.size(), 0);
  for (int macro_id : pos_seq) {
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;
    T& macro = macros[macro_id];

    if (!macro.isFixed()) {
      macro.setX(accumulated_length[neg_seq_pos]);
    }

    const int current_length = macro.getX() + macro.getWidth();
    for (size_t j = neg_seq_pos; j < neg_seq.size(); j++) {
      if (current_length > accumulated_length[j]) {
        accumulated_length[j] = current_length;
      } else {
        break;
      }
    }
  }

  const int width = accumulated_length[pos_seq.size() - 1];

  std::vector<int> reversed_pos_seq(pos_seq.size());
  for (size_t i = 0; i < reversed_pos_seq.size(); i++) {
    reversed_pos_seq[i] = pos_seq[reversed_pos_seq.size() - 1 - i];
  }

  for (size_t i = 0; i < pos_seq.size(); i++) {
    sequence_pair_pos[reversed_pos_seq[i]].first = static_cast<int>(i);
    sequence_pair_pos[neg_seq[i]].second = static_cast<int>(i);
    accumulated_length[i] = 0;
  }

  for (size_t i = 0; i < pos_seq.size(); i++) {
    const int macro_id = reversed_pos_seq[i];
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;
    T& macro = macros[macro_id];

    if (!macro.isFixed()) {
      macro.setY(accumulated_length[neg_seq_pos]);
    }

    const int current_height = macro.getY() + macro.getHeight();
    for (size_t j = neg_seq_pos; j < neg_seq.size(); j++) {
      if (current_height > accumulated_length[j]) {
        accumulated_length[j] = current_height;
      } else {
        break;
      }
    }
  }

  const int height = accumulated_length[pos_seq.size() - 1];
  return {width, height};
}

class TestAnnealerCore : public MplTest
{
 protected:
  void SetUp() override
  {
    MplTest::SetUp();
    db_->getTech()->setLefUnits(1000);
    db_->getTech()->setManufacturingGrid(10);

    block_ = db_->getChip()->getBlock();
    block_->setDefUnits(1000);
    block_->setCoreArea(odb::Rect(0, 0, die_width_, die_height_));

    tree_ = std::make_unique<PhysicalHierarchy>();
    tree_->die_area = odb::Rect(0, 0, die_width_, die_height_);
    tree_->floorplan_shape = odb::Rect(0, 0, die_width_, die_height_);
    tree_->root = std::make_unique<Cluster>(0, "root", &logger_);
    tree_->root->setClusterType(MixedCluster);
    auto root_soft = std::make_unique<SoftMacro>(tree_->root.get());
    root_soft->setShapeF(die_width_, die_height_);
    root_soft->setLocationF(0, 0);
    tree_->root->setSoftMacro(std::move(root_soft));
  }

  odb::dbBlock* block_ = nullptr;
  std::unique_ptr<PhysicalHierarchy> tree_;
};

// Verifies that SoftMacro::restoreState and SACoreSoftMacro move undo preserve
// exact x, y, width, height, and int64_t area_ even when height_ = area_ /
// width_ has integer truncation and after setWidth, setHeight, or
// resizeRandomly mutate area_.
TEST_F(TestAnnealerCore, SoftMacroMoveUndoRestoresExactDimensionsAndArea)
{
  auto mixed_cluster = std::make_unique<Cluster>(1, "mixed_1", &logger_);
  mixed_cluster->setClusterType(MixedCluster);

  auto macro_cluster = std::make_unique<Cluster>(2, "macro_2", &logger_);
  macro_cluster->setClusterType(HardMacroCluster);

  auto io_cluster = std::make_unique<Cluster>(3, "io_3", &logger_);
  io_cluster->setAsClusterOfUnplacedIOPins({0, 1000}, 1, 2000, false);

  // Use prime area 100003 so area / width truncates (width * height != area).
  const int64_t initial_mixed_area = 100003;
  SoftMacro mixed_macro(mixed_cluster.get());
  mixed_macro.setShapes({Interval(100, 200), Interval(250, 400)},
                        initial_mixed_area);
  mixed_macro.setLocationF(15, 25);
  ASSERT_EQ(mixed_macro.getWidth(), 100);
  ASSERT_EQ(mixed_macro.getHeight(), 1000);
  ASSERT_EQ(mixed_macro.getArea(), initial_mixed_area);
  ASSERT_NE(
      static_cast<int64_t>(mixed_macro.getWidth()) * mixed_macro.getHeight(),
      initial_mixed_area);

  // 1. Verify SoftMacro::restoreState after setWidth, setHeight,
  // and resizeRandomly.
  const int saved_x = mixed_macro.getX();
  const int saved_y = mixed_macro.getY();
  const int saved_w = mixed_macro.getWidth();
  const int saved_h = mixed_macro.getHeight();
  const int64_t saved_area = mixed_macro.getArea();

  mixed_macro.setWidth(300);
  mixed_macro.setLocationF(999, 888);
  EXPECT_NE(mixed_macro.getArea(), initial_mixed_area);
  mixed_macro.restoreState(saved_x, saved_y, saved_w, saved_h, saved_area);
  EXPECT_EQ(mixed_macro.getX(), 15);
  EXPECT_EQ(mixed_macro.getY(), 25);
  EXPECT_EQ(mixed_macro.getWidth(), 100);
  EXPECT_EQ(mixed_macro.getHeight(), 1000);
  EXPECT_EQ(mixed_macro.getArea(), initial_mixed_area);

  mixed_macro.setHeight(350);
  mixed_macro.setLocationF(777, 666);
  EXPECT_NE(mixed_macro.getArea(), initial_mixed_area);
  mixed_macro.restoreState(saved_x, saved_y, saved_w, saved_h, saved_area);
  EXPECT_EQ(mixed_macro.getX(), 15);
  EXPECT_EQ(mixed_macro.getY(), 25);
  EXPECT_EQ(mixed_macro.getWidth(), 100);
  EXPECT_EQ(mixed_macro.getHeight(), 1000);
  EXPECT_EQ(mixed_macro.getArea(), initial_mixed_area);

  std::mt19937 gen(7);
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  mixed_macro.resizeRandomly(dist, gen);
  mixed_macro.restoreState(saved_x, saved_y, saved_w, saved_h, saved_area);
  EXPECT_EQ(mixed_macro.getWidth(), 100);
  EXPECT_EQ(mixed_macro.getHeight(), 1000);
  EXPECT_EQ(mixed_macro.getArea(), initial_mixed_area);

  // 2. Verify SACoreSoftMacro move undo through public SACoreSoftMacro API when
  // rejected moves (both resize and sequence swaps) trigger saveState /
  // restoreState while skipping trailing fixed IO macros.
  SoftMacro hard_cluster_macro(macro_cluster.get());
  hard_cluster_macro.setShapes({Tiling(120, 300), Tiling(300, 120)},
                               /*force=*/true);
  hard_cluster_macro.setLocationF(50, 60);

  SoftMacro fixed_io_macro(
      odb::Point(0, 1000), "io_term", 1, 2000, io_cluster.get());

  std::vector<SoftMacro> macros
      = {mixed_macro, hard_cluster_macro, fixed_io_macro};

  SACoreWeights core_weights{1.0f, 10.0f, 5.0f, 0.0f, 0.0f};
  SASoftWeights soft_weights{1.0f, 1.0f, 1.0f};

  // Use a small outline (50x50) with disallowInvalidStates() so every perturbed
  // state in initialize() is outside the outline (!isValid()) and rejected,
  // forcing saveState() + perturb() + restoreState() on all 50 steps.
  SACoreSoftMacro sa(tree_.get(),
                     odb::Rect(0, 0, 50, 50),
                     macros,
                     core_weights,
                     soft_weights,
                     10.0f,
                     10.0f,
                     /*pos_swap_prob=*/0.15f,
                     /*neg_swap_prob=*/0.15f,
                     /*double_swap_prob=*/0.1f,
                     /*exchange_prob=*/0.1f,
                     /*resize_prob=*/0.5f,
                     0.9f,
                     20,
                     50,
                     42,
                     nullptr,
                     &logger_,
                     block_);
  sa.setNumberOfSequencePairMacros(2);
  sa.setInitialSequencePair({{0, 1}, {0, 1}});
  sa.disallowInvalidStates();
  sa.initialize();

  const std::vector<SoftMacro> final_macros = sa.getMacros();
  ASSERT_EQ(final_macros.size(), 3u);

  for (size_t i = 0; i < 2; ++i) {
    EXPECT_EQ(final_macros[i].getX(), macros[i].getX());
    EXPECT_EQ(final_macros[i].getY(), macros[i].getY());
    EXPECT_EQ(final_macros[i].getWidth(), macros[i].getWidth());
    EXPECT_EQ(final_macros[i].getHeight(), macros[i].getHeight());
    EXPECT_EQ(final_macros[i].getArea(), macros[i].getArea());
  }

  // Trailing fixed IO macro remains untouched.
  EXPECT_EQ(final_macros[2].getX(), 0);
  EXPECT_EQ(final_macros[2].getY(), 1000);
}

// Verifies SACoreHardMacro move undo restores movable macro (x, y) and sequence
// pair while skipping trailing fixed IO macros.
TEST_F(TestAnnealerCore, HardMacroMoveUndoRestoresCoordinatesAndSequencePair)
{
  auto c1 = std::make_unique<Cluster>(1, "c1", &logger_);
  auto c2 = std::make_unique<Cluster>(2, "c2", &logger_);
  auto c3 = std::make_unique<Cluster>(3, "io", &logger_);

  HardMacro m1(odb::Point(10, 20), "m1", 1000, 2000, c1.get());
  HardMacro m2(odb::Point(30, 40), "m2", 1500, 2500, c2.get());
  HardMacro io_term(odb::Point(0, 500), "io_term", 0, 0, c3.get());

  std::vector<HardMacro> macros = {m1, m2, io_term};
  SACoreWeights core_weights{1.0f, 10.0f, 5.0f, 0.0f, 0.0f};

  // Use a 50x50 outline with disallowInvalidStates() so every perturbation in
  // initialize() is rejected, exercising saveState() and restoreState() 50
  // times across all sequence-pair swap actions.
  SACoreHardMacro sa(tree_.get(),
                     odb::Rect(0, 0, 50, 50),
                     macros,
                     core_weights,
                     0.25f,
                     0.25f,
                     0.25f,
                     0.25f,
                     0.9f,
                     20,
                     50,
                     123,
                     nullptr,
                     &logger_,
                     block_);
  sa.setNumberOfSequencePairMacros(2);
  sa.setInitialSequencePair({{0, 1}, {0, 1}});
  sa.disallowInvalidStates();
  sa.initialize();

  const std::vector<HardMacro> final_macros = sa.getMacros();
  ASSERT_EQ(final_macros.size(), 3u);

  EXPECT_EQ(final_macros[0].getX(), 10);
  EXPECT_EQ(final_macros[0].getY(), 20);
  EXPECT_EQ(final_macros[1].getX(), 30);
  EXPECT_EQ(final_macros[1].getY(), 40);
  EXPECT_EQ(final_macros[2].getX(), 0);
  EXPECT_EQ(final_macros[2].getY(), 500);

  // After packFloorplan(), verify sequence pair ({0, 1}, {0, 1}) was preserved
  // by checking packed locations: m1 at (0, 0), m2 at (1000, 0).
  sa.packFloorplan();
  const std::vector<HardMacro> packed_macros = sa.getMacros();
  EXPECT_EQ(packed_macros[0].getX(), 0);
  EXPECT_EQ(packed_macros[0].getY(), 0);
  EXPECT_EQ(packed_macros[1].getX(), 1000);
  EXPECT_EQ(packed_macros[1].getY(), 0);
}

// Verifies O(n log n) Fenwick-tree packFloorplan produces bit-identical macro
// (x, y) coordinates and floorplan (width_, height_) compared to the reference
// O(n^2) algorithm across randomized permutations and fixed macros.
TEST_F(TestAnnealerCore, PackFloorplanFenwickBitExactWithFixedAndMovableMacros)
{
  odb::dbLib* lib = db_->findLib("lib");
  odb::dbMaster* master = odb::dbMaster::create(lib, "fixed_master");
  master->setType(odb::dbMasterType::BLOCK);
  master->setWidth(4000);
  master->setHeight(3000);
  master->setFrozen();

  odb::dbInst* fixed_inst = odb::dbInst::create(block_, master, "fixed_inst");
  fixed_inst->setLocation(12000, 8000);
  fixed_inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);

  HardMacro fixed_hard(fixed_inst, HardMacro::Halo{});
  const odb::Rect outline(0, 0, die_width_, die_height_);

  const int num_macros = 16;
  std::vector<std::unique_ptr<Cluster>> clusters;
  std::vector<SoftMacro> initial_macros;
  std::mt19937 rng(2026);
  std::uniform_int_distribution<int> dim_dist(500, 15000);

  for (int i = 0; i < num_macros; ++i) {
    if (i == 5) {
      // Fixed macro inside the sequence pair.
      initial_macros.emplace_back(&logger_, &fixed_hard, &outline);
    } else {
      auto cluster = std::make_unique<Cluster>(
          i + 1, "c_" + std::to_string(i), &logger_);
      cluster->setClusterType(HardMacroCluster);
      SoftMacro sm(cluster.get());
      sm.setShapeF(dim_dist(rng), dim_dist(rng));
      initial_macros.push_back(sm);
      clusters.push_back(std::move(cluster));
    }
  }

  SACoreWeights core_weights{1.0f, 1.0f, 1.0f, 0.0f, 0.0f};
  SASoftWeights soft_weights{0.0f, 0.0f, 0.0f};

  SACoreSoftMacro sa(tree_.get(),
                     outline,
                     initial_macros,
                     core_weights,
                     soft_weights,
                     10.0f,
                     10.0f,
                     0.25f,
                     0.25f,
                     0.25f,
                     0.25f,
                     0.0f,
                     0.9f,
                     10,
                     10,
                     1,
                     nullptr,
                     &logger_,
                     block_);

  std::vector<int> pos_seq(num_macros);
  std::vector<int> neg_seq(num_macros);
  for (int i = 0; i < num_macros; ++i) {
    pos_seq[i] = i;
    neg_seq[i] = i;
  }

  for (int trial = 0; trial < 100; ++trial) {
    std::shuffle(pos_seq.begin(), pos_seq.end(), rng);
    std::shuffle(neg_seq.begin(), neg_seq.end(), rng);

    sa.setInitialSequencePair({pos_seq, neg_seq});
    sa.packFloorplan();

    const std::vector<SoftMacro> packed_macros = sa.getMacros();
    std::vector<SoftMacro> ref_macros = packed_macros;
    const auto [ref_w, ref_h]
        = packFloorplanReference(pos_seq, neg_seq, ref_macros);

    ASSERT_EQ(sa.getWidth(), ref_w) << "Trial " << trial;
    ASSERT_EQ(sa.getHeight(), ref_h) << "Trial " << trial;
    for (int i = 0; i < num_macros; ++i) {
      ASSERT_EQ(packed_macros[i].getX(), ref_macros[i].getX())
          << "Trial " << trial << " macro " << i;
      ASSERT_EQ(packed_macros[i].getY(), ref_macros[i].getY())
          << "Trial " << trial << " macro " << i;
    }
  }
}

// Verifies computeDistToNearestRegion single-region overload and
// SimulatedAnnealingCore::setNets / computeWLForClusterOfUnplacedIOPins with
// flat io_cluster_constraints_by_id_ lookup and precomputed nets_weight_sum_.
TEST_F(TestAnnealerCore,
       WirelengthPrecomputedWeightSumAndSingleConstraintRegion)
{
  const BoundaryRegion region(odb::Line(0, 100, 0, 400), Boundary::L);
  const std::vector<BoundaryRegion> region_vec = {region};

  for (int x : {0, 50, 500, 12345}) {
    for (int y : {0, 50, 250, 400, 900}) {
      const odb::Point pt(x, y);
      odb::Point nearest_single;
      odb::Point nearest_vec;
      const int64_t dist_single
          = computeDistToNearestRegion(pt, region, &nearest_single);
      const int64_t dist_vec
          = computeDistToNearestRegion(pt, region_vec, &nearest_vec);
      EXPECT_EQ(dist_single, dist_vec);
      EXPECT_EQ(nearest_single, nearest_vec);
    }
  }

  auto c0 = std::make_unique<Cluster>(1, "c0", &logger_);
  c0->setClusterType(HardMacroCluster);
  auto c1 = std::make_unique<Cluster>(2, "c1", &logger_);
  c1->setClusterType(HardMacroCluster);
  auto io_cluster = std::make_unique<Cluster>(5, "constrained_io", &logger_);
  io_cluster->setAsClusterOfUnplacedIOPins(
      {0, 100}, 1, 300, /*is_unconstrained=*/false);

  tree_->io_cluster_to_constraint[io_cluster.get()] = region;

  SoftMacro m0(c0.get());
  m0.setShapes({Tiling(200, 200)}, /*force=*/true);
  SoftMacro m1(c1.get());
  m1.setShapes({Tiling(400, 200)}, /*force=*/true);
  SoftMacro io_macro(odb::Point(0, 100), "io_m", 1, 300, io_cluster.get());

  std::vector<SoftMacro> macros = {m0, m1, io_macro};
  SACoreWeights core_weights{1.0f, 1.0f, 10.0f, 0.0f, 0.0f};
  SASoftWeights soft_weights{0.0f, 0.0f, 0.0f};

  SACoreSoftMacro sa(tree_.get(),
                     odb::Rect(0, 0, die_width_, die_height_),
                     macros,
                     core_weights,
                     soft_weights,
                     10.0f,
                     10.0f,
                     /*pos_swap_prob=*/0.0f,
                     /*neg_swap_prob=*/0.0f,
                     /*double_swap_prob=*/0.0f,
                     /*exchange_prob=*/0.0f,
                     /*resize_prob=*/1.0f,
                     0.9f,
                     /*max_num_step=*/1,
                     /*num_perturb_per_step=*/1,
                     9,
                     nullptr,
                     &logger_,
                     block_);
  sa.setNumberOfSequencePairMacros(2);
  sa.setInitialSequencePair({{0, 1}, {0, 1}});

  BundledNetList nets = {
      BundledNet(0, 1, 2.0f),
      BundledNet(1, 2, 3.0f),
  };
  sa.setNets(nets);
  sa.initialize();

  // With sequence pair ({0, 1}, {0, 1}):
  // m0 is at (0, 0) with size 200x200 -> pin at (100, 100).
  // m1 is at (200, 0) with size 400x200 -> pin at (400, 100).
  // Net(0, 1) HPWL = |400 - 100| + |100 - 100| = 300, weighted = 300 * 2 = 600.
  // Net(1, 2) connects m1 pin (400, 100) to constrained IO region (0, 100)-(0,
  // 400): nearest point on region is (0, 100), distance = 400, weighted = 400 *
  // 3 = 1200. Normalized by nets_weight_sum_ (5.0) * (die_width_ + die_height_)
  // (1000000): 1800 / (5 * 1000000) = 0.00036.
  const float expected_wl = 1800.0f / (5.0f * (die_width_ + die_height_));
  EXPECT_FLOAT_EQ(sa.getWirelength(), expected_wl);
  EXPECT_TRUE(std::isfinite(sa.getNormCost()));
}

// Verifies MacroPlacer::setRandomSeed changes placement deterministically and
// resets to 0 on subsequent calls when not re-specified.
TEST_F(TestAnnealerCore, RandomSeedChangesPlacementAndResetsAcrossCalls)
{
  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      db_->getTech(), "M1", odb::dbTechLayerType::ROUTING);
  layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track = odb::dbTrackGrid::create(block_, layer);
  track->addGridPatternY(0, die_height_ / 100, 100);

  odb::dbLib* lib = db_->findLib("lib");
  odb::dbMaster* macro_master = odb::dbMaster::create(lib, "macro_m");
  macro_master->setType(odb::dbMasterType::BLOCK);
  macro_master->setWidth(80000);
  macro_master->setHeight(60000);
  odb::dbMTerm* mterm = odb::dbMTerm::create(
      macro_master, "p1", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin = odb::dbMPin::create(mterm);
  odb::dbBox::create(mpin, layer, 0, 0, 100, 100);
  macro_master->setFrozen();

  std::vector<odb::dbInst*> macros;
  for (int i = 0; i < 6; ++i) {
    odb::dbInst* inst = odb::dbInst::create(
        block_, macro_master, ("M" + std::to_string(i)).c_str());
    macros.push_back(inst);
  }

  // Connect macros in a chain so wirelength depends on macro arrangement.
  for (int i = 0; i < 5; ++i) {
    odb::dbNet* net
        = odb::dbNet::create(block_, ("n" + std::to_string(i)).c_str());
    macros[i]->findITerm("p1")->connect(net);
    macros[i + 1]->findITerm("p1")->connect(net);
  }

  MacroPlacer placer(db_.get(), nullptr, &logger_, nullptr);

  auto unfixAll = [&]() {
    for (odb::dbInst* inst : macros) {
      inst->setPlacementStatus(odb::dbPlacementStatus::UNPLACED);
      inst->setLocation(0, 0);
    }
  };

  auto captureLocations = [&]() {
    std::vector<odb::Point> locs;
    for (odb::dbInst* inst : macros) {
      locs.push_back(inst->getLocation());
    }
    return locs;
  };

  auto runPlace = [&]() {
    const std::string report_dir = ::testing::TempDir();
    placer.place(/*num_threads=*/1,
                 /*max_num_macro=*/0,
                 /*min_num_macro=*/0,
                 /*max_num_inst=*/0,
                 /*min_num_inst=*/0,
                 /*tolerance=*/0.1f,
                 /*max_num_level=*/1,
                 /*coarsening_ratio=*/10.0f,
                 /*large_net_threshold=*/50,
                 odb::Rect(0, 0, 0, 0),
                 /*area_weight=*/0.1f,
                 /*outline_weight=*/100.0f,
                 /*wirelength_weight=*/100.0f,
                 /*guidance_weight=*/10.0f,
                 /*fence_weight=*/10.0f,
                 /*boundary_weight=*/50.0f,
                 /*notch_weight=*/50.0f,
                 /*soft_blockage_weight=*/10.0f,
                 /*target_util=*/0.25f,
                 /*min_ar=*/0.33f,
                 report_dir.c_str(),
                 /*keep_clustering_data=*/false,
                 /*pin_aware_channels=*/false);
  };

  // Run 1: default seed (0).
  unfixAll();
  runPlace();
  const std::vector<odb::Point> locs_default_1 = captureLocations();

  // Run 2: explicit non-zero seed that explores a different SA trajectory.
  unfixAll();
  placer.setRandomSeed(7);
  runPlace();
  const std::vector<odb::Point> locs_seed_7 = captureLocations();

  // Run 3: repeat explicit seed 7 -> must be identical to Run 2.
  unfixAll();
  placer.setRandomSeed(7);
  runPlace();
  const std::vector<odb::Point> locs_seed_7_repeat = captureLocations();
  EXPECT_EQ(locs_seed_7, locs_seed_7_repeat);

  // Run 4: omit setRandomSeed -> must reset to default seed 0 and match Run 1.
  unfixAll();
  runPlace();
  const std::vector<odb::Point> locs_default_after_reset = captureLocations();
  EXPECT_EQ(locs_default_1, locs_default_after_reset);

  // Run 5: set explicit anneal_end_acceptance and anneal_patience, run place(),
  // then omit both on Run 6 and verify they reset to defaults (matching Run 1).
  unfixAll();
  placer.setAnnealEndAcceptance(0.0001f);
  placer.setAnnealPatience(0.2f);
  runPlace();

  unfixAll();
  runPlace();
  const std::vector<odb::Point> locs_after_anneal_opts_reset
      = captureLocations();
  EXPECT_EQ(locs_default_1, locs_after_anneal_opts_reset);

  // Also verify that changing random_seed changes the annealing trajectory on
  // a multi-macro SACoreHardMacro instance.
  auto c1 = std::make_unique<Cluster>(1, "c1", &logger_);
  auto c2 = std::make_unique<Cluster>(2, "c2", &logger_);
  auto c3 = std::make_unique<Cluster>(3, "c3", &logger_);
  auto c4 = std::make_unique<Cluster>(4, "c4", &logger_);
  std::vector<HardMacro> hm_list = {
      HardMacro(odb::Point(0, 0), "hm1", 40000, 30000, c1.get()),
      HardMacro(odb::Point(0, 0), "hm2", 50000, 40000, c2.get()),
      HardMacro(odb::Point(0, 0), "hm3", 60000, 25000, c3.get()),
      HardMacro(odb::Point(0, 0), "hm4", 35000, 45000, c4.get()),
  };
  SACoreWeights weights{1.0f, 10.0f, 5.0f, 0.0f, 0.0f};
  auto runSaWithSeed = [&](int seed) {
    SACoreHardMacro sa(tree_.get(),
                       odb::Rect(0, 0, die_width_, die_height_),
                       hm_list,
                       weights,
                       0.25f,
                       0.25f,
                       0.25f,
                       0.25f,
                       0.9f,
                       30,
                       10,
                       seed,
                       nullptr,
                       &logger_,
                       block_);
    sa.setNumberOfSequencePairMacros(4);
    sa.setInitialSequencePair({{0, 1, 2, 3}, {0, 1, 2, 3}});
    sa.initialize();
    sa.run();
    const std::vector<HardMacro> out = sa.getMacros();
    std::vector<odb::Point> pts;
    for (const HardMacro& hm : out) {
      pts.emplace_back(hm.getX(), hm.getY());
    }
    return pts;
  };
  EXPECT_NE(runSaWithSeed(0), runSaWithSeed(7));
}

// Verifies MPL-0003 error message reports the child macro minimum area as a
// percentage of outline area when coarse shaping finds no valid tilings.
TEST_F(TestAnnealerCore, Mpl0003ReportsMacroAreaPercentageOfOutline)
{
  odb::dbLib* lib = db_->findLib("lib");

  odb::dbMaster* std_master = odb::dbMaster::create(lib, "std_cell");
  std_master->setType(odb::dbMasterType::CORE);
  std_master->setWidth(1000);
  std_master->setHeight(1000);
  std_master->setFrozen();

  odb::dbMaster* macro_master_1 = odb::dbMaster::create(lib, "big_macro_1");
  macro_master_1->setType(odb::dbMasterType::BLOCK);
  macro_master_1->setWidth(300000);
  macro_master_1->setHeight(300000);
  macro_master_1->setFrozen();

  odb::dbMaster* macro_master_2 = odb::dbMaster::create(lib, "big_macro_2");
  macro_master_2->setType(odb::dbMasterType::BLOCK);
  macro_master_2->setWidth(300000);
  macro_master_2->setHeight(300000);
  macro_master_2->setFrozen();

  // Core area is 500000 x 500000 = 2.5e11 DBU^2.
  // Two movable macros of distinct masters (300000x300000 each) form two
  // separate HardMacroCluster children under the MixedCluster root.
  // Sum of macro areas = 1.8e11 <= 2.5e11 (72.0% of outline area), so MPL-0065
  // and MPL-0016 pass, and each macro fits in 500000x500000 individually, but
  // two 300000x300000 blocks cannot pack without overlap inside 500000x500000,
  // triggering MPL-0003 in calculateChildrenTilings.
  odb::dbInst::create(block_, macro_master_1, "M1");
  odb::dbInst::create(block_, macro_master_2, "M2");
  odb::dbInst::create(block_, std_master, "U0");

  MacroPlacer placer(db_.get(), nullptr, &logger_, nullptr);
  const std::string report_dir = ::testing::TempDir();

  testing::internal::CaptureStdout();
  EXPECT_LOGGER_ERROR((placer.place(/*num_threads=*/1,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0.1f,
                                    1,
                                    10.0f,
                                    50,
                                    odb::Rect(0, 0, 0, 0),
                                    0.1f,
                                    100.0f,
                                    100.0f,
                                    10.0f,
                                    10.0f,
                                    50.0f,
                                    50.0f,
                                    10.0f,
                                    0.25f,
                                    0.33f,
                                    report_dir.c_str(),
                                    false,
                                    false)),
                      "MPL-0003");
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("[ERROR MPL-0003]"), std::string::npos);
  EXPECT_NE(output.find("72.0%"), std::string::npos) << "Actual: " << output;
}

// Verifies MacroPlacer::setAnnealEndAcceptance and setAnnealPatience validate
// bounds with MPL-0081 and MPL-0082.
TEST_F(TestAnnealerCore, AnnealEndAcceptanceAndPatienceValidation)
{
  MacroPlacer placer(db_.get(), nullptr, &logger_, nullptr);

  EXPECT_NO_THROW(placer.setAnnealEndAcceptance(0.0001f));
  EXPECT_NO_THROW(placer.setAnnealEndAcceptance(0.5f));
  EXPECT_LOGGER_ERROR(placer.setAnnealEndAcceptance(0.0f), "MPL-0081");
  EXPECT_LOGGER_ERROR(placer.setAnnealEndAcceptance(-0.1f), "MPL-0081");
  EXPECT_LOGGER_ERROR(placer.setAnnealEndAcceptance(0.9f), "MPL-0081");
  EXPECT_LOGGER_ERROR(placer.setAnnealEndAcceptance(1.0f), "MPL-0081");

  EXPECT_NO_THROW(placer.setAnnealPatience(0.0f));
  EXPECT_NO_THROW(placer.setAnnealPatience(0.2f));
  EXPECT_NO_THROW(placer.setAnnealPatience(1.0f));
  EXPECT_LOGGER_ERROR(placer.setAnnealPatience(-0.01f), "MPL-0082");
  EXPECT_LOGGER_ERROR(placer.setAnnealPatience(1.01f), "MPL-0082");
}

}  // namespace
}  // namespace mpl
