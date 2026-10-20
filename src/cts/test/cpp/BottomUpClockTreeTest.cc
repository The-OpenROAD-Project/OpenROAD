// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "BottomUpBuffering.h"
#include "Clock.h"
#include "CtsOptions.h"
#include "TreeBuilder.h"
#include "cts/TritonCTS.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
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
    clones += inst->getName().starts_with("clone_");
  }
  int creates = 0;
  int splits = 0;
  int clones = 0;
};

class PostWriteCapLimit : public odb::dbBlockCallBackObj
{
 public:
  explicit PostWriteCapLimit(sta::dbNetwork* network) : network_(network) {}

  void inDbITermPostConnect(odb::dbITerm* pin) override
  {
    if (target == nullptr && pin->isOutputSignal()
        && pin->getInst()->getName().starts_with("clkbuf_")
        && pin->getNet()->getName().starts_with("clknet_")) {
      // This event occurs only during writeDataToDb, after all abstract
      // buffering decisions. The real STA checker consumes this limit.
      auto* port = network_->libertyPort(network_->dbToSta(pin));
      port->setCapacitanceLimit(1e-20, sta::MinMax::max());
      target = pin;
    }
  }

  odb::dbITerm* target = nullptr;

 private:
  sta::dbNetwork* network_;
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
    sta::StringSeq libraries{db_network_->defaultLibertyLibrary()->name()};
    if (macros()) {
      auto* library
          = readLiberty("_main/src/cts/test/array_tile_ins_delay.lib");
      ASSERT_NE(library, nullptr);
      libraries.push_back(library->name());
      ASSERT_NE(loadLibaryLef(lib_->getTech(),
                              "cts_macro",
                              "_main/src/cts/test/array_tile_ins_delay.lef"),
                nullptr);
    }
    block_ = readDef(macros() ? "_main/src/cts/test/insertion_delay.def"
                              : "_main/src/cts/test/16sinks.def");
    sta_->postReadDef(block_);
    // The first named mode/scene replaces default. Do that before clocks
    // and RC are initialized, so later additions preserve scene index zero.
    sta_->setCmdMode("func");
    sta_->makeScene("func", "func", libraries, libraries, "", "");
    ASSERT_EQ(sta_->scenes().size(), 1u);
    ASSERT_EQ(sta_->cmdScene()->mode()->name(), "func");
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
    auto* functional_scene = sta_->cmdScene();
    sta_->setCmdMode("test");
    sta_->makeScene("slow", "test", {}, {}, "", "");
    auto* scene = sta_->findScene("slow");
    // A separate Liberty read provides distinct per-scene port limits.
    if (readLiberty("_main/src/cts/test/Nangate45/Nangate45_typ.lib", scene)
        == nullptr) {
      throw std::runtime_error("Failed to load the test scene's Liberty");
    }
    EXPECT_EQ(sta_->scenes().size(), 2u);
    EXPECT_EQ(scene->index(), 1u);
    EXPECT_NE(scene->mode(), functional_scene->mode());
    if (configure_rc) {
      setRC(scene, 3);
    }
    sta::PinSet pins(db_network_);
    pins.insert(db_network_->dbToSta(source_->get1stBTerm()));
    sta_->makeClock("clk", pins, false, 5e-9, {0, 2.5e-9}, "", scene->mode());
    sta_->setPropagatedClock(scene->mode()->sdc()->findClock("clk"),
                             scene->mode());
    sta_->setCmdScene(functional_scene);
    return scene;
  }

  odb::dbInst* stage(const char* master,
                     const char* name,
                     const char* input = "A",
                     const char* output = "Z",
                     bool modeled = true)
  {
    auto* inst = odb::dbInst::create(block_, db_->findMaster(master), name);
    auto* input_pin = inst->findITerm(input);
    auto* output_pin = inst->findITerm(output);
    if (input_pin == nullptr || output_pin == nullptr) {
      throw std::runtime_error(std::string("Invalid fixture pins for ") + master
                               + ": " + input + " -> " + output);
    }
    inst->setLocation(1000 + stages_ * 2000, 1000);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    inst->setSourceType(odb::dbSourceType::TIMING);
    input_pin->connect(last_net_);
    auto* net
        = odb::dbNet::create(block_, (std::string(name) + "_net").c_str());
    net->setSigType(odb::dbSigType::CLOCK);
    output_pin->connect(net);
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
    stage("INV_X1", "clock_inverter", "A", "ZN");
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

  void planRedundantBranch(bool explicit_branch)
  {
    cts_->validateBottomUpWireRC();
    cts_->setupCharacterization();
    cts_->findClockRoots();
    cts_->populateTritonCTS();
    cts_->checkCharacterization();
    ASSERT_EQ(cts_->builders_.size(), 1u);
    builder_ = cts_->builders_.front().get();
    builder_->setTechChar(*cts_->techChar_);
    builder_->initBlockages();
    auto& clock = builder_->getClock();
    const auto point = clock.computeSinkRegion().getCenter();
    auto& root = clock.addClockBuffer(
        "clkbuf_0", "BUF_X8", point.getX(), point.getY());
    auto& branch = clock.addClockBuffer("clkbuf_redundant",
                                        explicit_branch ? "BUF_X4" : "BUF_X1",
                                        point.getX(),
                                        point.getY());
    builder_->addTreeLevelBuffer(&root);
    builder_->addTreeLevelBuffer(&branch);
    auto& root_net = clock.addSubNet("clknet_0");
    root_net.addInst(root);
    root_net.addInst(branch);
    auto& leaf = clock.addSubNet("clknet_redundant");
    leaf.addInst(branch);
    leaf.setLeafLevel(true);
    clock.forEachSink([&](ClockInst& sink) { leaf.addInst(sink); });
    auto* options = cts_->getParms();
    options->enableDummyLoad(false);
    options->setSinkBufferInferred(true);
    options->setMaxWl(1000000);
    options->setBufferDistance(1000000);
    if (explicit_branch) {
      options->setTreeBuffer("BUF_X4");
    }
    cts_->bufferClockTreeBottomUp(builder_);
    size_t count = 0;
    clock.forEachClockBuffer([&](const ClockInst& inst) {
      ++count;
      EXPECT_EQ(inst.getMaster(),
                inst.getName() == "clkbuf_0_clk" ? "BUF_X8" : "BUF_X4");
    });
    EXPECT_EQ(count, explicit_branch ? 2u : 1u)
        << "The unconstrained branch must actually disappear; explicit stages "
           "must remain";
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

TEST_F(BottomUpClockTreeTest, ActualSearchBypassesUnnecessaryBranchStage)
{
  planRedundantBranch(false);
  EXPECT_EQ(block_->findInst("clkbuf_redundant_clk"), nullptr);
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
  EXPECT_EQ(result.expected_nets, 1u);
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest, ActualSearchRetainsExplicitBranchStage)
{
  planRedundantBranch(true);
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
  EXPECT_EQ(result.expected_nets, 2u);
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_TRUE(result.error.empty()) << result.error;
}

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
  auto* pin = db_network_->dbToSta(delay_->getFirstOutput());
  db_network_->libertyPort(pin)->setCapacitanceLimit(1e-20, sta::MinMax::max());
  const auto result = audit();
  EXPECT_GT(result.cap_violations, 0u);
  EXPECT_EQ(result.first_cap_violation, db_network_->pathName(pin));
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
  auto* pin = db_network_->dbToSta(delay_->getFirstOutput());
  auto* port = db_network_->libertyPort(pin);
  auto* scene_port = port->scenePort(scene, sta::MinMax::max());
  ASSERT_NE(scene_port, port->scenePort(sta_->cmdScene(), sta::MinMax::max()));
  // Internal SDC master-port limits are not consulted by CheckSlews.
  // A unique downstream cell's scene-specific Liberty limit is effective.
  scene_port->setSlewLimit(1e-15, sta::MinMax::max());
  const auto result = audit();
  EXPECT_GT(result.slew_violations, 0u);
  EXPECT_EQ(result.first_slew_violation, db_network_->pathName(pin));
  EXPECT_EQ(result.first_slew_scene, scene->name());
  EXPECT_FALSE(result.error.empty());
  EXPECT_EQ(result.reached_sinks, originals_.size());
  sta::Slew slew;
  float limit, slack;
  const sta::RiseFall* rf;
  const sta::Scene* checked_scene;
  sta_->checkSlew(pin,
                  {scene},
                  sta::MinMax::max(),
                  true,
                  slew,
                  limit,
                  slack,
                  rf,
                  checked_scene);
  EXPECT_LT(slack, 0);
  EXPECT_FLOAT_EQ(limit, 1e-15);
  EXPECT_EQ(checked_scene, scene);
  sta_->checkSlew(pin,
                  {sta_->cmdScene()},
                  sta::MinMax::max(),
                  true,
                  slew,
                  limit,
                  slack,
                  rf,
                  checked_scene);
  EXPECT_GE(slack, 0);
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

TEST_F(BottomUpClockTreeTest, FailedFeasibilityUndoesRealGaterCloning)
{
  const double max_wire_dbu
      = resizer_.findMaxWireLength(false) * 1e6 * block_->getDbUnitsPerMicron();
  ASSERT_GT(max_wire_dbu, 0);
  ASSERT_LT(max_wire_dbu, std::numeric_limits<int>::max() / 32);
  const int pitch = static_cast<int>(std::ceil(4 * max_wire_dbu));
  block_->setDieArea(odb::Rect(0, 0, 6 * pitch, 2 * pitch));
  auto* gate = odb::dbInst::create(
      block_, db_->findMaster("CLKGATE_X1"), "rollback_gate");
  gate->setLocation(pitch / 2, pitch / 2);
  gate->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  gate->findITerm("CK")->connect(source_);
  auto* gated = odb::dbNet::create(block_, "rollback_gated_clock");
  gated->setSigType(odb::dbSigType::CLOCK);
  gate->findITerm("GCK")->connect(gated);
  sta_->setCaseAnalysis(db_network_->dbToSta(gate->findITerm("E")),
                        sta::LogicValue::one,
                        sta_->cmdMode());
  for (size_t i = 0; i < originals_.size(); ++i) {
    originals_[i]->connect(gated);
    originals_[i]->getInst()->setLocation(
        (1 + i / 4) * pitch, pitch / 2 + (i % 4) * std::max(1, pitch / 100));
  }
  SplitCounter counter;
  counter.addOwner(block_);
  const auto before = snapshot();
  const auto config = cts_->getParms()->saveRunConfiguration();
  auto* top = db_network_->cell(db_network_->topInstance());
  sta_->setCapacitanceLimit(
      top, sta::MinMax::max(), 1e-20, sta_->cmdMode()->sdc());
  EXPECT_LOGGER_ERROR(cts_->runTritonCts(), "CTS-0554");
  EXPECT_GT(counter.clones, 0) << "The case must actually clone clock gaters";
  EXPECT_EQ(snapshot(), before);
  EXPECT_EQ(cts_->getParms()->saveRunConfiguration(), config);
  EXPECT_EQ(reachableOriginals(), originals_.size());
  EXPECT_TRUE(cleanRunState());
  expectCallbacksRestored(counter);
  EXPECT_EQ(snapshot(), before);
}

TEST_F(BottomUpClockTreeTest, PublicCommandAuditFailureRetainsDiagnosticTree)
{
  const auto before = snapshot();
  SplitCounter observer;
  observer.addOwner(block_);
  PostWriteCapLimit inject(db_network_);
  inject.addOwner(block_);
  std::string failure;
  try {
    cts_->runTritonCts();
  } catch (const std::exception& error) {
    failure = error.what();
  }
  EXPECT_EQ(failure, "CTS-0557");
  ASSERT_NE(inject.target, nullptr)
      << "The case must inject its violation after real DB write begins";
  EXPECT_NE(snapshot(), before);
  EXPECT_EQ(block_->findInst(inject.target->getInst()->getName().c_str()),
            inject.target->getInst());
  EXPECT_TRUE(inject.target->getNet()->getName().starts_with("clknet_"));
  EXPECT_EQ(reachableOriginals(), originals_.size());
  EXPECT_TRUE(cleanRunState());
  float cap, limit, slack;
  const sta::RiseFall* rf;
  const sta::Scene* scene;
  sta_->checkCapacitance(db_network_->dbToSta(inject.target),
                         sta_->scenes(),
                         sta::MinMax::max(),
                         cap,
                         limit,
                         slack,
                         rf,
                         scene);
  EXPECT_LT(slack, 0);
  EXPECT_FLOAT_EQ(limit, 1e-20);
  EXPECT_GT(cap, limit);
  EXPECT_EQ(scene, sta_->cmdScene());
  EXPECT_TRUE(inject.hasOwner());
  expectCallbacksRestored(observer);
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
  cts_->getParms()->setMaxWl(12345);
  const auto config = cts_->getParms()->saveRunConfiguration();
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
  EXPECT_EQ(cts_->getParms()->saveRunConfiguration(), config);
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
  // Isolate the effective Liberty limit to this real balancer-added stage,
  // instead of constraining every CLKBUF_X3 in the tree.
  ASSERT_TRUE(delay->swapMaster(db_->findMaster("BUF_X1")));
  auto* pin = db_network_->dbToSta(delay->getFirstOutput());
  db_network_->libertyPort(pin)->setCapacitanceLimit(1e-20, sta::MinMax::max());
  const auto result = audit();
  EXPECT_GT(result.cap_violations, 0u);
  EXPECT_EQ(result.first_cap_violation, db_network_->pathName(pin));
  EXPECT_EQ(reachableOriginals(), originals_.size());
  EXPECT_EQ(result.reached_sinks, result.expected_sinks);
  EXPECT_FALSE(result.error.empty());
}
}  // namespace cts
