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

TEST_F(Mpl2PusherTest, ConstructPusherHardMacro)
{
  // Test whether a Cluster of type HardMacroCluster can be created
  // and then used to construct a Pusher object. Run pushMacrosToCoreBoundaries
  // and check that the XDBU, YDBU values remain the same.

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

  // Create cluster of type HardMacroCluster, and add 2 leaf macros
  Cluster* cluster = new Cluster(0, std::string("hard_macro_cluster"), logger);
  cluster->setClusterType(HardMacroCluster);
  
  odb::dbInst* inst1 = odb::dbInst::create(block, master, "leaf_macro1");
  odb::dbInst* inst2 = odb::dbInst::create(block, master, "leaf_macro2");
  HardMacro* macro1 = new HardMacro(inst1, 1, 1);
  HardMacro* macro2 = new HardMacro(inst2, 1, 1);
  cluster->addLeafMacro(inst1);
  cluster->addLeafMacro(inst2);

  macro1->setXDBU(1000);
  macro1->setYDBU(1000);
  macro2->setXDBU(5000);
  macro2->setYDBU(5000);
  
  // Construct Pusher object, indirectly run Pusher::SetIOBlockages
  std::map<Boundary, Rect> boundary_to_io_blockage;
  Pusher pusher(logger, cluster, block, boundary_to_io_blockage);

  // Should do nothing due to HardMacroCluster type
  pusher.pushMacrosToCoreBoundaries();

  EXPECT_TRUE(macro1->getXDBU() == 1000);
  EXPECT_TRUE(macro1->getYDBU() == 1000);
  EXPECT_TRUE(macro2->getXDBU() == 5000);
  EXPECT_TRUE(macro2->getYDBU() == 5000);

}  // ConstructPusherHardMacro

}  // namespace mpl2
