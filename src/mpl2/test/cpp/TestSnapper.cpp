#include "gtest/gtest.h"
#include "mpl2/rtl_mp.h"
#include "../../src/hier_rtlmp.h"
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
    block_ = OdbUniquePtr<odb::dbBlock>(
        chip_->getBlock(), &odb::dbBlock::destroy);
    
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
  // and then run setMacro(inst) on each instance;
  // this simply verifies that setMacro(inst) doesn't crash

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
  odb::dbMTerm::create(master_, "in", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMTerm::create(master_, "out", odb::dbIoType::OUTPUT, odb::dbSigType::SIGNAL);
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
}

TEST_F(Mpl2SnapperTest, CanSnapMacros)
{
  // when snapMacro is called, it later calls
  // computeSnapOrigin (which calls computeSnapParameters,
  // getOrigin, getPitch, getOffset, etc) then setOrigin
  //
  // computeSnapOrigin:
  // - gets instance master, then each MTerm->MPins in master
  // - and then getGeometry in MPin -> add layers to snap_layers
  // - direction of each layer is checked and used as input for
  //   computeSnapParameters
  // computeSnapParameters:
  // - receives input dbTechLayer*, dbBox*, layer direction
  // - gets block of instance, then track grid, then pitch,
  //   offset, pin width, lower-left to first pin
  // - returns computation results
  // inst->setOrigin
  // - can be checked using inst->getOrigin

  // During the construction of a new HardMacro object
  // with input dbInst* inst, the following values are retrieved:
  // inst->getBlock, inst->getName, inst->getMaster,
  // master->getWidth, master->getHeight, master->getMTerms,
  // mterm->getSigType, mterm->getMPins,
  // mpin->getGeometry, box->getBox (see: object.cpp)
  
  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db_ = odb::dbDatabase::create();
  db_->setLogger(logger);
  
  odb::dbTech* tech_ = odb::dbTech::create(db_, "tech");
  odb::dbLib* lib_ = odb::dbLib::create(db_, "lib", tech_, ',');
  odb::dbChip* chip_ = odb::dbChip::create(db_);

  // create simple block of size 1000x1000
  odb::dbBlock* block_ = odb::dbBlock::create(chip_, "simple_block");
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
  odb::dbGCellGrid* grid_ = odb::dbGCellGrid::create(block_);

  // create 2 layers and configure with corresponding grid patterns
  // grid pattern parameters: origin, line count, step
  // (0, 50, 20) -> 0 20 40 60 80 ... 980
  odb::dbTechLayer* layer1_ = odb::dbTechLayer::create(tech_, "layer1", odb::dbTechLayerType::CUT);
  odb::dbTrackGrid* track1 = odb::dbTrackGrid::create(block_, layer1_);
  track1->addGridPatternX(0, 50, 20);
  track1->addGridPatternY(0, 50, 20);

  odb::dbTechLayer* layer2_ = odb::dbTechLayer::create(tech_, "layer2", odb::dbTechLayerType::CUT);
  odb::dbTrackGrid* track2 = odb::dbTrackGrid::create(block_, layer2_);
  track2->addGridPatternX(0, 50, 20);
  track2->addGridPatternY(0, 50, 20);

  // set manufacturing grid size
  tech_->setManufacturingGrid(5); 

  // snapper expects a block type master
  odb::dbMaster* master_ = odb::dbMaster::create(lib_, "simple_master");
  master_->setWidth(1000);
  master_->setHeight(1000);
  master_->setType(odb::dbMasterType::BLOCK);
  
  // component with 1 input (layer1), 1 output (layer2) -> 2 terminals 
  // each pin occupies (0, 0) to (50, 50)
  odb::dbMTerm* mterm_i = odb::dbMTerm::create(master_, "in", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin_i = odb::dbMPin::create(mterm_i);
  odb::dbBox* box_i = odb::dbBox::create(mpin_i, layer1_, 0, 0, 50, 50);

  odb::dbMTerm* mterm_o = odb::dbMTerm::create(master_, "out", odb::dbIoType::OUTPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin_o = odb::dbMPin::create(mterm_o);
  odb::dbBox* box_o = odb::dbBox::create(mpin_o, layer2_, 0, 0, 50, 50);

  master_->setFrozen();

  // create 3 instances on block_
  odb::dbDatabase::beginEco(block_);
  odb::dbInst* inst1 = odb::dbInst::create(block_, master_, "cells_1");
  //odb::dbInst* inst2 = odb::dbInst::create(block_, master_, "cells_2");
  //odb::dbInst* inst3 = odb::dbInst::create(block_, master_, "cells_3");
  odb::dbDatabase::endEco(block_);

  // inst setup: (all quantities in internal DB units)
  // 1000 x 1000 master and block 
  // with 2 pins, one on each of the 2 layers, at (0, 0, 50, 50)
  // grid pattern is (0, 50, 20) -> 0 20 40 60 80 ... 980
  // manufacturing grid size is 5

  Snapper snapper;
  snapper.setMacro(inst1);

  inst1->setOrigin(435, 500);

  logger->report(
    "initial (x, y) of inst1: ({}, {})",
    inst1->getOrigin().x(),
    inst1->getOrigin().y()
  );

  snapper.snapMacro(); // expected to change inst1->getOrigin

  logger->report(
    "end (x, y) of inst1: ({}, {})",
    inst1->getOrigin().x(),
    inst1->getOrigin().y()
  );

  /*
  snapper.setMacro(inst2);
  
  inst2->setOrigin(0, 0);

  logger->report(
    "initial (x, y) of inst2: ({}, {})",
    inst2->getOrigin().x(),
    inst2->getOrigin().y()
  );

  snapper.snapMacro(); // expected to change inst2->getOrigin

  logger->report(
    "end (x, y) of inst1: ({}, {})",
    inst1->getOrigin().x(),
    inst1->getOrigin().y()
  );

  snapper.setMacro(inst3);
  
  inst3->setOrigin(250, 0);

  logger->report(
    "initial (x, y) of inst3: ({}, {})",
    inst3->getOrigin().x(),
    inst3->getOrigin().y()
  );

  snapper.snapMacro(); // expected to change inst3->getOrigin

  logger->report(
    "end (x, y) of inst3: ({}, {})",
    inst3->getOrigin().x(),
    inst3->getOrigin().y()
  );
  */
}

}  // namespace mpl2