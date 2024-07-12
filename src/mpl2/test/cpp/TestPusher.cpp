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
    db = OdbUniquePtr<odb::dbDatabase>(odb::dbDatabase::create(),
                                        &odb::dbDatabase::destroy);
    chip = OdbUniquePtr<odb::dbChip>(odb::dbChip::create(db.get()),
                                      &odb::dbChip::destroy);
    block
        = OdbUniquePtr<odb::dbBlock>(chip->getBlock(), &odb::dbBlock::destroy);
  }

  utl::Logger logger;
  OdbUniquePtr<odb::dbDatabase> db{nullptr, &odb::dbDatabase::destroy};
  OdbUniquePtr<odb::dbLib> lib_{nullptr, &odb::dbLib::destroy};
  OdbUniquePtr<odb::dbChip> chip{nullptr, &odb::dbChip::destroy};
  OdbUniquePtr<odb::dbBlock> block{nullptr, &odb::dbBlock::destroy};
};

TEST_F(Mpl2PusherTest, ConstructedCentralizedMacros)
{
  // Test whether a Cluster of type StdCellCluster can be created
  // and then used to construct a Pusher object. Run pushMacrosToCoreBoundaries
  // and check that the XDBU, YDBU values remain the same because Pusher detected
  // it as a centralized macros array (no SoftMacro area).

  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db = createSimpleDB();
  db->setLogger(logger);

  odb::dbMaster* master = createSimpleMaster(db->findLib("lib"),
                                              "simple_master",
                                              1000,
                                              1000,
                                              odb::dbMasterType::CORE,
                                              db->getTech()->findLayer("L1"));

  createMPinWithMTerm(
                      master, 
                      "in", 
                      odb::dbIoType::INPUT, 
                      odb::dbSigType::SIGNAL, 
                      db->getTech()->findLayer("L1"), 
                      odb::Rect(0, 0, 50, 50));  
  master->setFrozen();

  odb::dbBlock* block = odb::dbBlock::create(db->getChip(), "simple_block");
  block->setDieArea(odb::Rect(0, 0, 1000, 1000));

  Cluster* cluster = new Cluster(0, std::string("std_cell_cluster"), logger);
  cluster->setClusterType(StdCellCluster);

  odb::dbInst* inst1 = odb::dbInst::create(block, master, "std_cell_1");
  odb::dbInst* inst2 = odb::dbInst::create(block, master, "std_cell_2");

  cluster->addLeafStdCell(inst1);
  cluster->addLeafStdCell(inst2);

  std::vector<HardMacro*> hard_macros;
  HardMacro* macro1 = new HardMacro(inst1, 1, 1);
  HardMacro* macro2 = new HardMacro(inst2, 1, 1);
  
  macro1->setXDBU(1000);
  macro1->setYDBU(1000);
  macro2->setXDBU(5000);
  macro2->setYDBU(5000);
  
  hard_macros.push_back(macro1);
  hard_macros.push_back(macro2);
  cluster->specifyHardMacros(hard_macros);

  Metrics* metrics = new Metrics(0, 0, 0.0, 0.0);
  metrics->addMetrics(
      Metrics(1,
              0,
              block->dbuToMicrons(inst1->getBBox()->getBox().dx())
                  * block->dbuToMicrons(inst1->getBBox()->getBox().dy()),
              0.0));
  metrics->addMetrics(
      Metrics(1,
              0,
              block->dbuToMicrons(inst2->getBBox()->getBox().dx())
                  * block->dbuToMicrons(inst2->getBBox()->getBox().dy()),
              0.0));
  cluster->setMetrics(Metrics(metrics->getNumStdCell(),
                              metrics->getNumMacro(),
                              metrics->getStdCellArea(),
                              metrics->getMacroArea()));

  std::map<Boundary, Rect> boundary_to_io_blockage;
  Pusher pusher(logger, cluster, block, boundary_to_io_blockage);

  pusher.pushMacrosToCoreBoundaries();

  // criteria for bool designHasSingleCentralizedMacroArray
  for (Cluster* child : cluster->getChildren()) {
    EXPECT_TRUE(child->getSoftMacro()->getArea() == 0);
  }

  EXPECT_TRUE(macro1->getXDBU() == 1000);
  EXPECT_TRUE(macro1->getYDBU() == 1000);
  EXPECT_TRUE(macro2->getXDBU() == 5000);
  EXPECT_TRUE(macro2->getYDBU() == 5000);

}  // ConstructedCentralizedMacros

TEST_F(Mpl2PusherTest, PushSimpleCluster)
{

  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db = createSimpleDB();
  db->setLogger(logger);

  odb::dbMaster* master = createSimpleMaster(db->findLib("lib"),
                                              "simple_master",
                                              1000,
                                              1000,
                                              odb::dbMasterType::CORE,
                                              db->getTech()->findLayer("L1"));

  createMPinWithMTerm(
                      master, 
                      "in", 
                      odb::dbIoType::INPUT, 
                      odb::dbSigType::SIGNAL, 
                      db->getTech()->findLayer("L1"), 
                      odb::Rect(0, 0, 50, 50));  
  master->setFrozen();

  odb::dbBlock* block = odb::dbBlock::create(db->getChip(), "simple_block");
  block->setDieArea(odb::Rect(0, 0, 1000, 1000));
  
  odb::dbInst* inst1 = odb::dbInst::create(block, master, "hard_macro1");
  odb::dbInst* inst2 = odb::dbInst::create(block, master, "hard_macro2");
  HardMacro* macro1 = new HardMacro(inst1, 1, 1);
  HardMacro* macro2 = new HardMacro(inst2, 1, 1);
  macro1->setXDBU(1000);
  macro1->setYDBU(1000);
  macro2->setXDBU(5000);
  macro2->setYDBU(5000);

  std::vector<HardMacro*> macro_vector1;
  std::vector<HardMacro*> macro_vector2;
  macro_vector1.push_back(macro1);
  macro_vector2.push_back(macro2);


  Cluster* child_cluster1 = new Cluster(1, std::string("hard_macro_cluster_1"), logger);
  child_cluster1->setClusterType(HardMacroCluster);
  child_cluster1->addLeafMacro(inst1);
  child_cluster1->specifyHardMacros(macro_vector1);
  child_cluster1->setMetrics(Metrics(1,
                                0,
                                block->dbuToMicrons(inst1->getBBox()->getBox().dx())
                                    * block->dbuToMicrons(inst1->getBBox()->getBox().dy()),
                                0.0));
  
  Cluster* child_cluster2 = new Cluster(2, std::string("hard_macro_cluster_2"), logger);
  child_cluster2->setClusterType(HardMacroCluster);
  child_cluster2->addLeafMacro(inst2);
  child_cluster2->specifyHardMacros(macro_vector2);
  child_cluster2->setMetrics(Metrics(1,
                                0,
                                block->dbuToMicrons(inst2->getBBox()->getBox().dx())
                                    * block->dbuToMicrons(inst2->getBBox()->getBox().dy()),
                                0.0));

  SoftMacro* soft_macro1 = new SoftMacro(child_cluster1);
  child_cluster1->setSoftMacro(soft_macro1);
  child_cluster1->setX(0.1);
  child_cluster1->setY(0.2);

  SoftMacro* soft_macro2 = new SoftMacro(child_cluster2);
  child_cluster2->setSoftMacro(soft_macro2);
  child_cluster2->setX(0.5);
  child_cluster2->setY(0.6);
  
  Cluster* root_cluster = new Cluster(0, std::string("root_cluster"), logger); 
  root_cluster->setClusterType(StdCellCluster);
  root_cluster->addChild(child_cluster1);
  root_cluster->addChild(child_cluster2);

  // Construct Pusher object, indirectly run Pusher::SetIOBlockages
  std::map<Boundary, Rect> boundary_to_io_blockage;
  Pusher pusher(logger, root_cluster, block, boundary_to_io_blockage);

  logger->report("macro1 initial: {} {}", macro1->getXDBU(), macro2->getYDBU());
  logger->report("macro2 initial: {} {}", macro1->getXDBU(), macro2->getYDBU());
  pusher.pushMacrosToCoreBoundaries();
  
  logger->report("macro1 result: {} {}", macro1->getXDBU(), macro2->getYDBU());
  logger->report("macro2 result: {} {}", macro1->getXDBU(), macro2->getYDBU());

  for (Cluster* child : root_cluster->getChildren()) {
    logger->report("{}", child->getSoftMacro()->getArea());
  }

}  // PushSimpleCluster

}  // namespace mpl2
