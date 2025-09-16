#define BOOST_TEST_MODULE TestChips
#include <exception>
#include <set>
#include <vector>

#include "boost/test/included/unit_test.hpp"
#include "helper.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"

namespace odb {
namespace {

BOOST_AUTO_TEST_SUITE(test_suite)
struct F_CHIP_HIERARCHY
{
  F_CHIP_HIERARCHY()
  {
    utl::Logger* logger = new utl::Logger();
    db = dbDatabase::create();
    db->setLogger(logger);
    createLibsAndTechs();
    createChips();
    createChipRegions();
    createChipInsts();
  }
  void createLibsAndTechs()
  {
    tech1 = dbTech::create(db, "tech1");
    layer_l1 = dbTechLayer::create(tech1, "L1", dbTechLayerType::MASTERSLICE);
    lib1 = dbLib::create(db, "lib1", tech1, ',');
    tech2 = dbTech::create(db, "tech2");
    layer_M1 = dbTechLayer::create(tech2, "M1", dbTechLayerType::MASTERSLICE);
    lib2 = dbLib::create(db, "lib2", tech2, ',');
  }
  void createChips()
  {
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

  ~F_CHIP_HIERARCHY() { dbDatabase::destroy(db); }

  dbDatabase* db;
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

BOOST_FIXTURE_TEST_CASE(test_chip_creation, F_CHIP_HIERARCHY)
{
  BOOST_TEST(db->getChips().size() == 5);
  BOOST_TEST(db->findChip("system_chip") == system_chip);
  BOOST_TEST(db->findChip("cpu_chip") == cpu_chip);
  BOOST_TEST(db->findChip("memory_chip") == memory_chip);
  BOOST_TEST(db->findChip("io_chip") == io_chip);
  BOOST_TEST(db->findChip("cache_chip") == cache_chip);
  BOOST_TEST(db->findChip("dummy_chip") == nullptr);
  BOOST_TEST(db->getChip() == system_chip);

  // Test dbChip methods
  BOOST_TEST(system_chip->getName() == "system_chip");

  BOOST_TEST((system_chip->getChipType() == dbChip::ChipType::HIER));
  BOOST_TEST((cache_chip->getChipType() == dbChip::ChipType::DIE));

  BOOST_TEST(system_chip->getOffset() == Point(10, 0));
  BOOST_TEST(system_chip->getWidth() == 5000);
  BOOST_TEST(system_chip->getHeight() == 4000);
  BOOST_TEST(system_chip->getThickness() == 500);

  // Test dbDatabase methods
  db->setTopChip(cpu_chip);
  BOOST_TEST(db->getChip() == cpu_chip);
  dbChip::destroy(cpu_chip);
  BOOST_TEST(db->getChips().size() == 4);
  BOOST_TEST(db->findChip("cpu_chip") == nullptr);
  BOOST_TEST(db->getChip() == nullptr);
  try {
    dbChipInst::create(system_chip, cpu_chip, "cpu_inst");
    BOOST_TEST(false);
  } catch (const std::exception& e) {
    BOOST_TEST(true);
  }
}

BOOST_FIXTURE_TEST_CASE(test_chip_hierarchy, F_CHIP_HIERARCHY)
{
  // Test system level chip instances
  dbSet<dbChipInst> system_chipinsts = system_chip->getChipInsts();
  BOOST_TEST(system_chipinsts.size() == 3);

  // Test CPU level chip instances (nested)
  dbSet<dbChipInst> cpu_chipinsts = cpu_chip->getChipInsts();
  BOOST_TEST(cpu_chipinsts.size() == 1);

  // Verify the cache instance is in the CPU chip
  bool found_cache = false;
  for (auto ci : cpu_chipinsts) {
    if (ci == cache_inst) {
      found_cache = true;
      BOOST_TEST(ci->getParentChip() == cpu_chip);
      BOOST_TEST(ci->getMasterChip() == cache_chip);
      break;
    }
  }
  BOOST_TEST(found_cache == true);

  // Test that other chips have no instances
  BOOST_TEST(memory_chip->getChipInsts().size() == 0);
  BOOST_TEST(io_chip->getChipInsts().size() == 0);
  BOOST_TEST(cache_chip->getChipInsts().size() == 0);

  // Verify parent-child relationships
  BOOST_TEST(cpu_inst->getParentChip() == system_chip);
  BOOST_TEST(memory_inst->getParentChip() == system_chip);
  BOOST_TEST(io_inst->getParentChip() == system_chip);
  BOOST_TEST(cache_inst->getParentChip() == cpu_chip);
  BOOST_TEST(system_chip->findChipInst("cpu_inst") == cpu_inst);
  BOOST_TEST(cpu_chip->findChipInst("cache_inst") == cache_inst);
  BOOST_TEST(system_chip->findChipInst("cache_inst") == nullptr);

  // Verify master relationships
  BOOST_TEST(cpu_inst->getMasterChip() == cpu_chip);
  BOOST_TEST(memory_inst->getMasterChip() == memory_chip);
  BOOST_TEST(io_inst->getMasterChip() == io_chip);
  BOOST_TEST(cache_inst->getMasterChip() == cache_chip);

  // Test positioning
  Point3D memory_loc = memory_inst->getLoc();
  BOOST_TEST(memory_loc.x() == 2500);
  BOOST_TEST(memory_loc.y() == 500);
  BOOST_TEST(memory_loc.z() == 0);
  BOOST_TEST(memory_inst->getOrient().getOrientType2D() == dbOrientType::R0);
  BOOST_TEST(memory_inst->getOrient().isMirrorZ() == false);
  BOOST_TEST(memory_inst->getOrient().getString() == "R0");

  Point3D cache_loc = cache_inst->getLoc();
  BOOST_TEST(cache_loc.x() == 100);
  BOOST_TEST(cache_loc.y() == 100);
  BOOST_TEST(cache_loc.z() == 50);
  BOOST_TEST(cache_inst->getOrient().getOrientType2D() == dbOrientType::MYR90);
  BOOST_TEST(cache_inst->getOrient().isMirrorZ() == true);
  BOOST_TEST(cache_inst->getOrient().getString() == "MZ_MY_R90");
}

BOOST_FIXTURE_TEST_CASE(test_chip_complex_destroy, F_CHIP_HIERARCHY)
{
  // Test destroying nested hierarchy
  BOOST_TEST(cpu_chip->getChipInsts().size() == 1);

  // Destroy cache instance first
  dbChipInst::destroy(cache_inst);
  BOOST_TEST(cpu_chip->getChipInsts().size() == 0);

  // Now destroy CPU instance from system
  BOOST_TEST(system_chip->getChipInsts().size() == 3);
  dbChipInst::destroy(cpu_inst);
  BOOST_TEST(system_chip->getChipInsts().size() == 2);

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
  BOOST_TEST(found_memory == true);
  BOOST_TEST(found_io == true);

  // Clean up remaining instances
  dbChipInst::destroy(memory_inst);
  dbChipInst::destroy(io_inst);
  BOOST_TEST(db->getChipBumpInsts().size() == 0);
  BOOST_TEST(db->getChipRegionInsts().size() == 0);
  BOOST_TEST(system_chip->getChipInsts().size() == 0);
  BOOST_TEST(system_chip->findChipInst("cpu_inst") == nullptr);
  BOOST_TEST(system_chip->findChipInst("io_inst") == nullptr);
}

BOOST_FIXTURE_TEST_CASE(test_chip_regions, F_CHIP_HIERARCHY)
{
  auto iterateChipRegions = [](dbChip* chip,
                               std::set<std::string> region_names) {
    BOOST_TEST(chip->getChipRegions().size() == region_names.size());
    for (auto region : chip->getChipRegions()) {
      BOOST_TEST((region_names.find(region->getName()) != region_names.end()));
    }
  };
  auto iterateChipRegionInsts
      = [](dbChipInst* chipinst, std::set<std::string> region_names) {
          BOOST_TEST(chipinst->getRegions().size() == region_names.size());
          for (auto region : chipinst->getRegions()) {
            BOOST_TEST((region_names.find(region->getChipRegion()->getName())
                        != region_names.end()));
          }
        };
  // Test dbChipRegion methods
  BOOST_TEST(memory_chip_region_r1->getName() == "R1");
  BOOST_TEST((memory_chip_region_r1->getSide() == dbChipRegion::Side::FRONT));
  BOOST_TEST(memory_chip_region_r1->getLayer() == layer_l1);
  BOOST_TEST(memory_chip_region_r1->getChip() == memory_chip);
  BOOST_TEST(
      (memory_chip_region_r3->getSide() == dbChipRegion::Side::INTERNAL));
  BOOST_TEST(memory_chip_region_r3->getLayer() == nullptr);
  // Test dbChip::findChipRegion
  BOOST_TEST(memory_chip->findChipRegion("R1") == memory_chip_region_r1);
  BOOST_TEST(memory_chip->findChipRegion("R2") == memory_chip_region_r2);
  BOOST_TEST(memory_chip->findChipRegion("R3") == memory_chip_region_r3);
  BOOST_TEST(memory_chip->findChipRegion("R4") == nullptr);
  // Test dbChipInst::findChipRegionInst
  BOOST_TEST(memory_inst->findChipRegionInst("R1")->getChipRegion()
             == memory_chip_region_r1);
  BOOST_TEST(memory_inst->findChipRegionInst("R2")->getChipRegion()
             == memory_chip_region_r2);
  BOOST_TEST(memory_inst->findChipRegionInst("R3")->getChipRegion()
             == memory_chip_region_r3);
  BOOST_TEST(memory_inst->findChipRegionInst("R4") == nullptr);

  iterateChipRegions(memory_chip, {"R1", "R2", "R3"});
  iterateChipRegionInsts(memory_inst, {"R1", "R2", "R3"});
  iterateChipRegions(io_chip, {"R1"});

  try {
    dbChipRegion::create(cpu_chip, "R1", dbChipRegion::Side::FRONT, layer_l1);
    BOOST_TEST(false);
  } catch (const std::exception& e) {
    BOOST_TEST(true);
  }

  try {
    dbChipRegion::create(
        memory_chip, "region_M1", dbChipRegion::Side::FRONT, layer_M1);
    BOOST_TEST(false);
  } catch (const std::exception& e) {
    BOOST_TEST(true);
  }
}

BOOST_FIXTURE_TEST_CASE(test_chip_conns, F_CHIP_HIERARCHY)
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
  BOOST_TEST(memory_region_inst_r1 != nullptr);
  odb::dbChipConn* conn = odb::dbChipConn::create("CONN1",
                                                  system_chip,
                                                  {cpu_inst, cache_inst},
                                                  cache_region_inst_r1,
                                                  {memory_inst},
                                                  memory_region_inst_r1);
  BOOST_TEST(conn->getName() == "CONN1");
  BOOST_TEST(conn->getTopRegion() == cache_region_inst_r1);
  BOOST_TEST(conn->getBottomRegion() == memory_region_inst_r1);
  BOOST_TEST(conn->getTopRegionPath().size() == 2);
  BOOST_TEST(conn->getBottomRegionPath().size() == 1);
  BOOST_TEST(conn->getTopRegionPath()[0] == cpu_inst);
  BOOST_TEST(conn->getTopRegionPath()[1] == cache_inst);
  BOOST_TEST(conn->getBottomRegionPath()[0] == memory_inst);
  BOOST_TEST(db->getChipConns().size() == 1);
  BOOST_TEST(system_chip->getChipConns().size() == 1);
  dbChipConn::destroy(conn);
  BOOST_TEST(db->getChipConns().size() == 0);
  BOOST_TEST(system_chip->getChipConns().size() == 0);
  try {
    odb::dbChipConn::create("CONN2",
                            system_chip,
                            {cache_inst},
                            cache_region_inst_r1,
                            {memory_inst},
                            memory_region_inst_r1);
    BOOST_TEST(false);
  } catch (const std::exception& e) {
    BOOST_TEST(true);
  }
  try {
    odb::dbChipConn::create("CONN2",
                            system_chip,
                            {cache_inst},
                            memory_region_inst_r1,
                            {memory_inst},
                            cache_region_inst_r1);
    BOOST_TEST(false);
  } catch (const std::exception& e) {
    BOOST_TEST(true);
  }
}

BOOST_FIXTURE_TEST_CASE(test_chip_bumps, F_CHIP_HIERARCHY)
{
  BOOST_TEST(io_bump->getInst()->getName() == "io_bump");
  BOOST_TEST(io_bump->getChip() == io_chip);
  BOOST_TEST(io_bump->getChipRegion() == io_chip_region_r1);
  BOOST_TEST(io_bump->getNet() == nullptr);
  BOOST_TEST(io_bump->getBTerm() == nullptr);
  dbNet* net = dbNet::create(io_chip->getBlock(), "net1");
  dbBTerm* bterm = dbBTerm::create(net, "bterm1");
  io_bump->setNet(net);
  io_bump->setBTerm(bterm);
  BOOST_TEST(io_bump->getNet() == net);
  BOOST_TEST(io_bump->getBTerm() == bterm);

  BOOST_TEST(io_chip_region_r1->getChipBumps().size() == 1);
  BOOST_TEST(*io_chip_region_r1->getChipBumps().begin() == io_bump);

  BOOST_TEST(db->getChipBumpInsts().size() == 2);

  BOOST_TEST(io_inst->getRegions().size() == 1);
  auto io_inst_region_r1 = *io_inst->getRegions().begin();
  BOOST_TEST(io_inst_region_r1->getChipBumpInsts().size() == 1);
  auto io_inst_region_r1_bump_inst
      = *io_inst_region_r1->getChipBumpInsts().begin();
  BOOST_TEST(io_inst_region_r1_bump_inst->getChipBump() == io_bump);
  BOOST_TEST(io_inst_region_r1_bump_inst->getChipRegionInst()
             == io_inst_region_r1);

  // test chip nets
  dbChipNet* chip_net = dbChipNet::create(system_chip, "net1");
  BOOST_TEST(chip_net->getChip() == system_chip);
  BOOST_TEST(chip_net->getNumBumpInsts() == 0);
  BOOST_TEST(system_chip->getChipNets().size() == 1);
  BOOST_TEST(db->getChipNets().size() == 1);
  chip_net->addBumpInst(io_inst_region_r1_bump_inst, {io_inst});
  dbChipRegionInst* memory_inst_region_r1 = nullptr;
  for (auto region : memory_inst->getRegions()) {
    if (region->getChipRegion() == memory_chip_region_r1) {
      memory_inst_region_r1 = region;
      break;
    }
  }
  BOOST_TEST(memory_inst_region_r1->getChipBumpInsts().size() == 1);
  auto memory_inst_region_r1_bump_inst
      = (*memory_inst_region_r1->getChipBumpInsts().begin());
  chip_net->addBumpInst(memory_inst_region_r1_bump_inst, {memory_inst});
  BOOST_TEST(chip_net->getNumBumpInsts() == 2);
  std::vector<dbChipInst*> path;
  BOOST_TEST(chip_net->getBumpInst(0, path) == io_inst_region_r1_bump_inst);
  BOOST_TEST(path.size() == 1);
  BOOST_TEST(path[0] == io_inst);
  dbInst* io_cell = memory_chip->getBlock()->findInst("io_bump");
  try {
    dbChipBump::create(io_chip_region_r1, io_cell);
    BOOST_TEST(false);
  } catch (const std::exception& e) {
    BOOST_TEST(true);
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
