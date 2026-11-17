// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-, The OpenROAD Authors

#include "src/gpl/src/mbff.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "ant/AntennaChecker.hh"
#include "db_sta/dbReadVerilog.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "src/gpl/src/graphicsNone.h"
#include "stt/SteinerTreeBuilder.h"
#include "tst/fixture.h"
#include "utl/ServiceRegistry.h"

namespace gpl {

class MBFFTestPeer
{
 public:
  static bool IsValidTray(MBFF* uut, odb::dbInst* tray)
  {
    return uut->IsValidTray(tray);
  }

  static bool HaveSameMask(MBFF* uut, odb::dbInst* first, odb::dbInst* second)
  {
    const MBFF::Mask first_mask = uut->GetArrayMask(first, true);
    const MBFF::Mask second_mask = uut->GetArrayMask(second, true);
    return !(first_mask < second_mask) && !(second_mask < first_mask);
  }

  static void ReadLibs(MBFF* uut) { uut->ReadLibs(); }

  static void KMeans(MBFF* uut,
                     const std::vector<Flop>& flops,
                     int knn,
                     std::vector<std::vector<Flop>>& clusters,
                     const std::vector<int>& rand_nums)
  {
    uut->KMeans(flops, knn, clusters, rand_nums);
  }

  static float GetSilh(MBFF* uut,
                       const std::vector<Flop>& flops,
                       const std::vector<Tray>& trays,
                       const std::vector<std::pair<int, int>>& clusters)
  {
    return uut->GetSilh(flops, trays, clusters);
  }

  static float GetKSilh(MBFF* uut,
                        const std::vector<std::vector<Flop>>& clusters,
                        const std::vector<Point>& centers)
  {
    return uut->GetKSilh(clusters, centers);
  }

  static void KMeansDecomp(MBFF* uut,
                           const std::vector<Flop>& flops,
                           int max_sz,
                           std::vector<std::vector<Flop>>& pointsets)
  {
    uut->KMeansDecomp(flops, max_sz, pointsets);
  }
};

namespace {

class MBFFTestFixture : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    logger_ = getLogger();
    service_registry_ = std::make_unique<utl::ServiceRegistry>(logger_);
    verilog_network_ = std::make_unique<ord::dbVerilogNetwork>(getSta());
    stt_builder_ = std::make_unique<stt::SteinerTreeBuilder>(logger_);
    antenna_checker_ = std::make_unique<ant::AntennaChecker>(getDb(), logger_);
    opendp_ = std::make_unique<dpl::Opendp>(getDb(), logger_);
    global_router_
        = std::make_unique<grt::GlobalRouter>(logger_,
                                              service_registry_.get(),
                                              stt_builder_.get(),
                                              getDb(),
                                              getSta(),
                                              antenna_checker_.get(),
                                              opendp_.get());
    estimate_parasitics_
        = std::make_unique<est::EstimateParasitics>(logger_,
                                                    service_registry_.get(),
                                                    getDb(),
                                                    getSta(),
                                                    stt_builder_.get(),
                                                    global_router_.get());

    resizer_ = std::make_unique<rsz::Resizer>(getLogger(),
                                              getDb(),
                                              getSta(),
                                              stt_builder_.get(),
                                              global_router_.get(),
                                              opendp_.get(),
                                              estimate_parasitics_.get());

    loadTechAndLib("test0",
                   "test0",
                   getFilePath("openroad/src/gpl/test/library/test/test0.lef"));
    readLiberty(getFilePath("openroad/src/gpl/test/library/test/test0.lib"));

    chip_ = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip_, "top");
    block_->setDefUnits(1000);
    block_->setDieArea(odb::Rect(0, 0, 10000, 10000));

    mbff_
        = std::make_unique<MBFF>(getDb(),
                                 getSta(),
                                 logger_,
                                 resizer_.get(),
                                 /*threads=*/1,
                                 /*multistart=*/20,
                                 /*num_paths=*/0,
                                 /*debug_graphics=*/false,
                                 /*graphics=*/std::make_unique<GraphicsNone>());
  }

  // Create a cell insidde the main block of database db.
  odb::dbInst* CreateTmpCell(const char* name,
                             const char* lib_name,
                             const char* master_name)
  {
    // Find a unique name.
    int64_t idx = 0;
    std::string cell_name = name;

    while (block_->findInst(cell_name.c_str()) != nullptr) {
      cell_name = absl::StrCat(name, ++idx);
    }

    odb::dbLib* lib = db_->findLib(lib_name);
    odb::dbMaster* master = lib->findMaster(master_name);
    return odb::dbInst::create(block_, master, cell_name.c_str());
  }

  utl::Logger* logger_;
  std::unique_ptr<utl::ServiceRegistry> service_registry_;
  std::unique_ptr<ord::dbVerilogNetwork> verilog_network_;
  std::unique_ptr<stt::SteinerTreeBuilder> stt_builder_;
  std::unique_ptr<ant::AntennaChecker> antenna_checker_;
  std::unique_ptr<dpl::Opendp> opendp_;
  std::unique_ptr<grt::GlobalRouter> global_router_;
  std::unique_ptr<est::EstimateParasitics> estimate_parasitics_;
  std::unique_ptr<rsz::Resizer> resizer_;
  std::unique_ptr<MBFF> mbff_;

  odb::dbLib* lib_;
  odb::dbChip* chip_;
  odb::dbBlock* block_;
};

TEST_F(MBFFTestFixture, FlopsCanBeIdentifiedAsATrayAndNot)
{
  // Retreive masters, create a test cell, and assert that they are correctly
  // identified as either a tray or not a tray.
  EXPECT_EQ(db_->findLib("test0")->getMasters().size(), 7);

  EXPECT_FALSE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "INV")));
  EXPECT_FALSE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "DFF")));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "MBFF2")));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "MBFF2SE")));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "MBFF2CLPS")));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "MBFF2SECLPS")));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff_.get(), CreateTmpCell("test_tray", "test0", "MBLATCH2")));
}

TEST_F(MBFFTestFixture, RegisterAndLatchTraysUseDifferentMasks)
{
  // Given equivalent register-bank and latch-bank tray interfaces.
  odb::dbInst* register_tray = CreateTmpCell("register_tray", "test0", "MBFF2");
  odb::dbInst* latch_tray = CreateTmpCell("latch_tray", "test0", "MBLATCH2");

  // Then their sequential behavior must keep them in separate candidate pools.
  EXPECT_FALSE(
      MBFFTestPeer::HaveSameMask(mbff_.get(), register_tray, latch_tray));
}

TEST_F(MBFFTestFixture, ReadLibsSuccessfullyProcessesTestCells)
{
  // In test0.lib, cells like MBFF2SE have their sequential definition
  // nested inside a test_cell block. Without consistent Liberty cell views,
  // GetPinMapping returns empty vectors and triggers an out-of-bounds crash.
  EXPECT_NO_FATAL_FAILURE(MBFFTestPeer::ReadLibs(mbff_.get()));
}

TEST_F(MBFFTestFixture, KMeansHandlesColocatedFlopsWithoutCrashing)
{
  // Prior to legalization (dpl), flip-flops in global placement can sit on
  // top of each other at identical (x, y) coordinates (or have fewer than knn
  // unique locations), making tot_sum == 0 in KMeans. This must not crash.
  std::vector<Flop> flops = {
      Flop{.pt = Point{.x = 100.0, .y = 100.0}, .idx = 0, .prob = 0.0},
      Flop{.pt = Point{.x = 100.0, .y = 100.0}, .idx = 1, .prob = 0.0},
      Flop{.pt = Point{.x = 100.0, .y = 100.0}, .idx = 2, .prob = 0.0},
      Flop{.pt = Point{.x = 100.0, .y = 100.0}, .idx = 3, .prob = 0.0},
  };
  std::vector<std::vector<Flop>> clusters;
  std::vector<int> rand_nums = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  EXPECT_NO_FATAL_FAILURE(
      MBFFTestPeer::KMeans(mbff_.get(), flops, /*knn=*/2, clusters, rand_nums));
  ASSERT_EQ(clusters.size(), 2);
  // KMeans appends the cluster center to the back of each cluster vector.
  int total_assigned = 0;
  for (auto& cluster : clusters) {
    ASSERT_FALSE(cluster.empty());
    cluster.pop_back();  // remove center
    total_assigned += cluster.size();
  }
  EXPECT_EQ(total_assigned, flops.size());
}

TEST_F(MBFFTestFixture, KMeansHandlesColocatedFlopsWithMultipleClusters)
{
  std::vector<Flop> flops = {
      Flop{.pt = Point{.x = 50.0, .y = 50.0}, .idx = 0, .prob = 0.0},
      Flop{.pt = Point{.x = 50.0, .y = 50.0}, .idx = 1, .prob = 0.0},
      Flop{.pt = Point{.x = 50.0, .y = 50.0}, .idx = 2, .prob = 0.0},
      Flop{.pt = Point{.x = 50.0, .y = 50.0}, .idx = 3, .prob = 0.0},
      Flop{.pt = Point{.x = 50.0, .y = 50.0}, .idx = 4, .prob = 0.0},
  };
  std::vector<std::vector<Flop>> clusters;
  std::vector<int> rand_nums = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

  EXPECT_NO_FATAL_FAILURE(
      MBFFTestPeer::KMeans(mbff_.get(), flops, /*knn=*/3, clusters, rand_nums));
  ASSERT_EQ(clusters.size(), 3);
  int total_assigned = 0;
  for (auto& cluster : clusters) {
    ASSERT_FALSE(cluster.empty());
    cluster.pop_back();
    total_assigned += cluster.size();
  }
  EXPECT_EQ(total_assigned, flops.size());
}

TEST_F(MBFFTestFixture,
       KMeansHandlesLargeDistancesWithoutOverflowOrSamplingBias)
{
  // Place flop 0 at origin and flops 1..3 far enough away that
  // tot_sum * 100.0 exceeds INT_MAX (2,147,483,647):
  //   d[1]^2 = 1e8, d[2]^2 = 1e8, d[3]^2 = 1e8 -> tot_sum = 3e8,
  //   raw_scaled_sum = 3e10 > INT_MAX.
  // With rand_hi = 25 and rand_lo = 13456 (both < 32767),
  // safe_val = (25 << 15) ^ 13456 = 831200 (fraction = 0.8312, prob
  // = 2.4936e8), so KMeans++ samples past cum_sum = 2e8 and selects flop 3 at
  // (0, 10000).
  const std::vector<Flop> flops = {
      Flop{Point{0.0f, 0.0f}, 0, 0.0f},
      Flop{Point{10000.0f, 0.0f}, 1, 0.0f},
      Flop{Point{10000.0f, 0.0f}, 2, 0.0f},
      Flop{Point{0.0f, 10000.0f}, 3, 0.0f},
  };
  const std::vector<int> rand_nums = {0, 25, 13456};
  std::vector<std::vector<Flop>> clusters;

  MBFFTestPeer::KMeans(mbff_.get(), flops, /*knn=*/2, clusters, rand_nums);

  ASSERT_EQ(clusters.size(), 2);
  EXPECT_GT(clusters[1].back().pt.y, 0.0f);
}

TEST_F(MBFFTestFixture, KMeansHandlesNegativeRandNums)
{
  // Verify next_rand() masks the sign bit (& 0x7FFFFFFF) so INT_MIN
  // (-2147483648) does not trigger std::abs(INT_MIN) UB and negative entries
  // preserve the lower 31 bits.
  const std::vector<Flop> flops = {
      Flop{Point{0.0f, 0.0f}, 0, 0.0f},
      Flop{Point{100.0f, 0.0f}, 1, 0.0f},
      Flop{Point{100.0f, 0.0f}, 2, 0.0f},
      Flop{Point{0.0f, 100.0f}, 3, 0.0f},
  };
  const std::vector<int> rand_nums
      = {std::numeric_limits<int>::min(), -2147483648 + 2500000};
  std::vector<std::vector<Flop>> clusters;

  MBFFTestPeer::KMeans(mbff_.get(), flops, /*knn=*/2, clusters, rand_nums);

  ASSERT_EQ(clusters.size(), 2);
  EXPECT_GT(clusters[1].back().pt.y, 0.0f);
}

TEST_F(MBFFTestFixture, KMeansHandlesKnnGreaterThanNumFlopsAndEmptyInput)
{
  const std::vector<int> rand_nums(20, 3);
  std::vector<std::vector<Flop>> clusters;

  // Empty input should return immediately without division by zero.
  MBFFTestPeer::KMeans(mbff_.get(), {}, /*knn=*/4, clusters, rand_nums);
  EXPECT_TRUE(clusters.empty());

  // When knn > num_flops, KMeans must cap centers at actual_knn = num_flops
  // instead of spinning in while (chosen.size() < knn).
  const std::vector<Flop> small_flops = {
      Flop{Point{0.0f, 0.0f}, 0, 0.0f},
      Flop{Point{10.0f, 0.0f}, 1, 0.0f},
      Flop{Point{20.0f, 0.0f}, 2, 0.0f},
  };
  MBFFTestPeer::KMeans(
      mbff_.get(), small_flops, /*knn=*/8, clusters, rand_nums);
  ASSERT_EQ(clusters.size(), 3);

  // KMeansDecomp with num_flops < 8 and max_sz < num_flops must also succeed.
  const std::vector<Flop> five_flops = {
      Flop{Point{0.0f, 0.0f}, 0, 0.0f},
      Flop{Point{1.0f, 0.0f}, 1, 0.0f},
      Flop{Point{100.0f, 0.0f}, 2, 0.0f},
      Flop{Point{101.0f, 0.0f}, 3, 0.0f},
      Flop{Point{200.0f, 0.0f}, 4, 0.0f},
  };
  std::vector<std::vector<Flop>> pointsets;
  MBFFTestPeer::KMeansDecomp(mbff_.get(), five_flops, /*max_sz=*/2, pointsets);
  int total_decomposed = 0;
  for (const auto& ps : pointsets) {
    EXPECT_LE(ps.size(), 2);
    total_decomposed += ps.size();
  }
  EXPECT_EQ(total_decomposed, 5);
}

TEST_F(MBFFTestFixture,
       KMeansHandlesExtremeDistancesExceedingInt64MaxAndShortRandNums)
{
  // Place flops at 1e10 so d^2 = 1e20 and tot_sum * 100 > INT64_MAX (~9.22e18).
  // Checking raw_scaled_sum in double before casting avoids float-to-integer
  // overflow UB even for extreme unnormalized coordinates.
  const std::vector<Flop> extreme_flops = {
      Flop{Point{0.0f, 0.0f}, 0, 0.0f},
      Flop{Point{1e10f, 0.0f}, 1, 0.0f},
      Flop{Point{0.0f, 1e10f}, 2, 0.0f},
  };
  // Also test short and empty rand_nums vectors to verify wrap-around safety.
  std::vector<std::vector<Flop>> clusters;
  MBFFTestPeer::KMeans(mbff_.get(),
                       extreme_flops,
                       /*knn=*/3,
                       clusters,
                       /*rand_nums=*/{1});
  ASSERT_EQ(clusters.size(), 3);

  MBFFTestPeer::KMeans(mbff_.get(),
                       extreme_flops,
                       /*knn=*/2,
                       clusters,
                       /*rand_nums=*/{});
  ASSERT_EQ(clusters.size(), 2);
}

TEST_F(MBFFTestFixture, GetKSilhHandlesColocatedFlopsWithoutNaN)
{
  // Co-located flops across multiple clusters yield a_j == 0 and b_j == 0.
  const std::vector<std::vector<Flop>> clusters = {
      {Flop{Point{10.0f, 10.0f}, 0, 0.0f}, Flop{Point{10.0f, 10.0f}, 1, 0.0f}},
      {Flop{Point{10.0f, 10.0f}, 2, 0.0f}, Flop{Point{10.0f, 10.0f}, 3, 0.0f}},
  };
  const std::vector<Point> centers = {
      Point{10.0f, 10.0f},
      Point{10.0f, 10.0f},
  };

  const float silh = MBFFTestPeer::GetKSilh(mbff_.get(), clusters, centers);
  EXPECT_FALSE(std::isnan(silh));
  EXPECT_TRUE(std::isfinite(silh));
  EXPECT_FLOAT_EQ(silh, 0.0f);
}

TEST_F(MBFFTestFixture, GetSilhHandlesColocatedSlotsAndSingleTrayWithoutNaN)
{
  const std::vector<Flop> flops = {
      Flop{Point{5.0f, 5.0f}, 0, 0.0f},
      Flop{Point{5.0f, 5.0f}, 1, 0.0f},
  };
  const Tray colocated_tray{
      Point{5.0f, 5.0f}, {Point{5.0f, 5.0f}, Point{5.0f, 5.0f}}, {0, 1}};

  // Case 1: Multiple trays with co-located slots (max_den == 0.0f).
  const std::vector<Tray> two_trays = {colocated_tray, colocated_tray};
  const std::vector<std::pair<int, int>> two_tray_clusters = {{0, 0}, {1, 0}};
  const float silh_zero_den
      = MBFFTestPeer::GetSilh(mbff_.get(), flops, two_trays, two_tray_clusters);
  EXPECT_FALSE(std::isnan(silh_zero_den));
  EXPECT_TRUE(std::isfinite(silh_zero_den));
  EXPECT_FLOAT_EQ(silh_zero_den, 0.0f);

  // Case 2: Single tray (num_trays == 1, min_num remains float::max()).
  const std::vector<Tray> single_tray = {
      Tray{Point{0.0f, 0.0f}, {Point{0.0f, 0.0f}, Point{10.0f, 0.0f}}, {0, 1}}};
  const std::vector<std::pair<int, int>> single_tray_clusters
      = {{0, 1}, {0, 0}};
  const float silh_single_tray = MBFFTestPeer::GetSilh(
      mbff_.get(), flops, single_tray, single_tray_clusters);
  EXPECT_FALSE(std::isnan(silh_single_tray));
  EXPECT_TRUE(std::isfinite(silh_single_tray));
  EXPECT_FLOAT_EQ(silh_single_tray, 0.0f);
}

}  // namespace
}  // namespace gpl
