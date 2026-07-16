// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db_sta/dbNetwork.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/SdcClass.hh"
#include "sta/Search.hh"
#include "sta/Units.hh"
#include "tst/IntegratedFixture.h"

namespace est {

class TestEstimateParasitics : public tst::IntegratedFixture
{
 protected:
  TestEstimateParasitics()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/est/test/")
  {
  }

  sta::Pin* findTopPin(const char* port_name) const
  {
    sta::Instance* top_inst = db_network_->topInstance();
    sta::Cell* top_cell = db_network_->cell(top_inst);
    if (top_cell == nullptr) {
      ADD_FAILURE() << "missing top cell";
      return nullptr;
    }

    sta::Port* port = db_network_->findPort(top_cell, port_name);
    if (port == nullptr) {
      ADD_FAILURE() << "missing top port " << port_name;
      return nullptr;
    }

    sta::Pin* pin = db_network_->findPin(top_inst, port);
    if (pin == nullptr) {
      ADD_FAILURE() << "missing top pin " << port_name;
      return nullptr;
    }
    return pin;
  }

  sta::Net* flatNet(sta::Pin* pin) const
  {
    odb::dbNet* db_net = nullptr;
    if (db_network_->isTopLevelPort(pin)) {
      db_net = db_network_->flatNet(db_network_->term(pin));
    } else {
      db_net = db_network_->flatNet(pin);
    }
    if (db_net == nullptr) {
      ADD_FAILURE() << "missing flat net for " << db_network_->pathName(pin);
      return nullptr;
    }

    sta::Net* net = db_network_->dbToSta(db_net);
    if (net == nullptr) {
      ADD_FAILURE() << "missing sta net for " << db_net->getName();
      return nullptr;
    }
    return net;
  }

  void makeClock(const char* clock_name, sta::Pin* pin) const
  {
    sta::PinSet pins(db_network_);
    pins.insert(pin);

    const double period = sta_->units()->timeUnit()->userToSta(1.0);
    sta::FloatSeq waveform;
    waveform.push_back(0.0);
    waveform.push_back(period / 2.0);

    sta_->makeClock(
        clock_name, pins, false, period, waveform, "", sta_->cmdMode());
  }

  void resizeDff(const char* inst_name)
  {
    odb::dbInst* db_inst = block_->findInst(inst_name);
    ASSERT_NE(db_inst, nullptr) << "missing instance " << inst_name;

    sta::Instance* inst = db_network_->dbToSta(db_inst);
    ASSERT_NE(inst, nullptr) << "missing sta instance " << inst_name;

    sta::LibertyCell* dff_x2 = sta_->network()->findLibertyCell("DFF_X2");
    ASSERT_NE(dff_x2, nullptr);
    ASSERT_TRUE(resizer_.replaceCell(inst, dff_x2));
  }
};

// Verifies that an ideal clock net can be present in the incremental
// parasitic invalidation set without forcing STA delay invalidation.
//
// DFF resizing may mark the clock net parasitics invalid. For an ideal clock,
// those parasitics do not contribute to clock arrival/slew, so updateParasitics
// should skip both RC re-estimation and delaysInvalidFromFanin() for that net.
TEST_F(TestEstimateParasitics, IdealClockNetSkipsStaInvalidation)
{
  // Build a small clocked design and seed valid timing/parasitic state.
  readVerilogAndSetup("TestEstimateParasitics.v");
  sta_->updateTiming(true);

  // Use the default ideal clock from IntegratedFixture::initStaDefaultSdc().
  sta::Pin* clk_pin = findTopPin("clk");
  ASSERT_NE(clk_pin, nullptr);
  sta::Net* clk_net = flatNet(clk_pin);
  ASSERT_NE(clk_net, nullptr);

  // Model the ECO source: resizing a DFF is the class of netlist edit that can
  // make the clock net appear in the parasitic invalidation set.
  resizeDff("reg0");

  // Clear any ordinary resize-related STA invalidation so the assertions below
  // measure only updateParasitics() side effects.
  sta_->updateTiming(true);
  ASSERT_TRUE(sta_->search()->arrivalsValid());

  // Seed the exact condition under test: an ideal clock net is pending in
  // EstimateParasitics' incremental invalidation set.
  ep_.setParasiticsSrc(ParasiticsSrc::kPlacement);
  ep_.setIncrementalParasiticsEnabled(true);
  ep_.parasiticsInvalid(clk_net);
  ASSERT_TRUE(ep_.hasParasiticsInvalid());

  ep_.updateParasitics();

  // A regression calls sta_->delaysInvalidFromFanin(clk_net), which invalidates
  // the top clock port and every ideal CK load vertex.
  EXPECT_TRUE(sta_->search()->arrivalsValid());
  ep_.setIncrementalParasiticsEnabled(false);
}

// Verifies multi-mode ideal-clock classification for scan clocks.
//
// The original implementation rejected a pin if isIdealClock(pin, mode) was
// false in any mode. That is wrong for scan clocks that are only created in a
// test mode: the scan pin is not a clock in function mode, so that mode must be
// ignored. The fixed logic first checks isClock(pin, mode), then requires ideal
// status only in modes where the pin is actually a clock.
TEST_F(TestEstimateParasitics, ScanClockIdealOnlyInTestMode)
{
  // Do not create the default SDC. This test constructs function/test modes
  // explicitly so scan_clk is intentionally absent from function mode.
  readVerilogAndSetup("TestEstimateParasitics.v", false);

  // Create two modes, but create scan_clk only in test mode.
  sta_->setCmdMode("function");
  sta::Mode* function_mode = sta_->cmdMode();
  sta_->setCmdMode("test");
  sta::Mode* test_mode = sta_->cmdMode();

  sta::Pin* scan_clk_pin = findTopPin("scan_clk");
  ASSERT_NE(scan_clk_pin, nullptr);
  makeClock("scan_clk", scan_clk_pin);

  // This is the exact multi-mode condition being guarded:
  // scan_clk is not a function-mode clock, but it is an ideal test-mode clock.
  ASSERT_FALSE(sta_->isClock(scan_clk_pin, function_mode));
  ASSERT_TRUE(sta_->isClock(scan_clk_pin, test_mode));
  ASSERT_TRUE(sta_->isIdealClock(scan_clk_pin, test_mode));

  // Build timing/parasitics after the mode-specific clock setup is complete.
  sta_->ensureGraph();
  sta_->ensureLevelized();
  resizer_.initBlock();
  ep_.estimateWireParasitics();
  sta_->updateTiming(true);

  sta::Net* scan_clk_net = flatNet(scan_clk_pin);
  ASSERT_NE(scan_clk_net, nullptr);

  // Isolate the invalidation caused by updateParasitics().
  ASSERT_TRUE(sta_->search()->arrivalsValid());

  // Seed the scan clock net as invalid. A buggy all-modes ideal-clock check
  // treats this net as non-ideal because function mode has no scan clock.
  ep_.setParasiticsSrc(ParasiticsSrc::kPlacement);
  ep_.setIncrementalParasiticsEnabled(true);
  ep_.parasiticsInvalid(scan_clk_net);
  ASSERT_TRUE(ep_.hasParasiticsInvalid());

  ep_.updateParasitics();

  // If non-clock modes are not ignored, updateParasitics() invalidates the
  // scan clock port and scan_reg/CK through delaysInvalidFromFanin().
  EXPECT_TRUE(sta_->search()->arrivalsValid());
  ep_.setIncrementalParasiticsEnabled(false);
}

// Verifies that wire RC values are stored per chip: chip-specific values take
// precedence over the defaults, and chips without an entry use the defaults.
TEST_F(TestEstimateParasitics, WireRcPerChip)
{
  readVerilogAndSetup("TestEstimateParasitics.v");

  sta::Scene* scene = sta_->scenes().front();
  odb::dbChip* chip1 = db_->getChip();
  ASSERT_NE(chip1, nullptr);

  // A null chip sets the default values used by chips without an entry.
  ep_.initChip(chip1);
  ep_.setHWireSignalRC(nullptr, scene, 1.0e3, 1.0e-10);
  ep_.setVWireSignalRC(nullptr, scene, 2.0e3, 2.0e-10);
  EXPECT_DOUBLE_EQ(ep_.wireSignalHResistance(scene), 1.0e3);
  EXPECT_DOUBLE_EQ(ep_.wireSignalVCapacitance(scene), 2.0e-10);

  // A second chip on its own technology with chip-specific values.
  loadTechAndLib(
      "tech2", "lib2", getFilePath("_main/test/Nangate45/Nangate45.lef"));
  odb::dbTech* tech2 = db_->findTech("tech2");
  ASSERT_NE(tech2, nullptr);
  odb::dbChip* chip2 = odb::dbChip::create(
      db_.get(), tech2, "chip2", odb::dbChip::ChipType::DIE);
  ASSERT_NE(chip2, nullptr);
  odb::dbBlock::create(chip2, "chip2_block");
  ep_.setHWireSignalRC(chip2, scene, 3.0e3, 3.0e-10);
  ep_.setVWireSignalRC(chip2, scene, 4.0e3, 4.0e-10);

  // The chip-specific values do not leak into the default-valued chip.
  EXPECT_DOUBLE_EQ(ep_.wireSignalHResistance(scene), 1.0e3);

  // Rebinding to the second chip resolves its chip-specific values.
  ep_.initChip(chip2);
  EXPECT_DOUBLE_EQ(ep_.wireSignalHResistance(scene), 3.0e3);
  EXPECT_DOUBLE_EQ(ep_.wireSignalVCapacitance(scene), 4.0e-10);
  EXPECT_DOUBLE_EQ(ep_.wireClkHResistance(scene), 0.0);

  // Rebinding back to a chip without an entry falls back to the defaults.
  ep_.initChip(chip1);
  EXPECT_DOUBLE_EQ(ep_.wireSignalHResistance(scene), 1.0e3);
  EXPECT_DOUBLE_EQ(ep_.wireSignalVResistance(scene), 2.0e3);
}

}  // namespace est
