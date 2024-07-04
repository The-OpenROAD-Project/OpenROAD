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
  
  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db_ = odb::dbDatabase::create();
  db_->setLogger(logger);
  
  odb::dbTech* tech_ = odb::dbTech::create(db_, "tech");
  tech_->setManufacturingGrid(2);

  odb::dbLib* lib_ = odb::dbLib::create(db_, "lib", tech_, ',');
  odb::dbChip* chip_ = odb::dbChip::create(db_);

  odb::dbTechLayer* layer1_ = odb::dbTechLayer::create(tech_, "layer1", odb::dbTechLayerType::CUT);
  odb::dbTechLayer* layer2_ = odb::dbTechLayer::create(tech_, "layer2", odb::dbTechLayerType::CUT);

  // During the construction of a new HardMacro object
  // with input dbInst* inst, the following values are retrieved:
  // inst->getBlock, inst->getName, inst->getMaster,
  // master->getWidth, master->getHeight, master->getMTerms,
  // mterm->getSigType, mterm->getMPins,
  // mpin->getGeometry, box->getBox (see: object.cpp)

  odb::dbMaster* master_ = odb::dbMaster::create(lib_, "simple_master");
  master_->setWidth(1000);
  master_->setHeight(1000);
  master_->setType(odb::dbMasterType::BLOCK); // snapper expects a block type
  
  // component with 1 input, 1 output -> 2 terminals
  odb::dbMTerm* mterm_i = odb::dbMTerm::create(master_, "in", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin_i = odb::dbMPin::create(mterm_i);
  odb::dbBox* box_i = odb::dbBox::create(mpin_i, layer1_, 0, 0, 500, 500);

  odb::dbMTerm* mterm_o = odb::dbMTerm::create(master_, "out", odb::dbIoType::OUTPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin_o = odb::dbMPin::create(mterm_o);
  odb::dbBox* box_o = odb::dbBox::create(mpin_o, layer2_, 0, 0, 500, 500);

  master_->setFrozen();

  odb::dbBlock* block_ = odb::dbBlock::create(chip_, "simple_block");
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
  odb::dbGCellGrid* grid_ = odb::dbGCellGrid::create(block_);

  odb::dbTrackGrid* track1 = odb::dbTrackGrid::create(block_, layer1_);
  track1->addGridPatternX(0, 20, 10);
  track1->addGridPatternY(0, 20, 10);

  odb::dbTrackGrid* track2 = odb::dbTrackGrid::create(block_, layer2_);
  track2->addGridPatternX(0, 20, 10);
  track2->addGridPatternY(0, 20, 10);

  odb::dbDatabase::beginEco(block_);
  odb::dbInst* inst1 = odb::dbInst::create(block_, master_, "cells_1");
  odb::dbInst* inst2 = odb::dbInst::create(block_, master_, "cells_2");
  odb::dbInst* inst3 = odb::dbInst::create(block_, master_, "cells_3");
  odb::dbDatabase::endEco(block_);

  Snapper snapper;
  snapper.setMacro(inst1);
  
  inst1->setOrigin(-250, 0);

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
}

}  // namespace mpl2