#include "../../src/hier_rtlmp.h"
#include "gtest/gtest.h"
#include "helper.h"
#include "mpl2/rtl_mp.h"
#include "odb/db.h"
#include "odb/util.h"
#include "utl/Logger.h"

namespace mpl2 {
class Mpl2SnapperTest : public ::testing::Test
{
 protected:
  template <class T>
  using OdbUniquePtr = std::unique_ptr<T, void (*)(T*)>;

  void SetUp() override
  {
    db_ = OdbUniquePtr<odb::dbDatabase>(odb::dbDatabase::create(),
                                        &odb::dbDatabase::destroy);
    chip_ = OdbUniquePtr<odb::dbChip>(odb::dbChip::create(db_.get()),
                                      &odb::dbChip::destroy);
    block_
        = OdbUniquePtr<odb::dbBlock>(chip_->getBlock(), &odb::dbBlock::destroy);
  }

  utl::Logger logger;
  OdbUniquePtr<odb::dbDatabase> db_{nullptr, &odb::dbDatabase::destroy};
  OdbUniquePtr<odb::dbLib> lib_{nullptr, &odb::dbLib::destroy};
  OdbUniquePtr<odb::dbChip> chip_{nullptr, &odb::dbChip::destroy};
  OdbUniquePtr<odb::dbBlock> block_{nullptr, &odb::dbBlock::destroy};
};

TEST_F(Mpl2SnapperTest, CanSetMacroForEmptyInstances)
{
  // Create a simple block and then add 3 instances to that block
  // without any further configuration to each instance,
  // and then run setMacro(inst) on each instance.

  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db_ = createSimpleDB();
  db_->setLogger(logger);

  odb::dbMaster* master = createSimpleMaster(db_->findLib("lib"),
                                             "simple_master",
                                             1000,
                                             1000,
                                             odb::dbMasterType::CORE);

  odb::dbBlock* block = odb::dbBlock::create(db_->getChip(), "simple_block");
  block->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbInst* inst1 = odb::dbInst::create(block, master, "cells_1");
  odb::dbInst* inst2 = odb::dbInst::create(block, master, "cells_2");
  odb::dbInst* inst3 = odb::dbInst::create(block, master, "cells_3");

  Snapper snapper;
  snapper.setMacro(inst1);
  snapper.setMacro(inst2);
  snapper.setMacro(inst3);
}  // CanSetMacroForEmptyInstances

TEST_F(Mpl2SnapperTest, CanSnapMacros)
{
  // This test checks the snapMacro functionality of the
  // Snapper class defined in hier_rtlmp.cpp.
  //
  // snapMacro works by adjusting the position of the
  // macro so that the signal pins are aligned with the
  // track-grid.
  //
  // Therefore, this test inputs instances that have
  // not been aligned correctly as testcases, and then
  // checks whether snapMacro has aligned said instances
  // correctly.

  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db_ = createSimpleDB();
  db_->setLogger(logger);

  odb::dbBlock* block = odb::dbBlock::create(db_->getChip(), "simple_block");
  block->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbTrackGrid* track
      = odb::dbTrackGrid::create(block, db_->getTech()->findLayer("L1"));

  odb::dbGCellGrid::create(block);
  track->addGridPatternX(0, 50, 20);
  track->addGridPatternY(0, 50, 20);
  db_->getTech()->setManufacturingGrid(5);

  odb::dbMaster* master = createSimpleMaster(db_->findLib("lib"),
                                             "simple_master",
                                             1000,
                                             1000,
                                             odb::dbMasterType::BLOCK);

  odb::dbInst* inst = odb::dbInst::create(block, master, "macro1");

  Snapper snapper;
  snapper.setMacro(inst);

  // At the moment, this test only receives instance unaligned with
  // the track-grid as inputs, because right now, Snapper will not
  // necessarily compute the most optimal origin for instances that
  // are  already aligned (origins should've been left unchanged).
  //
  // However, this might change in the future if Snapper is optimized
  // further.

  // First, we want to test for when the layer
  // has a "horizontal" direction preference.
  db_->getTech()->findLayer("L1")->setDirection(
      odb::dbTechLayerDir::HORIZONTAL);

  // Considering the grid pattern configuration (0 20 40 ... 980)
  // manufacturing grid size (5), and direction preference (horizontal):
  // Valid alignments for x would include 10 15 20 25 30 35 40 ...
  // with 14 to 11 snapping to 10, 19 to 16 snapping to 15, etc.
  // Valid alignments for y would include 15 35 55 75 95 ...
  // with 40 to 59 snapping to 15, 60 to 79 mapping to 35, etc.

  inst->setOrigin(511, 540);
  snapper.snapMacro();
  EXPECT_TRUE(inst->getOrigin().x() == 510);
  EXPECT_TRUE(inst->getOrigin().y() == 515);

  inst->setOrigin(514, 559);
  snapper.snapMacro();
  EXPECT_TRUE(inst->getOrigin().x() == 510);
  EXPECT_TRUE(inst->getOrigin().y() == 515);

  inst->setOrigin(516, 560);
  snapper.snapMacro();
  EXPECT_TRUE(inst->getOrigin().x() == 515);
  EXPECT_TRUE(inst->getOrigin().y() == 535);

  inst->setOrigin(519, 579);
  snapper.snapMacro();
  EXPECT_TRUE(inst->getOrigin().x() == 515);
  EXPECT_TRUE(inst->getOrigin().y() == 535);

  // Now, we want to test for when the layer
  // has a "vertical" direction preference.
  db_->getTech()->findLayer("L1")->setDirection(odb::dbTechLayerDir::VERTICAL);

  // Considering the grid pattern configuration (0 20 40 ... 980)
  // manufacturing grid size (5), and direction preference (horizontal):
  // Valid alignments for x would include 15 35 55 75 95 ...
  // with 40 to 59 snapping to 15, 60 to 79 mapping to 35, etc.
  // Valid alignments for y would include 10 15 20 25 30 35 40 ...
  // with 14 to 11 snapping to 10, 19 to 16 snapping to 15, etc.

  inst->setOrigin(540, 511);
  snapper.snapMacro();
  EXPECT_TRUE(inst->getOrigin().x() == 515);
  EXPECT_TRUE(inst->getOrigin().y() == 510);

  inst->setOrigin(559, 514);
  snapper.snapMacro();
  EXPECT_TRUE(inst->getOrigin().x() == 515);
  EXPECT_TRUE(inst->getOrigin().y() == 510);

  inst->setOrigin(560, 516);
  snapper.snapMacro();
  EXPECT_TRUE(inst->getOrigin().x() == 535);
  EXPECT_TRUE(inst->getOrigin().y() == 515);

  inst->setOrigin(579, 519);
  snapper.snapMacro();
  EXPECT_TRUE(inst->getOrigin().x() == 535);
  EXPECT_TRUE(inst->getOrigin().y() == 515);

}  // CanSnapMacros

}  // namespace mpl2
