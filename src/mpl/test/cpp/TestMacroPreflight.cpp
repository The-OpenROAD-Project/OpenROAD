// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

#include "gtest/gtest.h"
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
    odb::dbTech* tech = odb::dbTech::create(db_.get(), "tech");
    lib_ = odb::dbLib::create(db_.get(), "lib", tech, ',');
    odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(db_->getChip(), "block");
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

  std::string snapshot()
  {
    std::ostringstream stream;
    db_->write(stream);
    return stream.str();
  }

  void expectPassUnchanged(odb::Rect fence = {}, int width = 0, int height = 0)
  {
    const std::string before = snapshot();
    EXPECT_NO_THROW(placer_->checkMacroPlacement(fence, width, height));
    EXPECT_EQ(snapshot(), before);
  }

  void expectErrorUnchanged(const char* id,
                            odb::Rect fence = {},
                            int width = 0,
                            int height = 0)
  {
    const std::string before = snapshot();
    EXPECT_LOGGER_ERROR(placer_->checkMacroPlacement(fence, width, height), id);
    EXPECT_EQ(snapshot(), before);
  }

  odb::dbLib* lib_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  std::unique_ptr<MacroPlacer> placer_;
};

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

TEST_F(TestMacroPreflight, DisjointFenceFailsEvenWithoutMovableMacros)
{
  expectErrorUnchanged("MPL-0068", odb::Rect(20000, 20000, 30000, 30000));
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
  fixed->setLocation(1000, 1000);
  expectErrorUnchanged("MPL-0065");
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
