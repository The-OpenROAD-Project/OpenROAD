// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// compute_skew_budget shifts register clock latencies during its LP and then
// restores the original annotations. A pin whose set_clock_latency is
// asymmetric (min != max) must come back unchanged; a regression once
// collapsed it to a single value, corrupting the hold (min) latency.

#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "tst/IntegratedFixture.h"

namespace rsz {

class SkewBudgetRestoreTest : public tst::IntegratedFixture
{
 protected:
  SkewBudgetRestoreTest()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/rsz/test/")
  {
  }

  void SetUp() override
  {
    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    odb::defin def_reader(db_.get(), &logger_, odb::defin::DEFAULT);
    std::vector<odb::dbLib*> search_libs;
    for (odb::dbLib* lib : db_->getLibs()) {
      search_libs.push_back(lib);
    }
    def_reader.readChip(
        search_libs,
        getFilePath("_main/src/rsz/test/repair_hold1.def").c_str(),
        chip);
    block_ = db_->getChip()->getBlock();
    sta_->postReadDef(block_);

    // create_clock -period 2 clk
    sta::PinSet clk_pins(db_network_);
    clk_pins.insert(db_network_->findPin("clk"));
    const float period = ns(2.0);
    sta_->makeClock("clk",
                    clk_pins,
                    /*add_to_pins=*/false,
                    period,
                    {0, period / 2},
                    /*comment=*/"",
                    sta_->cmdMode());
  }

  float ns(const double value) const
  {
    return sta_->units()->timeUnit()->userToSta(value);
  }

  float pinLatency(sta::Clock* clk, sta::Pin* pin, const sta::MinMax* min_max)
  {
    float latency = 0.0f;
    bool exists = false;
    sta_->cmdMode()->sdc()->clockLatency(
        clk, pin, sta::RiseFall::rise(), min_max, latency, exists);
    EXPECT_TRUE(exists);
    return latency;
  }
};

TEST_F(SkewBudgetRestoreTest, AsymmetricPinLatencyPreserved)
{
  sta::Sdc* sdc = sta_->cmdMode()->sdc();
  sta::Pin* ck = db_network_->findPin("r3/CK");
  ASSERT_NE(ck, nullptr);
  sta::Clock* clk = sdc->findClock("clk");
  ASSERT_NE(clk, nullptr);

  // set_clock_latency -rise -min 0.02 / -max 0.10 on r3/CK
  const float min_latency = ns(0.02);
  const float max_latency = ns(0.10);
  sta_->setClockLatency(clk,
                        ck,
                        sta::RiseFallBoth::rise(),
                        sta::MinMaxAll::min(),
                        min_latency,
                        sdc);
  sta_->setClockLatency(clk,
                        ck,
                        sta::RiseFallBoth::rise(),
                        sta::MinMaxAll::max(),
                        max_latency,
                        sdc);

  const float min_before = pinLatency(clk, ck, sta::MinMax::min());
  const float max_before = pinLatency(clk, ck, sta::MinMax::max());

  sta_->ensureGraph();
  sta_->ensureLevelized();
  resizer_.initBlock();
  ep_.estimateWireParasitics();

  Resizer::SkewBudgetOptions opts;
  opts.max_iterations = 1;
  resizer_.computeSkewBudget(opts);

  // The asymmetric annotation must survive unchanged.
  EXPECT_FLOAT_EQ(pinLatency(clk, ck, sta::MinMax::min()), min_before);
  EXPECT_FLOAT_EQ(pinLatency(clk, ck, sta::MinMax::max()), max_before);
}

}  // namespace rsz
