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

  // Give every instance and top port a legal location so that
  // estimateWireParasitics() can build Steiner trees (it skips unplaced nets).
  void placeDesign()
  {
    odb::dbTechLayer* layer = block_->getTech()->findRoutingLayer(1);
    ASSERT_NE(layer, nullptr);

    int x = 100;
    for (odb::dbInst* inst : block_->getInsts()) {
      inst->setLocation(x, x);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      x += 100;
    }

    int y = 50;
    for (odb::dbBTerm* bterm : block_->getBTerms()) {
      odb::dbBPin* bpin = odb::dbBPin::create(bterm);
      odb::dbBox::create(bpin, layer, y, y, y + 10, y + 10);
      bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      y += 100;
    }
  }

  // True if the net's driver has a reduced (pi-Elmore) parasitic, i.e. wire
  // parasitics were actually estimated for it.
  bool hasPi(sta::Net* net) const
  {
    sta::PinSet* drivers = db_network_->drivers(net);
    if (drivers == nullptr || drivers->empty()) {
      return false;
    }
    const sta::Pin* drvr = *drivers->begin();
    sta::Parasitics* par
        = sta_->scenes().front()->parasitics(sta::MinMax::max());
    return par->findPiElmore(drvr, sta::RiseFall::rise(), sta::MinMax::max())
           != nullptr;
  }
};

// Verifies that a net whose driver pin is held at a logic constant is skipped
// by updateParasitics(): its wire parasitics are not re-estimated, while an
// ordinary net in the same invalidation set is. A pin that is constant in every
// mode carries no parasitic-dependent timing.
TEST_F(TestEstimateParasitics, ConstantNetSkipsParasiticEstimation)
{
  readVerilogAndSetup("TestEstimateParasitics.v");
  placeDesign();
  ep_.estimateWireParasitics();

  // Hold the top data port at a constant in the only mode.
  sta::Pin* d_pin = findTopPin("d");
  ASSERT_NE(d_pin, nullptr);
  sta_->setCaseAnalysis(d_pin, sta::LogicValue::zero, sta_->cmdMode());
  ASSERT_TRUE(sta_->isConstant(d_pin, sta_->cmdMode()));

  sta::Net* d_net = flatNet(d_pin);
  sta::Net* q0_net = flatNet(findTopPin("q0"));
  ASSERT_NE(d_net, nullptr);
  ASSERT_NE(q0_net, nullptr);

  // Start from a clean slate so re-estimation is observable per net.
  sta_->scenes().front()->parasitics(sta::MinMax::max())->deleteParasitics();
  ASSERT_FALSE(hasPi(d_net));
  ASSERT_FALSE(hasPi(q0_net));

  ep_.setParasiticsSrc(ParasiticsSrc::kPlacement);
  ep_.setIncrementalParasiticsEnabled(true);
  ep_.parasiticsInvalid(d_net);
  ep_.parasiticsInvalid(q0_net);
  ep_.updateParasitics();

  // The ordinary net is re-estimated; the constant net is skipped.
  EXPECT_TRUE(hasPi(q0_net));
  EXPECT_FALSE(hasPi(d_net));
  ep_.setIncrementalParasiticsEnabled(false);
}

// Verifies that a net whose driver pin has a set_disable_timing constraint is
// skipped by updateParasitics(): its wire parasitics are not re-estimated,
// while an ordinary net in the same invalidation set is.
TEST_F(TestEstimateParasitics, DisabledConstraintNetSkipsParasiticEstimation)
{
  readVerilogAndSetup("TestEstimateParasitics.v");
  placeDesign();
  ep_.estimateWireParasitics();

  // Disable timing on the top data port so its net is a skip candidate.
  sta::Pin* d_pin = findTopPin("d");
  ASSERT_NE(d_pin, nullptr);
  sta_->disable(d_pin, sta_->cmdSdc());

  sta::Net* d_net = flatNet(d_pin);
  sta::Net* q0_net = flatNet(findTopPin("q0"));
  ASSERT_NE(d_net, nullptr);
  ASSERT_NE(q0_net, nullptr);

  // Start from a clean slate so re-estimation is observable per net.
  sta_->scenes().front()->parasitics(sta::MinMax::max())->deleteParasitics();
  ASSERT_FALSE(hasPi(d_net));
  ASSERT_FALSE(hasPi(q0_net));

  ep_.setParasiticsSrc(ParasiticsSrc::kPlacement);
  ep_.setIncrementalParasiticsEnabled(true);
  ep_.parasiticsInvalid(d_net);
  ep_.parasiticsInvalid(q0_net);
  ep_.updateParasitics();

  // The ordinary net is re-estimated; the disabled net is skipped.
  EXPECT_TRUE(hasPi(q0_net));
  EXPECT_FALSE(hasPi(d_net));
  ep_.setIncrementalParasiticsEnabled(false);
}

// Verifies that skippability is combined per mode: a pin that is timing
// irrelevant in every mode is skipped even when the reason differs across
// modes. Here d is constant in function mode and disabled in test mode; it is
// irrelevant in both, so its net must be skipped. A per-reason (all_constant ||
// all_disabled) check would wrongly re-estimate it.
TEST_F(TestEstimateParasitics, MixedConstantDisabledNetSkipsParasiticEstimation)
{
  readVerilogAndSetup("TestEstimateParasitics.v", false);

  // Two modes: d is constant only in function mode, disabled only in test mode.
  sta_->setCmdMode("function");
  sta::Mode* function_mode = sta_->cmdMode();
  sta_->setCmdMode("test");
  sta::Mode* test_mode = sta_->cmdMode();

  // Build the graph before adding constraints: disable() invalidates delays
  // through the delay calculator, which requires an existing graph.
  sta_->ensureGraph();
  sta_->ensureLevelized();
  resizer_.initBlock();
  placeDesign();

  sta::Pin* d_pin = findTopPin("d");
  ASSERT_NE(d_pin, nullptr);
  sta_->setCaseAnalysis(d_pin, sta::LogicValue::zero, function_mode);
  sta_->disable(d_pin, test_mode->sdc());
  ep_.estimateWireParasitics();
  ASSERT_TRUE(sta_->isConstant(d_pin, function_mode));
  ASSERT_FALSE(sta_->isConstant(d_pin, test_mode));

  sta::Net* d_net = flatNet(d_pin);
  sta::Net* q0_net = flatNet(findTopPin("q0"));
  ASSERT_NE(d_net, nullptr);
  ASSERT_NE(q0_net, nullptr);

  // Start from a clean slate so re-estimation is observable per net.
  sta_->scenes().front()->parasitics(sta::MinMax::max())->deleteParasitics();
  ASSERT_FALSE(hasPi(d_net));
  ASSERT_FALSE(hasPi(q0_net));

  ep_.setParasiticsSrc(ParasiticsSrc::kPlacement);
  ep_.setIncrementalParasiticsEnabled(true);
  ep_.parasiticsInvalid(d_net);
  ep_.parasiticsInvalid(q0_net);
  ep_.updateParasitics();

  // The ordinary net is re-estimated; the mixed-reason net is skipped.
  EXPECT_TRUE(hasPi(q0_net));
  EXPECT_FALSE(hasPi(d_net));
  ep_.setIncrementalParasiticsEnabled(false);
}

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
