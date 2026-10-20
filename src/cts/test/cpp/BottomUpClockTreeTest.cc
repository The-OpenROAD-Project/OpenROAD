// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "BottomUpBuffering.h"
#include "BufferingSearch.h"
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
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Transition.hh"
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

  void prepareTreeModel()
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
  }

  void selectCurrentTree() { cts_->bufferClockTreeBottomUp(builder_); }
  void writeCurrentTree() { cts_->runClockTreeSynthesis(); }
  void buildCurrentLegacyTree() { builder_->run(); }

  double worstCap(const sta::LibertyPort* port)
  {
    return std::max(
        port->capacitance(sta::RiseFall::rise(), sta::MinMax::max()),
        port->capacitance(sta::RiseFall::fall(), sta::MinMax::max()));
  }

  void planRedundantBranch(bool explicit_branch)
  {
    prepareTreeModel();
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

  void planLongSpan(bool explicit_distance,
                    int max_wl_microns = 500,
                    size_t expected_buffers = 0)
  {
    const int dbu = block_->getDbUnitsPerMicron();
    block_->setDieArea(odb::Rect(0, 0, 400 * dbu, 400 * dbu));
    for (auto* pin : originals_) {
      pin->getInst()->setLocation(150 * dbu, 100 * dbu);
    }
    cts_->setSinkBuffer("BUF_X4");
    prepareTreeModel();
    auto* options = cts_->getParms();
    options->enableDummyLoad(false);
    options->setTreeBuffer("BUF_X4");
    options->setMaxWl(max_wl_microns * dbu);
    options->resetBufferDistance();
    options->setSimpleSegmentsEnabled(explicit_distance);
    if (explicit_distance) {
      options->setBufferDistance(100 * dbu);
    }
    EXPECT_EQ(options->getBufferDistance(), 100 * dbu);
    auto& clock = builder_->getClock();
    const auto point = clock.computeSinkRegion().getCenter();
    auto& root = clock.addClockBuffer(
        "clkbuf_0", "BUF_X8", point.getX() - 110 * dbu, point.getY());
    auto& branch = clock.addClockBuffer(
        "clkbuf_long_span", "BUF_X4", point.getX(), point.getY());
    builder_->addTreeLevelBuffer(&root);
    builder_->addTreeLevelBuffer(&branch);
    auto& upper = clock.addSubNet("clknet_0");
    upper.addInst(root);
    upper.addInst(branch);
    auto& lower = clock.addSubNet("clknet_long_span");
    lower.addInst(branch);
    lower.setLeafLevel(true);
    clock.forEachSink([&](ClockInst& sink) { lower.addInst(sink); });
    ASSERT_NO_THROW(selectCurrentTree());
    size_t buffers = 0;
    clock.forEachClockBuffer([&](const ClockInst& inst) {
      ++buffers;
      EXPECT_EQ(inst.getMaster(),
                inst.getName() == "clkbuf_0_clk" ? "BUF_X8" : "BUF_X4");
    });
    if (expected_buffers == 0) {
      expected_buffers = explicit_distance ? 3u : 2u;
    }
    EXPECT_EQ(buffers, expected_buffers);
    EXPECT_EQ(clock.findClockByName("clkbuf_long_span_clk") != nullptr,
              !explicit_distance)
        << "The inferred-distance case must retain the original reference, "
           "while the explicit-distance case must really insert a stage";
    writeCurrentTree();
    const auto result = audit();
    EXPECT_EQ(result.cap_violations, 0u);
    EXPECT_EQ(result.slew_violations, 0u);
    EXPECT_EQ(result.reached_sinks, originals_.size());
  }

  double prepareSlewClosureTree(int sink_x = 10000)
  {
    cts_->setBufferList("BUF_X1");
    cts_->setRootBuffer("BUF_X1");
    cts_->setSinkBuffer("BUF_X1");
    for (auto* pin : originals_) {
      pin->getInst()->setLocation(sink_x, 10000);
    }
    prepareTreeModel();
    // Keep RC positive but negligible after LUT setup so these cases isolate
    // slew closure without changing characterization's wire-length range.
    for (auto* scene : sta_->scenes()) {
      setRC(scene, 1e-6);
    }
    auto* options = cts_->getParms();
    options->enableDummyLoad(false);
    options->setTreeBuffer("BUF_X1");
    options->setMaxWl(0);
    options->setSimpleSegmentsEnabled(false);
    auto& clock = builder_->getClock();
    const auto point = clock.computeSinkRegion().getCenter();
    auto& root = clock.addClockBuffer(
        "clkbuf_0", "BUF_X1", point.getX(), point.getY());
    auto& branch = clock.addClockBuffer(
        "clkbuf_slew_branch", "BUF_X1", point.getX(), point.getY());
    builder_->addTreeLevelBuffer(&root);
    builder_->addTreeLevelBuffer(&branch);
    auto& upper = clock.addSubNet("clknet_0");
    upper.addInst(root);
    upper.addInst(branch);
    auto& lower = clock.addSubNet("clknet_slew_branch");
    lower.addInst(branch);
    lower.setLeafLevel(true);
    double load = 0;
    clock.forEachSink([&](ClockInst& sink) {
      lower.addInst(sink);
      auto* port = db_network_->libertyPort(
          db_network_->dbToSta(sink.getDbInputPin()));
      double cap = 0;
      for (auto* scene : sta_->scenes()) {
        cap = std::max(cap,
                       worstCap(port->scenePort(scene, sta::MinMax::max())));
      }
      load += cap;
    });
    return load;
  }

  double bufferOutputSlew(double input_slew, double load)
  {
    sta::LibertyPort *input, *output;
    db_network_->findLibertyCell("BUF_X1")->bufferPorts(input, output);
    const sta::Slew inputs[]{static_cast<float>(input_slew),
                             static_cast<float>(input_slew)};
    double worst = 0;
    for (auto* scene : sta_->scenes()) {
      sta::ArcDelay delays[2];
      sta::Slew slews[2];
      resizer_.gateDelays(
          output, load, inputs, scene, sta::MinMax::max(), delays, slews);
      for (const auto& slew : slews) {
        const double value = sta::delayAsFloat(slew);
        if (!std::isfinite(value) || value <= 0) {
          throw std::runtime_error("Invalid fixture buffer slew");
        }
        worst = std::max(worst, value);
      }
    }
    return worst;
  }

  double closureSlewLimit(double load)
  {
    double high = cts_->getParms()->getMaxCharSlew();
    sta::LibertyPort *input, *output;
    db_network_->findLibertyCell("BUF_X1")->bufferPorts(input, output);
    const auto tighten = [&](sta::LibertyPort* port) {
      for (auto* scene : sta_->scenes()) {
        float limit;
        bool exists;
        sta_->findSlewLimit(port, scene, sta::MinMax::max(), limit, exists);
        if (exists) {
          high = std::min(high, static_cast<double>(limit));
        }
      }
    };
    tighten(input);
    tighten(output);
    for (auto* pin : originals_) {
      tighten(db_network_->libertyPort(db_network_->dbToSta(pin)));
    }
    const double ceiling = high;
    if (!(high > 0) || bufferOutputSlew(high, load) >= high) {
      throw std::runtime_error("Fixture has no full-limit slew closure");
    }
    double low = 0;
    for (int i = 0; i < 40; ++i) {
      const double mid = (low + high) / 2;
      if (bufferOutputSlew(mid, load) > mid) {
        low = mid;
      } else {
        high = mid;
      }
    }
    const double limit = high * 1.15;
    if (limit >= ceiling) {
      throw std::runtime_error(
          "Fixture lacks margin below its true slew limit");
    }
    return limit;
  }

  double unrestorableInputSlew(double output_limit)
  {
    double input = output_limit;
    for (int i = 0; i < 12; ++i) {
      input *= 2;
      if (bufferOutputSlew(input, 0) > 1.2 * output_limit) {
        return input;
      }
    }
    throw std::runtime_error("Fixture slow input does not defeat slew closure");
  }

  sta::Clock* replaceTestClock(sta::Scene* scene,
                               const char* name,
                               odb::dbBTerm* source)
  {
    auto* sdc = scene->mode()->sdc();
    sta_->removeClock(sdc->findClock("clk"), sdc);
    sta::PinSet pins(db_network_);
    pins.insert(db_network_->dbToSta(source));
    sta_->makeClock(name, pins, false, 5e-9, {0, 2.5e-9}, "", scene->mode());
    return sdc->findClock(name);
  }

  void expectSlowPhysicalRootAliasRejected(bool explicit_root)
  {
    auto* scene = addTestScene();
    auto* alias = replaceTestClock(
        scene, "physical_root_alias", source_->get1stBTerm());
    ASSERT_NE(alias, nullptr);
    ASSERT_TRUE(alias->isIdeal());
    if (explicit_root) {
      ASSERT_EQ(cts_->setClockNets("clk"), 0);
    }
    const double load = prepareSlewClosureTree();
    const double limit = closureSlewLimit(load);
    const double slow_input = unrestorableInputSlew(limit);
    ASSERT_LT(bufferOutputSlew(limit, load), limit);
    ASSERT_GT(bufferOutputSlew(slow_input, 0), limit);
    EXPECT_EQ(builder_->getClock().getSdcName(), explicit_root ? "" : "clk");
    ASSERT_NE(builder_->getClock().getSdcName(), alias->name());
    sta_->setClockSlew(alias,
                       sta::RiseFallBoth::fall(),
                       sta::MinMaxAll::max(),
                       slow_input,
                       scene->mode()->sdc());
    auto* vertex = sta_->ensureGraph()->pinDrvrVertex(
        db_network_->dbToSta(source_->get1stBTerm()));
    ASSERT_NE(vertex, nullptr);
    ASSERT_LT(sta::delayAsFloat(sta_->slew(vertex,
                                           sta::RiseFallBoth::riseFall(),
                                           sta_->scenes(),
                                           sta::MinMax::max())),
              limit)
        << "The ideal alias must be found by physical clock membership";
    cts_->getParms()->setMaxCharSlew(limit);
    const auto before = snapshot();
    EXPECT_LOGGER_ERROR(selectCurrentTree(), "CTS-0554");
    EXPECT_EQ(snapshot(), before);
  }

  struct DummyPrediction
  {
    size_t count = 0;
    double cap = 0;
    double area = 0;
  };

  DummyPrediction predictSelectedDummies()
  {
    const size_t dimensions = 2 * sta_->scenes().size();
    const auto choice = [&](sta::LibertyCell* cell) {
      sta::LibertyPort *input, *output;
      cell->bufferPorts(input, output);
      return BufferingCellChoice{{cell->name(), worstCap(input)},
                                 input->capacitance(),
                                 cell->area(),
                                 std::vector<double>(dimensions, 0)};
    };
    auto& clock = builder_->getClock();
    std::vector<BufferingNode> nodes;
    std::map<ClockInst*, size_t> indices;
    const auto add = [&](ClockInst& inst) {
      indices[&inst] = nodes.size();
      BufferingNode node;
      node.point = {static_cast<double>(inst.getX()),
                    static_cast<double>(inst.getY())};
      node.sink = !inst.isClockBuffer();
      if (node.sink) {
        auto* port = db_network_->libertyPort(
            db_network_->dbToSta(inst.getDbInputPin()));
        node.input_cap = worstCap(port);
        node.nominal_input_cap = port->capacitance();
      } else {
        const auto cell
            = choice(db_network_->findLibertyCell(inst.getMaster().c_str()));
        node.master = node.fixed_master = cell.buffer.master;
        node.input_cap = cell.buffer.input_cap;
        node.nominal_input_cap = cell.nominal_input_cap;
      }
      nodes.push_back(std::move(node));
    };
    clock.forEachClockBuffer(add);
    clock.forEachSink(add);
    clock.forEachSubNet([&](const ClockSubNet& net) {
      net.forEachSink([&](ClockInst* sink) {
        nodes[indices.at(net.getDriver())].children.push_back(indices.at(sink));
      });
    });
    std::vector<sta::LibertyCell*> dummy_cells;
    cts_->findCandidateDummyCells(dummy_cells);
    DummyPrediction prediction;
    const BufferingDummy dummy
        = [&](float actual,
              float ideal,
              size_t fanout) -> std::optional<BufferingCellChoice> {
      if (sta::fuzzyEqual(actual, ideal)
          || fanout == cts_->getParms()->getMaxFanout()) {
        return std::nullopt;
      }
      auto* cell = cts_->findBestDummyCell(dummy_cells, ideal - actual);
      if (cell == nullptr) {
        return std::nullopt;
      }
      auto load = choice(cell);
      ++prediction.count;
      prediction.cap += load.buffer.input_cap;
      return load;
    };
    const BufferingChoices fixed = [&](const auto&, const std::string& master) {
      return std::vector{choice(db_network_->findLibertyCell(master.c_str()))};
    };
    // This independently checks nominal dummy policy, actual input cap, and
    // cell area on the selected tree. Real timing/electricals are audited
    // after write; zero test delays do not substitute for that STA audit.
    const auto plan
        = evaluateBuffering(nodes,
                            indices.at(clock.findClockByName("clkbuf_0_clk")),
                            {0, 0, 1e9, dimensions, 16},
                            fixed,
                            dummy);
    if (!plan) {
      throw std::runtime_error(
          "Selected tree cannot be evaluated for dummy prediction");
    }
    prediction.area = plan->area;
    return prediction;
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

TEST_F(BottomUpClockTreeTest, FullSlewLimitClosesWhereHalfBudgetRejects)
{
  const double load = prepareSlewClosureTree();
  const double limit = closureSlewLimit(load);
  ASSERT_LT(bufferOutputSlew(limit, load), limit);
  ASSERT_GT(bufferOutputSlew(limit / 2, load), limit / 2)
      << "This must reproduce the old half-budget false rejection";
  cts_->getParms()->setMaxCharSlew(limit);
  sta_->setSlewLimit(db_network_->cell(db_network_->topInstance()),
                     sta::MinMax::max(),
                     limit,
                     sta_->cmdMode()->sdc());
  ASSERT_NO_THROW(selectCurrentTree());
  size_t buffers = 0;
  builder_->getClock().forEachClockBuffer([&](const ClockInst& inst) {
    ++buffers;
    EXPECT_EQ(inst.getMaster(), "BUF_X1");
  });
  EXPECT_EQ(buffers, 2u) << "Both fixed stages must satisfy the same bound";
  writeCurrentTree();
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
  EXPECT_EQ(result.expected_nets, 2u);
  EXPECT_EQ(result.reached_nets, result.expected_nets);
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_EQ(result.cap_violations, 0u);
  EXPECT_EQ(result.slew_violations, 0u);
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest, FullBudgetDoesNotRelaxTrueTightOutputLimit)
{
  prepareSlewClosureTree();
  const double limit = bufferOutputSlew(0, 0) / 2;
  ASSERT_GT(bufferOutputSlew(limit, 0), limit)
      << "Even an unloaded root cannot meet this true output constraint";
  sta_->setSlewLimit(db_network_->cell(db_network_->topInstance()),
                     sta::MinMax::max(),
                     limit,
                     sta_->cmdMode()->sdc());
  // Leave max characterization slew unchanged: the real SDC limit must bind.
  ASSERT_GT(cts_->getParms()->getMaxCharSlew(), limit);
  const auto before = snapshot();
  EXPECT_LOGGER_ERROR(selectCurrentTree(), "CTS-0554");
  EXPECT_EQ(snapshot(), before);
  EXPECT_EQ(reachableOriginals(), originals_.size());
}

TEST_F(BottomUpClockTreeTest, SlowIdealClockInOtherModeBoundsRootInput)
{
  auto* scene = addTestScene();
  auto* clock = scene->mode()->sdc()->findClock("clk");
  ASSERT_NE(clock, nullptr);
  sta_->removePropagatedClock(clock, scene->mode());
  const double load = prepareSlewClosureTree();
  const double limit = closureSlewLimit(load);
  const double slow_input = unrestorableInputSlew(limit);
  ASSERT_LT(bufferOutputSlew(limit, load), limit)
      << "Ignoring the incoming ideal-clock transition would accept this tree";
  ASSERT_GT(bufferOutputSlew(slow_input, 0), limit);
  ASSERT_NE(scene->mode(), sta_->cmdMode());
  ASSERT_TRUE(clock->isIdeal());
  sta_->setClockSlew(clock,
                     sta::RiseFallBoth::fall(),
                     sta::MinMaxAll::max(),
                     slow_input,
                     scene->mode()->sdc());
  EXPECT_FLOAT_EQ(clock->slew(sta::RiseFall::fall(), sta::MinMax::max()),
                  static_cast<float>(slow_input));
  auto* source_pin = db_network_->dbToSta(source_->get1stBTerm());
  auto* vertex = sta_->ensureGraph()->pinDrvrVertex(source_pin);
  ASSERT_NE(vertex, nullptr);
  EXPECT_LT(sta::delayAsFloat(sta_->slew(vertex,
                                         sta::RiseFallBoth::riseFall(),
                                         sta_->scenes(),
                                         sta::MinMax::max())),
            limit)
      << "Raw source slew must not substitute for ideal clock transition";
  cts_->getParms()->setMaxCharSlew(limit);
  const auto before = snapshot();
  EXPECT_LOGGER_ERROR(selectCurrentTree(), "CTS-0554");
  EXPECT_EQ(snapshot(), before);
}

TEST_F(BottomUpClockTreeTest, PropagatedSourceSlewInOtherSceneBoundsRootInput)
{
  auto* scene = addTestScene();
  const double load = prepareSlewClosureTree();
  const double limit = closureSlewLimit(load);
  const double slow_input = unrestorableInputSlew(limit);
  ASSERT_LT(bufferOutputSlew(limit, load), limit);
  ASSERT_GT(bufferOutputSlew(slow_input, 0), limit);
  ASSERT_NE(scene, sta_->cmdScene());
  auto* clock = scene->mode()->sdc()->findClock("clk");
  ASSERT_NE(clock, nullptr);
  ASSERT_TRUE(clock->isPropagated());
  EXPECT_FLOAT_EQ(clock->slew(sta::RiseFall::fall(), sta::MinMax::max()), 0);
  auto* source_pin = db_network_->dbToSta(source_->get1stBTerm());
  sta_->setInputSlew(db_network_->port(source_pin),
                     sta::RiseFallBoth::fall(),
                     sta::MinMaxAll::max(),
                     slow_input,
                     scene->mode()->sdc());
  auto* vertex = sta_->ensureGraph()->pinDrvrVertex(source_pin);
  ASSERT_NE(vertex, nullptr);
  EXPECT_FLOAT_EQ(
      sta::delayAsFloat(sta_->slew(
          vertex, sta::RiseFallBoth::fall(), {scene}, sta::MinMax::max())),
      static_cast<float>(slow_input));
  EXPECT_LT(sta::delayAsFloat(sta_->slew(vertex,
                                         sta::RiseFallBoth::riseFall(),
                                         {sta_->cmdScene()},
                                         sta::MinMax::max())),
            limit);
  cts_->getParms()->setMaxCharSlew(limit);
  const auto before = snapshot();
  EXPECT_LOGGER_ERROR(selectCurrentTree(), "CTS-0554");
  EXPECT_EQ(snapshot(), before);
}

TEST_F(BottomUpClockTreeTest, DifferentClockNameOnSamePhysicalRootBoundsInput)
{
  expectSlowPhysicalRootAliasRejected(false);
}

TEST_F(BottomUpClockTreeTest, ExplicitClockNetStillFindsIdealPhysicalRootAlias)
{
  expectSlowPhysicalRootAliasRejected(true);
}

TEST_F(BottomUpClockTreeTest, UnrelatedSameNameClockDoesNotTightenRootInput)
{
  auto* scene = addTestScene();
  auto* unrelated_net = odb::dbNet::create(block_, "unrelated_clock_net");
  unrelated_net->setSigType(odb::dbSigType::CLOCK);
  auto* unrelated_source
      = odb::dbBTerm::create(unrelated_net, "unrelated_clock_port");
  unrelated_source->setIoType(odb::dbIoType::INPUT);
  unrelated_source->setSigType(odb::dbSigType::CLOCK);
  auto* bpin = odb::dbBPin::create(unrelated_source);
  bpin->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  ASSERT_NE(
      odb::dbBox::create(
          bpin, block_->getTech()->findLayer("metal6"), 1000, 1000, 1280, 1280),
      nullptr);
  auto* unrelated = replaceTestClock(scene, "clk", unrelated_source);
  ASSERT_NE(unrelated, nullptr);
  ASSERT_TRUE(unrelated->isIdeal());
  const double load = prepareSlewClosureTree();
  const double limit = closureSlewLimit(load);
  const double slow_input = unrestorableInputSlew(limit);
  ASSERT_LT(bufferOutputSlew(limit, load), limit);
  ASSERT_GT(bufferOutputSlew(slow_input, 0), limit)
      << "Including the unrelated same-name clock would falsely reject";
  ASSERT_EQ(builder_->getClock().getSdcName(), unrelated->name());
  ASSERT_FALSE(
      unrelated->pins().contains(db_network_->dbToSta(source_->get1stBTerm())));
  sta_->setClockSlew(unrelated,
                     sta::RiseFallBoth::fall(),
                     sta::MinMaxAll::max(),
                     slow_input,
                     scene->mode()->sdc());
  cts_->getParms()->setMaxCharSlew(limit);
  ASSERT_NO_THROW(selectCurrentTree());
  writeCurrentTree();
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
  EXPECT_EQ(result.expected_nets, 2u);
  EXPECT_EQ(result.reached_nets, result.expected_nets);
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest, SourceDriverSlewAloneDoesNotBoundLongIngressWire)
{
  const int dbu = block_->getDbUnitsPerMicron();
  block_->setDieArea(odb::Rect(0, 0, 6000 * dbu, 100 * dbu));
  const double load = prepareSlewClosureTree(5000 * dbu);
  setRC(sta_->cmdScene(), 3);
  auto* clock = sta_->cmdMode()->sdc()->findClock("clk");
  ASSERT_NE(clock, nullptr);
  sta_->setPropagatedClock(clock, sta_->cmdMode());
  const double limit = closureSlewLimit(load);
  ASSERT_LT(bufferOutputSlew(limit, load), limit);
  auto* root = builder_->getClock().findClockByName("clkbuf_0_clk");
  ASSERT_NE(root, nullptr);
  auto* source = source_->get1stBTerm();
  int x, y;
  ASSERT_TRUE(source->getFirstPinLocation(x, y));
  const double ingress_length
      = (std::abs(root->getX() - x) + std::abs(root->getY() - y)) * 1e-6 / dbu;
  ASSERT_GT(ingress_length, 4e-3);
  sta::LibertyPort *input, *output;
  db_network_->findLibertyCell("BUF_X1")->bufferPorts(input, output);
  const auto rc = bound();
  const double ingress_slew
      = 2.2 * rc.resistance * ingress_length
        * (worstCap(input) + rc.capacitance * ingress_length);
  ASSERT_GT(bufferOutputSlew(ingress_slew, 0), limit)
      << "Ingress alone must defeat even unloaded root slew restoration";
  auto* vertex
      = sta_->ensureGraph()->pinDrvrVertex(db_network_->dbToSta(source));
  ASSERT_NE(vertex, nullptr);
  ASSERT_LT(sta::delayAsFloat(sta_->slew(vertex,
                                         sta::RiseFallBoth::riseFall(),
                                         sta_->scenes(),
                                         sta::MinMax::max())),
            limit)
      << "Checking the source driver but not its ingress would accept";
  cts_->getParms()->setMaxCharSlew(limit);
  const auto before = snapshot();
  EXPECT_LOGGER_ERROR(selectCurrentTree(), "CTS-0554");
  EXPECT_EQ(snapshot(), before);
}

TEST_F(BottomUpClockTreeTest,
       InferredDistanceCannotDiscardFeasibleLegacyReference)
{
  planLongSpan(false);
  EXPECT_NE(block_->findInst("clkbuf_long_span_clk"), nullptr);
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
  EXPECT_EQ(result.expected_nets, 2u);
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest, UnsetWireLengthDoesNotInventDefaultDistanceLimit)
{
  planLongSpan(false, 0);
  EXPECT_NE(block_->findInst("clkbuf_long_span_clk"), nullptr);
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
  EXPECT_EQ(result.expected_nets, 2u);
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest, ExplicitDistanceBoundsOtherwiseUnsetWireLength)
{
  planLongSpan(true, 0);
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
  EXPECT_EQ(result.expected_nets, 3u);
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest,
       ExplicitDistanceStillRequiresActualElectricalStage)
{
  planLongSpan(true);
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
  EXPECT_EQ(result.expected_nets, 3u);
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest,
       FiniteWireLimitTakesPrecedenceOverExplicitDistance)
{
  // 40 um binds before both the 100 um explicit spacing and 110 um branch.
  // The distance-only case needs three cells; this case really needs more.
  planLongSpan(true, 40, 5);
  const int limit = 40 * block_->getDbUnitsPerMicron();
  EXPECT_LT(limit, cts_->getParms()->getBufferDistance());
  size_t edges = 0;
  builder_->getClock().forEachSubNet([&](const ClockSubNet& subnet) {
    auto* net = subnet.getNetObj();
    ASSERT_NE(net, nullptr);
    auto* driver = net->getFirstOutput();
    ASSERT_NE(driver, nullptr);
    int x, y;
    ASSERT_TRUE(driver->getAvgXY(&x, &y));
    for (auto* pin : net->getITerms()) {
      if (pin == driver) {
        continue;
      }
      int sink_x, sink_y;
      ASSERT_TRUE(pin->getAvgXY(&sink_x, &sink_y));
      EXPECT_LE(std::abs(x - sink_x) + std::abs(y - sink_y), limit)
          << net->getName() << ": " << pin->getName();
      ++edges;
    }
  });
  EXPECT_EQ(edges, originals_.size() + 4);
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
  EXPECT_EQ(result.expected_nets, 5u);
  EXPECT_EQ(result.reached_nets, result.expected_nets);
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_EQ(result.cap_violations, 0u);
  EXPECT_EQ(result.slew_violations, 0u);
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest, OriginalSiteSearchSurvivesInfeasibleFastestSeed)
{
  cts_->setBufferList("BUF_X2 BUF_X8");
  cts_->setRootBuffer("BUF_X1");
  cts_->setSinkBuffer("BUF_X2");
  for (auto* pin : originals_) {
    pin->getInst()->setLocation(10000, 10000);
  }
  prepareTreeModel();
  auto* options = cts_->getParms();
  options->enableDummyLoad(false);
  options->setSinkBufferInferred(true);
  options->setMaxWl(1000000);
  options->setBufferDistance(1000000);
  sta::LibertyPort *small_input, *small_output, *large_input, *large_output;
  db_network_->findLibertyCell("BUF_X2")->bufferPorts(small_input,
                                                      small_output);
  db_network_->findLibertyCell("BUF_X8")->bufferPorts(large_input,
                                                      large_output);
  const double root_limit = (worstCap(small_input) + worstCap(large_input)) / 2;
  sta::LibertyPort *root_input, *root_output;
  db_network_->findLibertyCell("BUF_X1")->bufferPorts(root_input, root_output);
  root_output->setCapacitanceLimit(root_limit, sta::MinMax::max());
  EXPECT_LT(worstCap(small_input), root_limit);
  EXPECT_GT(worstCap(large_input), root_limit);
  double load = 0;
  for (auto* pin : originals_) {
    load += worstCap(db_network_->libertyPort(db_network_->dbToSta(pin)));
  }
  EXPECT_GT(load, root_limit) << "The root cannot simply bypass its child";
  const auto slew = static_cast<float>(options->getMaxCharSlew() * 0.5);
  const sta::Slew input_slews[]{slew, slew};
  const auto delay = [&](sta::LibertyPort* port) {
    sta::ArcDelay delays[2];
    sta::Slew slews[2];
    resizer_.gateDelays(port,
                        load,
                        input_slews,
                        sta_->cmdScene(),
                        sta::MinMax::max(),
                        delays,
                        slews);
    EXPECT_LE(
        std::max(sta::delayAsFloat(slews[0]), sta::delayAsFloat(slews[1])),
        slew);
    return std::max(sta::delayAsFloat(delays[0]), sta::delayAsFloat(delays[1]));
  };
  EXPECT_LT(delay(large_output), delay(small_output))
      << "The real Liberty characterization must make the oversized child "
         "fastest";
  auto& clock = builder_->getClock();
  const auto point = clock.computeSinkRegion().getCenter();
  auto& root
      = clock.addClockBuffer("clkbuf_0", "BUF_X1", point.getX(), point.getY());
  auto& child = clock.addClockBuffer(
      "clkbuf_oversized", "BUF_X8", point.getX(), point.getY());
  builder_->addTreeLevelBuffer(&root);
  builder_->addTreeLevelBuffer(&child);
  auto& upper = clock.addSubNet("clknet_0");
  upper.addInst(root);
  upper.addInst(child);
  auto& lower = clock.addSubNet("clknet_oversized");
  lower.addInst(child);
  lower.setLeafLevel(true);
  clock.forEachSink([&](ClockInst& sink) { lower.addInst(sink); });
  // The legacy child overloads the fixed root. Coincident sites prevent
  // greedy expansion from repairing its fastest-child choice by splitting.
  ASSERT_NO_THROW(selectCurrentTree());
  size_t buffers = 0;
  clock.forEachClockBuffer([&](const ClockInst& inst) {
    ++buffers;
    EXPECT_EQ(inst.getMaster(),
              inst.getName() == "clkbuf_0_clk" ? "BUF_X1" : "BUF_X2");
  });
  EXPECT_EQ(buffers, 2u);
  writeCurrentTree();
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
  EXPECT_EQ(result.reached_sinks, originals_.size());
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest, ZeroHTreeLevelsKeepActualPreclusteredFrontier)
{
  constexpr size_t sink_count = 201;
  for (size_t i = originals_.size(); i < sink_count; ++i) {
    auto* inst
        = odb::dbInst::create(block_,
                              db_->findMaster("DFF_X1"),
                              ("cluster_ff" + std::to_string(i)).c_str());
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    auto* pin = inst->findITerm("CK");
    pin->connect(source_);
    originals_.push_back(pin);
  }
  for (size_t i = 0; i < originals_.size(); ++i) {
    originals_[i]->getInst()->setLocation(10000 + (i % 16) * 2000,
                                          10000 + (i / 16) * 2000);
  }
  cts_->setSinkBuffer("BUF_X4");
  auto* options = cts_->getParms();
  options->setSinkClustering(true);
  options->setSinkClusteringSize(16);
  options->setMaxDiameter(1000);
  options->setClockTreeMaxDepth(0);
  options->setEnableFakeLutEntries(false);
  prepareTreeModel();
  buildCurrentLegacyTree();
  auto& clock = builder_->getClock();
  size_t clusters = 0;
  clock.forEachClockBuffer([&](ClockInst& inst) {
    if (inst.getName() != "clkbuf_0_clk") {
      EXPECT_TRUE(inst.getName().starts_with("clkbuf_leaf_"));
      EXPECT_TRUE(builder_->isFirstLevelSinkDriver(&inst));
      ++clusters;
    }
  });
  ASSERT_GT(clusters, 1u) << "The case must create real precluster buffers";
  size_t root_children = 0;
  clock.forEachSubNet([&](const ClockSubNet& net) {
    if (net.getDriver()->getName() == "clkbuf_0_clk") {
      net.forEachSink([&](ClockInst* sink) {
        EXPECT_TRUE(sink->isClockBuffer())
            << "Original pins below clusters must not bypass them";
        ++root_children;
      });
    }
  });
  EXPECT_EQ(root_children, clusters);
  ASSERT_NO_THROW(selectCurrentTree());
  writeCurrentTree();
  EXPECT_EQ(reachableOriginals(), sink_count);
  const auto result = audit();
  EXPECT_EQ(result.reached_sinks, sink_count);
  EXPECT_EQ(result.reached_nets, result.expected_nets);
  EXPECT_TRUE(result.error.empty()) << result.error;
}

TEST_F(BottomUpClockTreeTest, PredictedDummyLoadAndAreaMatchActualDbInsertion)
{
  prepareTreeModel();
  auto* options = cts_->getParms();
  options->enableDummyLoad(true);
  options->setTreeBuffer("BUF_X8");
  options->setMaxWl(1000000);
  options->setBufferDistance(1000000);
  auto& clock = builder_->getClock();
  const auto point = clock.computeSinkRegion().getCenter();
  auto& root
      = clock.addClockBuffer("clkbuf_0", "BUF_X8", point.getX(), point.getY());
  auto& light = clock.addClockBuffer(
      "clkbuf_light", "BUF_X8", point.getX(), point.getY());
  auto& heavy = clock.addClockBuffer(
      "clkbuf_heavy", "BUF_X8", point.getX(), point.getY());
  for (auto* inst : {&root, &light, &heavy}) {
    builder_->addTreeLevelBuffer(inst);
  }
  auto& upper = clock.addSubNet("clknet_0");
  upper.addInst(root);
  upper.addInst(light);
  upper.addInst(heavy);
  auto& light_net = clock.addSubNet("clknet_light");
  light_net.addInst(light);
  light_net.setLeafLevel(true);
  auto& heavy_net = clock.addSubNet("clknet_heavy");
  heavy_net.addInst(heavy);
  heavy_net.setLeafLevel(true);
  size_t index = 0;
  clock.forEachSink([&](ClockInst& sink) {
    (index++ == 0 ? light_net : heavy_net).addInst(sink);
  });
  ASSERT_NO_THROW(selectCurrentTree());
  const auto prediction = predictSelectedDummies();
  ASSERT_EQ(prediction.count, 1u)
      << "Unequal real leaf loads must require a dummy";
  ASSERT_GT(prediction.cap, 0);
  writeCurrentTree();
  size_t count = 0;
  double cap = 0, area = 0;
  for (auto* inst : block_->getInsts()) {
    const bool dummy
        = inst->getName().starts_with(options->getDummyLoadPrefix());
    if (dummy || inst->getName().starts_with("clkbuf_")) {
      auto* cell
          = db_network_->findLibertyCell(inst->getMaster()->getConstName());
      area += cell->area();
      if (dummy) {
        ++count;
        sta::LibertyPort *input, *output;
        cell->bufferPorts(input, output);
        cap += worstCap(input);
      }
    }
  }
  EXPECT_EQ(count, prediction.count);
  EXPECT_DOUBLE_EQ(cap, prediction.cap);
  EXPECT_NEAR(area, prediction.area, 1e-6);
  EXPECT_EQ(reachableOriginals(), originals_.size());
  const auto result = audit();
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
