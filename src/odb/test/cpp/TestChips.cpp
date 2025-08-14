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
    cpu_chip = dbChip::create(db, "cpu_chip");
    memory_chip = dbChip::create(db, "memory_chip");
    io_chip = dbChip::create(db, "io_chip");
    cache_chip = dbChip::create(db, "cache_chip");

    system_chip->setWidth(5000);
    system_chip->setHeight(4000);
    system_chip->setThickness(500);
    system_chip->setOffset(Point(10, 0));
  }

  ~F_CHIP_HIERARCHY() { dbDatabase::destroy(db); }

  dbDatabase* db;
  dbChip* system_chip;
  dbChip* cpu_chip;
  dbChip* memory_chip;
  dbChip* io_chip;
  dbChip* cache_chip;
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
  BOOST_TEST((cpu_chip->getChipType() == dbChip::ChipType::DIE));

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

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
