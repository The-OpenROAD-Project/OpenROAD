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
  odb::dbDatabase* db_ = createSimpleDB();
  db_->setLogger(logger);

  // Create setup for case 1 (HardMacroCluster):
  // Simple master with 1 input, 1 output, 1 simple block, 1 instance
  odb::dbMaster* master_ = createSimpleMaster(
                                        db_->findLib("lib"), 
                                        "simple_master", 
                                        1000, 
                                        1000, 
                                        odb::dbMasterType::CORE);

  odb::dbBlock* block_ = odb::dbBlock::create(
                                        db_->getChip(), 
                                        "simple_block");
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbInst* inst1 = odb::dbInst::create(block_, master_, "cells_1");

  // Case 1:
  // cluster1 is HardMacroCluster
  Cluster* cluster1 = new Cluster(0, std::string("hard_macro_cluster"), logger);
  cluster1->addDbModule(block_->getTopModule());
  cluster1->addLeafMacro(inst1);
  cluster1->setClusterType(HardMacroCluster); 

  std::map<Boundary, Rect> boundary_to_io_blockage_;
  Pusher pusher(logger, cluster1, block_, boundary_to_io_blockage_);
  
  // Due to getX, getY, getWidth, and getHeight (see mpl2/src/object.h),
  // it is expected that boundary_to_io_blockage_ here will be empty.
  EXPECT_TRUE(boundary_to_io_blockage_.size() == 0);
}

}  // namespace mpl2
