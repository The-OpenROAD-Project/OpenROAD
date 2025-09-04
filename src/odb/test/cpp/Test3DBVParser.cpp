#define BOOST_TEST_MODULE Test3DBVParser
#include <iostream>
#include <string>

#include "boost/test/included/unit_test.hpp"
#include "env.h"
#include "odb/3dblox.h"
#include "odb/db.h"
namespace odb {
namespace {

BOOST_AUTO_TEST_SUITE(test_suite)

struct F_DBV_PARSER
{
  F_DBV_PARSER()
  {
    db = dbDatabase::create();
    logger = new utl::Logger();
    db->setLogger(logger);
    ThreeDBlox parser(logger, db);
    std::string path = testTmpPath("data", "example.3dbx");
    parser.readDbx(path);
  }

  ~F_DBV_PARSER()
  {
    dbDatabase::destroy(db);
    delete logger;
  }
  odb::dbDatabase* db;
  utl::Logger* logger;
};

BOOST_FIXTURE_TEST_CASE(test_3dbv, F_DBV_PARSER)
{
  // Test that database precision was set correctly
  BOOST_CHECK_EQUAL(db->getDbuPerMicron(), 100000);

  auto chips = db->getChips();
  BOOST_CHECK_EQUAL(chips.size(), 2);
  auto chip = *chips.begin();
  BOOST_CHECK_EQUAL(chip->getName(), "SoC");
  BOOST_TEST((chip->getChipType() == dbChip::ChipType::DIE));

  // Test chip dimensions (converted to DBU)
  int expected_width = 955 * db->getDbuPerMicron();
  int expected_height = 1082 * db->getDbuPerMicron();
  int expected_thickness = 300 * db->getDbuPerMicron();

  BOOST_CHECK_EQUAL(chip->getWidth(), expected_width);
  BOOST_CHECK_EQUAL(chip->getHeight(), expected_height);
  BOOST_CHECK_EQUAL(chip->getThickness(), expected_thickness);

  // Test chip properties
  BOOST_CHECK_CLOSE(chip->getShrink(), 1.0, 0.001);
  BOOST_CHECK_EQUAL(chip->isTsv(), false);

  // Test chip regions were created
  auto regions = chip->getChipRegions();
  BOOST_CHECK_EQUAL(regions.size(), 1);

  auto region = *regions.begin();
  BOOST_CHECK_EQUAL(region->getName(), "r1");
  BOOST_TEST((region->getSide() == dbChipRegion::Side::FRONT));

  // Test region bounding box
  Rect region_box = region->getBox();
  int expected_x1 = 0 * db->getDbuPerMicron();
  int expected_y1 = 0 * db->getDbuPerMicron();
  int expected_x2 = 955 * db->getDbuPerMicron();
  int expected_y2 = 1082 * db->getDbuPerMicron();

  BOOST_CHECK_EQUAL(region_box.xMin(), expected_x1);
  BOOST_CHECK_EQUAL(region_box.yMin(), expected_y1);
  BOOST_CHECK_EQUAL(region_box.xMax(), expected_x2);
  BOOST_CHECK_EQUAL(region_box.yMax(), expected_y2);
}
BOOST_FIXTURE_TEST_CASE(test_3dbx, F_DBV_PARSER)
{
  auto chip_insts = db->getChipInsts();
  BOOST_CHECK_EQUAL(chip_insts.size(), 1);
  auto chip_inst = *chip_insts.begin();
  BOOST_CHECK_EQUAL(chip_inst->getName(), "soc_inst");
  BOOST_CHECK_EQUAL(chip_inst->getMasterChip()->getName(), "SoC");
  BOOST_CHECK_EQUAL(chip_inst->getParentChip()->getName(), "TopDesign");
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
