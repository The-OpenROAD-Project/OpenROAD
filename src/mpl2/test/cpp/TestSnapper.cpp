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
}

}  // namespace mpl2
