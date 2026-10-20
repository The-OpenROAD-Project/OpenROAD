// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include "clusterEngine.h"
#include "gtest/gtest.h"
#include "mpl-util.h"
#include "mpl/rtl_mp.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tst/db_fixture.h"
#include "tst/logger_checks.h"

namespace mpl {
namespace {

class TestMacroPreflight : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    db_->setDbuPerMicron(1000);
    odb::dbTech* tech = odb::dbTech::create(db_.get(), "tech");
    tech->setLefUnits(1000);
    lib_ = odb::dbLib::create(db_.get(), "lib", tech, ',');
    lib_->setLefUnits(1000);
    odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(db_->getChip(), "block");
    block_->setDefUnits(1000);
    block_->setDieArea(odb::Rect(0, 0, 30000, 30000));
    block_->setCoreArea(odb::Rect(0, 0, 10000, 10000));
    placer_
        = std::make_unique<MacroPlacer>(db_.get(), nullptr, &logger_, nullptr);
  }

  odb::dbInst* createInst(const std::string& name,
                          int width,
                          int height,
                          odb::dbMasterType type = odb::dbMasterType::BLOCK,
                          odb::dbModule* parent = nullptr)
  {
    odb::dbMaster* master
        = odb::dbMaster::create(lib_, (name + "_master").c_str());
    master->setType(type);
    master->setWidth(width);
    master->setHeight(height);
    master->setFrozen();
    return makeInst(block_, master, name.c_str(), {.parent_module = parent});
  }

  odb::dbInst* createPinnedMacro()
  {
    odb::dbTechLayer* layer = odb::dbTechLayer::create(
        db_->getTech(), "routing", odb::dbTechLayerType::ROUTING);
    layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);
    layer->setSpacing(100);
    odb::dbMaster* master = odb::dbMaster::create(lib_, "pinned_master");
    master->setType(odb::dbMasterType::BLOCK);
    master->setWidth(5000);
    master->setHeight(5000);
    odb::dbMTerm* term = odb::dbMTerm::create(
        master, "left_pin", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
    odb::dbMPin* pin = odb::dbMPin::create(term);
    odb::dbBox::create(pin, layer, 0, 2500, 100, 2600);
    master->setFrozen();
    return makeInst(block_, master, "pinned_macro");
  }

  std::string snapshot()
  {
    std::ostringstream stream;
    db_->write(stream);
    return stream.str();
  }

  void expectPassUnchanged(std::optional<odb::Rect> fence = std::nullopt,
                           int width = 0,
                           int height = 0,
                           bool pin_aware = false)
  {
    const std::string before = snapshot();
    EXPECT_NO_THROW(
        placer_->checkMacroPlacement(fence, width, height, pin_aware));
    EXPECT_EQ(snapshot(), before);
  }

  void expectErrorUnchanged(const char* id,
                            std::optional<odb::Rect> fence = std::nullopt,
                            int width = 0,
                            int height = 0,
                            bool pin_aware = false)
  {
    const std::string before = snapshot();
    EXPECT_LOGGER_ERROR(
        placer_->checkMacroPlacement(fence, width, height, pin_aware), id);
    EXPECT_EQ(snapshot(), before);
  }

  void expectInvalidFencesUnchanged()
  {
    const odb::Rect fences[] = {odb::Rect(11000, 1000, 20000, 9000),
                                odb::Rect(-10000, 1000, -1000, 9000),
                                odb::Rect(1000, 11000, 9000, 20000),
                                odb::Rect(1000, -10000, 9000, -1000),
                                odb::Rect(11000, 11000, 20000, 20000),
                                odb::Rect(-10000, -10000, -1000, -1000),
                                odb::Rect(10000, 1000, 20000, 9000),
                                odb::Rect(-10000, 1000, 0, 9000),
                                odb::Rect(1000, 10000, 9000, 20000),
                                odb::Rect(1000, -10000, 9000, 0),
                                odb::Rect(10000, 10000, 20000, 20000),
                                odb::Rect(-10000, -10000, 0, 0)};
    for (const odb::Rect& fence : fences) {
      SCOPED_TRACE(::testing::Message()
                   << "fence " << fence.xMin() << "," << fence.yMin() << " to "
                   << fence.xMax() << "," << fence.yMax());
      expectErrorUnchanged("MPL-0068", fence);
    }
  }

  odb::dbLib* lib_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  std::unique_ptr<MacroPlacer> placer_;
};

TEST_F(TestMacroPreflight, CheckedMicronConversionValidatesBeforeIntegerCast)
{
  const std::string before = snapshot();
  const double units = block_->getDbUnitsPerMicron();
  const int min_dbu = std::numeric_limits<int>::min();
  const int max_dbu = std::numeric_limits<int>::max();
  EXPECT_EQ(checkedMicronsToDbu(
                block_, (min_dbu - 0.25) / units, "-fence_lx", &logger_),
            min_dbu);
  EXPECT_EQ(checkedMicronsToDbu(
                block_, (max_dbu + 0.25) / units, "-fence_ux", &logger_),
            max_dbu);
  EXPECT_EQ(checkedMicronsToDbu(
                block_, 0.5 / units, "-min_channel_size width", &logger_),
            1);
  EXPECT_EQ(checkedMicronsToDbu(
                block_, -0.5 / units, "-min_channel_size height", &logger_),
            -1);
  const double invalid_values[] = {(min_dbu - 0.75) / units,
                                   (max_dbu + 0.75) / units,
                                   -1e9,
                                   1e9,
                                   -1e308,
                                   1e308,
                                   -std::numeric_limits<double>::infinity(),
                                   std::numeric_limits<double>::infinity(),
                                   std::numeric_limits<double>::quiet_NaN()};
  for (const double value : invalid_values) {
    EXPECT_LOGGER_ERROR(
        checkedMicronsToDbu(block_, value, "-fence_lx", &logger_), "MPL-0083");
  }
  EXPECT_EQ(snapshot(), before);
}

TEST_F(TestMacroPreflight, FeasiblePlacedDesignIsUnchanged)
{
  odb::dbInst* macro = createInst("macro", 5000, 5000);
  macro->setLocation(1000, 2000);
  macro->setOrient(odb::dbOrientType::MX);
  macro->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  macro->setHalo(100, 200, 300, 400, false);
  odb::dbIntProperty::create(macro, "preflight_marker", 42);
  createInst("cell", 1000, 1000, odb::dbMasterType::CORE);

  expectPassUnchanged();
  expectPassUnchanged();
}

TEST_F(TestMacroPreflight, MovableCellsExceedPlacementArea)
{
  createInst("macro", 5000, 5000);
  createInst("cell", 10000, 10000, odb::dbMasterType::CORE);
  expectErrorUnchanged("MPL-0065");
}

TEST_F(TestMacroPreflight, MacroHalosExceedPlacementArea)
{
  createInst("macro_a", 6000, 6000);
  createInst("macro_b", 6000, 6000);
  expectPassUnchanged();
  expectErrorUnchanged("MPL-0065", {}, 2000, 2000);
  expectPassUnchanged();
}

TEST_F(TestMacroPreflight, InstanceAreaBudgetIncludesMacroHalos)
{
  odb::dbInst* macro = createInst("macro", 5000, 5000);
  // The movable-area check clips fixed cells to the placement area, whereas
  // the placer's module-area budget also counts standard cells outside it.
  odb::dbInst* cell
      = createInst("outside_cell", 8000, 8000, odb::dbMasterType::CORE);
  cell->setLocation(12000, 0);
  cell->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  expectPassUnchanged();
  macro->setHalo(1000, 1000, 1000, 1000, false);
  expectErrorUnchanged("MPL-0016");
}

TEST_F(TestMacroPreflight, OversizedMacroIncludingHalo)
{
  odb::dbInst* macro = createInst("macro", 9000, 5000);
  expectPassUnchanged();
  macro->setHalo(600, 0, 600, 0, false);
  expectErrorUnchanged("MPL-0006");
}

TEST_F(TestMacroPreflight, ExplicitMalformedFencesDoNotDefaultToCore)
{
  createInst("macro", 5000, 5000);
  odb::Rect fences[6];
  fences[1].reset(2000, 0, 2000, 10000);
  fences[2].reset(0, 2000, 10000, 2000);
  fences[3].reset(8000, 0, 2000, 10000);
  fences[4].reset(0, 8000, 10000, 2000);
  fences[5].reset(8000, 8000, 2000, 2000);
  for (int phase = 0; phase < 2; ++phase) {
    if (phase == 1) {
      placer_->setMinChannelSize(400, 800);
    }
    for (const odb::Rect& fence : fences) {
      expectErrorUnchanged("MPL-0082", fence);
      expectPassUnchanged();
    }
  }
}

TEST_F(TestMacroPreflight, ReusedTreeClearsDisjointIntersectionOutput)
{
  PhysicalHierarchy tree;
  ClusteringEngine engine(block_, &logger_, nullptr, nullptr);
  engine.setTree(&tree);
  engine.setChannel({}, false);
  const std::string before = snapshot();
  const odb::Rect core = block_->getCoreArea();
  const odb::Rect fences[] = {odb::Rect(11000, 1000, 20000, 9000),
                              odb::Rect(1000, 11000, 9000, 20000),
                              odb::Rect(11000, 11000, 20000, 20000)};
  for (const odb::Rect& fence : fences) {
    tree.global_fence = core;
    EXPECT_NO_THROW(engine.checkFeasibility());
    ASSERT_EQ(tree.floorplan_shape, core);
    tree.global_fence = fence;
    EXPECT_LOGGER_ERROR(engine.checkFeasibility(), "MPL-0068");
    EXPECT_EQ(tree.floorplan_shape, odb::Rect());
    EXPECT_EQ(snapshot(), before);
  }
}

TEST_F(TestMacroPreflight, DisjointAndEdgeFencesFailForEmptyDesign)
{
  expectInvalidFencesUnchanged();
}

TEST_F(TestMacroPreflight, DisjointAndEdgeFencesFailForMovableMacros)
{
  createInst("macro", 5000, 5000);
  expectInvalidFencesUnchanged();
}

TEST_F(TestMacroPreflight, DisjointAndEdgeFencesFailForAllFixedMacros)
{
  odb::dbInst* macro = createInst("macro", 5000, 5000);
  macro->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  expectInvalidFencesUnchanged();
}

TEST_F(TestMacroPreflight, PartiallyOverlappingFenceIsClipped)
{
  createInst("macro_a", 6000, 6000);
  expectPassUnchanged(odb::Rect(-5000, -5000, 8000, 8000));
  createInst("macro_b", 6000, 6000);
  expectErrorUnchanged("MPL-0065", odb::Rect(-5000, -5000, 8000, 8000));
  expectPassUnchanged();
}

TEST_F(TestMacroPreflight, FixedStandardCellInsidePlacementArea)
{
  createInst("macro", 5000, 5000);
  odb::dbInst* cell
      = createInst("fixed_cell", 1000, 1000, odb::dbMasterType::CORE);
  cell->setLocation(2000, 2000);
  cell->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  expectErrorUnchanged("MPL-0050");
}

TEST_F(TestMacroPreflight, FixedStandardCellInChildModule)
{
  createInst("macro", 5000, 5000);
  odb::dbModule* child = odb::dbModule::create(block_, "child");
  odb::dbModInst::create(block_->getTopModule(), child, "u_child");
  odb::dbInst* cell = createInst(
      "u_child/fixed_cell", 1000, 1000, odb::dbMasterType::CORE, child);
  cell->setLocation(2000, 2000);
  cell->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
  expectErrorUnchanged("MPL-0050");
}

TEST_F(TestMacroPreflight, FixedStandardCellOutsidePlacementArea)
{
  createInst("macro", 5000, 5000);
  odb::dbInst* cell
      = createInst("fixed_cell", 1000, 1000, odb::dbMasterType::CORE);
  cell->setLocation(12000, 2000);
  cell->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  expectPassUnchanged();
}

TEST_F(TestMacroPreflight, CoverInsidePlacementAreaIsAllowed)
{
  createInst("macro", 5000, 5000);
  odb::dbInst* cover
      = createInst("cover", 1000, 1000, odb::dbMasterType::COVER);
  cover->setLocation(2000, 2000);
  cover->setPlacementStatus(odb::dbPlacementStatus::COVER);
  expectPassUnchanged();
}

TEST_F(TestMacroPreflight, AllFixedSkipsCellFeasibility)
{
  odb::dbInst* macro = createInst("macro", 5000, 5000);
  macro->setLocation(1000, 1000);
  macro->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
  odb::dbInst* cell
      = createInst("fixed_cell", 10000, 10000, odb::dbMasterType::CORE);
  cell->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  expectPassUnchanged();
  macro->setPlacementStatus(odb::dbPlacementStatus::NONE);
  expectErrorUnchanged("MPL-0065");
}

TEST_F(TestMacroPreflight, EmptyDesign)
{
  expectPassUnchanged();
}

TEST_F(TestMacroPreflight, OverlappingBlockagesAreCountedOnce)
{
  createInst("macro", 5000, 5000);
  odb::dbBlockage::create(block_, 5000, 0, 10000, 10000);
  odb::dbBlockage::create(block_, 5000, 0, 10000, 10000);
  expectPassUnchanged();
  odb::dbBlockage::create(block_, 0, 0, 5000, 10000);
  expectErrorUnchanged("MPL-0065");
}

TEST_F(TestMacroPreflight, FixedMacroOccupationIsClippedToPlacementArea)
{
  createInst("movable", 5000, 5000);
  odb::dbInst* fixed = createInst("fixed", 8000, 8000);
  fixed->setLocation(9000, 9000);
  fixed->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  createInst("cell", 4000, 4000, odb::dbMasterType::CORE);
  expectPassUnchanged();
  fixed->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  fixed->setLocation(1000, 1000);
  fixed->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  expectErrorUnchanged("MPL-0065");
}

TEST_F(TestMacroPreflight, PinAwareChannelsTrimOnlyUnpinnedEdges)
{
  odb::dbInst* macro = createPinnedMacro();
  expectErrorUnchanged("MPL-0006", {}, 6000, 6000);
  expectPassUnchanged({}, 6000, 6000, true);
  expectErrorUnchanged("MPL-0006", {}, 6000, 6000);
  expectErrorUnchanged("MPL-0006", {}, 10000, 6000, true);
  // A DEF halo takes precedence over pin-aware channel trimming.
  macro->setHalo(3000, 3000, 3000, 3000, false);
  expectErrorUnchanged("MPL-0006", {}, 6000, 6000, true);
}

TEST_F(TestMacroPreflight, TechnologySpacingFloorsUnpinnedEdges)
{
  createPinnedMacro();
  odb::dbTechLayer* layer = db_->getTech()->findLayer("routing");
  layer->setSpacing(2500);
  expectPassUnchanged({}, 0, 0, true);
  layer->setSpacing(2501);
  expectErrorUnchanged("MPL-0006", {}, 0, 0, true);
}

TEST_F(TestMacroPreflight, ConfiguredChannelSurvivesSuccessAndFailure)
{
  createInst("movable", 5000, 5000);
  odb::dbInst* fixed = createInst("fixed", 2000, 2000);
  fixed->setLocation(4000, 4000);
  fixed->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  placer_->setMinChannelSize(400, 800);
  expectPassUnchanged({}, 2000, 4000);
  expectErrorUnchanged("MPL-0006", {}, 10000, 10000);
  expectPassUnchanged();

  // This later operation observes the original configured channel, not either
  // preflight invocation's channel. The preflight snapshots above allow no
  // edits.
  placer_->blockMacroChannels();
  ASSERT_EQ(block_->getBlockages().size(), 1);
  odb::dbBlockage* blockage = *block_->getBlockages().begin();
  EXPECT_TRUE(blockage->isSoft());
  EXPECT_EQ(blockage->getBBox()->getBox(), odb::Rect(3800, 3600, 6200, 6400));
}

TEST_F(TestMacroPreflight, ChannelGeometryStressDoesNotLeakState)
{
  createInst("macro_a", 2500, 2500);
  createInst("macro_b", 2500, 2500);
  int passes = 0;
  int area_failures = 0;
  int dimension_failures = 0;
  // Exercise asymmetric channel geometry on both sides of each boundary,
  // including failures followed by success, without running any placement.
  for (int width = 0; width <= 10000; width += 1000) {
    for (int height = 0; height <= 10000; height += 1000) {
      SCOPED_TRACE(::testing::Message()
                   << "channel " << width << " x " << height);
      const int64_t macro_width = 2500 + width;
      const int64_t macro_height = 2500 + height;
      if (macro_width > 10000 || macro_height > 10000) {
        expectErrorUnchanged("MPL-0006", {}, width, height);
        ++dimension_failures;
      } else if (2 * macro_width * macro_height > 100000000) {
        expectErrorUnchanged("MPL-0065", {}, width, height);
        ++area_failures;
      } else {
        expectPassUnchanged({}, width, height);
        ++passes;
      }
      expectPassUnchanged();
    }
  }
  EXPECT_GT(passes, 0);
  EXPECT_GT(area_failures, 0);
  EXPECT_GT(dimension_failures, 0);
}

}  // namespace
}  // namespace mpl
