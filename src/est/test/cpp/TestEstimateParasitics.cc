// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db_sta/dbNetwork.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"
#include "sta/Scene.hh"
#include "sta/SdcClass.hh"
#include "sta/Search.hh"
#include "sta/Transition.hh"
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

// Verifies that set_bump_rc values are used for nets terminating on a
// dbChipBump instance, replacing the small pad connectivity resistor.
TEST_F(TestEstimateParasitics, BumpRcOnPadNet)
{
  readVerilogAndSetup("TestEstimateParasitics.v");

  sta::Scene* scene = sta_->scenes().front();
  // scan_clk drives only scan_reg/CK: a two-pin port-to-instance net.
  sta::Pin* scan_clk_pin = findTopPin("scan_clk");
  ASSERT_NE(scan_clk_pin, nullptr);
  sta::Net* net = flatNet(scan_clk_pin);
  ASSERT_NE(net, nullptr);

  // Make scan_reg a chip bump on the scan_clk net; the bump association
  // alone classifies the port-to-bump net as a pad net.
  odb::dbInst* scan_reg = block_->findInst("scan_reg");
  ASSERT_NE(scan_reg, nullptr);
  odb::dbChipRegion* region = odb::dbChipRegion::create(
      db_->getChip(), "f2f", odb::dbChipRegion::Side::FRONT, nullptr);
  ASSERT_NE(region, nullptr);
  odb::dbChipBump* bump = odb::dbChipBump::create(region, scan_reg);
  ASSERT_NE(bump, nullptr);
  bump->setNet(db_network_->staToDb(net));

  // Without bump values the legacy small connectivity resistor is used.
  ep_.estimateWireParasitic(net);
  sta::Parasitics* parasitics = scene->parasitics(sta::MinMax::max());
  sta::Parasitic* pi = parasitics->findPiElmore(
      scan_clk_pin, sta::RiseFall::rise(), sta::MinMax::max());
  ASSERT_NE(pi, nullptr);
  float c2, rpi, c1;
  parasitics->piModel(pi, c2, rpi, c1);
  EXPECT_FLOAT_EQ(rpi, 0.001f);
  // The reduced pi model includes the load pin caps; save them as baseline.
  const float pin_caps = c2 + c1;

  // With bump values the lumped bump RC is added on top of the pin caps.
  ep_.setBumpRC(scene, 2.5, 4.0e-14);
  ep_.estimateWireParasitic(net);
  pi = parasitics->findPiElmore(
      scan_clk_pin, sta::RiseFall::rise(), sta::MinMax::max());
  ASSERT_NE(pi, nullptr);
  parasitics->piModel(pi, c2, rpi, c1);
  EXPECT_FLOAT_EQ(rpi, 2.5f);
  EXPECT_NEAR(c2 + c1 - pin_caps, 4.0e-14, 1.0e-16);

  // A bump resistance below the connectivity floor is used as given.
  ep_.setBumpRC(scene, 5.0e-4, 4.0e-14);
  ep_.estimateWireParasitic(net);
  pi = parasitics->findPiElmore(
      scan_clk_pin, sta::RiseFall::rise(), sta::MinMax::max());
  ASSERT_NE(pi, nullptr);
  parasitics->piModel(pi, c2, rpi, c1);
  EXPECT_FLOAT_EQ(rpi, 5.0e-4f);

  // A bump with a recorded net is a bump terminal only on that net: with the
  // bump net pointing elsewhere, scan_clk is no longer a pad net, so the new
  // bump values are not applied. The single-net API does not delete the
  // previous annotation; the full pass below verifies it is wiped.
  odb::dbNet* d_net = scan_reg->findITerm("D")->getNet();
  ASSERT_NE(d_net, nullptr);
  bump->setNet(d_net);
  ep_.setBumpRC(scene, 7.5, 8.0e-14);
  ep_.estimateWireParasitic(net);
  pi = parasitics->findPiElmore(
      scan_clk_pin, sta::RiseFall::rise(), sta::MinMax::max());
  ASSERT_NE(pi, nullptr);
  parasitics->piModel(pi, c2, rpi, c1);
  EXPECT_FLOAT_EQ(rpi, 5.0e-4f);

  // On the recorded bump net the new lumped RC applies.
  bump->setNet(db_network_->staToDb(net));
  ep_.estimateWireParasitic(net);
  pi = parasitics->findPiElmore(
      scan_clk_pin, sta::RiseFall::rise(), sta::MinMax::max());
  ASSERT_NE(pi, nullptr);
  parasitics->piModel(pi, c2, rpi, c1);
  EXPECT_FLOAT_EQ(rpi, 7.5f);

  // A net with more than two pins never takes the two-node bump model, even
  // as the recorded bump net; it stays unannotated (unplaced, so the wire
  // estimator makes no tree) instead of dropping loads.
  bump->setNet(d_net);
  sta::Pin* d_pin = findTopPin("d");
  ASSERT_NE(d_pin, nullptr);
  ep_.estimateWireParasitic(db_network_->dbToSta(d_net));
  EXPECT_EQ(parasitics->findPiElmore(
                d_pin, sta::RiseFall::rise(), sta::MinMax::max()),
            nullptr);

  // Without a recorded net, a bump on a multi-signal-pin instance is
  // ambiguous and never classifies: reg0's two-pin q0 net stays unannotated.
  odb::dbInst* reg0 = block_->findInst("reg0");
  ASSERT_NE(reg0, nullptr);
  ASSERT_NE(odb::dbChipBump::create(region, reg0), nullptr);
  sta::Pin* q0_pin = findTopPin("q0");
  ASSERT_NE(q0_pin, nullptr);
  sta::Pin* q0_drvr = db_network_->dbToSta(reg0->findITerm("Q"));
  ASSERT_NE(q0_drvr, nullptr);
  ep_.estimateWireParasitic(flatNet(q0_pin));
  EXPECT_EQ(parasitics->findPiElmore(
                q0_drvr, sta::RiseFall::rise(), sta::MinMax::max()),
            nullptr);

  // Without a recorded net, a single-signal-pin bump is unambiguous and
  // takes the lumped RC.
  odb::dbMaster* logic0 = db_->findMaster("LOGIC0_X1");
  ASSERT_NE(logic0, nullptr);
  odb::dbInst* u_bump = odb::dbInst::create(block_, logic0, "u_bump");
  ASSERT_NE(u_bump, nullptr);
  odb::dbNet* b_net = odb::dbNet::create(block_, "b_net");
  u_bump->findITerm("Z")->connect(b_net);
  ASSERT_NE(odb::dbBTerm::create(b_net, "b_port"), nullptr);
  ASSERT_NE(odb::dbChipBump::create(region, u_bump), nullptr);
  ep_.estimateWireParasitic(db_network_->dbToSta(b_net));
  pi = parasitics->findPiElmore(db_network_->dbToSta(u_bump->findITerm("Z")),
                                sta::RiseFall::rise(),
                                sta::MinMax::max());
  ASSERT_NE(pi, nullptr);
  parasitics->piModel(pi, c2, rpi, c1);
  EXPECT_FLOAT_EQ(rpi, 7.5f);

  // dbChipBump::setNet emits no invalidation, but the full estimation pass
  // deletes all parasitics up front: scan_clk's obsolete bump annotation
  // (its bump net still points at d) does not survive it.
  ep_.setHWireSignalRC(nullptr, scene, 1.0e3, 1.0e-10);
  ep_.setVWireSignalRC(nullptr, scene, 1.0e3, 1.0e-10);
  ep_.estimateWireParasitics();
  EXPECT_EQ(parasitics->findPiElmore(
                scan_clk_pin, sta::RiseFall::rise(), sta::MinMax::max()),
            nullptr);
}

// Verifies that wire RC values are stored per chip: chip-specific values take
// precedence over the defaults, and chips without an entry use the defaults.
TEST_F(TestEstimateParasitics, WireRcPerTech)
{
  readVerilogAndSetup("TestEstimateParasitics.v");

  sta::Scene* scene = sta_->scenes().front();
  odb::dbChip* chip1 = db_->getChip();
  ASSERT_NE(chip1, nullptr);

  // A null tech sets the default values used by techs without an entry.
  ep_.initChip(chip1);
  ep_.setHWireSignalRC(nullptr, scene, 1.0e3, 1.0e-10);
  ep_.setVWireSignalRC(nullptr, scene, 2.0e3, 2.0e-10);
  ep_.setHWireClkRC(nullptr, scene, 5.0e3, 5.0e-10);
  ep_.setVWireClkRC(nullptr, scene, 5.0e3, 5.0e-10);
  EXPECT_DOUBLE_EQ(ep_.wireSignalHResistance(scene), 1.0e3);
  EXPECT_DOUBLE_EQ(ep_.wireSignalVCapacitance(scene), 2.0e-10);

  // A second technology with tech-specific values, used by a second chip.
  loadTechAndLib(
      "tech2", "lib2", getFilePath("_main/test/Nangate45/Nangate45.lef"));
  odb::dbTech* tech2 = db_->findTech("tech2");
  ASSERT_NE(tech2, nullptr);
  odb::dbChip* chip2 = odb::dbChip::create(
      db_.get(), tech2, "chip2", odb::dbChip::ChipType::DIE);
  ASSERT_NE(chip2, nullptr);
  odb::dbBlock::create(chip2, "chip2_block");
  ep_.setHWireSignalRC(tech2, scene, 3.0e3, 3.0e-10);
  ep_.setVWireSignalRC(tech2, scene, 4.0e3, 4.0e-10);

  // The tech-specific values do not leak into the default-valued tech.
  EXPECT_DOUBLE_EQ(ep_.wireSignalHResistance(scene), 1.0e3);

  // Rebinding to the second chip resolves tech2's signal values; its unset
  // clock values fall back to the defaults independently.
  ep_.initChip(chip2);
  EXPECT_DOUBLE_EQ(ep_.wireSignalHResistance(scene), 3.0e3);
  EXPECT_DOUBLE_EQ(ep_.wireSignalVCapacitance(scene), 4.0e-10);
  EXPECT_DOUBLE_EQ(ep_.wireClkHResistance(scene), 5.0e3);

  // Rebinding back to a chip whose tech has no entry falls back to defaults.
  ep_.initChip(chip1);
  EXPECT_DOUBLE_EQ(ep_.wireSignalHResistance(scene), 1.0e3);
  EXPECT_DOUBLE_EQ(ep_.wireSignalVResistance(scene), 2.0e3);
}

}  // namespace est
