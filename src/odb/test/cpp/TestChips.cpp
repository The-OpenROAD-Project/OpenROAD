#include <exception>
#include <set>
#include <vector>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"

namespace odb {
namespace {

struct ChipHierarchyFixture : public SimpleDbFixture
{
  ChipHierarchyFixture()
  {
    createLibsAndTechs();
    createChips();
    createChipRegions();
    createChipInsts();
  }
  void createLibsAndTechs()
  {
    dbDatabase* db = db_.get();
    tech1 = dbTech::create(db, "tech1");
    layer_l1 = dbTechLayer::create(tech1, "L1", dbTechLayerType::MASTERSLICE);
    lib1 = dbLib::create(db, "lib1", tech1, ',');
    tech2 = dbTech::create(db, "tech2");
    layer_M1 = dbTechLayer::create(tech2, "M1", dbTechLayerType::MASTERSLICE);
    lib2 = dbLib::create(db, "lib2", tech2, ',');
  }
  void createChips()
  {
    dbDatabase* db = db_.get();
    system_chip
        = dbChip::create(db, nullptr, "system_chip", dbChip::ChipType::HIER);
    system_chip->setWidth(5000);
    system_chip->setHeight(4000);
    system_chip->setThickness(500);
    system_chip->setOffset(Point(10, 0));
    cpu_chip = dbChip::create(db, nullptr, "cpu_chip", dbChip::ChipType::HIER);
    memory_chip = dbChip::create(db, tech1, "memory_chip");
    io_chip = dbChip::create(db, tech2, "io_chip");
    cache_chip = dbChip::create(db, tech2, "cache_chip");
    // Create blocks
    dbBlock::create(memory_chip, "memory_block");
    dbBlock::create(io_chip, "io_block");
  }
  void createChipRegions()
  {
    memory_chip_region_r1 = dbChipRegion::create(
        memory_chip, "R1", dbChipRegion::Side::FRONT, layer_l1);
    memory_chip_region_r2 = dbChipRegion::create(
        memory_chip, "R2", dbChipRegion::Side::BACK, layer_l1);
    memory_chip_region_r3 = dbChipRegion::create(
        memory_chip, "R3", dbChipRegion::Side::INTERNAL, nullptr);
    io_chip_region_r1 = dbChipRegion::create(
        io_chip, "R1", dbChipRegion::Side::FRONT, layer_M1);
    dbChipRegion::create(cache_chip, "R1", dbChipRegion::Side::BACK, nullptr);
    // create chip bumps
    dbMaster* io_master
        = createMaster1X1(lib2, "io_master", 100, 100, "in", "out");
    dbInst* io_cell = dbInst::create(io_chip->getBlock(), io_master, "io_bump");
    io_bump = dbChipBump::create(io_chip_region_r1, io_cell);
    io_master = createMaster1X1(lib1, "io_master", 100, 100, "in", "out");
    io_cell = dbInst::create(memory_chip->getBlock(), io_master, "io_bump");
    dbChipBump::create(memory_chip_region_r1, io_cell);
  }
  void createChipInsts()
  {
    cpu_inst = dbChipInst::create(system_chip, cpu_chip, "cpu_inst");
    memory_inst = dbChipInst::create(system_chip, memory_chip, "memory_inst");
    io_inst = dbChipInst::create(system_chip, io_chip, "io_inst");

    // Create cache instance in CPU chip (nested hierarchy)
    cache_inst = dbChipInst::create(cpu_chip, cache_chip, "cache_inst");

    // Position components
    memory_inst->setLoc(Point3D(2500, 500, 0));
    cache_inst->setLoc(Point3D(100, 100, 50));
    cache_inst->setOrient(dbOrientType3D("MZ_MY_R90"));
  }

  dbTech* tech1;
  dbTech* tech2;
  dbLib* lib1;
  dbLib* lib2;
  dbTechLayer* layer_l1;
  dbTechLayer* layer_M1;
  dbChip* system_chip;
  dbChip* cpu_chip;
  dbChip* memory_chip;
  dbChip* io_chip;
  dbChip* cache_chip;
  dbChipInst* cpu_inst;
  dbChipInst* memory_inst;
  dbChipInst* io_inst;
  dbChipInst* cache_inst;
  dbChipRegion* memory_chip_region_r1;
  dbChipRegion* memory_chip_region_r2;
  dbChipRegion* memory_chip_region_r3;
  dbChipRegion* io_chip_region_r1;
  dbChipBump* io_bump;
};

TEST_F(ChipHierarchyFixture, test_chip_creation)
{
  EXPECT_EQ(db_->getChips().size(), 5);
  EXPECT_EQ(db_->findChip("system_chip"), system_chip);
  EXPECT_EQ(db_->findChip("cpu_chip"), cpu_chip);
  EXPECT_EQ(db_->findChip("memory_chip"), memory_chip);
  EXPECT_EQ(db_->findChip("io_chip"), io_chip);
  EXPECT_EQ(db_->findChip("cache_chip"), cache_chip);
  EXPECT_EQ(db_->findChip("dummy_chip"), nullptr);
  EXPECT_EQ(db_->getChip(), system_chip);

  // Test dbChip methods
  EXPECT_STREQ(system_chip->getName(), "system_chip");

  EXPECT_EQ(system_chip->getChipType(), dbChip::ChipType::HIER);
  EXPECT_EQ(cache_chip->getChipType(), dbChip::ChipType::DIE);

  EXPECT_EQ(system_chip->getOffset(), Point(10, 0));
  EXPECT_EQ(system_chip->getWidth(), 5000);
  EXPECT_EQ(system_chip->getHeight(), 4000);
  EXPECT_EQ(system_chip->getThickness(), 500);

  // Test dbDatabase methods
  db_->setTopChip(cpu_chip);
  EXPECT_EQ(db_->getChip(), cpu_chip);
  dbChip::destroy(cpu_chip);
  EXPECT_EQ(db_->getChips().size(), 4);
  EXPECT_EQ(db_->findChip("cpu_chip"), nullptr);
  EXPECT_EQ(db_->getChip(), nullptr);
  EXPECT_THROW(dbChipInst::create(system_chip, cpu_chip, "cpu_inst"),
               std::exception);
}

TEST_F(ChipHierarchyFixture, test_chip_hierarchy)
{
  // Test system level chip instances
  dbSet<dbChipInst> system_chipinsts = system_chip->getChipInsts();
  EXPECT_EQ(system_chipinsts.size(), 3);

  // Test CPU level chip instances (nested)
  dbSet<dbChipInst> cpu_chipinsts = cpu_chip->getChipInsts();
  EXPECT_EQ(cpu_chipinsts.size(), 1);

  // Verify the cache instance is in the CPU chip
  bool found_cache = false;
  for (auto ci : cpu_chipinsts) {
    if (ci == cache_inst) {
      found_cache = true;
      EXPECT_EQ(ci->getParentChip(), cpu_chip);
      EXPECT_EQ(ci->getMasterChip(), cache_chip);
      break;
    }
  }
  EXPECT_TRUE(found_cache);

  // Test that other chips have no instances
  EXPECT_EQ(memory_chip->getChipInsts().size(), 0);
  EXPECT_EQ(io_chip->getChipInsts().size(), 0);
  EXPECT_EQ(cache_chip->getChipInsts().size(), 0);

  // Verify parent-child relationships
  EXPECT_EQ(cpu_inst->getParentChip(), system_chip);
  EXPECT_EQ(memory_inst->getParentChip(), system_chip);
  EXPECT_EQ(io_inst->getParentChip(), system_chip);
  EXPECT_EQ(cache_inst->getParentChip(), cpu_chip);
  EXPECT_EQ(system_chip->findChipInst("cpu_inst"), cpu_inst);
  EXPECT_EQ(cpu_chip->findChipInst("cache_inst"), cache_inst);
  EXPECT_EQ(system_chip->findChipInst("cache_inst"), nullptr);

  // Verify master relationships
  EXPECT_EQ(cpu_inst->getMasterChip(), cpu_chip);
  EXPECT_EQ(memory_inst->getMasterChip(), memory_chip);
  EXPECT_EQ(io_inst->getMasterChip(), io_chip);
  EXPECT_EQ(cache_inst->getMasterChip(), cache_chip);

  // Test positioning
  Point3D memory_loc = memory_inst->getLoc();
  EXPECT_EQ(memory_loc.x(), 2500);
  EXPECT_EQ(memory_loc.y(), 500);
  EXPECT_EQ(memory_loc.z(), 0);
  EXPECT_EQ(memory_inst->getOrient().getOrientType2D(), dbOrientType::R0);
  EXPECT_FALSE(memory_inst->getOrient().isMirrorZ());
  EXPECT_EQ(memory_inst->getOrient().getString(), "R0");

  Point3D cache_loc = cache_inst->getLoc();
  EXPECT_EQ(cache_loc.x(), 100);
  EXPECT_EQ(cache_loc.y(), 100);
  EXPECT_EQ(cache_loc.z(), 50);
  EXPECT_EQ(cache_inst->getOrient().getOrientType2D(), dbOrientType::MYR90);
  EXPECT_TRUE(cache_inst->getOrient().isMirrorZ());
  EXPECT_EQ(cache_inst->getOrient().getString(), "MZ_MY_R90");
}

TEST_F(ChipHierarchyFixture, test_chip_complex_destroy)
{
  // Test destroying nested hierarchy
  EXPECT_EQ(cpu_chip->getChipInsts().size(), 1);

  // Destroy cache instance first
  dbChipInst::destroy(cache_inst);
  EXPECT_EQ(cpu_chip->getChipInsts().size(), 0);

  // Now destroy CPU instance from system
  EXPECT_EQ(system_chip->getChipInsts().size(), 3);
  dbChipInst::destroy(cpu_inst);
  EXPECT_EQ(system_chip->getChipInsts().size(), 2);

  // Verify remaining system instances
  bool found_memory = false, found_io = false;
  for (auto ci : system_chip->getChipInsts()) {
    if (ci == memory_inst) {
      found_memory = true;
    }
    if (ci == io_inst) {
      found_io = true;
    }
  }
  EXPECT_TRUE(found_memory);
  EXPECT_TRUE(found_io);

  // Clean up remaining instances
  dbChipInst::destroy(memory_inst);
  dbChipInst::destroy(io_inst);
  EXPECT_EQ(db_->getChipBumpInsts().size(), 0);
  EXPECT_EQ(db_->getChipRegionInsts().size(), 0);
  EXPECT_EQ(system_chip->getChipInsts().size(), 0);
  EXPECT_EQ(system_chip->findChipInst("cpu_inst"), nullptr);
  EXPECT_EQ(system_chip->findChipInst("io_inst"), nullptr);
}

TEST_F(ChipHierarchyFixture, test_chip_regions)
{
  auto iterateChipRegions
      = [](dbChip* chip, std::set<std::string> region_names) {
          EXPECT_EQ(chip->getChipRegions().size(), region_names.size());
          for (auto region : chip->getChipRegions()) {
            EXPECT_NE(region_names.find(region->getName()), region_names.end());
          }
        };
  auto iterateChipRegionInsts
      = [](dbChipInst* chipinst, std::set<std::string> region_names) {
          EXPECT_EQ(chipinst->getRegions().size(), region_names.size());
          for (auto region : chipinst->getRegions()) {
            EXPECT_NE(region_names.find(region->getChipRegion()->getName()),
                      region_names.end());
          }
        };
  // Test dbChipRegion methods
  EXPECT_EQ(memory_chip_region_r1->getName(), "R1");
  EXPECT_EQ(memory_chip_region_r1->getSide(), dbChipRegion::Side::FRONT);
  EXPECT_EQ(memory_chip_region_r1->getLayer(), layer_l1);
  EXPECT_EQ(memory_chip_region_r1->getChip(), memory_chip);
  EXPECT_EQ(memory_chip_region_r3->getSide(), dbChipRegion::Side::INTERNAL);
  EXPECT_EQ(memory_chip_region_r3->getLayer(), nullptr);
  // Test dbChip::findChipRegion
  EXPECT_EQ(memory_chip->findChipRegion("R1"), memory_chip_region_r1);
  EXPECT_EQ(memory_chip->findChipRegion("R2"), memory_chip_region_r2);
  EXPECT_EQ(memory_chip->findChipRegion("R3"), memory_chip_region_r3);
  EXPECT_EQ(memory_chip->findChipRegion("R4"), nullptr);
  // Test dbChipInst::findChipRegionInst
  EXPECT_EQ(memory_inst->findChipRegionInst("R1")->getChipRegion(),
            memory_chip_region_r1);
  EXPECT_EQ(memory_inst->findChipRegionInst("R2")->getChipRegion(),
            memory_chip_region_r2);
  EXPECT_EQ(memory_inst->findChipRegionInst("R3")->getChipRegion(),
            memory_chip_region_r3);
  EXPECT_EQ(memory_inst->findChipRegionInst("R4"), nullptr);

  iterateChipRegions(memory_chip, {"R1", "R2", "R3"});
  iterateChipRegionInsts(memory_inst, {"R1", "R2", "R3"});
  iterateChipRegions(io_chip, {"R1"});

  EXPECT_THROW(
      dbChipRegion::create(cpu_chip, "R1", dbChipRegion::Side::FRONT, layer_l1),
      std::exception);

  EXPECT_THROW(
      dbChipRegion::create(
          memory_chip, "region_M1", dbChipRegion::Side::FRONT, layer_M1),
      std::exception);
}

TEST_F(ChipHierarchyFixture, test_chip_conns)
{
  dbChipRegionInst* cache_region_inst_r1
      = (dbChipRegionInst*) (*cache_inst->getRegions().begin());
  dbChipRegionInst* memory_region_inst_r1 = nullptr;
  for (auto region : memory_inst->getRegions()) {
    if (region->getChipRegion() == memory_chip_region_r1) {
      memory_region_inst_r1 = region;
      break;
    }
  }
  ASSERT_NE(memory_region_inst_r1, nullptr);
  odb::dbChipConn* conn = odb::dbChipConn::create("CONN1",
                                                  system_chip,
                                                  {cpu_inst, cache_inst},
                                                  cache_region_inst_r1,
                                                  {memory_inst},
                                                  memory_region_inst_r1);
  EXPECT_EQ(conn->getName(), "CONN1");
  EXPECT_EQ(conn->getTopRegion(), cache_region_inst_r1);
  EXPECT_EQ(conn->getBottomRegion(), memory_region_inst_r1);
  EXPECT_EQ(conn->getTopRegionPath().size(), 2);
  EXPECT_EQ(conn->getBottomRegionPath().size(), 1);
  EXPECT_EQ(conn->getTopRegionPath()[0], cpu_inst);
  EXPECT_EQ(conn->getTopRegionPath()[1], cache_inst);
  EXPECT_EQ(conn->getBottomRegionPath()[0], memory_inst);
  EXPECT_EQ(db_->getChipConns().size(), 1);
  EXPECT_EQ(system_chip->getChipConns().size(), 1);
  dbChipConn::destroy(conn);
  EXPECT_EQ(db_->getChipConns().size(), 0);
  EXPECT_EQ(system_chip->getChipConns().size(), 0);
  EXPECT_THROW(odb::dbChipConn::create("CONN2",
                                       system_chip,
                                       {cache_inst},
                                       cache_region_inst_r1,
                                       {memory_inst},
                                       memory_region_inst_r1),
               std::exception);

  EXPECT_THROW(odb::dbChipConn::create("CONN2",
                                       system_chip,
                                       {cache_inst},
                                       memory_region_inst_r1,
                                       {memory_inst},
                                       cache_region_inst_r1),
               std::exception);
}

TEST_F(ChipHierarchyFixture, test_chip_bumps)
{
  EXPECT_EQ(io_bump->getInst()->getName(), "io_bump");
  EXPECT_EQ(io_bump->getChip(), io_chip);
  EXPECT_EQ(io_bump->getChipRegion(), io_chip_region_r1);
  EXPECT_EQ(io_bump->getNet(), nullptr);
  EXPECT_EQ(io_bump->getBTerm(), nullptr);
  dbNet* net = dbNet::create(io_chip->getBlock(), "net1");
  dbBTerm* bterm = dbBTerm::create(net, "bterm1");
  io_bump->setNet(net);
  io_bump->setBTerm(bterm);
  EXPECT_EQ(io_bump->getNet(), net);
  EXPECT_EQ(io_bump->getBTerm(), bterm);

  EXPECT_EQ(io_chip_region_r1->getChipBumps().size(), 1);
  EXPECT_EQ(*io_chip_region_r1->getChipBumps().begin(), io_bump);

  EXPECT_EQ(db_->getChipBumpInsts().size(), 2);

  EXPECT_EQ(io_inst->getRegions().size(), 1);
  auto io_inst_region_r1 = *io_inst->getRegions().begin();
  EXPECT_EQ(io_inst_region_r1->getChipBumpInsts().size(), 1);
  auto io_inst_region_r1_bump_inst
      = *io_inst_region_r1->getChipBumpInsts().begin();
  EXPECT_EQ(io_inst_region_r1_bump_inst->getChipBump(), io_bump);
  EXPECT_EQ(io_inst_region_r1_bump_inst->getChipRegionInst(),
            io_inst_region_r1);

  // test chip nets
  dbChipNet* chip_net = dbChipNet::create(system_chip, "net1");
  EXPECT_EQ(chip_net->getChip(), system_chip);
  EXPECT_EQ(chip_net->getNumBumpInsts(), 0);
  EXPECT_EQ(system_chip->getChipNets().size(), 1);
  EXPECT_EQ(db_->getChipNets().size(), 1);
  chip_net->addBumpInst(io_inst_region_r1_bump_inst, {io_inst});
  dbChipRegionInst* memory_inst_region_r1 = nullptr;
  for (auto region : memory_inst->getRegions()) {
    if (region->getChipRegion() == memory_chip_region_r1) {
      memory_inst_region_r1 = region;
      break;
    }
  }
  EXPECT_EQ(memory_inst_region_r1->getChipBumpInsts().size(), 1);
  auto memory_inst_region_r1_bump_inst
      = (*memory_inst_region_r1->getChipBumpInsts().begin());
  chip_net->addBumpInst(memory_inst_region_r1_bump_inst, {memory_inst});
  EXPECT_EQ(chip_net->getNumBumpInsts(), 2);
  std::vector<dbChipInst*> path;
  EXPECT_EQ(chip_net->getBumpInst(0, path), io_inst_region_r1_bump_inst);
  EXPECT_EQ(path.size(), 1);
  EXPECT_EQ(path[0], io_inst);
  dbInst* io_cell = memory_chip->getBlock()->findInst("io_bump");
  EXPECT_THROW(dbChipBump::create(io_chip_region_r1, io_cell), std::exception);
}

}  // namespace
}  // namespace odb
