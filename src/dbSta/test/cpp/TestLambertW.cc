// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include "DmpCeffLambertWDelayCalc.hh"
#include "db_sta/dbSta.hh"
#include "sta/DelayCalc.hh"
#include "tst/IntegratedFixture.h"

namespace sta {

class TestLambertW : public tst::IntegratedFixture
{
 protected:
  TestLambertW()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/dbSta/test/")
  {
  }
};

TEST_F(TestLambertW, ValidateRandomPImodels)
{
  readVerilogAndSetup("TestDbSta_0.v");

  std::unique_ptr<DmpCeffLambertWDelayCalc> lambert_calc
      = std::make_unique<DmpCeffLambertWDelayCalc>(sta_.get());
  std::unique_ptr<DmpCeffTwoPoleDelayCalc> twopole_calc
      = std::make_unique<DmpCeffTwoPoleDelayCalc>(sta_.get());

  const Scene* scene = sta_->cmdScene();
  const MinMax* min_max = MinMax::max();
  const RiseFall* rf = RiseFall::rise();

  Pin* drvr_pin = db_network_->findPin("buf/Z");
  Pin* load_pin = db_network_->findPin("load/A");
  ASSERT_NE(drvr_pin, nullptr);
  ASSERT_NE(load_pin, nullptr);

  Instance* inst = db_network_->instance(drvr_pin);
  LibertyCell* cell = db_network_->libertyCell(inst);
  ASSERT_NE(cell, nullptr);
  TimingArcSet* arc_set = cell->timingArcSets().front();
  ASSERT_NE(arc_set, nullptr);
  TimingArc* arc = arc_set->arcs().front();
  ASSERT_NE(arc, nullptr);

  LoadPinIndexMap load_pin_map(db_network_);
  load_pin_map[load_pin] = 0;

  Parasitics* parasitics = scene->parasitics(min_max);
  ASSERT_NE(parasitics, nullptr);

  std::mt19937 gen(42);
  std::uniform_real_distribution<float> dis_c2(1e-15f, 50e-15f);
  std::uniform_real_distribution<float> dis_rpi(10.0f, 500.0f);
  std::uniform_real_distribution<float> dis_c1(1e-15f, 50e-15f);
  std::uniform_real_distribution<float> dis_slew(0.01e-9f, 0.5e-9f);

  const Net* net = db_network_->net(drvr_pin);
  ASSERT_NE(net, nullptr);

  static constexpr int kNumSamples = 10000;
  std::vector<float> rel_errors;
  rel_errors.reserve(kNumSamples);
  int count_under_6pct = 0;
  int count_under_10pct = 0;

  for (int i = 0; i < kNumSamples; ++i) {
    float c2 = dis_c2(gen);
    float rpi = dis_rpi(gen);
    float c1 = dis_c1(gen);

    // Rejection sampling for physically realistic layout parasitics:
    // 1. Physical wire constraint: C2 must be >= half the wire capacitance
    // associated with Rpi (min c/r ratio ~0.04 fF/Ohm)
    float min_c2_from_rpi = 0.5f * rpi * 0.04e-15f;
    if (c2 < min_c2_from_rpi) {
      --i;
      continue;
    }

    // 2. Capacitance ratio constraint: For long wires (Rpi > 100 Ohm), y =
    // C2/(C1+C2) is bounded in [0.20, 0.80]
    float y_ratio = c2 / (c1 + c2);
    if (rpi > 100.0f && (y_ratio < 0.20f || y_ratio > 0.80f)) {
      --i;
      continue;
    }

    Slew in_slew(dis_slew(gen));

    // Construct a detailed RC network representing the Pi model
    Parasitic* pnet = parasitics->makeParasiticNetwork(net, false);
    ParasiticNode* drvr_node
        = parasitics->ensureParasiticNode(pnet, drvr_pin, db_network_);
    ParasiticNode* load_node
        = parasitics->ensureParasiticNode(pnet, load_pin, db_network_);

    parasitics->incrCap(drvr_node, c2);
    parasitics->incrCap(load_node, c1);
    parasitics->makeResistor(pnet, 1, rpi, drvr_node, load_node);

    // Reduce the RC network using OpenSTA's parasitic reduction engine
    Parasitic* parasitic = parasitics->reduceToPiPoleResidue2(
        pnet, drvr_pin, rf, scene, min_max);
    ASSERT_NE(parasitic, nullptr);

    float load_cap = c1 + c2;

    ArcDcalcResult res_lambert = lambert_calc->gateDelay(drvr_pin,
                                                         arc,
                                                         in_slew,
                                                         load_cap,
                                                         parasitic,
                                                         load_pin_map,
                                                         scene,
                                                         min_max);

    ArcDcalcResult res_twopole = twopole_calc->gateDelay(drvr_pin,
                                                         arc,
                                                         in_slew,
                                                         load_cap,
                                                         parasitic,
                                                         load_pin_map,
                                                         scene,
                                                         min_max);

    float gd_lambert = delayAsFloat(res_lambert.gateDelay());
    float gd_twopole = delayAsFloat(res_twopole.gateDelay());

    float diff = std::abs(gd_lambert - gd_twopole);
    float max_val = std::max(std::abs(gd_lambert), std::abs(gd_twopole));

    if (max_val > 0.0f) {
      float rel_err = diff / max_val;
      rel_errors.push_back(rel_err);
      if (rel_err < 0.06f) {
        count_under_6pct++;
      }
      if (rel_err < 0.10f) {
        count_under_10pct++;
      }
    }

    parasitics->deleteParasiticNetwork(net);
    parasitics->deleteDrvrReducedParasitics(drvr_pin);
  }

  std::sort(rel_errors.begin(), rel_errors.end());
  float mean_err = 0.0f;
  for (float err : rel_errors) {
    mean_err += err;
  }
  mean_err /= rel_errors.size();

  float p95_err = rel_errors[static_cast<size_t>(0.95 * rel_errors.size())];
  float p99_err = rel_errors[static_cast<size_t>(0.99 * rel_errors.size())];
  float max_err = rel_errors.back();

  std::cout << "\n=======================================================\n";
  std::cout << " [ STATISTICAL ANALYSIS ] 10,000 Random Pi Models\n";
  std::cout << "-------------------------------------------------------\n";
  std::cout << "  Mean Relative Error           : " << mean_err * 100.0f
            << "%\n";
  std::cout << "  95th Percentile Relative Error: " << p95_err * 100.0f
            << "%\n";
  std::cout << "  99th Percentile Relative Error: " << p99_err * 100.0f
            << "%\n";
  std::cout << "  Max Relative Error            : " << max_err * 100.0f
            << "%\n";
  std::cout << "  Samples with Error < 6.0%     : "
            << (count_under_6pct * 100.0f / rel_errors.size()) << "%\n";
  std::cout << "  Samples with Error < 10.0%    : "
            << (count_under_10pct * 100.0f / rel_errors.size()) << "%\n";
  std::cout << "=======================================================\n\n";

  EXPECT_LT(p99_err, 0.15);
}
}  // namespace sta
