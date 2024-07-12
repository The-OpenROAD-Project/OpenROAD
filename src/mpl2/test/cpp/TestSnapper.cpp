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
  odb::dbDatabase* db = createSimpleDB();
  db->setLogger(logger);

  odb::dbMaster* master = createSimpleMaster(db->findLib("lib"),
                                             "simple_master",
                                             1000,
                                             1000,
                                             odb::dbMasterType::CORE,
                                             db->getTech()->findLayer("L1"));

  odb::dbBlock* block = odb::dbBlock::create(db->getChip(), "simple_block");
  block->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbInst* inst = odb::dbInst::create(block, master, "cells");

  Snapper snapper;
  snapper.setMacro(inst);
}  // CanSetMacroForEmptyInstances

// The three following tests checks the snapMacro functionality of the
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

// At the moment, these tests only receive instances unaligned with
// the track-grid as inputs, because right now, Snapper will not
// necessarily compute the most optimal origin for instances that
// are already aligned (origins should've been left unchanged).
//
// However, this might change in the future if Snapper is optimized
// further.

TEST_F(Mpl2SnapperTest, HorizontalSnap)
{
  // This test checks snapMacro functionality
  // for a block with 1 master (horizontal direction layer preference)

  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db = createSimpleDB();
  db->setLogger(logger);

  odb::dbBlock* block = odb::dbBlock::create(db->getChip(), "simple_block");
  block->setDieArea(odb::Rect(0, 0, 1000, 1000));

  createSimpleTrack(block, db->getTech()->findLayer("L1"), 0, 50, 20, 5);

  db->getTech()->findLayer("L1")->setDirection(
      odb::dbTechLayerDir::HORIZONTAL);

  odb::dbMaster* master = createSimpleMaster(db->findLib("lib"),
                                             "simple_master",
                                             1000,
                                             1000,
                                             odb::dbMasterType::BLOCK,
                                             db->getTech()->findLayer("L1"));

  odb::dbInst* inst = odb::dbInst::create(block, master, "macro1");

  Snapper snapper;
  snapper.setMacro(inst);

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

}  // HorizontalSnap

TEST_F(Mpl2SnapperTest, VerticalSnap)
{
  // This test checks snapMacro functionality
  // for a block with 1 master (vertical direction layer preference)

  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db = createSimpleDB();
  db->setLogger(logger);

  odb::dbBlock* block = odb::dbBlock::create(db->getChip(), "simple_block");
  block->setDieArea(odb::Rect(0, 0, 1000, 1000));

  createSimpleTrack(block, db->getTech()->findLayer("L1"), 0, 50, 20, 5);

  db->getTech()->findLayer("L1")->setDirection(odb::dbTechLayerDir::VERTICAL);

  odb::dbMaster* master = createSimpleMaster(db->findLib("lib"),
                                             "simple_master",
                                             1000,
                                             1000,
                                             odb::dbMasterType::BLOCK,
                                             db->getTech()->findLayer("L1"));

  odb::dbInst* inst = odb::dbInst::create(block, master, "macro1");

  Snapper snapper;
  snapper.setMacro(inst);

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

}  // VerticalSnap

TEST_F(Mpl2SnapperTest, SnapMacrosDouble)
{
  // This test checks snapMacro functionality
  // for a block with 2 masters (ensure the 2 layers aren't interfering with
  // each other) (layer 1, 2 with horizontal, vertical preferences respectively)

  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db = createSimpleDB();
  db->setLogger(logger);

  odb::dbBlock* block = odb::dbBlock::create(db->getChip(), "simple_block");
  block->setDieArea(odb::Rect(0, 0, 1000, 1000));

  createSimpleTrack(block, db->getTech()->findLayer("L1"), 0, 50, 20, 5);
  createSimpleTrack(block, db->getTech()->findLayer("L2"), 0, 50, 20, 5);

  db->getTech()->findLayer("L1")->setDirection(
      odb::dbTechLayerDir::HORIZONTAL);
  db->getTech()->findLayer("L2")->setDirection(odb::dbTechLayerDir::VERTICAL);

  odb::dbMaster* master1 = createSimpleMaster(db->findLib("lib"),
                                              "master_horizontal",
                                              500,
                                              1000,
                                              odb::dbMasterType::BLOCK,
                                              db->getTech()->findLayer("L1"));

  odb::dbMaster* master2 = createSimpleMaster(db->findLib("lib"),
                                              "master_vertical",
                                              500,
                                              1000,
                                              odb::dbMasterType::BLOCK,
                                              db->getTech()->findLayer("L2"));

  odb::dbInst* inst1 = odb::dbInst::create(block, master1, "macro1");
  odb::dbInst* inst2 = odb::dbInst::create(block, master2, "macro2");

  Snapper snapper;

  // same testcases as HorizontalSnap
  snapper.setMacro(inst1);

  inst1->setOrigin(511, 540);
  snapper.snapMacro();
  EXPECT_TRUE(inst1->getOrigin().x() == 510);
  EXPECT_TRUE(inst1->getOrigin().y() == 515);

  inst1->setOrigin(514, 559);
  snapper.snapMacro();
  EXPECT_TRUE(inst1->getOrigin().x() == 510);
  EXPECT_TRUE(inst1->getOrigin().y() == 515);

  inst1->setOrigin(516, 560);
  snapper.snapMacro();
  EXPECT_TRUE(inst1->getOrigin().x() == 515);
  EXPECT_TRUE(inst1->getOrigin().y() == 535);

  inst1->setOrigin(519, 579);
  snapper.snapMacro();
  EXPECT_TRUE(inst1->getOrigin().x() == 515);
  EXPECT_TRUE(inst1->getOrigin().y() == 535);

  // same testcases as VerticalSnap
  snapper.setMacro(inst2);

  inst2->setOrigin(540, 511);
  snapper.snapMacro();
  EXPECT_TRUE(inst2->getOrigin().x() == 515);
  EXPECT_TRUE(inst2->getOrigin().y() == 510);

  inst2->setOrigin(559, 514);
  snapper.snapMacro();
  EXPECT_TRUE(inst2->getOrigin().x() == 515);
  EXPECT_TRUE(inst2->getOrigin().y() == 510);

  inst2->setOrigin(560, 516);
  snapper.snapMacro();
  EXPECT_TRUE(inst2->getOrigin().x() == 535);
  EXPECT_TRUE(inst2->getOrigin().y() == 515);

  inst2->setOrigin(579, 519);
  snapper.snapMacro();
  EXPECT_TRUE(inst2->getOrigin().x() == 535);
  EXPECT_TRUE(inst2->getOrigin().y() == 515);

}  // SnapMacrosDouble

}  // namespace mpl2
