#include "../../src/hier_rtlmp.h"
#include "gtest/gtest.h"
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
    utl::Logger* logger = new utl::Logger();

    db_ = OdbUniquePtr<odb::dbDatabase>(odb::dbDatabase::create(),
                                        &odb::dbDatabase::destroy);
    chip_ = OdbUniquePtr<odb::dbChip>(odb::dbChip::create(db_.get()),
                                      &odb::dbChip::destroy);
    block_
        = OdbUniquePtr<odb::dbBlock>(chip_->getBlock(), &odb::dbBlock::destroy);
  }

  utl::Logger logger_;
  OdbUniquePtr<odb::dbDatabase> db_{nullptr, &odb::dbDatabase::destroy};
  OdbUniquePtr<odb::dbLib> lib_{nullptr, &odb::dbLib::destroy};
  OdbUniquePtr<odb::dbChip> chip_{nullptr, &odb::dbChip::destroy};
  OdbUniquePtr<odb::dbBlock> block_{nullptr, &odb::dbBlock::destroy};
};

TEST_F(Mpl2SnapperTest, CanSetMacroForEmptyInstances)
{
  // create a simple block and then add 3 instances to that block
  // without any further configuration to each instance,
  // and then run setMacro(inst) on each instance

  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db_ = odb::dbDatabase::create();
  db_->setLogger(logger);

  odb::dbTech* tech_ = odb::dbTech::create(db_, "tech");
  odb::dbLib* lib_ = odb::dbLib::create(db_, "lib", tech_, ',');
  odb::dbTechLayer::create(tech_, "L1", odb::dbTechLayerType::MASTERSLICE);
  odb::dbChip* chip_ = odb::dbChip::create(db_);

  odb::dbMaster* master_ = odb::dbMaster::create(lib_, "simple_master");
  master_->setWidth(1000);
  master_->setHeight(1000);
  master_->setType(odb::dbMasterType::CORE);
  odb::dbMTerm::create(
      master_, "in", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMTerm::create(
      master_, "out", odb::dbIoType::OUTPUT, odb::dbSigType::SIGNAL);
  master_->setFrozen();

  odb::dbBlock* block_ = odb::dbBlock::create(chip_, "simple_block");
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbDatabase::beginEco(block_);
  odb::dbInst* inst1 = odb::dbInst::create(block_, master_, "cells_1");
  odb::dbInst* inst2 = odb::dbInst::create(block_, master_, "cells_2");
  odb::dbInst* inst3 = odb::dbInst::create(block_, master_, "cells_3");
  odb::dbDatabase::endEco(block_);

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
  // snapMacro works by computing a valid origin for a
  // particular macro instance by taking its pin alignment
  // into account, and then setting the instance's origin
  // accordingly using setOrigin.
  //
  // Therefore, this test inputs invalid instance origins
  // as testcases, and then checks whether snapMacro
  // has aligned those instances correctly.

  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db_ = odb::dbDatabase::create();
  db_->setLogger(logger);

  odb::dbTech* tech_ = odb::dbTech::create(db_, "tech");
  odb::dbLib* lib_ = odb::dbLib::create(db_, "lib", tech_, ',');
  odb::dbChip* chip_ = odb::dbChip::create(db_);

  // create simple block of size 1000x1000, then add 1 layer
  odb::dbBlock* block_ = odb::dbBlock::create(chip_, "simple_block");
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbTechLayer* layer_
      = odb::dbTechLayer::create(tech_, "layer", odb::dbTechLayerType::CUT);
  odb::dbTrackGrid* track_ = odb::dbTrackGrid::create(block_, layer_);

  // grid pattern parameters: origin, line count, step
  // (0, 50, 20) -> 0 20 40 60 80 ... 980
  // with manufacturing grid 5
  odb::dbGCellGrid* grid_ = odb::dbGCellGrid::create(block_);
  track_->addGridPatternX(0, 50, 20);
  track_->addGridPatternY(0, 50, 20);
  tech_->setManufacturingGrid(5);

  // add a master
  odb::dbMaster* master_ = odb::dbMaster::create(lib_, "simple_master");
  master_->setWidth(1000);
  master_->setHeight(1000);
  master_->setType(odb::dbMasterType::BLOCK);  // snapper expects block type

  // add two pins each of size 50x50
  // input at (0,0) and output at (100, 100)
  // snapper will only take signal pins into consideration
  odb::dbMTerm* mterm_i = odb::dbMTerm::create(
      master_, "in", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin_i = odb::dbMPin::create(mterm_i);
  odb::dbBox* box_i = odb::dbBox::create(mpin_i, layer_, 0, 0, 50, 50);

  odb::dbMTerm* mterm_o = odb::dbMTerm::create(
      master_, "out", odb::dbIoType::OUTPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin_o = odb::dbMPin::create(mterm_o);
  odb::dbBox* box_o = odb::dbBox::create(mpin_o, layer_, 100, 100, 150, 150);
  master_->setFrozen();

  // create a macro instance
  odb::dbDatabase::beginEco(block_);
  odb::dbInst* inst_ = odb::dbInst::create(block_, master_, "macro1");
  odb::dbDatabase::endEco(block_);

  // A summary of the set-up (all quantities in internal DB units):
  // -> master and block both 1000 x 1000, with only 1 layer
  // -> 2 signal pins at (0, 0, 50, 50) and (100, 100, 150, 150)
  // -> grid pattern is (0, 50, 20) -> 0 20 40 60 80 ... 980
  // -> manufacturing grid size is 5
  // -> set-up used to create 1 macro instance

  Snapper snapper;
  snapper.setMacro(inst_);

  // At the moment, this test only tests invalid input origins,
  // because right now, Snapper will not necessarily compute
  // the most optimal origin for valid input origins (i.e. no change)
  // However, this might change in the future if Snapepr is optimized
  // further.

  // First, we want to test for when the layer
  // has a "horizontal" direction preference.
  layer_->setDirection(odb::dbTechLayerDir::HORIZONTAL);

  // Considering the grid pattern configuration (0 20 40 ... 980)
  // manufacturing grid size (5), and direction preference (horizontal):
  // Valid alignments for x would include 10 15 20 25 30 35 40 ...
  // with 14 to 11 snapping to 10, 19 to 16 snapping to 15, etc.
  // Valid alignments for y would include 15 35 55 75 95 ...
  // with 40 to 59 snapping to 15, 60 to 79 mapping to 35, etc.

  inst_->setOrigin(511, 540);
  snapper.snapMacro();
  EXPECT_TRUE(inst_->getOrigin().x() == 510);
  EXPECT_TRUE(inst_->getOrigin().y() == 515);

  inst_->setOrigin(514, 559);
  snapper.snapMacro();
  EXPECT_TRUE(inst_->getOrigin().x() == 510);
  EXPECT_TRUE(inst_->getOrigin().y() == 515);

  inst_->setOrigin(516, 560);
  snapper.snapMacro();
  EXPECT_TRUE(inst_->getOrigin().x() == 515);
  EXPECT_TRUE(inst_->getOrigin().y() == 535);

  inst_->setOrigin(519, 579);
  snapper.snapMacro();
  EXPECT_TRUE(inst_->getOrigin().x() == 515);
  EXPECT_TRUE(inst_->getOrigin().y() == 535);

  // Now, we want to test for when the layer
  // has a "vertical" direction preference.
  layer_->setDirection(odb::dbTechLayerDir::VERTICAL);

  // Considering the grid pattern configuration (0 20 40 ... 980)
  // manufacturing grid size (5), and direction preference (horizontal):
  // Valid alignments for x would include 15 35 55 75 95 ...
  // with 40 to 59 snapping to 15, 60 to 79 mapping to 35, etc.
  // Valid alignments for y would include 10 15 20 25 30 35 40 ...
  // with 14 to 11 snapping to 10, 19 to 16 snapping to 15, etc.

  inst_->setOrigin(540, 511);
  snapper.snapMacro();
  EXPECT_TRUE(inst_->getOrigin().x() == 515);
  EXPECT_TRUE(inst_->getOrigin().y() == 510);

  inst_->setOrigin(559, 514);
  snapper.snapMacro();
  EXPECT_TRUE(inst_->getOrigin().x() == 515);
  EXPECT_TRUE(inst_->getOrigin().y() == 510);

  inst_->setOrigin(560, 516);
  snapper.snapMacro();
  EXPECT_TRUE(inst_->getOrigin().x() == 535);
  EXPECT_TRUE(inst_->getOrigin().y() == 515);

  inst_->setOrigin(579, 519);
  snapper.snapMacro();
  EXPECT_TRUE(inst_->getOrigin().x() == 535);
  EXPECT_TRUE(inst_->getOrigin().y() == 515);

}  // CanSnapMacros

}  // namespace mpl2