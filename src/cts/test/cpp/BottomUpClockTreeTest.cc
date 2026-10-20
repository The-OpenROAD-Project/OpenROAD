// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "BottomUpBuffering.h"
#include "Clock.h"
#include "CtsOptions.h"
#include "TreeBuilder.h"
#include "cts/TritonCTS.h"
#include "gtest/gtest.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "sta/Clock.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "tst/IntegratedFixture.h"
#include "tst/logger_checks.h"

namespace cts {
namespace {
class SplitCounter : public odb::dbBlockCallBackObj
{
 public:
  void inDbInstCreate(odb::dbInst* inst) override
  {
    ++creates;
    splits += inst->getName().starts_with("clkbuf_regs_");
  }
  int creates = 0;
  int splits = 0;
};
}  // namespace

class BottomUpClockTreeTest : public tst::IntegratedFixture
{
 protected:
  BottomUpClockTreeTest()
      : IntegratedFixture(Technology::kNangate45, "_main/src/cts/test/")
  {
  }

  virtual bool macros() const { return false; }

  void SetUp() override
  {
    if (macros()) {
      readLiberty("_main/src/cts/test/array_tile_ins_delay.lib");
      ASSERT_NE(loadLibaryLef(lib_->getTech(),
                              "cts_macro",
                              "_main/src/cts/test/array_tile_ins_delay.lef"),
                nullptr);
    }
    block_ = readDef(macros() ? "_main/src/cts/test/insertion_delay.def"
                              : "_main/src/cts/test/16sinks.def");
    sta_->postReadDef(block_);
    setRC(sta_->cmdScene(), 1);
    initStaDefaultSdc();
    source_ = block_->findNet("clk");
    for (auto* pin : source_->getITerms()) {
      if (pin->isInputSignal()) {
        originals_.push_back(pin);
      }
    }
    cts_ = std::make_unique<TritonCTS>(
        &logger_, db_.get(), db_network_, sta_.get(), &stt_, &resizer_, &ep_);
    configure();
  }

  void TearDown() override
  {
    if (cts_ != nullptr) {
      cts_->resetRunState();
      cts_.reset();
    }
  }

  void configure()
  {
    cts_->setBufferList("BUF_X1 BUF_X2 BUF_X4 BUF_X8");
    cts_->setRootBuffer("BUF_X8");
    cts_->setSinkBuffer("BUF_X1");
    auto* options = cts_->getParms();
    options->setDbUnits(block_->getDbUnitsPerMicron());
    options->setBottomUpBuffering(true);
    options->setSinkClustering(false);
    options->setCapSteps(2);
    options->setSlewSteps(2);
    options->setCharWirelengthIterations(1);
    options->setObstructionAware(false);
  }

  void setRC(sta::Scene* scene, double factor)
  {
    auto* tech = block_->getTech();
    ep_.setHWireClkRC(tech, scene, 1e5 * factor, 1e-11 * factor);
    ep_.setVWireClkRC(tech, scene, 2e5 * factor, 2e-11 * factor);
    ep_.setHWireSignalRC(tech, scene, 3e5 * factor, 3e-11 * factor);
    ep_.setVWireSignalRC(tech, scene, 4e5 * factor, 4e-11 * factor);
  }

  sta::Scene* addTestScene(bool configure_rc = true)
  {
    const auto lib
        = getFilePath("_main/src/cts/test/Nangate45/Nangate45_typ.lib");
    sta_->makeScene("slow", "test", {lib}, {lib}, "", "");
    auto* scene = sta_->findScene("slow");
    if (configure_rc) {
      setRC(scene, 3);
    }
    sta::PinSet pins(db_network_);
    pins.insert(db_network_->dbToSta(source_->get1stBTerm()));
    sta_->makeClock("clk", pins, false, 5e-9, {0, 2.5e-9}, "", scene->mode());
    sta_->setPropagatedClock(scene->mode()->sdc()->findClock("clk"),
                             scene->mode());
    return scene;
  }

  odb::dbInst* stage(const char* master,
                     const char* name,
                     const char* input = "A",
                     const char* output = "Z",
                     bool modeled = true)
  {
    auto* inst = odb::dbInst::create(block_, db_->findMaster(master), name);
    inst->setLocation(1000 + stages_ * 2000, 1000);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    inst->setSourceType(odb::dbSourceType::TIMING);
    inst->findITerm(input)->connect(last_net_);
    auto* net
        = odb::dbNet::create(block_, (std::string(name) + "_net").c_str());
    net->setSigType(odb::dbSigType::CLOCK);
    inst->findITerm(output)->connect(net);
    last_net_ = net;
    ++stages_;
    if (modeled) {
      auto& subnet = builder_->getClock().addSubNet(name);
      subnet.setNetObj(net);
    }
    for (auto* pin : originals_) {
      pin->connect(last_net_);
    }
    return inst;
  }

  void physicalTree()
  {
    Clock clock("clk", "clk", "clk", 0, 0);
    clock.setNetObj(source_);
    for (auto* pin : originals_) {
      clock.addSink(pin->getName(), 0, 0, pin, 0);
    }
    builder_ = cts_->addBuilder(
        cts_->getParms(), clock, source_, nullptr, &logger_, db_.get());
    last_net_ = source_;
    stage("BUF_X8", "clkbuf_0_clk");
    stage("BUF_X2", "clkbuf_branch");
    stage("INV_X1", "clock_inverter");
    gate_ = stage("CLKGATE_X1", "clock_gate", "CK", "GCK");
    sta_->setCaseAnalysis(db_network_->dbToSta(gate_->findITerm("E")),
                          sta::LogicValue::one,
                          sta_->cmdMode());
    delay_ = stage("BUF_X1", "delaybuf_after_model", "A", "Z", false);
    sta_->setPropagatedClock(sta_->cmdMode()->sdc()->findClock("clk"),
                             sta_->cmdMode());
  }

  // Independently walk actual output connectivity, with no CTS model or
  // initial staClockNets_ lookup. Every original sink must have a root path.
  size_t reachableOriginals()
  {
    odb::PtrSet<odb::dbNet> nets;
    odb::PtrSet<odb::dbITerm> pins;
    std::vector<odb::dbNet*> pending{source_};
    while (!pending.empty()) {
      auto* net = pending.back();
      pending.pop_back();
      if (!nets.insert(net).second) {
        continue;
      }
      for (auto* pin : net->getITerms()) {
        if (!pin->isInputSignal()) {
          continue;
        }
        pins.insert(pin);
        if (std::ranges::find(originals_, pin) == originals_.end()) {
          for (auto* output : pin->getInst()->getITerms()) {
            if (output->isOutputSignal() && output->getNet() != nullptr) {
              pending.push_back(output->getNet());
            }
          }
        }
      }
    }
    return std::ranges::count_if(originals_,
                                 [&](auto* pin) { return pins.contains(pin); });
  }

  BufferingAudit audit() { return cts_->checkClockTreeElectricals(); }
  BufferingWireRC bound() { return cts_->validateBottomUpWireRC(); }

  bool cleanRunState()
  {
    return !cts_->getParms()->hasOwner() && cts_->builders_.empty()
           && cts_->techChar_ == nullptr && cts_->staClockNets_.empty()
           && cts_->visitedClockNets_.empty() && cts_->net2builder_.empty()
           && cts_->inst2clkbuf_.empty() && cts_->driver2subnet_.empty();
  }

  struct DatabaseState
  {
    std::vector<std::tuple<std::string,
                           uint32_t,
                           std::string,
                           int,
                           int,
                           int,
                           int,
                           int,
                           bool>>
        instances;
    std::vector<std::tuple<std::string, uint32_t, int, int, int, bool>> nets;
    std::vector<std::tuple<std::string, uint32_t, std::string>> connections;
    std::vector<std::tuple<std::string, uint32_t, std::string, int, int>> ports;
    bool operator==(const DatabaseState&) const = default;
  };

  DatabaseState snapshot()
  {
    DatabaseState state;
    for (auto* inst : block_->getInsts()) {
      int x, y;
      inst->getLocation(x, y);
      state.instances.emplace_back(inst->getName(),
                                   inst->getId(),
                                   inst->getMaster()->getName(),
                                   x,
                                   y,
                                   inst->getOrient().getValue(),
                                   inst->getPlacementStatus().getValue(),
                                   inst->getSourceType().getValue(),
                                   inst->isDoNotTouch());
      for (auto* pin : inst->getITerms()) {
        auto* net = pin->getNet();
        state.connections.emplace_back(
            pin->getName(), pin->getId(), net == nullptr ? "" : net->getName());
      }
    }
    for (auto* net : block_->getNets()) {
      state.nets.emplace_back(net->getName(),
                              net->getId(),
                              net->getSigType().getValue(),
                              net->getSourceType().getValue(),
                              net->getWireType().getValue(),
                              net->isDoNotTouch());
    }
    for (auto* port : block_->getBTerms()) {
      auto* net = port->getNet();
      state.ports.emplace_back(port->getName(),
                               port->getId(),
                               net == nullptr ? "" : net->getName(),
                               port->getIoType().getValue(),
                               port->getSigType().getValue());
    }
    std::ranges::sort(state.instances);
    std::ranges::sort(state.nets);
    std::ranges::sort(state.connections);
    std::ranges::sort(state.ports);
    return state;
  }

  void expectCallbacksRestored(SplitCounter& observer)
  {
    EXPECT_TRUE(observer.hasOwner());
    const auto creates = observer.creates;
    const auto buffers = cts_->getParms()->getBufferCount();
    const auto dummies = cts_->getParms()->getDummyCount();
    auto* probe = odb::dbInst::create(
        block_, db_->findMaster("BUF_X1"), "cts_retry_callback_probe");
    ASSERT_NE(probe, nullptr);
    EXPECT_EQ(observer.creates, creates + 1);
    EXPECT_EQ(cts_->getParms()->getBufferCount(), buffers);
    EXPECT_EQ(cts_->getParms()->getDummyCount(), dummies);
    odb::dbInst::destroy(probe);
    EXPECT_FALSE(cts_->getParms()->hasOwner());
  }

  void planWriteAndBalance()
  {
    cts_->validateBottomUpWireRC();
    cts_->setupCharacterization();
    cts_->findClockRoots();
    cts_->populateTritonCTS();
    cts_->checkCharacterization();
    cts_->buildClockTrees();
    cts_->runClockTreeSynthesis();
  }

  std::unique_ptr<TritonCTS> cts_;
  TreeBuilder* builder_ = nullptr;
  odb::dbNet* source_ = nullptr;
  odb::dbNet* last_net_ = nullptr;
  odb::dbInst* delay_ = nullptr;
  odb::dbInst* gate_ = nullptr;
  int stages_ = 0;
  std::vector<odb::dbITerm*> originals_;
};

TEST_F(BottomUpClockTreeTest, EveryOriginalSinkAndUnmodeledDelayNetIsVisited)
{
  physicalTree();
  const auto result = audit();
  EXPECT_EQ(reachableOriginals(), originals_.size());
  EXPECT_EQ(result.checked_nets, 6u);
  EXPECT_EQ(result.expected_nets, 4u);
  EXPECT_EQ(result.reached_nets, result.expected_nets);
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest, DownstreamCapacitanceViolationCannotPass)
{
  physicalTree();
  sta_->setCapacitanceLimit(db_network_->dbToSta(delay_->getFirstOutput()),
                            sta::MinMax::max(),
                            1e-20,
                            sta_->cmdMode()->sdc());
  const auto result = audit();
  EXPECT_GT(result.cap_violations, 0u);
  EXPECT_NE(result.first_cap_violation.find(delay_->getName()),
            std::string::npos);
  EXPECT_FALSE(result.error.empty());
  EXPECT_EQ(result.reached_sinks, originals_.size());
}

TEST_F(BottomUpClockTreeTest, GatedMultimodeDownstreamSlewViolationCannotPass)
{
  physicalTree();
  auto* scene = addTestScene();
  auto* enable = db_network_->dbToSta(gate_->findITerm("E"));
  sta_->setCaseAnalysis(enable, sta::LogicValue::zero, sta_->cmdMode());
  sta_->setCaseAnalysis(enable, sta::LogicValue::one, scene->mode());
  sta_->setSlewLimit(
      db_network_->port(db_network_->dbToSta(originals_.front())),
      sta::MinMax::max(),
      1e-15,
      scene->mode()->sdc());
  const auto result = audit();
  EXPECT_GT(result.slew_violations, 0u);
  EXPECT_FALSE(result.first_slew_violation.empty());
  EXPECT_FALSE(result.error.empty());
  EXPECT_EQ(result.reached_sinks, originals_.size());
  const auto rc = bound();
  EXPECT_DOUBLE_EQ(rc.resistance, 12e5);
  EXPECT_DOUBLE_EQ(rc.capacitance, 12e-11);
}

TEST_F(BottomUpClockTreeTest,
       DisconnectedOriginalsCannotProduceAZeroViolationPass)
{
  physicalTree();
  delay_->findITerm("A")->disconnect();
  const auto result = audit();
  EXPECT_EQ(reachableOriginals(), 0u);
  EXPECT_LT(result.reached_sinks, result.expected_sinks);
  EXPECT_FALSE(result.error.empty());
}

TEST_F(BottomUpClockTreeTest, MissingDirectionalRCInOneSceneFailsBeforeMutation)
{
  auto* scene = addTestScene();
  ep_.setVWireSignalRC(block_->getTech(), scene, 0, 0);
  const auto before = snapshot();
  const auto count = block_->getInsts().size();
  EXPECT_LOGGER_ERROR(cts_->runTritonCts(), "CTS-0558");
  EXPECT_EQ(snapshot(), before);
  EXPECT_EQ(block_->getInsts().size(), count);
  EXPECT_TRUE(cleanRunState());
  EXPECT_EQ(reachableOriginals(), originals_.size());
}

TEST_F(BottomUpClockTreeTest, NewlyAddedSceneWithoutRCFailsBeforeMutation)
{
  auto* scene = addTestScene(false);
  EXPECT_DOUBLE_EQ(ep_.wireClkResistance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireClkHResistance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireClkVResistance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireClkCapacitance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireClkHCapacitance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireClkVCapacitance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireSignalResistance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireSignalHResistance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireSignalVResistance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireSignalCapacitance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireSignalHCapacitance(scene), 0);
  EXPECT_DOUBLE_EQ(ep_.wireSignalVCapacitance(scene), 0);
  double resistance, capacitance;
  ep_.wireSignalRC(scene, resistance, capacitance);
  EXPECT_DOUBLE_EQ(resistance, 0);
  EXPECT_DOUBLE_EQ(capacitance, 0);
  const auto before = snapshot();
  const auto count = block_->getInsts().size();
  EXPECT_LOGGER_ERROR(cts_->runTritonCts(), "CTS-0558");
  EXPECT_EQ(snapshot(), before);
  EXPECT_EQ(block_->getInsts().size(), count);
  EXPECT_TRUE(cleanRunState());
  EXPECT_EQ(reachableOriginals(), originals_.size());
}

class BottomUpMacroClockTreeTest : public BottomUpClockTreeTest
{
 protected:
  bool macros() const override { return true; }
};

TEST_F(BottomUpMacroClockTreeTest,
       FailedFeasibilityUndoesMacroSplitAndAllowsRetry)
{
  SplitCounter counter;
  counter.addOwner(block_);
  const auto before = snapshot();
  const auto inst_count = block_->getInsts().size();
  const auto net_count = block_->getNets().size();
  const auto buffers = cts_->getParms()->getBufferCount();
  auto* top = db_network_->cell(db_network_->topInstance());
  sta_->setCapacitanceLimit(
      top, sta::MinMax::max(), 1e-20, sta_->cmdMode()->sdc());
  EXPECT_LOGGER_ERROR(cts_->runTritonCts(), "CTS-0554");
  EXPECT_GT(counter.splits, 0);
  EXPECT_EQ(snapshot(), before);
  EXPECT_EQ(block_->getInsts().size(), inst_count);
  EXPECT_EQ(block_->getNets().size(), net_count);
  for (auto* pin : originals_) {
    EXPECT_EQ(pin->getNet(), source_);
  }
  EXPECT_EQ(cts_->getParms()->getBufferCount(), buffers);
  EXPECT_TRUE(cleanRunState());
  EXPECT_TRUE(odb::dbDatabase::ecoStackEmpty(block_));
  expectCallbacksRestored(counter);
  EXPECT_EQ(snapshot(), before);
  sta_->setCapacitanceLimit(
      top, sta::MinMax::max(), 1e-12, sta_->cmdMode()->sdc());
  EXPECT_NO_THROW(cts_->runTritonCts());
  EXPECT_TRUE(cleanRunState());
  expectCallbacksRestored(counter);
  EXPECT_GT(block_->getInsts().size(), inst_count);
  EXPECT_EQ(reachableOriginals(), originals_.size());
}

TEST_F(BottomUpMacroClockTreeTest, ActualLatencyBalancerAdditionsAreAudited)
{
  cts_->setBufferList("CLKBUF_X3");
  cts_->setRootBuffer("CLKBUF_X3");
  cts_->setSinkBuffer("CLKBUF_X3");
  planWriteAndBalance();
  odb::dbInst* delay = nullptr;
  for (auto* inst : block_->getInsts()) {
    if (inst->getName().starts_with("delaybuf")) {
      delay = inst;
      break;
    }
  }
  ASSERT_NE(delay, nullptr)
      << "The case must exercise actual latency balancing";
  sta_->setCapacitanceLimit(db_network_->dbToSta(delay->getFirstOutput()),
                            sta::MinMax::max(),
                            1e-20,
                            sta_->cmdMode()->sdc());
  const auto result = audit();
  EXPECT_GT(result.cap_violations, 0u);
  EXPECT_NE(result.first_cap_violation.find(delay->getName()),
            std::string::npos);
  EXPECT_EQ(reachableOriginals(), originals_.size());
  EXPECT_EQ(result.reached_sinks, result.expected_sinks);
  EXPECT_FALSE(result.error.empty());
}
}  // namespace cts
