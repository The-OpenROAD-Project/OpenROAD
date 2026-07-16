// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <string>
#include <vector>

#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "RepairTargetCollector.hh"
#include "gtest/gtest.h"
#include "move/MoveGenerator.hh"
#include "odb/db.h"
#include "odb/defin.h"
#include "sta/Clock.hh"
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

  using MoveGenerator::strongerCellFirst;
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

TEST_F(TestResizer, StrongerCellFirstOrdersLowerDriveResistanceFirst)
{
  const sta::LibertyCell* weaker_cell
      = sta_->network()->findLibertyCell("BUF_X1");
  const sta::LibertyCell* stronger_cell
      = sta_->network()->findLibertyCell("BUF_X16");
  ASSERT_NE(weaker_cell, nullptr);
  ASSERT_NE(stronger_cell, nullptr);

  const int lib_ap = sta_->cmdScene()->libertyIndex(sta::MinMax::max());
  const sta::LibertyPort* weaker_base_port = weaker_cell->findLibertyPort("Z");
  const sta::LibertyPort* stronger_base_port
      = stronger_cell->findLibertyPort("Z");
  ASSERT_NE(weaker_base_port, nullptr);
  ASSERT_NE(stronger_base_port, nullptr);
  const sta::LibertyPort* weaker_port = weaker_base_port->scenePort(lib_ap);
  const sta::LibertyPort* stronger_port = stronger_base_port->scenePort(lib_ap);
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
      generator.strongerCellFirst(stronger_cell, weaker_cell, "Z", lib_ap));
  EXPECT_FALSE(
      generator.strongerCellFirst(weaker_cell, stronger_cell, "Z", lib_ap));
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

}  // namespace rsz
