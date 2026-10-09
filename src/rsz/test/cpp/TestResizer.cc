// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupContext.hh"
#include "RepairTargetCollector.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "move/MoveGenerator.hh"
#include "odb/db.h"
#include "odb/defin.h"
#include "policy/SetupLegacyPolicy.hh"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Sta.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "tst/IntegratedFixture.h"

namespace rsz {

class TestMoveGenerator : public MoveGenerator
{
 public:
  explicit TestMoveGenerator(const GeneratorContext& context)
      : MoveGenerator(context)
  {
  }

  MoveType type() const override { return MoveType::kSizeUp; }

  std::vector<std::unique_ptr<MoveCandidate>> generate(const Target&) override
  {
    return {};
  }

  using MoveGenerator::weakerCellFirst;
};

class TestSetupLegacyPolicy : public SetupLegacyPolicy
{
 public:
  using SetupLegacyBase::EndpointRepairState;
  using SetupLegacyBase::refreshEndpointSlacks;
  using SetupLegacyBase::repairBudget;
  using SetupLegacyPolicy::MainRepairState;
  using SetupLegacyPolicy::repairEndpoint;
  using SetupLegacyPolicy::SetupLegacyPolicy;

  void setRepairPathStep(std::function<bool()> step)
  {
    repair_path_step_ = std::move(step);
  }

  const std::vector<bool>& recordedForceSingleRepair() const
  {
    return recorded_force_single_repair_;
  }

 protected:
  bool repairPath(sta::Path* /*path*/,
                  sta::Slack /*path_slack*/,
                  bool force_single_repair) override
  {
    recorded_force_single_repair_.push_back(force_single_repair);
    return repair_path_step_ ? repair_path_step_() : false;
  }

 private:
  std::function<bool()> repair_path_step_;
  std::vector<bool> recorded_force_single_repair_;
};

class TestResizer : public tst::IntegratedFixture
{
 public:
  TestResizer()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/rsz/test/")
  {
  }

 protected:
  static odb::dbITerm* findITerm(odb::dbInst* inst, const char* pin_name)
  {
    odb::dbITerm* iterm = inst->findITerm(pin_name);
    EXPECT_NE(iterm, nullptr);
    return iterm;
  }

  static std::string modNetName(odb::dbITerm* iterm)
  {
    odb::dbModNet* modnet = iterm->getModNet();
    return modnet ? modnet->getName() : "None";
  }

  float staTime(const float value) const
  {
    return sta_->units()->timeUnit()->userToSta(value);
  }

  void readDefForTiming(const char* def_file)
  {
    odb::dbChip* chip = db_->getChip();
    if (chip == nullptr) {
      chip = odb::dbChip::create(db_.get(), db_->getTech());
    }

    std::vector<odb::dbLib*> search_libs;
    for (odb::dbLib* db_lib : db_->getLibs()) {
      search_libs.push_back(db_lib);
    }

    const std::string def_path = getFilePath(test_root_path_ + def_file);
    odb::defin def_reader(db_.get(), &logger_);
    def_reader.readChip(search_libs, def_path.c_str(), chip);

    sta_->postReadDb(db_.get());
    block_ = chip->getBlock();
    sta_->postReadDef(block_);
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

  void makeClock(const char* clock_name, sta::Pin* pin) const
  {
    sta::PinSet pins(db_network_);
    if (pin != nullptr) {
      pins.insert(pin);
    }

    const float period = staTime(1.0);
    sta::FloatSeq waveform;
    waveform.push_back(0.0);
    waveform.push_back(period / 2.0);

    sta_->makeClock(
        clock_name, pins, false, period, waveform, "", sta_->cmdMode());
  }

  void setInputDelay(const char* port_name,
                     const char* clock_name,
                     const float delay) const
  {
    sta::Sdc* sdc = sta_->cmdMode()->sdc();
    sta::Clock* clock = sdc->findClock(clock_name);
    ASSERT_NE(clock, nullptr);

    sta::Pin* pin = findTopPin(port_name);
    ASSERT_NE(pin, nullptr);

    sta_->setInputDelay(pin,
                        sta::RiseFallBoth::riseFall(),
                        clock,
                        sta::RiseFall::rise(),
                        nullptr,
                        false,
                        false,
                        sta::MinMaxAll::all(),
                        true,
                        staTime(delay),
                        sdc);
  }

  void setupTimeBorrowTiming(const char* def_file, const float input_delay)
  {
    readDefForTiming(def_file);

    sta::Pin* clk_pin = findTopPin("clk");
    ASSERT_NE(clk_pin, nullptr);

    makeClock("clk", clk_pin);
    makeClock("vclk", nullptr);

    sta::Clock* clk = sta_->cmdMode()->sdc()->findClock("clk");
    ASSERT_NE(clk, nullptr);
    sta_->setPropagatedClock(clk, sta_->cmdMode());
    setInputDelay("en_in", "vclk", input_delay);

    sta_->ensureGraph();
    sta_->ensureLevelized();
    resizer_.initBlock();
    ep_.estimateWireParasitics();
    sta_->updateTiming(true);
  }

  sta::Vertex* loadVertex(const char* pin_name) const
  {
    sta::Pin* pin = db_network_->findPin(pin_name);
    if (pin == nullptr) {
      ADD_FAILURE() << "missing pin " << pin_name;
      return nullptr;
    }

    sta::Vertex* endpoint = sta_->graph()->pinLoadVertex(pin);
    if (endpoint == nullptr) {
      ADD_FAILURE() << "missing load vertex " << pin_name;
      return nullptr;
    }
    return endpoint;
  }

  bool hasTargetPin(const std::vector<Target>& targets,
                    const char* pin_name) const
  {
    for (const Target& target : targets) {
      if (target.driver_pin != nullptr
          && std::string(db_network_->pathName(target.driver_pin))
                 == pin_name) {
        return true;
      }
    }
    return false;
  }

  sta::LibertyPort* findLibertyPort(sta::Instance* inst, const char* port_name)
  {
    std::unique_ptr<sta::InstancePinIterator> pin_iter{
        sta_->network()->pinIterator(inst)};
    while (pin_iter->hasNext()) {
      sta::Pin* pin = pin_iter->next();
      sta::LibertyPort* port = sta_->network()->libertyPort(pin);
      if (port != nullptr && port->name() == port_name) {
        return port;
      }
    }
    return nullptr;
  }
};

// Exclude non-timing drivers from GPL's slack ranking.
TEST_F(TestResizer, ResizeSlacksExcludeNonTimingDrivers)
{
  readVerilogAndSetup("TestResizerResizeSlacks.v",
                      /*init_default_sdc=*/false);
  makeClock("vclk", nullptr);
  setInputDelay("a", "vclk", 0.028);
  setInputDelay("unused", "vclk", 0.270);
  setInputDelay("loose_in", "vclk", 0.270);

  sta::Sdc* sdc = sta_->cmdMode()->sdc();
  sta::Clock* clock = sdc->findClock("vclk");
  sta::Pin* output = findTopPin("y");
  ASSERT_NE(output, nullptr);
  sta_->setOutputDelay(output,
                       sta::RiseFallBoth::riseFall(),
                       clock,
                       sta::RiseFall::rise(),
                       nullptr,
                       false,
                       false,
                       sta::MinMaxAll::all(),
                       true,
                       staTime(1.0),
                       sdc);
  sta_->ensureGraph();
  sta_->ensureLevelized();
  resizer_.initBlock();
  sta_->updateTiming(true);

  const auto* max = sta::MinMax::max();
  const float tolerance = staTime(1e-6);
  sta::Vertex* constrained = loadVertex("a");
  sta::Vertex* isolated = loadVertex("unused");
  sta::Vertex* unconstrained = loadVertex("loose_in");
  ASSERT_NE(constrained, nullptr);
  ASSERT_NE(isolated, nullptr);
  ASSERT_NE(unconstrained, nullptr);
  ASSERT_TRUE(constrained->hasFanout());
  ASSERT_FALSE(isolated->hasFanout());
  ASSERT_TRUE(unconstrained->hasFanout());
  const sta::Slack wns = sta_->worstSlack(max);
  EXPECT_NEAR(wns, staTime(-0.028), tolerance);
  EXPECT_NEAR(sta_->slack(isolated, max), staTime(-0.270), tolerance);
  EXPECT_TRUE(sta::fuzzyInf(sta_->slack(unconstrained, max)));

  const odb::dbNet* timed_net = db_network_->flatNet(constrained->pin());
  const odb::dbNet* isolated_net = db_network_->flatNet(isolated->pin());
  const odb::dbNet* unconstrained_net
      = db_network_->flatNet(unconstrained->pin());
  resizer_.findResizeSlacks1();
  const auto timed_slack = resizer_.resizeNetSlack(timed_net);
  ASSERT_TRUE(timed_slack.has_value());
  EXPECT_NEAR(*timed_slack, wns, tolerance);
  EXPECT_FALSE(resizer_.resizeNetSlack(isolated_net).has_value());
  EXPECT_FALSE(resizer_.resizeNetSlack(unconstrained_net).has_value());

  // Preserve constrained slack on nets with mixed fanout.
  resizer_.setWorstSlackNetsPercent(100);
  EXPECT_EQ(resizer_.resizeWorstSlackNets().size(), 1);

  // Clear cached slack when the last output constraint is removed.
  sta_->removeOutputDelay(output,
                          sta::RiseFallBoth::riseFall(),
                          clock,
                          sta::RiseFall::rise(),
                          sta::MinMaxAll::all(),
                          sdc);
  sta_->updateTiming(true);
  resizer_.findResizeSlacks1();
  EXPECT_FALSE(resizer_.resizeNetSlack(timed_net).has_value());
  EXPECT_TRUE(resizer_.resizeWorstSlackNets().empty());
}

// Verify dont_touch preserves a hierarchical name and protects its flat net.
TEST_F(TestResizer, HierarchicalNetDontTouch)
{
  readVerilogAndSetup("TestBufferRemoval3_feedthrough.v",
                      /*init_default_sdc=*/false);

  odb::dbModule* child_module = block_->findModule("child_mod");
  ASSERT_NE(child_module, nullptr);
  odb::dbModBTerm* input_port = child_module->findModBTerm("data_i");
  ASSERT_NE(input_port, nullptr);
  odb::dbModNet* mod_net = input_port->getModNet();
  ASSERT_NE(mod_net, nullptr);
  odb::dbNet* flat_net = mod_net->findRelatedNet();
  ASSERT_NE(flat_net, nullptr);

  // Compare addresses without dereferencing a potentially corrupted name.
  const std::uintptr_t name_address
      = reinterpret_cast<std::uintptr_t>(mod_net->getConstName());
  const sta::Net* sta_net = db_network_->dbToSta(mod_net);
  ASSERT_NE(sta_net, nullptr);

  resizer_.setDontTouch(sta_net, true);
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(mod_net->getConstName()),
            name_address);
  EXPECT_TRUE(flat_net->isDoNotTouch());
  EXPECT_TRUE(resizer_.dontTouch(sta_net));

  resizer_.setDontTouch(sta_net, false);
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(mod_net->getConstName()),
            name_address);
  EXPECT_FALSE(flat_net->isDoNotTouch());
}

TEST_F(TestResizer, WeakerCellFirstOrdersHigherDriveResistanceFirst)
{
  const sta::LibertyCell* weaker_cell
      = sta_->network()->findLibertyCell("BUF_X1");
  const sta::LibertyCell* stronger_cell
      = sta_->network()->findLibertyCell("BUF_X16");
  ASSERT_NE(weaker_cell, nullptr);
  ASSERT_NE(stronger_cell, nullptr);

  const int lib_ap_index = sta_->cmdScene()->libertyIndex(sta::MinMax::max());
  const sta::LibertyPort* weaker_base_port = weaker_cell->findLibertyPort("Z");
  const sta::LibertyPort* stronger_base_port
      = stronger_cell->findLibertyPort("Z");
  ASSERT_NE(weaker_base_port, nullptr);
  ASSERT_NE(stronger_base_port, nullptr);
  const sta::LibertyPort* weaker_port
      = weaker_base_port->scenePort(lib_ap_index);
  const sta::LibertyPort* stronger_port
      = stronger_base_port->scenePort(lib_ap_index);
  ASSERT_NE(weaker_port, nullptr);
  ASSERT_NE(stronger_port, nullptr);
  ASSERT_LT(stronger_port->driveResistance(), weaker_port->driveResistance());

  MoveCommitter committer(resizer_);
  const OptimizerRunConfig run_config;
  const OptimizationPolicyConfig policy_config;
  const GeneratorContext context{.resizer = resizer_,
                                 .committer = committer,
                                 .run_config = run_config,
                                 .policy_config = policy_config};
  const TestMoveGenerator generator(context);

  EXPECT_TRUE(
      generator.weakerCellFirst(weaker_cell, stronger_cell, "Z", lib_ap_index));
  EXPECT_FALSE(
      generator.weakerCellFirst(stronger_cell, weaker_cell, "Z", lib_ap_index));
}

TEST_F(TestResizer, WeakerCellFirstOrdersHigherIntrinsicDelayFirst)
{
  sta::LibertyLibrary* library
      = readLiberty(test_root_path_ + "cpp/TestResizerIntrinsic.lib");
  ASSERT_NE(library, nullptr);

  const sta::LibertyCell* faster_cell
      = library->findLibertyCell("INTRINSIC_FAST");
  const sta::LibertyCell* slower_cell
      = library->findLibertyCell("INTRINSIC_SLOW");
  ASSERT_NE(faster_cell, nullptr);
  ASSERT_NE(slower_cell, nullptr);

  const int lib_ap_index = sta_->cmdScene()->libertyIndex(sta::MinMax::max());
  const sta::LibertyPort* faster_base_port = faster_cell->findLibertyPort("Z");
  const sta::LibertyPort* slower_base_port = slower_cell->findLibertyPort("Z");
  ASSERT_NE(faster_base_port, nullptr);
  ASSERT_NE(slower_base_port, nullptr);
  const sta::LibertyPort* faster_port
      = faster_base_port->scenePort(lib_ap_index);
  const sta::LibertyPort* slower_port
      = slower_base_port->scenePort(lib_ap_index);
  ASSERT_NE(faster_port, nullptr);
  ASSERT_NE(slower_port, nullptr);
  ASSERT_FLOAT_EQ(faster_port->driveResistance(),
                  slower_port->driveResistance());
  const sta::ArcDelay faster_intrinsic
      = faster_port->intrinsicDelay(resizer_.staState());
  const sta::ArcDelay slower_intrinsic
      = slower_port->intrinsicDelay(resizer_.staState());
  ASSERT_LT(faster_intrinsic, slower_intrinsic);

  MoveCommitter committer(resizer_);
  const OptimizerRunConfig run_config;
  const OptimizationPolicyConfig policy_config;
  const GeneratorContext context{.resizer = resizer_,
                                 .committer = committer,
                                 .run_config = run_config,
                                 .policy_config = policy_config};
  const TestMoveGenerator generator(context);

  EXPECT_TRUE(
      generator.weakerCellFirst(slower_cell, faster_cell, "Z", lib_ap_index));
  EXPECT_FALSE(
      generator.weakerCellFirst(faster_cell, slower_cell, "Z", lib_ap_index));
}

TEST_F(TestResizer, SwapPinsFeedthroughModNet)
{
  const testing::TestInfo* test_info
      = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  readVerilogAndSetup(test_name + "_pre.v");

  odb::dbNet* net = block_->findNet("src_net");
  ASSERT_NE(net, nullptr);

  odb::dbInst* gate = block_->findInst("target");
  odb::dbInst* drv = block_->findInst("drv_ff");
  odb::dbInst* probe = block_->findInst("probe");
  ASSERT_NE(gate, nullptr);
  ASSERT_NE(drv, nullptr);
  ASSERT_NE(probe, nullptr);

  // Seed state matching the cva6 hierarchy:
  // one gate input already carries the feed-through output-side modnet,
  // while the rest of the flat src_net still uses the input-side name.
  EXPECT_EQ(modNetName(findITerm(gate, "A1")), "tap_out");
  EXPECT_EQ(modNetName(findITerm(drv, "Q")), "src_net");
  EXPECT_EQ(modNetName(findITerm(probe, "A")), "src_net");

  sta::Instance* sta_gate = db_network_->dbToSta(gate);
  ASSERT_NE(sta_gate, nullptr);
  sta::LibertyPort* port_a1 = findLibertyPort(sta_gate, "A1");
  sta::LibertyPort* port_a2 = findLibertyPort(sta_gate, "A2");
  ASSERT_NE(port_a1, nullptr);
  ASSERT_NE(port_a2, nullptr);

  // This is the exact reconnect path used by repair_timing's swap-pins move.
  ASSERT_TRUE(resizer_.swapPins(sta_gate, port_a1, port_a2));

  // The correct post-swap state should keep the output-side modnet local to
  // the swapped gate input. The rest of the flat src_net should preserve
  // its input-side modnet name.
  EXPECT_EQ(modNetName(findITerm(gate, "A2")), "tap_out");
  EXPECT_EQ(modNetName(findITerm(drv, "Q")), "src_net");
  EXPECT_EQ(modNetName(findITerm(probe, "A")), "src_net");

  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestResizer, LatchThroughPathCollectsLatchDataFaninTargets)
{
  setupTimeBorrowTiming("inferred_clock_gator_time_borrow.def", 0.98);

  RepairTargetCollector collector(&resizer_);
  collector.init(0.0f);

  sta::Vertex* endpoint = loadVertex("gated_ff0/D");
  ASSERT_NE(endpoint, nullptr);

  sta::Path* path = sta_->vertexWorstSlackPath(endpoint, sta::MinMax::max());
  ASSERT_NE(path, nullptr);

  const std::vector<Target> targets
      = collector.collectPathDriverTargets(path, path->slack(sta_.get()));

  EXPECT_TRUE(hasTargetPin(targets, "enable_buf0/Z"));
  EXPECT_TRUE(hasTargetPin(targets, "enable_buf1/Z"));
}

TEST_F(TestResizer, ChainedLatchFaninTargets)
{
  setupTimeBorrowTiming("latch_borrow_chain.def", 0.84);

  RepairTargetCollector collector(&resizer_);
  collector.init(0.0f);

  sta::Vertex* endpoint = loadVertex("gated_ff0/D");
  ASSERT_NE(endpoint, nullptr);

  sta::Path* path = sta_->vertexWorstSlackPath(endpoint, sta::MinMax::max());
  ASSERT_NE(path, nullptr);

  const std::vector<Target> targets
      = collector.collectPathDriverTargets(path, path->slack(sta_.get()));

  EXPECT_TRUE(hasTargetPin(targets, "mid_buf0/Z"));
  EXPECT_TRUE(hasTargetPin(targets, "deep_buf0/Z"));
}

TEST_F(TestResizer, RepairBudgetClampsToAtLeastOneWhenSlackImprovesPastMinViol)
{
  readVerilogAndSetup("TestResizer_SwapPinsFeedthroughModNet_pre.v",
                      /*init_default_sdc=*/true,
                      /*hierarchy=*/false);

  sta::Pin* out1_pin = findTopPin("out1");
  sta::Pin* out2_pin = findTopPin("out2");
  ASSERT_NE(out1_pin, nullptr);
  ASSERT_NE(out2_pin, nullptr);

  resizer_.initBlock();
  sta_->updateTiming(true);

  const sta::MinMax* max = sta::MinMax::max();
  const sta::Slack out1_base_slack
      = sta_->slack(sta_->graph()->pinLoadVertex(out1_pin), max);
  const sta::Slack out2_base_slack
      = sta_->slack(sta_->graph()->pinLoadVertex(out2_pin), max);

  sta::Sdc* sdc = sta_->cmdMode()->sdc();
  sta::Clock* clk = sdc->findClock("clk");
  ASSERT_NE(clk, nullptr);

  // Give out1 an initial violation of 0.20 ns (max_viol) and out2 an initial
  // violation of 0.10 ns (min_viol).
  sta_->setOutputDelay(out1_pin,
                       sta::RiseFallBoth::riseFall(),
                       clk,
                       sta::RiseFall::rise(),
                       nullptr,
                       false,
                       false,
                       sta::MinMaxAll::all(),
                       /*add=*/false,
                       out1_base_slack + staTime(0.20f),
                       sdc);
  sta_->setOutputDelay(out2_pin,
                       sta::RiseFallBoth::riseFall(),
                       clk,
                       sta::RiseFall::rise(),
                       nullptr,
                       false,
                       false,
                       sta::MinMaxAll::all(),
                       /*add=*/false,
                       out2_base_slack + staTime(0.10f),
                       sdc);
  sta_->updateTiming(true);

  RepairTargetCollector collector(&resizer_);
  collector.init(0.0f);
  ASSERT_EQ(collector.getMaxEndpointCount(), 2);
  collector.setToEndpoint(0);

  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  setup_context.min_viol = 0.067f;
  setup_context.max_viol = 0.256f;
  setup_context.max_repairs_per_pass = 10;
  const OptimizerRunConfig run_config;
  const TestSetupLegacyPolicy policy(
      resizer_, committer, setup_context, run_config);

  // Once a path's violation (-path_slack) shrinks below the initial min_viol,
  // repairBudget must still allow at least 1 repair per pass rather than
  // returning 0 or a negative budget.
  EXPECT_EQ(policy.repairBudget(-0.044f, /*force_single_repair=*/false), 1);
  EXPECT_EQ(policy.repairBudget(-0.005f, /*force_single_repair=*/false), 1);

  // Improve out1's violation to 0.05 ns (below the initial min_viol of 0.10 ns
  // recorded in collector, while remaining negative-slack).
  sta_->setOutputDelay(out1_pin,
                       sta::RiseFallBoth::riseFall(),
                       clk,
                       sta::RiseFall::rise(),
                       nullptr,
                       false,
                       false,
                       sta::MinMaxAll::all(),
                       /*add=*/false,
                       out1_base_slack + staTime(0.05f),
                       sdc);
  sta_->updateTiming(true);
  ASSERT_LT(collector.getCurrentEndpointSlack(), 0.0f);
  ASSERT_GT(collector.getCurrentEndpointSlack(),
            collector.getViolatingEndpoints().back().second);
  EXPECT_EQ(collector.repairsPerPass(10), 1);
}

TEST_F(TestResizer, SetupLegacyPolicyResetsForceSingleRepairAfterImprovingPass)
{
  readVerilogAndSetup("TestResizer_SwapPinsFeedthroughModNet_pre.v",
                      /*init_default_sdc=*/true,
                      /*hierarchy=*/false);

  odb::dbInst* target_db = block_->findInst("target");
  ASSERT_NE(target_db, nullptr);
  sta::Instance* target_inst = db_network_->dbToSta(target_db);
  ASSERT_NE(target_inst, nullptr);
  sta::LibertyCell* or2_x1 = sta_->network()->findLibertyCell("OR2_X1");
  sta::LibertyCell* or2_x4 = sta_->network()->findLibertyCell("OR2_X4");
  ASSERT_NE(or2_x1, nullptr);
  ASSERT_NE(or2_x4, nullptr);
  ASSERT_TRUE(resizer_.replaceCell(target_inst, or2_x1));
  resizer_.initBlock();
  sta_->updateTiming(true);

  sta::Pin* out1_pin = findTopPin("out1");
  ASSERT_NE(out1_pin, nullptr);
  sta::Port* out1_port = db_network_->port(out1_pin);
  ASSERT_NE(out1_port, nullptr);
  sta::Sdc* sdc = sta_->cmdMode()->sdc();
  sta::Clock* clk = sdc->findClock("clk");
  ASSERT_NE(clk, nullptr);
  const float ext_cap = sta_->units()->capacitanceUnit()->userToSta(50.0f);
  sta_->setPortExtPinCap(out1_port,
                         sta::RiseFallBoth::riseFall(),
                         sta::MinMaxAll::all(),
                         ext_cap,
                         sdc);
  sta_->updateTiming(true);

  sta::Vertex* endpoint = sta_->graph()->pinLoadVertex(out1_pin);
  ASSERT_NE(endpoint, nullptr);
  const sta::Slack out1_base_slack = sta_->slack(endpoint, sta::MinMax::max());

  sta_->setOutputDelay(out1_pin,
                       sta::RiseFallBoth::riseFall(),
                       clk,
                       sta::RiseFall::rise(),
                       nullptr,
                       false,
                       false,
                       sta::MinMaxAll::all(),
                       /*add=*/false,
                       out1_base_slack + staTime(0.10f),
                       sdc);
  sta_->updateTiming(true);

  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig run_config;
  run_config.setup_slack_margin = 0.0f;
  TestSetupLegacyPolicy policy(resizer_, committer, setup_context, run_config);
  ASSERT_TRUE(policy.start());

  TestSetupLegacyPolicy::EndpointRepairState endpoint_state;
  endpoint_state.end = endpoint;
  policy.refreshEndpointSlacks(endpoint_state);
  ASSERT_LT(endpoint_state.end_slack, 0.0f);
  endpoint_state.prev_end_slack = endpoint_state.end_slack;
  endpoint_state.prev_worst_slack = endpoint_state.worst_slack;
  // Simulate a prior non-improving pass that latched force_single_repair=true.
  endpoint_state.force_single_repair = true;
  endpoint_state.decreasing_slack_passes = 1;

  TestSetupLegacyPolicy::MainRepairState main_state;
  main_state.end_index = 2;
  main_state.max_end_count = 2;
  main_state.num_viols = 2;

  int call_count = 0;
  policy.setRepairPathStep([&]() {
    ++call_count;
    if (call_count == 1) {
      // Pass 1 improves slack by upsizing target from OR2_X1 to OR2_X4.
      return resizer_.replaceCell(target_inst, or2_x4);
    }
    return false;
  });

  {
    est::IncrementalParasiticsGuard guard(&ep_);
    policy.repairEndpoint(endpoint_state, main_state);
  }

  ASSERT_EQ(policy.recordedForceSingleRepair().size(), 2u);
  EXPECT_TRUE(policy.recordedForceSingleRepair()[0]);
  EXPECT_FALSE(policy.recordedForceSingleRepair()[1]);
  EXPECT_FALSE(endpoint_state.force_single_repair);
}

}  // namespace rsz
