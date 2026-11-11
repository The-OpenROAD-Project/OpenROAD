// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "gpl/Replace.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "power/Power.hh"
#include "power/SaifReader.hh"
#include "rsz/Resizer.hh"
#include "src/gpl/src/nesterovBase.h"
#include "src/gpl/src/placerBase.h"
#include "src/gpl/src/powerBase.h"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/PowerClass.hh"
#include "stt/SteinerTreeBuilder.h"
#include "tst/fixture.h"
#include "utl/Logger.h"
#include "utl/ServiceRegistry.h"

namespace gpl {
namespace {

constexpr float kPeriod = 2e-9;  // seconds
constexpr float kTolerance = 1e-4;

// simple01-td (nangate45 gcd) with a 2 ns clock on clk.
class PowerDriven : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    odb::dbLib* lib
        = loadTechAndLib("ng45", "ng45", "_main/src/gpl/test/nangate45.lef");
    ASSERT_NE(lib, nullptr);
    readLiberty(
        getFilePath("_main/src/gpl/test/library/nangate45/"
                    "NangateOpenCellLibrary_typical.lib"));
    odb::dbChip* chip = odb::dbChip::create(db_.get(), lib->getTech());
    std::vector<odb::dbLib*> libs{lib};
    odb::defin reader(db_.get(), &logger_);
    const std::string def = getFilePath("_main/src/gpl/test/simple01-td.def");
    ASSERT_TRUE(reader.readChip(libs, def.c_str(), chip));
    block_ = chip->getBlock();
    sta_->postReadDef(block_);
    network_ = sta_->getDbNetwork();

    sta::PinSet clk_pins(network_);
    clk_pins.insert(network_->dbToSta(block_->findBTerm("clk")));
    sta_->makeClock(
        "core_clock", clk_pins, false, kPeriod, {0, kPeriod / 2}, "", mode());
  }

  const sta::Mode* mode() const { return sta_->cmdScene()->mode(); }

  // Extra (power) weight per net name from one setPowerNetWeights() run.
  // Timing and routability weights stay 1, so the extra is total - 1.
  // Nets with one pin have no wirelength and are left out.
  std::map<std::string, float> powerWeights(const float net_weight)
  {
    PlaceOptions options;
    const PlacerBaseVars pb_vars(options);
    auto pbc = std::make_shared<PlacerBaseCommon>(db_.get(), pb_vars, &logger_);
    const NesterovBaseVars nb_vars(options);
    auto nbc = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc, &logger_, /* num_threads = */ 1, Clusters{});
    PowerBase power(nbc, sta_.get(), net_weight, &logger_);
    power.setPowerNetWeights();
    std::map<std::string, float> weights;
    for (GNet* gnet : nbc->getGNets()) {
      if (gnet->getGPins().size() <= 1) {
        EXPECT_EQ(gnet->getTotalWeight(), 1);
        continue;
      }
      weights[gnet->getPbNet()->getDbNet()->getName()]
          = gnet->getTotalWeight() - 1;
    }
    return weights;
  }

  const sta::Pin* driver(const odb::dbNet* net) const
  {
    sta::PinSet* drivers = network_->drivers(network_->dbToSta(net));
    return drivers == nullptr || drivers->empty() ? nullptr : *drivers->begin();
  }

  float toggles(const sta::Pin* pin, const float period = kPeriod) const
  {
    return std::min(sta_->activity(pin, sta_->cmdScene()).density() * period,
                    1.0f);
  }

  bool isBufferOrInverter(const sta::Pin* pin) const
  {
    const sta::LibertyCell* cell
        = network_->libertyCell(network_->instance(pin));
    return cell != nullptr && (cell->isBuffer() || cell->isInverter());
  }

  // The net on the single input of the buffer or inverter driving pin.
  odb::dbNet* inputNet(const sta::Pin* pin) const
  {
    odb::dbInst* inst = network_->staToDb(network_->instance(pin));
    for (odb::dbITerm* iterm : inst->getITerms()) {
      if (iterm->getSigType().isSupply()) {
        continue;
      }
      if (iterm->getIoType() == odb::dbIoType::INPUT) {
        return iterm->getNet();
      }
    }
    return nullptr;
  }

  // The first non-clock net below max_extra driven by a buffer or inverter
  // (buffered) or by another cell (not buffered).
  odb::dbNet* quietNet(const std::map<std::string, float>& weights,
                       const bool buffered,
                       const float max_extra) const
  {
    for (odb::dbNet* net : block_->getNets()) {
      const sta::Pin* drvr = driver(net);
      if (drvr != nullptr && !sta_->isClock(drvr, mode())
          && !network_->isTopLevelPort(drvr)
          && isBufferOrInverter(drvr) == buffered
          && weights.contains(net->getName())
          && weights.at(net->getName()) < max_extra) {
        return net;
      }
    }
    return nullptr;
  }

  odb::dbBlock* block_ = nullptr;
  sta::dbNetwork* network_ = nullptr;
};

// - extra weight = net weight x driver toggles per cycle, capped at 1
// - buffer/inverter nets copy their input net
// - clock nets get none
TEST_F(PowerDriven, WeightFollowsDriverActivity)
{
  constexpr float kWeight = 1;
  const auto weights = powerWeights(kWeight);
  EXPECT_EQ(weights.at("clk"), 0);
  int active_nets = 0;
  int buffered_nets = 0;
  for (odb::dbNet* net : block_->getNets()) {
    const auto it = weights.find(net->getName());
    if (it == weights.end()) {
      continue;
    }
    const float extra = it->second;
    const sta::Pin* drvr = driver(net);
    ASSERT_NE(drvr, nullptr) << net->getName();
    EXPECT_GE(extra, 0) << net->getName();
    EXPECT_LE(extra, kWeight + kTolerance) << net->getName();
    if (sta_->isClock(drvr, mode())) {
      continue;
    }
    if (isBufferOrInverter(drvr)) {
      odb::dbNet* in = inputNet(drvr);
      ASSERT_NE(in, nullptr) << net->getName();
      EXPECT_NEAR(extra, weights.at(in->getName()), kTolerance)
          << net->getName() << " vs its buffer input " << in->getName();
      buffered_nets++;
    } else {
      EXPECT_NEAR(extra, kWeight * toggles(drvr), kTolerance) << net->getName();
    }
    if (extra > 0) {
      active_nets++;
    }
  }
  EXPECT_GT(active_nets, 0);
  EXPECT_GT(buffered_nets, 0);
}

// Toggles count per cycle of the fastest clock, virtual clocks included.
TEST_F(PowerDriven, VirtualClockSetsCycle)
{
  sta::PinSet no_pins(network_);
  sta_->makeClock(
      "io_clock", no_pins, false, kPeriod / 10, {0, kPeriod / 20}, "", mode());
  int checked_nets = 0;
  for (const auto& [name, extra] : powerWeights(1)) {
    const sta::Pin* drvr = driver(block_->findNet(name.c_str()));
    if (drvr == nullptr || sta_->isClock(drvr, mode())
        || isBufferOrInverter(drvr)) {
      continue;
    }
    EXPECT_NEAR(extra, toggles(drvr, kPeriod / 10), kTolerance) << name;
    checked_nets++;
  }
  EXPECT_GT(checked_nets, 0);
}

// Extra weight scales linearly with -power_driven_net_weight.
TEST_F(PowerDriven, ExtraWeightScalesWithNetWeight)
{
  const auto at1 = powerWeights(1);
  const auto at4 = powerWeights(4);
  int active_nets = 0;
  for (const auto& [name, extra] : at1) {
    EXPECT_NEAR(at4.at(name), 4 * extra, kTolerance) << name;
    if (extra > 0) {
      active_nets++;
    }
  }
  EXPECT_GT(active_nets, 0);
}

// set_power_activity -pins wins over propagation, on buffer outputs too.
TEST_F(PowerDriven, UserPinActivityOverridesPropagation)
{
  constexpr float kWeight = 1;
  constexpr float kToggles = 0.8;
  const auto before = powerWeights(kWeight);
  odb::dbNet* cell_net = quietNet(before, false, 0.5 * kToggles);
  odb::dbNet* buffer_net = quietNet(before, true, 0.5 * kToggles);
  ASSERT_NE(cell_net, nullptr);
  ASSERT_NE(buffer_net, nullptr);
  for (const odb::dbNet* net : {cell_net, buffer_net}) {
    sta_->power()->setUserActivity(
        driver(net), kToggles / kPeriod, 0.5, sta::PwrActivityOrigin::user);
  }
  const auto after = powerWeights(kWeight);
  EXPECT_NEAR(after.at(cell_net->getName()), kToggles, kTolerance);
  EXPECT_NEAR(after.at(buffer_net->getName()), kToggles, kTolerance);
}

// A new pin reusing a deleted annotated pin's address does not inherit it.
TEST_F(PowerDriven, DestroyedAnnotatedBufferIsForgotten)
{
  constexpr float kWeight = 1;
  constexpr float kToggles = 0.8;
  odb::dbNet* target = quietNet(powerWeights(kWeight), true, 0.5 * kToggles);
  ASSERT_NE(target, nullptr);
  const sta::Pin* old_out = driver(target);
  odb::dbNet* in_net = inputNet(old_out);
  ASSERT_NE(in_net, nullptr);
  sta_->power()->setUserActivity(
      old_out, kToggles / kPeriod, 0.5, sta::PwrActivityOrigin::user);

  PlaceOptions options;
  const PlacerBaseVars pb_vars(options);
  auto pbc = std::make_shared<PlacerBaseCommon>(db_.get(), pb_vars, &logger_);
  const NesterovBaseVars nb_vars(options);
  auto nbc = std::make_shared<NesterovBaseCommon>(
      nb_vars, pbc, &logger_, /* num_threads = */ 1, Clusters{});
  PowerBase power(nbc, sta_.get(), kWeight, &logger_);
  auto weight_of = [&](const odb::dbNet* net) {
    for (GNet* gnet : nbc->getGNets()) {
      if (gnet->getPbNet()->getDbNet() == net) {
        return gnet->getTotalWeight() - 1;
      }
    }
    return -1.0f;
  };
  power.setPowerNetWeights();
  ASSERT_NEAR(weight_of(target), kToggles, kTolerance);

  odb::dbInst* old_inst = network_->staToDb(network_->instance(old_out));
  odb::dbMaster* master = old_inst->getMaster();
  odb::dbInst::destroy(old_inst);
  odb::dbInst* new_inst = odb::dbInst::create(block_, master, "new_buf");
  const sta::Pin* new_out = nullptr;
  for (odb::dbITerm* iterm : new_inst->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      iterm->connect(in_net);
    } else {
      iterm->connect(target);
      new_out = network_->dbToSta(iterm);
    }
  }
  ASSERT_EQ(new_out, old_out) << "odb no longer reuses the pin's address";
  power.setPowerNetWeights();
  EXPECT_NEAR(weight_of(target), weight_of(in_net), kTolerance);
}

// set_power_activity -global gives every non-clock net the same weight.
TEST_F(PowerDriven, GlobalActivityOverridesPropagation)
{
  constexpr float kToggles = 0.3;
  sta_->power()->setGlobalActivity(kToggles / kPeriod, 0.5);
  for (const auto& [name, extra] : powerWeights(2)) {
    if (name != "clk") {
      EXPECT_NEAR(extra, 2 * kToggles, kTolerance) << name;
    }
  }
}

// read_saif annotations win over propagation.
TEST_F(PowerDriven, SaifActivityOverridesPropagation)
{
  const std::string saif = ::testing::TempDir() + "power_base_test_"
                           + std::to_string(getpid()) + ".saif";
  {
    std::ofstream out(saif);
    // TC 400 transitions over 1000 ns is 0.8 toggles per 2 ns cycle.
    out << "(SAIFILE (SAIFVERSION \"2.0\") (DIRECTION \"backward\")\n"
           "(DIVIDER / ) (TIMESCALE 1ns) (DURATION 1000)\n"
           " (INSTANCE tb (INSTANCE dut (NET\n"
           "  (reset (T0 500) (T1 500) (TZ 0) (TX 0) (TB 0) (TC 400))\n"
           " )))\n)\n";
  }
  ASSERT_TRUE(sta::readSaif(saif.c_str(), "tb/dut", sta_.get()));
  std::remove(saif.c_str());
  EXPECT_NEAR(powerWeights(1).at("reset"), 0.8, kTolerance);
}

class PowerDrivenPlacement : public PowerDriven
{
 protected:
  void SetUp() override
  {
    PowerDriven::SetUp();
    service_registry_ = std::make_unique<utl::ServiceRegistry>(&logger_);
    stt_builder_ = std::make_unique<stt::SteinerTreeBuilder>(&logger_);
    antenna_checker_
        = std::make_unique<ant::AntennaChecker>(db_.get(), &logger_);
    opendp_ = std::make_unique<dpl::Opendp>(db_.get(), &logger_);
    global_router_
        = std::make_unique<grt::GlobalRouter>(&logger_,
                                              service_registry_.get(),
                                              stt_builder_.get(),
                                              db_.get(),
                                              sta_.get(),
                                              antenna_checker_.get(),
                                              opendp_.get());
    estimate_parasitics_
        = std::make_unique<est::EstimateParasitics>(&logger_,
                                                    service_registry_.get(),
                                                    db_.get(),
                                                    sta_.get(),
                                                    stt_builder_.get(),
                                                    global_router_.get());
    resizer_ = std::make_unique<rsz::Resizer>(&logger_,
                                              db_.get(),
                                              sta_.get(),
                                              stt_builder_.get(),
                                              global_router_.get(),
                                              opendp_.get(),
                                              estimate_parasitics_.get());
    // set_wire_rc -signal/-clock with nangate45 metal3-like values.
    odb::dbTech* tech = db_->getTech();
    const sta::Scene* scene = sta_->cmdScene();
    constexpr double kRes = 2.5e5;  // ohms/meter
    constexpr double kCap = 2e-10;  // farads/meter
    estimate_parasitics_->setHWireSignalRC(tech, scene, kRes, kCap);
    estimate_parasitics_->setVWireSignalRC(tech, scene, kRes, kCap);
    estimate_parasitics_->setHWireClkRC(tech, scene, kRes, kCap);
    estimate_parasitics_->setVWireClkRC(tech, scene, kRes, kCap);
  }

  std::unique_ptr<utl::ServiceRegistry> service_registry_;
  std::unique_ptr<stt::SteinerTreeBuilder> stt_builder_;
  std::unique_ptr<ant::AntennaChecker> antenna_checker_;
  std::unique_ptr<dpl::Opendp> opendp_;
  std::unique_ptr<grt::GlobalRouter> global_router_;
  std::unique_ptr<est::EstimateParasitics> estimate_parasitics_;
  std::unique_ptr<rsz::Resizer> resizer_;
};

// - power + timing + routability driven placement finishes
// - nets rsz adds mid-placement get power weights
TEST_F(PowerDrivenPlacement, WorksWithTimingAndRoutabilityDriven)
{
  Replace replace(db_.get(),
                  sta_.get(),
                  resizer_.get(),
                  global_router_.get(),
                  nullptr,
                  &logger_);
  PlaceOptions options;
  options.powerDrivenMode = true;
  options.timingDrivenMode = true;
  options.routabilityDrivenMode = true;
  std::set<std::string> original_nets;
  for (odb::dbNet* net : block_->getNets()) {
    original_nets.insert(net->getName());
  }
  logger_.setDebugLevel(utl::GPL, "power", 1);
  logger_.redirectStringBegin();
  replace.doPlace(1, options);
  const std::string log = logger_.redirectStringEnd();

  EXPECT_NE(log.find("Timing-driven iteration"), std::string::npos) << log;
  EXPECT_NE(log.find("Routability pass"), std::string::npos) << log;
  int weighted_new_nets = 0;
  for (size_t at = log.find("net:"); at != std::string::npos;
       at = log.find("net:", at + 1)) {
    char name[256];
    float toggles = 0;
    float weight = 0;
    if (std::sscanf(log.c_str() + at,
                    "net:%255s toggles:%f power weight:%f",
                    name,
                    &toggles,
                    &weight)
            == 3
        && weight > 0 && !original_nets.contains(name)) {
      weighted_new_nets++;
    }
  }
  EXPECT_GT(weighted_new_nets, 0) << log;
  for (odb::dbInst* inst : block_->getInsts()) {
    EXPECT_TRUE(inst->getPlacementStatus().isPlaced()) << inst->getName();
  }
}

}  // namespace
}  // namespace gpl
