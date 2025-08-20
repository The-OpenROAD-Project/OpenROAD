#define BOOST_TEST_MODULE TestChips
#include <boost/test/included/unit_test.hpp>

#include "helper.h"
#include "odb/db.h"

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

    system_chip = dbChip::create(db, "system_chip", dbChip::ChipType::HIER);
    cpu_chip = dbChip::create(db, "cpu_chip", dbChip::ChipType::HIER);
    memory_chip = dbChip::create(db, "memory_chip");
    io_chip = dbChip::create(db, "io_chip");
    cache_chip = dbChip::create(db, "cache_chip");

    system_chip->setWidth(5000);
    system_chip->setHeight(4000);
    system_chip->setThickness(500);
    system_chip->setOffset(Point(10, 0));
    // Create chip instances in system
    cpu_inst = dbChipInst::create(system_chip, cpu_chip, "cpu_inst");
    memory_inst = dbChipInst::create(system_chip, memory_chip, "memory_inst");
    io_inst = dbChipInst::create(system_chip, io_chip, "io_inst");

    // Create cache instance in CPU chip (nested hierarchy)
    cache_inst = dbChipInst::create(cpu_chip, cache_chip, "cache_inst");

    // Position components
    memory_inst->setLoc(Point3D(2500, 500, 0));
    cache_inst->setLoc(Point3D(100, 100, 50));
  }

  ~F_CHIP_HIERARCHY() { dbDatabase::destroy(db); }

  dbDatabase* db;
  dbChip* system_chip;
  dbChip* cpu_chip;
  dbChip* memory_chip;
  dbChip* io_chip;
  dbChip* cache_chip;
  dbChipInst* cpu_inst;
  dbChipInst* memory_inst;
  dbChipInst* io_inst;
  dbChipInst* cache_inst;
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

  Point3D cache_loc = cache_inst->getLoc();
  BOOST_TEST(cache_loc.x() == 100);
  BOOST_TEST(cache_loc.y() == 100);
  BOOST_TEST(cache_loc.z() == 50);
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
  BOOST_TEST(system_chip->getChipInsts().size() == 0);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
