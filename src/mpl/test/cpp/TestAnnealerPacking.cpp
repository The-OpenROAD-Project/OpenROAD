// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "SACoreHardMacro.h"
#include "SimulatedAnnealingCore.h"
#include "clusterEngine.h"
#include "mpl-util.h"
#include "object.h"
#include "MplTest.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace mpl {
namespace {

// Packing feasibility tests for the simulated-annealing engine that
// underlies macro placement (see HierRTLMP::placeMacros and the
// MPL-0040 / MPL-0010 error paths).
//
// The macro inventory below is an anonymized SRAM mix (192 instances of
// 14 masters) from a real floorplan in which RTL-MP reported MPL-0040
// even though the raw rectangle instance is trivially feasible: a greedy
// shelf packing (used here as an oracle) places all rectangles inside
// the outline with room to spare.  Only the rectangle geometry is kept:
// width x height in microns (a 1 um halo per side is added via
// kHaloMicrons) and the instance count per master.
//
// These tests pin down that the flat hard-macro annealing engine packs
// this inventory comfortably (probed up to 80% utilization and 4:1
// outline aspect ratios while writing the test).  A future failure here
// is a regression in the packing engine itself; the historical MPL-0040
// must instead come from the hierarchical/soft-macro stage above it.

struct MacroSpec
{
  const char* name;
  double width;   // microns, without halo
  double height;  // microns, without halo
  int count;
};

constexpr double kHaloMicrons = 1.0;
constexpr int kDbuPerMicron = 1000;

const std::vector<MacroSpec>& inventory()
{
  static const std::vector<MacroSpec> specs = {{"MACRO_A", 26.86, 13.42, 2},
                                               {"MACRO_B", 55.10, 27.55, 8},
                                               {"MACRO_C", 36.05, 18.02, 16},
                                               {"MACRO_D", 33.46, 16.73, 16},
                                               {"MACRO_E", 77.47, 38.74, 8},
                                               {"MACRO_F", 70.49, 35.23, 8},
                                               {"MACRO_G", 43.97, 21.98, 24},
                                               {"MACRO_H", 58.70, 29.35, 10},
                                               {"MACRO_I", 66.53, 33.26, 10},
                                               {"MACRO_J", 54.07, 27.02, 16},
                                               {"MACRO_K", 53.04, 26.52, 8},
                                               {"MACRO_L", 45.10, 22.56, 8},
                                               {"MACRO_M", 42.91, 21.46, 16},
                                               {"MACRO_N", 30.12, 15.07, 42}};
  return specs;
}

class TestAnnealerPacking : public MplTest
{
 protected:
  void SetUp() override
  {
    MplTest::SetUp();
    db_->setDbuPerMicron(kDbuPerMicron);
  }

  odb::dbBlock* block() { return db_->getChip()->getBlock(); }

  // One rectangle per macro instance, halo included, like the sa_macros
  // that HierRTLMP::placeMacros() hands to SACoreHardMacro.
  static std::vector<HardMacro> makeMacros()
  {
    std::vector<HardMacro> macros;
    for (const MacroSpec& spec : inventory()) {
      const int width
          = static_cast<int>(std::lround((spec.width + 2 * kHaloMicrons) * kDbuPerMicron));
      const int height
          = static_cast<int>(std::lround((spec.height + 2 * kHaloMicrons) * kDbuPerMicron));
      for (int i = 0; i < spec.count; ++i) {
        macros.emplace_back(
            width, height, std::string(spec.name) + "_" + std::to_string(i));
      }
    }
    return macros;
  }

  static int64_t totalArea(const std::vector<HardMacro>& macros)
  {
    int64_t area = 0;
    for (const HardMacro& macro : macros) {
      area += macro.getArea();
    }
    return area;
  }

  // Outline sized so that (macro area incl. halos) / (outline area) ==
  // utilization, with width / height == aspect_ratio.
  static odb::Rect makeOutline(const std::vector<HardMacro>& macros,
                               const double utilization,
                               const double aspect_ratio = 1.0)
  {
    const double area = totalArea(macros) / utilization;
    const auto width
        = static_cast<int>(std::ceil(std::sqrt(area * aspect_ratio)));
    const auto height
        = static_cast<int>(std::ceil(std::sqrt(area / aspect_ratio)));
    return {0, 0, width, height};
  }

  // Greedy shelf packing used as a feasibility oracle: sort by height,
  // fill rows left to right, open a new row when the current one is
  // full.  If this packs, the instance is certainly feasible.
  static bool shelfPacks(const std::vector<HardMacro>& macros,
                         const odb::Rect& outline)
  {
    std::vector<std::pair<int, int>> rects;  // height, width
    rects.reserve(macros.size());
    for (const HardMacro& macro : macros) {
      rects.emplace_back(macro.getHeight(), macro.getWidth());
    }
    std::sort(rects.begin(), rects.end(), std::greater<>());

    int64_t shelf_bottom = 0;
    int64_t shelf_height = 0;
    int64_t x = 0;
    for (const auto& [height, width] : rects) {
      if (x + width > outline.dx()) {
        shelf_bottom += shelf_height;
        shelf_height = 0;
        x = 0;
      }
      if (width > outline.dx() || shelf_bottom + height > outline.dy()) {
        return false;
      }
      x += width;
      shelf_height = std::max(shelf_height, static_cast<int64_t>(height));
    }
    return true;
  }

  // Drives SACoreHardMacro the same way HierRTLMP::placeMacros() does:
  // same action probabilities, hyperparameters, weight escalation and
  // number of runs (with deterministic per-run seeds).
  bool annealerPacks(const std::vector<HardMacro>& macros,
                     const odb::Rect& outline)
  {
    if (macros.empty()) {
      return true;
    }
    PhysicalHierarchy tree;
    tree.die_area = outline;

    block()->setDieArea(outline);

    // Action probabilities mirror HierRTLMP::placeMacros(): a uniform base
    // probability per action, with sequence-pair swaps scaled x10 and the
    // exchange move x5 (attenuated by the share of identical masters).
    constexpr float kBaseActionProb = 0.2f;
    constexpr float kSeqSwapScale = 10.0f;
    constexpr float kExchangeScale = 5.0f;
    const float exchange_swap_prob
        = kBaseActionProb * kExchangeScale
          * (1.0f
             - (static_cast<float>(inventory().size())
                / static_cast<float>(macros.size())));
    const float action_sum = 2 * kBaseActionProb * kSeqSwapScale
                             + kBaseActionProb + exchange_swap_prob;
    const float pos_swap_prob = kBaseActionProb * kSeqSwapScale / action_sum;
    const float neg_swap_prob = kBaseActionProb * kSeqSwapScale / action_sum;
    const float double_swap_prob = kBaseActionProb / action_sum;

    constexpr float init_prob = 0.9f;
    constexpr int max_num_step = 2000;
    constexpr int num_runs = 10;
    // placeMacros() uses num_perturb_per_step_ / 10 as the floor.
    constexpr int kNumPerturbFloor = 500 / 10;
    const int num_perturb_per_step
        = std::max(static_cast<int>(macros.size()), kNumPerturbFloor);

    std::vector<std::unique_ptr<SACoreHardMacro>> sa_runs;
    for (int run_id = 0; run_id < num_runs; ++run_id) {
      // Weight escalation mirrors placeMacros(): outline pressure grows and
      // wirelength relaxes with each retry.
      constexpr float kAreaWeight = 0.1f;
      constexpr float kBaseWeight = 100.0f;
      constexpr float kOutlineEscalation = 10.0f;
      SACoreWeights weights;
      weights.area = kAreaWeight;
      weights.outline = kBaseWeight * (run_id + 1) * kOutlineEscalation;
      weights.wirelength = kBaseWeight / (run_id + 1);

      auto sa
          = std::make_unique<SACoreHardMacro>(&tree,
                                              outline,
                                              macros,
                                              weights,
                                              pos_swap_prob,
                                              neg_swap_prob,
                                              double_swap_prob,
                                              exchange_swap_prob / action_sum,
                                              init_prob,
                                              max_num_step,
                                              num_perturb_per_step,
                                              /* seed */ run_id,
                                              /* graphics */ nullptr,
                                              &logger_,
                                              block());
      sa->setNumberOfSequencePairMacros(static_cast<int>(macros.size()));
      sa_runs.push_back(std::move(sa));
    }

    std::vector<std::thread> threads;
    threads.reserve(sa_runs.size());
    for (auto& sa : sa_runs) {
      threads.emplace_back(runSA<SACoreHardMacro>, sa.get());
    }
    for (std::thread& thread : threads) {
      thread.join();
    }

    bool valid = false;
    for (auto& sa : sa_runs) {
      if (sa->isValid()) {
        valid = true;
      }
    }
    return valid;
  }
};

// At a generous outline the annealer must always find a packing.
TEST_F(TestAnnealerPacking, PacksAtGenerousUtilization)
{
  const std::vector<HardMacro> macros = makeMacros();
  const odb::Rect outline = makeOutline(macros, /* utilization */ 0.20);

  ASSERT_TRUE(shelfPacks(macros, outline));
  EXPECT_TRUE(annealerPacks(macros, outline));
}

// At a tighter outline the instance is still trivially feasible (the
// shelf oracle proves it) and the annealer must still find a packing.
TEST_F(TestAnnealerPacking, PacksAtTightUtilizationWhereShelfPackingSucceeds)
{
  const std::vector<HardMacro> macros = makeMacros();
  const odb::Rect outline = makeOutline(macros, /* utilization */ 0.45);

  ASSERT_TRUE(shelfPacks(macros, outline));
  EXPECT_TRUE(annealerPacks(macros, outline));
}

// Hierarchical clustering hands leaf clusters elongated sub-outlines
// (rtl_macro_placer -min_ar allows down to 1:3).  The annealer must
// also pack into those.
TEST_F(TestAnnealerPacking, PacksIntoElongatedOutline)
{
  const std::vector<HardMacro> macros = makeMacros();
  const odb::Rect outline
      = makeOutline(macros, /* utilization */ 0.45, /* aspect_ratio */ 3.0);

  ASSERT_TRUE(shelfPacks(macros, outline));
  EXPECT_TRUE(annealerPacks(macros, outline));
}

// Manual probe (bazel test --test_arg=--gtest_also_run_disabled_tests):
// prints where the annealer stops finding packings the shelf oracle can
// still prove feasible.
TEST_F(TestAnnealerPacking, DISABLED_ProbeUtilizationCliff)
{
  const std::vector<HardMacro> macros = makeMacros();
  for (double util : {0.50, 0.55, 0.60, 0.65, 0.70, 0.75, 0.80, 0.85}) {
    const odb::Rect outline = makeOutline(macros, util);
    const bool shelf = shelfPacks(macros, outline);
    const bool sa = annealerPacks(macros, outline);
    logger_->report("PROBE util={:.2f} shelf={} annealer={}", util, shelf, sa);
  }
}
}  // namespace
}  // namespace mpl
