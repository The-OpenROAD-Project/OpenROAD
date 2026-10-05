// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// repair_hold on a driver whose hold-violating loads differ in setup slack.
// Design: repair_hold_shared_driver.def
//
//   r1 --+-- u1 --+-- rA/D    setup slack: large
//        |        +-- rB/D    setup slack: tight
//        |        +-- rD/D    setup slack: large
//        +-- rC/D             worst setup slack (unrelated to u1)
//
// set_min_delay makes u1's loads violate hold. A hold buffer delays every load
// moved behind it, so the setup guard must judge u1 by its tightest load.
// rC/D owns the worst setup slack, so the worstSlack() check alone cannot see
// a moved load that breaks.

#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "tst/IntegratedFixture.h"

namespace rsz {

class RepairHoldSharedDriverTest : public tst::IntegratedFixture
{
 protected:
  RepairHoldSharedDriverTest()
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
        getFilePath("_main/src/rsz/test/repair_hold_shared_driver.def").c_str(),
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
    setDelay("rC/D", sta::MinMax::max(), 0.0);
  }

  float ns(const double value) const
  {
    return sta_->units()->timeUnit()->userToSta(value);
  }

  // set_max_delay / set_min_delay <delay_ns> -to <pin_name>
  void setDelay(const char* pin_name,
                const sta::MinMax* min_max,
                const double delay_ns)
  {
    sta::Sdc* sdc = sta_->cmdMode()->sdc();
    auto* to_pins = new sta::PinSet(db_network_);
    to_pins->insert(db_network_->findPin(pin_name));
    sta::ExceptionTo* to = sta_->makeExceptionTo(to_pins,
                                                 /*to_clks=*/nullptr,
                                                 /*to_insts=*/nullptr,
                                                 sta::RiseFallBoth::riseFall(),
                                                 sta::RiseFallBoth::riseFall(),
                                                 sdc);
    sta_->makePathDelay(/*from=*/nullptr,
                        /*thrus=*/nullptr,
                        to,
                        min_max,
                        /*ignore_clk_latency=*/false,
                        /*break_path=*/false,
                        ns(delay_ns),
                        /*comment=*/"",
                        sdc);
  }

  void repairHold()
  {
    sta_->ensureGraph();
    sta_->ensureLevelized();
    resizer_.initBlock();
    ep_.estimateWireParasitics();
    resizer_.repairHold(/*setup_margin=*/0.0,
                        /*hold_margin=*/0.0,
                        /*allow_setup_violations=*/false,
                        /*max_buffer_percent=*/1.0,
                        /*max_passes=*/10000,
                        /*max_iterations=*/-1,
                        /*match_cell_footprint=*/false,
                        /*verbose=*/false);
  }

  sta::Slack setupSlack(const char* pin_name)
  {
    sta::Pin* pin = db_network_->findPin(pin_name);
    return sta_->slack(sta_->graph()->pinLoadVertex(pin), sta::MinMax::max());
  }

  odb::dbNet* netOf(const char* iterm_name)
  {
    return block_->findITerm(iterm_name)->getNet();
  }
};

//   u1 --+-- rA/D    hold violated, setup slack large
//        +-- rB/D    hold violated, setup slack below a buffer delay
//        +-- rD/D    hold violated, setup slack large
//
// A buffer in front of all three would push rB/D setup negative. The old
// guard judged u1 by its loosest load, buffered all three and broke rB/D
// setup; worstSlack() did not see it because rC/D owns the worst slack.
TEST_F(RepairHoldSharedDriverTest, KeepsSetupOfTightestLoad)
{
  setDelay("rA/D", sta::MinMax::min(), 0.125);
  setDelay("rD/D", sta::MinMax::min(), 0.125);
  setDelay("rB/D", sta::MinMax::min(), 0.11);
  setDelay("rB/D", sta::MinMax::max(), 0.16);
  repairHold();

  EXPECT_EQ(netOf("rB/D"), netOf("u1/Z"));  // not buffered
  for (const char* load : {"rA/D", "rB/D", "rD/D"}) {
    EXPECT_GT(setupSlack(load), 0.0) << load;
  }
}

}  // namespace rsz
