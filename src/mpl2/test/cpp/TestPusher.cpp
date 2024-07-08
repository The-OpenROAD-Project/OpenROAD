#include "../../src/hier_rtlmp.h"
#include "gtest/gtest.h"
#include "helper.h"
#include "mpl2/rtl_mp.h"
#include "odb/db.h"
#include "odb/util.h"
#include "utl/Logger.h"

namespace mpl2 {
class Mpl2PusherTest : public ::testing::Test
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

TEST_F(Mpl2PusherTest, CanConstructPusher)
{
  // This tests whether a Pusher object can be constructed
  // without halting, and indirectly tests private function
  // Pusher::SetIOBlockages.
  // 
  // There are several cases based on the cluster type:
  // 1. HardMacroCluster (cluster1)
  // 2. StdCellCluster (cluster2)
  // 3. MixedCluster (cluster3)

  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db_ = odb::createSimpleDB();
  db_->setLogger(logger);

  odb::dbTech* tech_ = db_->getTech();
  odb::dbLib* lib_ = db_->findLib("lib");
  odb::dbChip* chip_ = db_->getChip();
  odb::dbTechLayer* layer_ = tech_->findLayer("L1");
  
  odb::dbBlock* block_ = odb::dbBlock::create(chip_, "simple_block");
  odb::dbMaster* master_ = odb::dbMaster::create(lib_, "simple_master");
  master_->setWidth(1000);
  master_->setHeight(1000);
  master_->setType(odb::dbMasterType::CORE);
  odb::dbMTerm::create(
      master_, "in", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMTerm::create(
      master_, "out", odb::dbIoType::OUTPUT, odb::dbSigType::SIGNAL);
  master_->setFrozen();

  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbDatabase::beginEco(block_);
  odb::dbInst* inst1 = odb::dbInst::create(block_, master_, "cells_1");
  odb::dbDatabase::endEco(block_);

  // Cluster class is defined in mpl2/src/object.h

  // cluster1 is HardMacroCluster
  Cluster* cluster1 = new Cluster(0, std::string("hard_macro_cluster"), logger);
  cluster1->addDbModule(block_->getTopModule());
  cluster1->addLeafMacro(inst1);
  cluster1->setClusterType(HardMacroCluster); // width, height, coordinates will all be 0

  logger->report("getNumMacro is {}", cluster1->getNumMacro());
  cluster1->printBasicInformation(logger);

  std::map<Boundary, Rect> boundary_to_io_blockage_;

  Pusher pusher(logger, cluster1, block_, boundary_to_io_blockage_);

  logger->report("size of boundary_to_io_blockage_ is {}", boundary_to_io_blockage_.size());
}

}  // namespace mpl2
