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

// The ConstructPusher tests whether a Pusher object can be constructed
// without halting, and indirectly tests private function Pusher::SetIOBlockages. 
// 
// There are several cases based on the cluster type (see mpl2/object.h):
// 1. HardMacroCluster (ConstructPusherHardMacro)
//     -> Cluster only has leaf_macros_
// 2. StdCellCluster   (ConstructPusherStdCell)
//     -> Cluster only has leaf_std_cells_ and dbModules_
// 3. MixedCluster     (ConstructPusherMixed)     
//     -> Cluster has both std cells and hard macros

TEST_F(Mpl2PusherTest, ConstructPusherHardMacro)
{
  // Test whether a Cluster of type HardMacroCluster can be created
  // and then used to construct a Pusher object, and then whether the 
  // boundary_to_io_blockage_ created during construction has the expected
  // value (empty).

  utl::Logger* logger_ = new utl::Logger();
  odb::dbDatabase* db_ = createSimpleDB();
  db_->setLogger(logger_);

  odb::dbMaster* master_ = createSimpleMaster(
      db_->findLib("lib"), "simple_master", 1000, 1000, odb::dbMasterType::CORE);

  odb::dbBlock* block_ = odb::dbBlock::create(db_->getChip(), "simple_block");
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));

  Cluster* cluster_ = new Cluster(0, std::string("hard_macro_cluster"), logger_);
  cluster_->setClusterType(HardMacroCluster);

  odb::dbInst* inst1 = odb::dbInst::create(block_, master_, "leaf_macro");
  cluster_->addLeafMacro(inst1); 

  std::map<Boundary, Rect> boundary_to_io_blockage_;
  Pusher pusher(logger_, cluster_, block_, boundary_to_io_blockage_);
  
  // Due to getX, getY, getWidth, and getHeight (see mpl2/src/object.h),
  // it is expected that boundary_to_io_blockage_ here will be empty.
  EXPECT_TRUE(boundary_to_io_blockage_.size() == 0);
  
}  // ConstructPusherHardMacro

TEST_F(Mpl2PusherTest, ConstructPusherStdCell)
{
  // Test whether a Cluster of type StdCellCluster can be created
  // and then used to construct a Pusher object, and then whether the 
  // boundary_to_io_blockage_ created during construction has the expected
  // values.

  utl::Logger* logger_ = new utl::Logger();
  odb::dbDatabase* db_ = createSimpleDB();
  db_->setLogger(logger_);

  odb::dbMaster* master_ = createSimpleMaster(
      db_->findLib("lib"), "simple_master", 1000, 1000, odb::dbMasterType::CORE);

  odb::dbBlock* block_ = odb::dbBlock::create(db_->getChip(), "simple_block");
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbInst::create(block_, master_, "leaf_std_cell1");
  odb::dbInst::create(block_, master_, "leaf_std_cell2");
  odb::dbInst::create(block_, master_, "leaf_std_cell3");

  Cluster* cluster_ = new Cluster(0, std::string("stdcell_cluster"), logger_);
  cluster_->setClusterType(StdCellCluster);
  cluster_->addDbModule(block_->getTopModule());

  // add declared instances as leaf std cells to cluster
  // and compute metrics alongside
  
  Metrics* metrics_ = new Metrics(0, 0, 0.0, 0.0);
  for (auto inst : block_->getInsts()) {

    const float inst_width = block_->dbuToMicrons(
        inst->getBBox()->getBox().dx());
    const float inst_height = block_->dbuToMicrons(
        inst->getBBox()->getBox().dy());
    
    cluster_->addLeafStdCell(inst);
    metrics_->addMetrics(Metrics(1, 0, inst_width * inst_height, 0.0));

  }

  cluster_->setMetrics(Metrics(
      metrics_->getNumStdCell(),
      metrics_->getNumMacro(),
      metrics_->getStdCellArea(), 
      metrics_->getMacroArea()
  ));

  cluster_->printBasicInformation(logger_);
  std::map<Boundary, Rect> boundary_to_io_blockage_;
  Pusher pusher(logger_, cluster_, block_, boundary_to_io_blockage_);

  logger_->report("area: {}", cluster_->getArea());

}  // ConstructPusherStdCell

}  // namespace mpl2
