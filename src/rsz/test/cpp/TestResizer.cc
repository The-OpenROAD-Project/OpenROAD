// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "BufferedNet.hh"
#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "RepairDesign.hh"
#include "RepairSetupContext.hh"
#include "RepairTargetCollector.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "move/MoveCandidate.hh"
#include "move/MoveGenerator.hh"
#include "move/SizeUpGenerator.hh"
#include "move/VtSwapGenerator.hh"
#include "odb/db.h"
#include "odb/defin.h"
#include "policy/SetupLegacyPolicy.hh"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
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
  using SetupLegacyBase::terminateProgress;
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

class TestSizeUpGenerator : public SizeUpGenerator
{
 public:
  using MoveGenerator::weakerCellFirst;
  using SizeUpGenerator::SizeUpGenerator;
  using SizeUpGenerator::upsizeCell;
};

namespace {

class TestRepairDesign : public RepairDesign
{
 public:
  explicit TestRepairDesign(Resizer* resizer) : RepairDesign(resizer) {}

  using RepairDesign::findBufferUnderSlew;
  using RepairDesign::init;
  using RepairDesign::isBufferDrcClean;
  using RepairDesign::repairNet;
};

}  // namespace

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

  void setupRepairNetTest(TestRepairDesign& repair_design)
  {
    setupTimeBorrowTiming("inferred_clock_gator_time_borrow.def",
                          /*input_delay=*/0.98);
    const sta::Scene* corner = sta_->cmdScene();
    ep_.setHWireSignalRC(
        /*tech=*/nullptr, corner, /*res=*/1.0e3, /*cap=*/1.0e-10);
    ep_.setVWireSignalRC(
        /*tech=*/nullptr, corner, /*res=*/1.0e3, /*cap=*/1.0e-10);
    ep_.setHWireClkRC(
        /*tech=*/nullptr, corner, /*res=*/1.0e3, /*cap=*/1.0e-10);
    ep_.setVWireClkRC(
        /*tech=*/nullptr, corner, /*res=*/1.0e3, /*cap=*/1.0e-10);
    block_->setDieArea(odb::Rect(0, 0, 2000000, 240000));
    block_->setCoreArea(odb::Rect(0, 0, 2000000, 240000));

    odb::dbInst* gated_ff0 = block_->findInst("gated_ff0");
    odb::dbInst* gated_ff2 = block_->findInst("gated_ff2");
    odb::dbInst* gated_ff3 = block_->findInst("gated_ff3");
    odb::dbInst* sibling_branch = block_->findInst("sibling_branch");
    ASSERT_NE(gated_ff0, nullptr);
    ASSERT_NE(gated_ff2, nullptr);
    ASSERT_NE(gated_ff3, nullptr);
    ASSERT_NE(sibling_branch, nullptr);

    gated_ff0->setLocation(1720000, 150000);
    gated_ff2->setLocation(920000, 158985);
    gated_ff3->setLocation(920000, 167045);
    sibling_branch->setLocation(1800000, 89470);
    block_->findNet("clk")->setSigType(odb::dbSigType::CLOCK);

    resizer_.initBlock();
    ep_.estimateWireParasitics();
    sta_->updateTiming(true);
    resizer_.resizePreamble();
    repair_design.init();
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

  const Target* findTarget(const std::vector<Target>& targets,
                           const char* pin_name) const
  {
    for (const Target& target : targets) {
      if (target.driver_pin != nullptr
          && std::string(db_network_->pathName(target.driver_pin))
                 == pin_name) {
        return &target;
      }
    }
    return nullptr;
  }

  bool hasTargetPin(const std::vector<Target>& targets,
                    const char* pin_name) const
  {
    return findTarget(targets, pin_name) != nullptr;
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

  void createSkewBufMaster(odb::dbLib* db_lib,
                           const char* name,
                           const int width_scale,
                           odb::dbTechLayer* implant_layer)
  {
    odb::dbMaster* template_master = db_->findMaster("BUF_X1");
    ASSERT_NE(template_master, nullptr);

    odb::dbMaster* master = odb::dbMaster::create(db_lib, name);
    ASSERT_NE(master, nullptr);
    master->setType(odb::dbMasterType::CORE);
    master->setSite(template_master->getSite());
    master->setWidth(width_scale * template_master->getWidth());
    master->setHeight(template_master->getHeight());

    for (odb::dbMTerm* template_mterm : template_master->getMTerms()) {
      odb::dbMTerm::create(master,
                           template_mterm->getConstName(),
                           template_mterm->getIoType(),
                           template_mterm->getSigType());
    }
    odb::dbBox::create(master,
                       implant_layer,
                       0,
                       0,
                       width_scale * template_master->getWidth(),
                       template_master->getHeight());
    master->setFrozen();
  }

  sta::LibertyLibrary* loadSkewVtLibraryAndSetup()
  {
    odb::dbTech* tech = db_->getTech();
    EXPECT_NE(tech, nullptr);
    odb::dbTechLayer* vtr_layer
        = odb::dbTechLayer::create(tech, "VTR", odb::dbTechLayerType::IMPLANT);
    odb::dbTechLayer* vtl_layer
        = odb::dbTechLayer::create(tech, "VTL", odb::dbTechLayerType::IMPLANT);
    odb::dbTechLayer* vtsl_layer
        = odb::dbTechLayer::create(tech, "VTSL", odb::dbTechLayerType::IMPLANT);

    odb::dbLib* skew_db_lib
        = odb::dbLib::create(db_.get(), "TestResizerSkewVt", tech);
    EXPECT_NE(skew_db_lib, nullptr);
    createSkewBufMaster(skew_db_lib, "SKEW_BUF_D1_RVT", 2, vtr_layer);
    createSkewBufMaster(skew_db_lib, "SKEW_BUF_SKRD2_RVT", 3, vtr_layer);
    createSkewBufMaster(skew_db_lib, "SKEW_BUF_D2_RVT", 4, vtr_layer);
    createSkewBufMaster(skew_db_lib, "SKEW_BUF_D2_LVT", 4, vtl_layer);
    createSkewBufMaster(skew_db_lib, "SKEW_BUF_SKFD2_SLVT", 4, vtsl_layer);
    createSkewBufMaster(skew_db_lib, "SKEW_BUF_D3_RVT", 6, vtr_layer);
    db_network_->readLefAfter(skew_db_lib);

    sta::LibertyLibrary* skew_lib
        = readLiberty(test_root_path_ + "cpp/TestResizerSkewVt.lib");
    EXPECT_NE(skew_lib, nullptr);

    setupTimeBorrowTiming("inferred_clock_gator_time_borrow.def", 0.98);
    return skew_lib;
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
  // An input with no fanout is on no timing path (OpenSTA #523).
  EXPECT_TRUE(sta::fuzzyInf(sta_->slack(isolated, max)));
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

TEST_F(TestResizer, BufferRemovalPreservesCandidateAndSurvivorFlags)
{
  readDefForTiming("remove_buffers3.def");
  resizer_.initBlock();

  odb::dbInst* db_buffer = block_->findInst("b2");
  odb::dbNet* db_survivor = block_->findNet("n1");
  odb::dbNet* db_removed = block_->findNet("n2");
  ASSERT_NE(db_buffer, nullptr);
  ASSERT_NE(db_survivor, nullptr);
  ASSERT_NE(db_removed, nullptr);
  sta::Instance* buffer = db_network_->dbToSta(db_buffer);
  ASSERT_NE(buffer, nullptr);

  // Given a protected candidate, when queried, then all flags stay unchanged.
  db_buffer->setDoNotTouch(true);
  db_buffer->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  db_survivor->setDoNotTouch(true);
  db_removed->setDoNotTouch(true);
  EXPECT_FALSE(resizer_.canRemoveBuffer(buffer, true));
  ASSERT_TRUE(resizer_.canRemoveBuffer(buffer, false));
  EXPECT_TRUE(db_buffer->isDoNotTouch());
  EXPECT_TRUE(db_buffer->isFixed());
  EXPECT_TRUE(db_survivor->isDoNotTouch());
  EXPECT_TRUE(db_removed->isDoNotTouch());

  // Given an eligible candidate, when removed, then the survivor stays
  // protected.
  ASSERT_TRUE(resizer_.removeBuffer(buffer, /*honor_dont_touch_fixed=*/false));
  EXPECT_EQ(block_->findInst("b2"), nullptr);
  EXPECT_EQ(block_->findNet("n2"), nullptr);
  EXPECT_TRUE(db_survivor->isDoNotTouch());
  EXPECT_EQ(block_->findInst("b3")->findITerm("A")->getNet(), db_survivor);
}

TEST_F(TestResizer, BufferRemovalWithDisconnectedOutputPreservesInputNet)
{
  readDefForTiming("remove_buffers3.def");
  resizer_.initBlock();

  odb::dbInst* db_buffer = block_->findInst("b2");
  odb::dbNet* db_input = block_->findNet("n1");
  ASSERT_NE(db_buffer, nullptr);
  ASSERT_NE(db_input, nullptr);
  sta::Instance* buffer = db_network_->dbToSta(db_buffer);
  ASSERT_NE(buffer, nullptr);
  sta::Pin* output_pin = db_network_->dbToSta(db_buffer->findITerm("Z"));
  ASSERT_NE(output_pin, nullptr);
  sta_->disconnectPin(output_pin);

  // Given an unconnected output, when removed, then the input net stays
  // protected.
  db_buffer->setDoNotTouch(true);
  db_buffer->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  db_input->setDoNotTouch(true);
  ASSERT_TRUE(resizer_.removeBuffer(buffer, /*honor_dont_touch_fixed=*/false));
  EXPECT_EQ(block_->findInst("b2"), nullptr);
  EXPECT_TRUE(db_input->isDoNotTouch());
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
  const Target* data_target = findTarget(targets, "enable_buf0/Z");
  ASSERT_NE(data_target, nullptr);
  EXPECT_EQ(db_network_->pathName(data_target->endpointPin(resizer_)),
            "enable_latch/D");
  EXPECT_EQ(data_target->violatedEndpointPin(resizer_), path->pin(sta_.get()));
  EXPECT_NE(data_target->endpointPin(resizer_),
            data_target->violatedEndpointPin(resizer_));

  const Target* latch_target = findTarget(targets, "enable_latch/Q");
  ASSERT_NE(latch_target, nullptr);
  EXPECT_EQ(latch_target->endpointPin(resizer_), path->pin(sta_.get()));
  EXPECT_EQ(latch_target->violatedEndpointPin(resizer_), path->pin(sta_.get()));
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
  const Target* deep_target = findTarget(targets, "deep_buf0/Z");
  ASSERT_NE(deep_target, nullptr);
  EXPECT_EQ(db_network_->pathName(deep_target->endpointPin(resizer_)),
            "deep_latch/D");
  EXPECT_EQ(deep_target->violatedEndpointPin(resizer_), path->pin(sta_.get()));
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

// With a positive slack margin, repair can continue after TNS reaches 0. The
// fix rate is TNS based, so it must not end that margin-only repair.
TEST_F(TestResizer, TerminateProgressIgnoresFixRateWhenTnsIsZero)
{
  readVerilogAndSetup("TestResizer_SwapPinsFeedthroughModNet_pre.v",
                      /*init_default_sdc=*/true,
                      /*hierarchy=*/false);
  resizer_.initBlock();
  sta_->updateTiming(true);

  const sta::MinMax* max = sta::MinMax::max();
  sta::Pin* out1_pin = findTopPin("out1");
  ASSERT_NE(out1_pin, nullptr);
  sta::Sdc* sdc = sta_->cmdMode()->sdc();
  sta::Clock* clk = sdc->findClock("clk");
  ASSERT_NE(clk, nullptr);
  const sta::Slack out1_base_slack
      = sta_->slack(sta_->graph()->pinLoadVertex(out1_pin), max);
  auto set_out1_slack = [&](const float slack) {
    sta_->setOutputDelay(out1_pin,
                         sta::RiseFallBoth::riseFall(),
                         clk,
                         sta::RiseFall::rise(),
                         nullptr,
                         false,
                         false,
                         sta::MinMaxAll::all(),
                         /*add=*/false,
                         out1_base_slack - staTime(slack),
                         sdc);
    sta_->updateTiming(true);
  };

  MoveCommitter committer(resizer_);
  RepairSetupContext setup_context(resizer_);
  OptimizerRunConfig run_config;
  run_config.setup_slack_margin = staTime(0.10f);
  TestSetupLegacyPolicy policy(resizer_, committer, setup_context, run_config);
  ASSERT_TRUE(policy.start());

  // Past iteration 1000, on a fix-rate check interval.
  constexpr int kIteration = 1100;
  float threshold = 0.0001f;

  // TNS went from -0.20 to 0: no TNS progress left, but margin work remains.
  set_out1_slack(0.05f);
  ASSERT_EQ(sta_->totalNegativeSlack(max), 0.0f);
  float prev_tns = 0.0f;
  EXPECT_FALSE(policy.terminateProgress(
      kIteration, staTime(-0.20f), prev_tns, threshold, 0, 1, "TEST", 'a'));

  // Without a margin, TNS 0 leaves no work, so the fix rate still ends it.
  run_config.setup_slack_margin = 0.0;
  TestSetupLegacyPolicy no_margin_policy(
      resizer_, committer, setup_context, run_config);
  ASSERT_TRUE(no_margin_policy.start());
  prev_tns = 0.0f;
  EXPECT_TRUE(no_margin_policy.terminateProgress(
      kIteration, staTime(-0.20f), prev_tns, threshold, 0, 1, "TEST", 'a'));

  // A true violation that stopped improving still ends the repair.
  set_out1_slack(-0.20f);
  prev_tns = sta_->totalNegativeSlack(max);
  EXPECT_TRUE(policy.terminateProgress(
      kIteration, staTime(-0.20f), prev_tns, threshold, 0, 1, "TEST", 'a'));
}

TEST_F(TestResizer, GainBufferingBuffersOnlyNegativeSlackOutputPorts)
{
  readVerilogAndSetup("TestResizer_SwapPinsFeedthroughModNet_pre.v",
                      /*init_default_sdc=*/true,
                      /*hierarchy=*/false);

  odb::dbInst* target_db = block_->findInst("target");
  odb::dbInst* probe_db = block_->findInst("probe");
  ASSERT_NE(target_db, nullptr);
  ASSERT_NE(probe_db, nullptr);

  sta::Instance* probe_inst = db_network_->dbToSta(probe_db);
  ASSERT_NE(probe_inst, nullptr);
  sta::LibertyCell* clkbuf_x1 = sta_->network()->findLibertyCell("CLKBUF_X1");
  ASSERT_NE(clkbuf_x1, nullptr);
  ASSERT_TRUE(resizer_.replaceCell(probe_inst, clkbuf_x1));

  // Constrain target (OR2_X1) and probe (CLKBUF_X1) to their X1 sizes so both
  // output drivers have a small maximum swappable input capacitance (~1 fF)
  // while leaving BUF_X* available for gain buffering.
  for (const char* cell_name : {"OR2_X2", "OR2_X4", "CLKBUF_X2", "CLKBUF_X3"}) {
    sta::LibertyCell* cell = sta_->network()->findLibertyCell(cell_name);
    ASSERT_NE(cell, nullptr);
    resizer_.setDontUse(cell, true);
  }

  sta::Pin* out1_pin = findTopPin("out1");
  sta::Pin* out2_pin = findTopPin("out2");
  ASSERT_NE(out1_pin, nullptr);
  ASSERT_NE(out2_pin, nullptr);
  sta::Port* out1_port = db_network_->port(out1_pin);
  sta::Port* out2_port = db_network_->port(out2_pin);
  ASSERT_NE(out1_port, nullptr);
  ASSERT_NE(out2_port, nullptr);

  // Apply the same heavy external pin load (150 fF) to both top-level output
  // ports. Make out1 timing-violating (negative slack) via output delay while
  // keeping out2 positive-slack.
  const float ext_cap = sta_->units()->capacitanceUnit()->userToSta(150.0f);
  sta::Sdc* sdc = sta_->cmdMode()->sdc();
  sta_->setPortExtPinCap(out1_port,
                         sta::RiseFallBoth::riseFall(),
                         sta::MinMaxAll::all(),
                         ext_cap,
                         sdc);
  sta_->setPortExtPinCap(out2_port,
                         sta::RiseFallBoth::riseFall(),
                         sta::MinMaxAll::all(),
                         ext_cap,
                         sdc);

  sta::Clock* clk = sdc->findClock("clk");
  ASSERT_NE(clk, nullptr);
  sta_->setOutputDelay(out1_pin,
                       sta::RiseFallBoth::riseFall(),
                       clk,
                       sta::RiseFall::rise(),
                       nullptr,
                       false,
                       false,
                       sta::MinMaxAll::all(),
                       true,
                       staTime(0.45f),
                       sdc);
  sta_->updateTiming(true);

  const sta::MinMax* max = sta::MinMax::max();
  ASSERT_LT(sta_->slack(sta_->graph()->pinLoadVertex(out1_pin), max), 0.0f);
  ASSERT_GT(sta_->slack(sta_->graph()->pinLoadVertex(out2_pin), max), 0.0f);

  resizer_.repairDesign(/*max_wire_length=*/0.0,
                        /*slew_margin=*/0.0,
                        /*cap_margin=*/0.0,
                        /*pre_placement=*/true,
                        /*match_cell_footprint=*/false,
                        /*reroute=*/false,
                        /*verbose=*/false);

  // The negative-slack output port (out1) should be gain-buffered with a
  // multi-stage buffer cascade, whereas the positive-slack output port (out2)
  // should remain directly driven by probe.
  EXPECT_GE(resizer_.repairDesignBufferCount(), 2);
  EXPECT_NE(block_->findInst("gain1"), nullptr);
  EXPECT_NE(block_->findInst("gain2"), nullptr);

  odb::dbBTerm* out1_bterm = block_->findBTerm("out1");
  odb::dbBTerm* out2_bterm = block_->findBTerm("out2");
  ASSERT_NE(out1_bterm, nullptr);
  ASSERT_NE(out2_bterm, nullptr);
  EXPECT_NE(findITerm(target_db, "ZN")->getNet(), out1_bterm->getNet());
  EXPECT_EQ(findITerm(probe_db, "Z")->getNet(), out2_bterm->getNet());
}

TEST_F(TestResizer,
       GainBufferingBuffersPositiveSlackOutputPortWhenDriverHasNegativeSlack)
{
  readVerilogAndSetup("TestResizer_SwapPinsFeedthroughModNet_pre.v",
                      /*init_default_sdc=*/true,
                      /*hierarchy=*/false);

  odb::dbInst* target_db = block_->findInst("target");
  odb::dbInst* probe_db = block_->findInst("probe");
  ASSERT_NE(target_db, nullptr);
  ASSERT_NE(probe_db, nullptr);

  sta::Instance* probe_inst = db_network_->dbToSta(probe_db);
  ASSERT_NE(probe_inst, nullptr);
  sta::LibertyCell* clkbuf_x1 = sta_->network()->findLibertyCell("CLKBUF_X1");
  ASSERT_NE(clkbuf_x1, nullptr);
  ASSERT_TRUE(resizer_.replaceCell(probe_inst, clkbuf_x1));

  for (const char* cell_name : {"OR2_X2", "OR2_X4", "CLKBUF_X2", "CLKBUF_X3"}) {
    sta::LibertyCell* cell = sta_->network()->findLibertyCell(cell_name);
    ASSERT_NE(cell, nullptr);
    resizer_.setDontUse(cell, true);
  }

  // Rewire target/A1 onto probe/Z's net (out2) so probe/Z drives both the
  // positive-slack top-level output port out2 and the negative-slack internal
  // sink target/A1 (which feeds out1).
  odb::dbBTerm* out2_bterm = block_->findBTerm("out2");
  ASSERT_NE(out2_bterm, nullptr);
  odb::dbNet* out2_net = out2_bterm->getNet();
  ASSERT_NE(out2_net, nullptr);
  sta::Instance* target_inst = db_network_->dbToSta(target_db);
  ASSERT_NE(target_inst, nullptr);
  sta::LibertyPort* a1_port = findLibertyPort(target_inst, "A1");
  ASSERT_NE(a1_port, nullptr);
  sta::Pin* target_a1_pin = db_network_->findPin(target_inst, a1_port);
  ASSERT_NE(target_a1_pin, nullptr);
  sta_->disconnectPin(target_a1_pin);
  sta_->connectPin(target_inst, a1_port, db_network_->dbToSta(out2_net));

  sta::Pin* out1_pin = findTopPin("out1");
  sta::Pin* out2_pin = findTopPin("out2");
  ASSERT_NE(out1_pin, nullptr);
  ASSERT_NE(out2_pin, nullptr);
  sta::Port* out2_port = db_network_->port(out2_pin);
  ASSERT_NE(out2_port, nullptr);

  // Put a heavy 150 fF external load on out2, but keep out2 itself
  // positive-slack (output_delay = 0.0) while making out1 (downstream of
  // target/A1) negative-slack (output_delay = 0.45 ns).
  const float ext_cap = sta_->units()->capacitanceUnit()->userToSta(150.0f);
  sta::Sdc* sdc = sta_->cmdMode()->sdc();
  sta_->setPortExtPinCap(out2_port,
                         sta::RiseFallBoth::riseFall(),
                         sta::MinMaxAll::all(),
                         ext_cap,
                         sdc);

  sta::Clock* clk = sdc->findClock("clk");
  ASSERT_NE(clk, nullptr);
  sta_->setOutputDelay(out1_pin,
                       sta::RiseFallBoth::riseFall(),
                       clk,
                       sta::RiseFall::rise(),
                       nullptr,
                       false,
                       false,
                       sta::MinMaxAll::all(),
                       true,
                       staTime(0.45f),
                       sdc);
  sta_->updateTiming(true);

  const sta::MinMax* max = sta::MinMax::max();
  ASSERT_GT(sta_->slack(sta_->graph()->pinLoadVertex(out2_pin), max), 0.0f);
  const sta::Slack target_a1_slack_before
      = sta_->slack(sta_->graph()->pinLoadVertex(target_a1_pin), max);
  ASSERT_LT(target_a1_slack_before, 0.0f);

  resizer_.repairDesign(/*max_wire_length=*/0.0,
                        /*slew_margin=*/0.0,
                        /*cap_margin=*/0.0,
                        /*pre_placement=*/true,
                        /*match_cell_footprint=*/false,
                        /*reroute=*/false,
                        /*verbose=*/false);
  sta_->updateTiming(true);

  // Because probe/Z has negative worst slack (via target/A1), gain buffering
  // includes out2's 150 fF load and buffers probe/Z's net, improving slack at
  // the critical internal sink target/A1.
  EXPECT_NE(findITerm(probe_db, "Z")->getNet(), out2_bterm->getNet());
  EXPECT_GT(sta_->slack(sta_->graph()->pinLoadVertex(target_a1_pin), max),
            target_a1_slack_before);
}

TEST_F(TestResizer, SizeUpSelectsActiveTransitionSkewedCell)
{
  sta::LibertyLibrary* skew_lib = loadSkewVtLibraryAndSetup();
  ASSERT_NE(skew_lib, nullptr);

  sta::LibertyCell* d1_rvt = skew_lib->findLibertyCell("SKEW_BUF_D1_RVT");
  sta::LibertyCell* skrd2_rvt = skew_lib->findLibertyCell("SKEW_BUF_SKRD2_RVT");
  sta::LibertyCell* d2_rvt = skew_lib->findLibertyCell("SKEW_BUF_D2_RVT");
  sta::LibertyCell* d3_rvt = skew_lib->findLibertyCell("SKEW_BUF_D3_RVT");
  ASSERT_NE(d1_rvt, nullptr);
  ASSERT_NE(skrd2_rvt, nullptr);
  ASSERT_NE(d2_rvt, nullptr);
  ASSERT_NE(d3_rvt, nullptr);

  sta::LibertyPort* in_port = d1_rvt->findLibertyPort("A");
  sta::LibertyPort* drvr_port = d1_rvt->findLibertyPort("Z");
  ASSERT_NE(in_port, nullptr);
  ASSERT_NE(drvr_port, nullptr);

  MoveCommitter committer(resizer_);
  const OptimizerRunConfig run_config;
  const OptimizationPolicyConfig policy_config;
  const GeneratorContext context{.resizer = resizer_,
                                 .committer = committer,
                                 .run_config = run_config,
                                 .policy_config = policy_config};
  const TestSizeUpGenerator generator(context);

  const sta::Scene* scene = sta_->cmdScene();
  const sta::MinMax* max = sta::MinMax::max();
  const float load_cap = sta_->units()->capacitanceUnit()->userToSta(0.01f);

  // SKEW_BUF_SKRD2_RVT has a faster rise delay than SKEW_BUF_D1_RVT, but its
  // fall delay exceeds SKEW_BUF_D1_RVT's worst-case delay. On a rising edge,
  // transition-aware sizing picks SKEW_BUF_SKRD2_RVT. On a falling edge it must
  // skip SKEW_BUF_SKRD2_RVT (which weakens the fall edge); among the remaining
  // eligible cells the power-aware score picks SKEW_BUF_D3_RVT, whose larger
  // delay gain per unit cost beats the smaller SKEW_BUF_D2_RVT.
  sta::LibertyCell* rise_replacement
      = generator.upsizeCell(in_port,
                             sta::RiseFall::rise(),
                             drvr_port,
                             sta::RiseFall::rise(),
                             load_cap,
                             /*prev_drive=*/0.0f,
                             scene,
                             max);
  ASSERT_NE(rise_replacement, nullptr);
  EXPECT_EQ(rise_replacement, skrd2_rvt);

  sta::LibertyCell* fall_replacement
      = generator.upsizeCell(in_port,
                             sta::RiseFall::fall(),
                             drvr_port,
                             sta::RiseFall::fall(),
                             load_cap,
                             /*prev_drive=*/0.0f,
                             scene,
                             max);
  ASSERT_NE(fall_replacement, nullptr);
  EXPECT_EQ(fall_replacement, d3_rvt);

  odb::dbInst* db_inst = block_->findInst("enable_buf0");
  ASSERT_NE(db_inst, nullptr);
  sta::Instance* inst = db_network_->dbToSta(db_inst);
  ASSERT_NE(inst, nullptr);
  ASSERT_TRUE(resizer_.replaceCell(inst, d1_rvt));
  sta_->updateTiming(true);

  RepairTargetCollector collector(&resizer_);
  collector.init(0.0f);
  sta::Vertex* endpoint = loadVertex("enable_latch/D");
  ASSERT_NE(endpoint, nullptr);
  sta::Path* path = sta_->vertexWorstSlackPath(endpoint, max);
  ASSERT_NE(path, nullptr);
  const std::vector<Target> targets
      = collector.collectPathDriverTargets(path, path->slack(sta_.get()));

  const Target* buf0_target = nullptr;
  for (const Target& target : targets) {
    if (target.driver_pin != nullptr
        && std::string(db_network_->pathName(target.driver_pin))
               == "enable_buf0/Z") {
      buf0_target = &target;
      break;
    }
  }
  ASSERT_NE(buf0_target, nullptr);

  SizeUpGenerator live_generator(context);
  std::vector<std::unique_ptr<MoveCandidate>> candidates
      = live_generator.generate(*buf0_target);
  ASSERT_EQ(candidates.size(), 1u);
  EXPECT_TRUE(candidates[0]->apply().accepted);
  EXPECT_EQ(db_network_->libertyCell(inst), skrd2_rvt);
}

TEST_F(TestResizer, SizeUpRejectsCandidateThatWeakensNonCriticalEdge)
{
  sta::LibertyLibrary* skew_lib = loadSkewVtLibraryAndSetup();
  ASSERT_NE(skew_lib, nullptr);

  sta::LibertyCell* d2_rvt = skew_lib->findLibertyCell("SKEW_BUF_D2_RVT");
  sta::LibertyCell* skrd2_rvt = skew_lib->findLibertyCell("SKEW_BUF_SKRD2_RVT");
  sta::LibertyCell* d2_lvt = skew_lib->findLibertyCell("SKEW_BUF_D2_LVT");
  sta::LibertyCell* skfd2_slvt
      = skew_lib->findLibertyCell("SKEW_BUF_SKFD2_SLVT");
  sta::LibertyCell* d3_rvt = skew_lib->findLibertyCell("SKEW_BUF_D3_RVT");
  ASSERT_NE(d2_rvt, nullptr);
  ASSERT_NE(skrd2_rvt, nullptr);
  ASSERT_NE(d2_lvt, nullptr);
  ASSERT_NE(skfd2_slvt, nullptr);
  ASSERT_NE(d3_rvt, nullptr);

  sta::LibertyPort* in_port = d2_rvt->findLibertyPort("A");
  sta::LibertyPort* drvr_port = d2_rvt->findLibertyPort("Z");
  ASSERT_NE(in_port, nullptr);
  ASSERT_NE(drvr_port, nullptr);

  MoveCommitter committer(resizer_);
  const OptimizerRunConfig run_config;
  const OptimizationPolicyConfig policy_config;
  const GeneratorContext context{.resizer = resizer_,
                                 .committer = committer,
                                 .run_config = run_config,
                                 .policy_config = policy_config};
  const TestSizeUpGenerator generator(context);

  const sta::Scene* scene = sta_->cmdScene();
  const sta::MinMax* max = sta::MinMax::max();
  const int lib_ap = scene->libertyIndex(max);
  const float load_cap = sta_->units()->capacitanceUnit()->userToSta(0.01f);

  // Transition-blind weakerCellFirst orders SKEW_BUF_SKFD2_SLVT before
  // SKEW_BUF_D2_LVT (worst R 2.40 > 1.25), whereas transition-aware
  // weakerCellFirst on a falling edge orders SKEW_BUF_D2_LVT before
  // SKEW_BUF_SKFD2_SLVT (fall R 0.95 > 0.90).
  EXPECT_TRUE(generator.weakerCellFirst(skfd2_slvt, d2_lvt, "Z", lib_ap));
  EXPECT_TRUE(generator.weakerCellFirst(
      skfd2_slvt, d2_lvt, "Z", lib_ap, sta::RiseFall::rise(), max));
  EXPECT_TRUE(generator.weakerCellFirst(
      d2_lvt, skfd2_slvt, "Z", lib_ap, sta::RiseFall::fall(), max));

  // Within the same RVT class, SKEW_BUF_SKRD2_RVT improves rise drive
  // resistance (2.00 < 2.50) and rise delay relative to SKEW_BUF_D2_RVT, but
  // weakens fall drive resistance (2.05 > 2.00). SizeUp must reject it and
  // choose SKEW_BUF_D3_RVT instead.
  resizer_.setDontUse(skfd2_slvt, true);
  resizer_.setDontUse(d2_lvt, true);
  sta::LibertyCell* replacement = generator.upsizeCell(in_port,
                                                       sta::RiseFall::rise(),
                                                       drvr_port,
                                                       sta::RiseFall::rise(),
                                                       load_cap,
                                                       /*prev_drive=*/0.0f,
                                                       scene,
                                                       max);
  ASSERT_NE(replacement, nullptr);
  EXPECT_EQ(replacement, d3_rvt);

  // When SKEW_BUF_D3_RVT is marked dont_use, no legal non-weakening upsize
  // candidate in RVT remains.
  resizer_.setDontUse(d3_rvt, true);
  EXPECT_EQ(generator.upsizeCell(in_port,
                                 sta::RiseFall::rise(),
                                 drvr_port,
                                 sta::RiseFall::rise(),
                                 load_cap,
                                 /*prev_drive=*/0.0f,
                                 scene,
                                 max),
            nullptr);
}

TEST_F(TestResizer, VtSwapAdvancesOnlyToNonWeakeningEquivCells)
{
  sta::LibertyLibrary* skew_lib = loadSkewVtLibraryAndSetup();
  ASSERT_NE(skew_lib, nullptr);

  sta::LibertyCell* d2_rvt = skew_lib->findLibertyCell("SKEW_BUF_D2_RVT");
  sta::LibertyCell* d2_lvt = skew_lib->findLibertyCell("SKEW_BUF_D2_LVT");
  sta::LibertyCell* skfd2_slvt
      = skew_lib->findLibertyCell("SKEW_BUF_SKFD2_SLVT");
  ASSERT_NE(d2_rvt, nullptr);
  ASSERT_NE(d2_lvt, nullptr);
  ASSERT_NE(skfd2_slvt, nullptr);

  const sta::LibertyCellSeq equiv_cells = resizer_.getVTEquivCells(d2_rvt);
  ASSERT_EQ(equiv_cells.size(), 3u);
  EXPECT_EQ(equiv_cells[0], d2_rvt);
  EXPECT_EQ(equiv_cells[1], d2_lvt);
  EXPECT_EQ(equiv_cells[2], skfd2_slvt);

  odb::dbInst* db_inst = block_->findInst("enable_buf0");
  ASSERT_NE(db_inst, nullptr);
  sta::Instance* inst = db_network_->dbToSta(db_inst);
  ASSERT_NE(inst, nullptr);
  ASSERT_TRUE(resizer_.replaceCell(inst, d2_rvt));

  // Stepping from SKEW_BUF_D2_RVT (R_rise=2.50, R_fall=2.00) to
  // SKEW_BUF_D2_LVT (R_rise=1.25, R_fall=0.95) strengthens both edges, whereas
  // stepping further to SKEW_BUF_SKFD2_SLVT (R_rise=2.40, R_fall=0.90) would
  // weaken R_rise relative to SKEW_BUF_D2_LVT. Both checkAndMarkVTSwappable
  // and VtSwapGenerator must stop at SKEW_BUF_D2_LVT.
  std::unordered_set<sta::Instance*> not_swappable;
  sta::LibertyCell* best_cell = nullptr;
  EXPECT_TRUE(resizer_.checkAndMarkVTSwappable(inst, not_swappable, best_cell));
  EXPECT_EQ(best_cell, d2_lvt);

  MoveCommitter committer(resizer_);
  const OptimizerRunConfig run_config;
  const OptimizationPolicyConfig policy_config;
  const GeneratorContext context{.resizer = resizer_,
                                 .committer = committer,
                                 .run_config = run_config,
                                 .policy_config = policy_config};
  VtSwapGenerator vt_generator(context, &not_swappable);

  sta::Pin* drvr_pin = db_network_->findPin("enable_buf0/Z");
  ASSERT_NE(drvr_pin, nullptr);
  Target target;
  target.driver_pin = drvr_pin;

  std::vector<std::unique_ptr<MoveCandidate>> candidates
      = vt_generator.generate(target);
  ASSERT_EQ(candidates.size(), 1u);
  EXPECT_TRUE(candidates[0]->apply().accepted);
  EXPECT_EQ(db_network_->libertyCell(inst), d2_lvt);

  // Once at SKEW_BUF_D2_LVT, no further VT swap should be proposed because
  // SKEW_BUF_SKFD2_SLVT weakens R_rise.
  EXPECT_FALSE(
      resizer_.checkAndMarkVTSwappable(inst, not_swappable, best_cell));
  EXPECT_TRUE(vt_generator.generate(target).empty());
}

TEST_F(TestResizer, GetVtEquivCellsDeduplicatesNonAdjacentSameVtCells)
{
  // Place BUF_X1 and BUF_X4 in the LVT category while leaving BUF_X2 in RVT,
  // and give all three the same dbMaster area so they qualify as VT-equivalent.
  // Because cellLeakage(BUF_X1) < cellLeakage(BUF_X2) < cellLeakage(BUF_X4),
  // sorting by leakage alone separates the two LVT cells with the RVT cell and
  // prevents adjacent deduplication from collapsing the duplicate LVT entry.
  odb::dbTechLayer* lvt_implant = odb::dbTechLayer::create(
      db_->getTech(), "LVT_IMPLANT", odb::dbTechLayerType::IMPLANT);
  ASSERT_NE(lvt_implant, nullptr);
  odb::dbMaster* buf_x1_master = db_->findMaster("BUF_X1");
  odb::dbMaster* buf_x2_master = db_->findMaster("BUF_X2");
  odb::dbMaster* buf_x4_master = db_->findMaster("BUF_X4");
  ASSERT_NE(buf_x1_master, nullptr);
  ASSERT_NE(buf_x2_master, nullptr);
  ASSERT_NE(buf_x4_master, nullptr);

  buf_x2_master->setWidth(buf_x1_master->getWidth());
  buf_x2_master->setHeight(buf_x1_master->getHeight());
  buf_x4_master->setWidth(buf_x1_master->getWidth());
  buf_x4_master->setHeight(buf_x1_master->getHeight());

  odb::dbBox::create(buf_x1_master, lvt_implant, 0, 0, 10, 10);
  odb::dbBox::create(buf_x4_master, lvt_implant, 0, 0, 10, 10);

  readVerilogAndSetup("TestResizerMt_DelayEstimator.v");
  resizer_.initBlock();
  resizer_.runRepairSetupPreamble();

  sta::LibertyCell* buf_x1_cell
      = db_network_->libertyCell(db_network_->dbToSta(buf_x1_master));
  sta::LibertyCell* buf_x2_cell
      = db_network_->libertyCell(db_network_->dbToSta(buf_x2_master));
  sta::LibertyCell* buf_x4_cell
      = db_network_->libertyCell(db_network_->dbToSta(buf_x4_master));
  ASSERT_NE(buf_x1_cell, nullptr);
  ASSERT_NE(buf_x2_cell, nullptr);
  ASSERT_NE(buf_x4_cell, nullptr);

  const VTCategory lvt_cat = resizer_.cellVTType(buf_x1_master);
  const VTCategory rvt_cat = resizer_.cellVTType(buf_x2_master);
  EXPECT_NE(lvt_cat, rvt_cat);
  EXPECT_EQ(lvt_cat, resizer_.cellVTType(buf_x4_master));

  const sta::LibertyCellSeq vt_equiv = resizer_.getVTEquivCells(buf_x2_cell);
  ASSERT_EQ(vt_equiv.size(), 2u);
  odb::dbMaster* first_master = db_network_->staToDb(vt_equiv[0]);
  odb::dbMaster* second_master = db_network_->staToDb(vt_equiv[1]);
  ASSERT_NE(first_master, nullptr);
  ASSERT_NE(second_master, nullptr);
  EXPECT_NE(resizer_.cellVTType(first_master),
            resizer_.cellVTType(second_master));
  EXPECT_LE(resizer_.cellLeakage(vt_equiv[0]).value_or(0.0f),
            resizer_.cellLeakage(vt_equiv[1]).value_or(0.0f));
}

TEST_F(TestResizer, DisabledConditionalArcIgnoredInDriveResistance)
{
  readDefForTiming("remove_buffers3.def");
  resizer_.initBlock();

  odb::dbInst* gate
      = odb::dbInst::create(block_, db_->findMaster("XNOR2_X1"), "g1");
  sta::Instance* sta_gate = db_network_->dbToSta(gate);
  sta_->connectPin(sta_gate,
                   findLibertyPort(sta_gate, "A"),
                   db_network_->dbToSta(block_->findNet("in1")));
  sta_->connectPin(sta_gate,
                   findLibertyPort(sta_gate, "B"),
                   db_network_->dbToSta(block_->findNet("n1")));

  sta::Pin* b_pin = db_network_->dbToSta(gate->findITerm("B"));
  sta::Pin* zn_pin = db_network_->dbToSta(gate->findITerm("ZN"));
  const float port_res = db_network_->libertyPort(zn_pin)->driveResistance();

  sta_->updateTiming(false);
  EXPECT_FLOAT_EQ(resizer_.driveResistance(zn_pin), port_res);

  // B=1 disables B->ZN and the slower `when : "!B"` arc, leaving `when : "B"`.
  sta_->setCaseAnalysis(b_pin, sta::LogicValue::one, sta_->cmdMode());
  sta_->updateTiming(false);
  EXPECT_GT(resizer_.driveResistance(zn_pin), 0.0f);
  EXPECT_LT(resizer_.driveResistance(zn_pin), 0.6f * port_res);

  // B=0 re-enables the slower `when : "!B"` A->ZN arc.
  sta_->setCaseAnalysis(b_pin, sta::LogicValue::zero, sta_->cmdMode());
  sta_->updateTiming(false);
  EXPECT_GT(resizer_.driveResistance(zn_pin), 0.99f * port_res);
}

TEST_F(TestResizer, FindBufferUnderSlewSelectsSmallestBufferForEasyLoad)
{
  setupTimeBorrowTiming("inferred_clock_gator_time_borrow.def",
                        /*input_delay=*/0.98);
  resizer_.resizePreamble();
  TestRepairDesign repair_design(&resizer_);
  repair_design.init();

  sta::LibertyCell* buf_x1 = sta_->network()->findLibertyCell("BUF_X1");
  ASSERT_NE(buf_x1, nullptr);

  EXPECT_EQ(repair_design.findBufferUnderSlew(/*max_slew=*/1e-9f,
                                              /*load_cap=*/1e-15f),
            buf_x1);
}

TEST_F(TestResizer, FindBufferUnderSlewStaysWithinSizingLimitsWhenDrcClean)
{
  setupTimeBorrowTiming("inferred_clock_gator_time_borrow.def",
                        /*input_delay=*/0.98);
  resizer_.resizePreamble();
  TestRepairDesign repair_design(&resizer_);
  repair_design.init();

  sta::LibertyCell* buf_x1 = sta_->network()->findLibertyCell("BUF_X1");
  sta::LibertyCell* buf_x2 = sta_->network()->findLibertyCell("BUF_X2");
  ASSERT_NE(buf_x1, nullptr);
  ASSERT_NE(buf_x2, nullptr);

  const sta::Scene* corner = sta_->cmdScene();

  // In Nangate45, Resizer's default sizing limits (4.0x area/leakage) around
  // BUF_X1 (area 0.798, leakage 21.44) include BUF_X2 (1.33x area, 2.01x
  // leakage) and exclude BUF_X4 (2.33x area, 4.02x leakage). At 80% of
  // BUF_X1's max_cap (~24 fF), both BUF_X1 and BUF_X2 are DRC-clean yet both
  // exceed targetSlew; findBufferUnderSlew selects the lowest-slew DRC-clean
  // buffer within the sizing limits (BUF_X2) without expanding past them.
  const float target_slew
      = std::max(resizer_.targetSlew(sta::RiseFall::rise()),
                 resizer_.targetSlew(sta::RiseFall::fall()));
  const float max_cap_x1 = resizer_.maxLoad(db_network_->cell(buf_x1), corner);
  EXPECT_FLOAT_EQ(max_cap_x1, resizer_.maxLoad(db_network_->cell(buf_x1)));
  const float mod_load = 0.8f * max_cap_x1;
  const auto eval_x1 = repair_design.isBufferDrcClean(
      buf_x1, /*load_cap=*/mod_load, /*max_slew=*/1e-9f, corner);
  const auto eval_x2 = repair_design.isBufferDrcClean(
      buf_x2, /*load_cap=*/mod_load, /*max_slew=*/1e-9f, corner);
  ASSERT_TRUE(eval_x1.drc_clean);
  ASSERT_TRUE(eval_x2.drc_clean);
  ASSERT_GT(eval_x2.slew, target_slew);
  EXPECT_EQ(repair_design.findBufferUnderSlew(/*max_slew=*/eval_x1.slew * 1.1f,
                                              /*load_cap=*/mod_load),
            buf_x2);
}

TEST_F(TestResizer, FindBufferUnderSlewExpandsBeyondSizingLimitsForDrc)
{
  setupTimeBorrowTiming("inferred_clock_gator_time_borrow.def",
                        /*input_delay=*/0.98);
  resizer_.resizePreamble();
  TestRepairDesign repair_design(&resizer_);
  repair_design.init();

  sta::LibertyCell* buf_x1 = sta_->network()->findLibertyCell("BUF_X1");
  sta::LibertyCell* buf_x4 = sta_->network()->findLibertyCell("BUF_X4");
  sta::LibertyCell* buf_x8 = sta_->network()->findLibertyCell("BUF_X8");
  ASSERT_NE(buf_x1, nullptr);
  ASSERT_NE(buf_x4, nullptr);
  ASSERT_NE(buf_x8, nullptr);

  const sta::Scene* corner = sta_->cmdScene();
  const sta::MinMax* max_mm = sta::MinMax::max();

  // When DRC max_slew is tighter than any buffer within the resizer sizing
  // limits can meet, expand past the sizing limits to close DRC.
  const float slew_x4
      = resizer_.bufferSlew(buf_x4, /*load_cap=*/50e-15f, corner, max_mm);
  const float slew_x8
      = resizer_.bufferSlew(buf_x8, /*load_cap=*/50e-15f, corner, max_mm);
  const float tight_slew = 0.5f * (slew_x8 + slew_x4);
  sta::LibertyCell* drc_expanded = repair_design.findBufferUnderSlew(
      /*max_slew=*/tight_slew, /*load_cap=*/50e-15f);
  ASSERT_NE(drc_expanded, nullptr);
  EXPECT_GT(db_network_->staToDb(drc_expanded)->getArea(),
            db_network_->staToDb(buf_x1)->getArea() * 4);
  EXPECT_LE(
      resizer_.bufferSlew(drc_expanded, /*load_cap=*/50e-15f, corner, max_mm),
      tight_slew);
}

TEST_F(TestResizer,
       FindBufferUnderSlewPrefersSmallestCapCleanBufferWhenSlewUnachievable)
{
  setupTimeBorrowTiming("inferred_clock_gator_time_borrow.def",
                        /*input_delay=*/0.98);
  // Lift sizing limits completely so BUF_X1..BUF_X32 are all in softCandidates,
  // matching unconstrained repair_design runs on unachievable slew targets.
  odb::dbDoubleProperty::create(block_, "limit_sizing_area", 1000.0);
  odb::dbDoubleProperty::create(block_, "limit_sizing_leakage", 1000.0);
  resizer_.initBlock();
  resizer_.resizePreamble();
  TestRepairDesign repair_design(&resizer_);
  repair_design.init();

  sta::LibertyCell* buf_x1 = sta_->network()->findLibertyCell("BUF_X1");
  sta::LibertyCell* buf_x2 = sta_->network()->findLibertyCell("BUF_X2");
  sta::LibertyCell* buf_x4 = sta_->network()->findLibertyCell("BUF_X4");
  ASSERT_NE(buf_x1, nullptr);
  ASSERT_NE(buf_x2, nullptr);
  ASSERT_NE(buf_x4, nullptr);

  const sta::Scene* corner = sta_->cmdScene();
  const sta::MinMax* max_mm = sta::MinMax::max();

  // When max_slew is physically unachievable (0.0f), pick the smallest-area
  // cap-clean buffer (BUF_X1 at 10 fF; BUF_X2 at 80 fF > BUF_X1 max_cap) rather
  // than escalating to BUF_X32 and compounding upstream input capacitance.
  EXPECT_EQ(repair_design.findBufferUnderSlew(/*max_slew=*/0.0f,
                                              /*load_cap=*/10e-15f),
            buf_x1);
  EXPECT_EQ(repair_design.findBufferUnderSlew(/*max_slew=*/0.0f,
                                              /*load_cap=*/80e-15f),
            buf_x2);

  // Restore default 4.0x limits and verify expansion to BUF_X4 only when
  // max_capacitance also fails inside the sizing limits.
  odb::dbDoubleProperty::find(block_, "limit_sizing_area")->setValue(4.0);
  odb::dbDoubleProperty::find(block_, "limit_sizing_leakage")->setValue(4.0);
  resizer_.initBlock();
  resizer_.resizePreamble();
  repair_design.init();
  sta::Sdc* sdc = corner->sdc();
  for (sta::LibertyCell* soft_buf : {buf_x1, buf_x2}) {
    sta_->setCapacitanceLimit(
        db_network_->cell(soft_buf), max_mm, /*cap=*/0.5e-15f, sdc);
  }
  EXPECT_FLOAT_EQ(resizer_.maxLoad(db_network_->cell(buf_x1), corner),
                  0.5e-15f);
  EXPECT_EQ(repair_design.findBufferUnderSlew(/*max_slew=*/0.0f,
                                              /*load_cap=*/50e-15f),
            buf_x4);
}

TEST_F(TestResizer, FindBufferUnderSlewRespectsDontUse)
{
  setupTimeBorrowTiming("inferred_clock_gator_time_borrow.def",
                        /*input_delay=*/0.98);
  resizer_.resizePreamble();
  TestRepairDesign repair_design(&resizer_);
  repair_design.init();

  sta::LibertyCell* buf_x1 = sta_->network()->findLibertyCell("BUF_X1");
  sta::LibertyCell* buf_x2 = sta_->network()->findLibertyCell("BUF_X2");
  sta::LibertyCell* buf_x4 = sta_->network()->findLibertyCell("BUF_X4");
  sta::LibertyCell* buf_x8 = sta_->network()->findLibertyCell("BUF_X8");
  ASSERT_NE(buf_x1, nullptr);
  ASSERT_NE(buf_x2, nullptr);
  ASSERT_NE(buf_x4, nullptr);
  ASSERT_NE(buf_x8, nullptr);

  const sta::Scene* corner = sta_->cmdScene();
  const sta::MinMax* max_mm = sta::MinMax::max();
  sta::Sdc* sdc = corner->sdc();
  for (sta::LibertyCell* soft_buf : {buf_x1, buf_x2}) {
    sta_->setCapacitanceLimit(
        db_network_->cell(soft_buf), max_mm, /*cap=*/0.5e-15f, sdc);
  }

  // Marking BUF_X4 dont_use and re-initializing RepairDesign must invalidate
  // cached candidates and advance to BUF_X8; clearing dont_use restores BUF_X4.
  resizer_.setDontUse(buf_x4, /*dont_use=*/true);
  resizer_.resizePreamble();
  repair_design.init();
  EXPECT_EQ(repair_design.findBufferUnderSlew(/*max_slew=*/0.0f,
                                              /*load_cap=*/50e-15f),
            buf_x8);
  resizer_.setDontUse(buf_x4, /*dont_use=*/false);
  resizer_.resizePreamble();
  repair_design.init();
  EXPECT_EQ(repair_design.findBufferUnderSlew(/*max_slew=*/0.0f,
                                              /*load_cap=*/50e-15f),
            buf_x4);
}

TEST_F(TestResizer, RepairNetSizesWireRepeatersToSegmentLoad)
{
  TestRepairDesign repair_design(&resizer_);
  setupRepairNetTest(repair_design);

  const sta::Scene* corner = sta_->cmdScene();
  odb::dbInst* enable_latch = block_->findInst("enable_latch");
  odb::dbInst* gated_ff0 = block_->findInst("gated_ff0");
  odb::dbMaster* buf_x4_master = db_->findMaster("BUF_X4");
  ASSERT_NE(enable_latch, nullptr);
  ASSERT_NE(gated_ff0, nullptr);
  ASSERT_NE(buf_x4_master, nullptr);

  // An 800 um wire (~81 fF > 35 fF max_cap) inserts multiple wire repeaters
  // (~33 fF downstream load each), and each repeater stays within the resizer
  // sizing limits (< BUF_X4) instead of double-sizing to BUF_X4.
  sta::Pin* drvr_pin = db_network_->dbToSta(enable_latch->findITerm("Q"));
  sta::Pin* ff0_d = db_network_->dbToSta(gated_ff0->findITerm("D"));
  auto load0 = std::make_shared<BufferedNet>(BufferedNetType::load,
                                             db_network_->location(ff0_d),
                                             ff0_d,
                                             corner,
                                             &resizer_);
  auto long_wire = std::make_shared<BufferedNet>(BufferedNetType::wire,
                                                 odb::Point(120000, 150000),
                                                 BufferedNet::null_layer,
                                                 load0,
                                                 corner,
                                                 &resizer_,
                                                 &ep_);
  std::unordered_set<odb::dbInst*> before;
  for (odb::dbInst* inst : block_->getInsts()) {
    before.insert(inst);
  }
  {
    est::IncrementalParasiticsGuard guard(&ep_);
    repair_design.repairNet(
        long_wire, drvr_pin, /*max_cap=*/35e-15f, /*max_length=*/0, corner);
  }
  size_t wire_reps = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (!before.contains(inst)) {
      ++wire_reps;
      EXPECT_LT(inst->getMaster()->getArea(), buf_x4_master->getArea());
    }
  }
  EXPECT_GE(wire_reps, 2u);
}

TEST_F(TestResizer, RepairNetSizesJunctionRepeaterToBranchLoad)
{
  TestRepairDesign repair_design(&resizer_);
  setupRepairNetTest(repair_design);

  const sta::Scene* corner = sta_->cmdScene();
  odb::dbInst* enable_latch = block_->findInst("enable_latch");
  odb::dbInst* gated_ff2 = block_->findInst("gated_ff2");
  odb::dbInst* gated_ff3 = block_->findInst("gated_ff3");
  odb::dbMaster* buf_x4_master = db_->findMaster("BUF_X4");
  ASSERT_NE(enable_latch, nullptr);
  ASSERT_NE(gated_ff2, nullptr);
  ASSERT_NE(gated_ff3, nullptr);
  ASSERT_NE(buf_x4_master, nullptr);

  // Two 400 um branches (~41 fF each, ~82 fF combined > 55 fF max_cap) size
  // the isolated branch repeater to its single-branch load.
  sta::Pin* drvr_pin = db_network_->dbToSta(enable_latch->findITerm("Q"));
  sta::Pin* ff2_d = db_network_->dbToSta(gated_ff2->findITerm("D"));
  sta::Pin* ff3_d = db_network_->dbToSta(gated_ff3->findITerm("D"));
  auto load2 = std::make_shared<BufferedNet>(BufferedNetType::load,
                                             db_network_->location(ff2_d),
                                             ff2_d,
                                             corner,
                                             &resizer_);
  auto load3 = std::make_shared<BufferedNet>(BufferedNetType::load,
                                             db_network_->location(ff3_d),
                                             ff3_d,
                                             corner,
                                             &resizer_);
  auto wire_l = std::make_shared<BufferedNet>(BufferedNetType::wire,
                                              odb::Point(120000, 158985),
                                              BufferedNet::null_layer,
                                              load2,
                                              corner,
                                              &resizer_,
                                              &ep_);
  auto wire_r = std::make_shared<BufferedNet>(BufferedNetType::wire,
                                              odb::Point(120000, 167045),
                                              BufferedNet::null_layer,
                                              load3,
                                              corner,
                                              &resizer_,
                                              &ep_);
  auto cap_junc = std::make_shared<BufferedNet>(BufferedNetType::junction,
                                                odb::Point(120000, 162000),
                                                wire_l,
                                                wire_r,
                                                &resizer_);
  std::unordered_set<odb::dbInst*> before;
  for (odb::dbInst* inst : block_->getInsts()) {
    before.insert(inst);
  }
  {
    est::IncrementalParasiticsGuard guard(&ep_);
    repair_design.repairNet(
        cap_junc, drvr_pin, /*max_cap=*/55e-15f, /*max_length=*/0, corner);
  }
  size_t junc_reps = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (!before.contains(inst)) {
      ++junc_reps;
      EXPECT_LT(inst->getMaster()->getArea(), buf_x4_master->getArea());
    }
  }
  EXPECT_EQ(junc_reps, 1u);
}

TEST_F(TestResizer, RepairClkNetsUsesClockBuffersAndRestoresSizingLimits)
{
  TestRepairDesign repair_design(&resizer_);
  setupRepairNetTest(repair_design);

  // Run data-net repair first, then clock-net repair, then data-net selection
  // in the same session to exercise candidate-cache isolation across both.
  std::unordered_set<odb::dbInst*> before_data;
  for (odb::dbInst* inst : block_->getInsts()) {
    before_data.insert(inst);
  }
  resizer_.repairDesign(/*max_wire_length=*/250e-6,
                        /*slew_margin=*/0.0,
                        /*cap_margin=*/0.0,
                        /*buffer_gain=*/0.0,
                        /*match_cell_footprint=*/false,
                        /*reroute=*/false,
                        /*verbose=*/false);
  size_t data_reps = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (!before_data.contains(inst)) {
      ++data_reps;
      sta::LibertyCell* cell = db_network_->libertyCell(inst);
      ASSERT_NE(cell, nullptr);
      EXPECT_NE(resizer_.getBufferUse(cell), BufferUse::CLOCK);
    }
  }
  EXPECT_GT(data_reps, 0u);

  const sta::LibertyCellSeq swappable_before
      = resizer_.getSwappableCells(resizer_.lowestDriveBufferCell());
  std::unordered_set<odb::dbInst*> before_clk;
  for (odb::dbInst* inst : block_->getInsts()) {
    before_clk.insert(inst);
  }
  // On 150 um clock wire segments, Nangate45 CLKBUF_X* meets target_slew and
  // DRC so repairClkNets must insert dedicated clock buffers.
  repair_design.repairClkNets(/*max_wire_length=*/150e-6);
  size_t clk_reps = 0;
  size_t clk_use_count = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (!before_clk.contains(inst)) {
      ++clk_reps;
      sta::LibertyCell* cell = db_network_->libertyCell(inst);
      ASSERT_NE(cell, nullptr);
      if (resizer_.getBufferUse(cell) == BufferUse::CLOCK) {
        ++clk_use_count;
      }
    }
  }
  EXPECT_GT(clk_reps, 0u);
  EXPECT_GT(clk_use_count, 0u);

  EXPECT_EQ(resizer_.getSwappableCells(resizer_.lowestDriveBufferCell()),
            swappable_before);
  repair_design.init();
  sta::LibertyCell* post_clk = repair_design.findBufferUnderSlew(
      /*max_slew=*/1e-9f, /*load_cap=*/15e-15f);
  ASSERT_NE(post_clk, nullptr);
  EXPECT_NE(resizer_.getBufferUse(post_clk), BufferUse::CLOCK);

  sta::LibertyCell* buf_x1 = sta_->network()->findLibertyCell("BUF_X1");
  sta::LibertyCell* buf_x2 = sta_->network()->findLibertyCell("BUF_X2");
  sta::LibertyCell* buf_x4 = sta_->network()->findLibertyCell("BUF_X4");
  ASSERT_NE(buf_x1, nullptr);
  ASSERT_NE(buf_x2, nullptr);
  ASSERT_NE(buf_x4, nullptr);
  auto* area_prop = odb::dbDoubleProperty::find(block_, "limit_sizing_area");
  auto* leak_prop = odb::dbDoubleProperty::find(block_, "limit_sizing_leakage");
  ASSERT_NE(area_prop, nullptr);
  ASSERT_NE(leak_prop, nullptr);

  // Verify sizing_area_limit_ restoration on uncached BUF_X2 -> BUF_X4
  // (1.75x area > 1.5, 2.00x leakage <= 100.0).
  area_prop->setValue(1.5);
  leak_prop->setValue(100.0);
  resizer_.initBlock();
  resizer_.resizePreamble();
  repair_design.repairClkNets(/*max_wire_length=*/0);
  const sta::LibertyCellSeq area_limited = resizer_.getSwappableCells(buf_x2);
  EXPECT_EQ(std::ranges::find(area_limited, buf_x4), area_limited.end());

  // Verify sizing_leakage_limit_ restoration on uncached BUF_X1 -> BUF_X4
  // (2.33x area <= 100.0, 4.02x leakage > 3.0).
  area_prop->setValue(100.0);
  leak_prop->setValue(3.0);
  resizer_.initBlock();
  resizer_.resizePreamble();
  repair_design.repairClkNets(/*max_wire_length=*/0);
  const sta::LibertyCellSeq leak_limited = resizer_.getSwappableCells(buf_x1);
  EXPECT_EQ(std::ranges::find(leak_limited, buf_x4), leak_limited.end());
}

}  // namespace rsz
