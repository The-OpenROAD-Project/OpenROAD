#include <iostream>
#include <string>

#include "gtest/gtest.h"
#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "tst/fixture.h"

namespace odb {
namespace {

static const std::string prefix("_main/src/odb/test/");

class DbvFixture : public tst::Fixture
{
 protected:
  DbvFixture()
  {
    dbTech::create(db_.get(), "tech");
    ThreeDBlox parser(&logger_, db_.get());
    std::string path = getFilePath(prefix + "data/example.3dbx");
    parser.readDbx(path);
  }
};

TEST_F(DbvFixture, test_3dbv)
{
  // Test that database precision was set correctly
  EXPECT_EQ(db_->getDbuPerMicron(), 100000);

  auto chips = db_->getChips();
  EXPECT_EQ(chips.size(), 2);
  auto chip = *chips.begin();
  EXPECT_STREQ(chip->getName(), "SoC");
  EXPECT_EQ(chip->getChipType(), dbChip::ChipType::DIE);

  // Test chip dimensions (converted to DBU)
  int expected_width = 955 * db_->getDbuPerMicron();
  int expected_height = 1082 * db_->getDbuPerMicron();
  int expected_thickness = 300 * db_->getDbuPerMicron();

  EXPECT_EQ(chip->getWidth(), expected_width);
  EXPECT_EQ(chip->getHeight(), expected_height);
  EXPECT_EQ(chip->getThickness(), expected_thickness);

  // Test chip properties
  EXPECT_NEAR(chip->getShrink(), 1.0, 0.001);
  EXPECT_EQ(chip->isTsv(), false);

  // Test chip regions were created
  auto regions = chip->getChipRegions();
  EXPECT_EQ(regions.size(), 1);

  auto region = *regions.begin();
  EXPECT_EQ(region->getName(), "r1");
  EXPECT_EQ(region->getSide(), dbChipRegion::Side::FRONT);

  // Test region bounding box
  Rect region_box = region->getBox();
  int expected_x1 = 0 * db_->getDbuPerMicron();
  int expected_y1 = 0 * db_->getDbuPerMicron();
  int expected_x2 = 955 * db_->getDbuPerMicron();
  int expected_y2 = 1082 * db_->getDbuPerMicron();

  EXPECT_EQ(region_box.xMin(), expected_x1);
  EXPECT_EQ(region_box.yMin(), expected_y1);
  EXPECT_EQ(region_box.xMax(), expected_x2);
  EXPECT_EQ(region_box.yMax(), expected_y2);
}

TEST_F(DbvFixture, test_3dbx)
{
  auto chip_insts = db_->getChipInsts();
  EXPECT_EQ(chip_insts.size(), 2);
  auto soc_inst = db_->getChip()->findChipInst("soc_inst");
  auto soc_inst_duplicate = db_->getChip()->findChipInst("soc_inst_duplicate");
  EXPECT_EQ(soc_inst->getName(), "soc_inst");
  EXPECT_STREQ(soc_inst->getMasterChip()->getName(), "SoC");
  EXPECT_STREQ(soc_inst->getParentChip()->getName(), "TopDesign");
  EXPECT_EQ(soc_inst_duplicate->getName(), "soc_inst_duplicate");
  EXPECT_STREQ(soc_inst_duplicate->getMasterChip()->getName(), "SoC");
  EXPECT_STREQ(soc_inst_duplicate->getParentChip()->getName(), "TopDesign");
  EXPECT_EQ(soc_inst->getLoc().x(), 100.0 * db_->getDbuPerMicron());
  EXPECT_EQ(soc_inst->getLoc().y(), 200.0 * db_->getDbuPerMicron());
  EXPECT_EQ(soc_inst->getLoc().z(), 0.0);
  EXPECT_EQ(soc_inst->getOrient().getString(), "R0");
  EXPECT_EQ(soc_inst_duplicate->getLoc().x(), 100.0 * db_->getDbuPerMicron());
  EXPECT_EQ(soc_inst_duplicate->getLoc().y(), 200.0 * db_->getDbuPerMicron());
  EXPECT_EQ(soc_inst_duplicate->getLoc().z(), 300.0 * db_->getDbuPerMicron());
  EXPECT_EQ(soc_inst_duplicate->getOrient().getString(), "MZ");
  auto connections = db_->getChipConns();
  EXPECT_EQ(connections.size(), 2);
  auto itr = connections.begin();
  auto connection = *itr++;
  EXPECT_EQ(connection->getName(), "soc_to_soc");
  EXPECT_EQ(connection->getTopRegion()->getChipInst()->getName(),
            "soc_inst_duplicate");
  EXPECT_EQ(connection->getBottomRegion()->getChipInst()->getName(),
            "soc_inst");
  EXPECT_EQ(connection->getThickness(), 2.0 * db_->getDbuPerMicron());
  connection = *itr;
  EXPECT_EQ(connection->getName(), "soc_to_virtual");
  EXPECT_EQ(connection->getTopRegion()->getChipInst()->getName(),
            "soc_inst_duplicate");
  EXPECT_EQ(connection->getTopRegionPath().size(), 1);
  EXPECT_EQ(connection->getTopRegionPath()[0]->getName(), "soc_inst_duplicate");
  EXPECT_EQ(connection->getTopRegion()->getChipRegion()->getName(), "r1");
  EXPECT_EQ(connection->getBottomRegionPath().size(), 0);
  EXPECT_EQ(connection->getBottomRegion(), nullptr);
  EXPECT_EQ(connection->getThickness(), 0.0);
}

TEST_F(DbvFixture, test_bump_map_parser)
{
  auto region = db_->findChip("SoC")->findChipRegion("r1");
  EXPECT_EQ(region->getChipBumps().size(), 2);
  auto bump = *region->getChipBumps().begin();
  EXPECT_EQ(bump->getInst()->getName(), "bump1");
  EXPECT_EQ(bump->getInst()->getMaster()->getName(), "BUMP");
  EXPECT_EQ(bump->getInst()->getOrigin().x(), 100.0 * db_->getDbuPerMicron());
  EXPECT_EQ(bump->getInst()->getOrigin().y(), 200.0 * db_->getDbuPerMicron());
  EXPECT_EQ(bump->getNet(), nullptr);
  EXPECT_EQ(bump->getBTerm(), nullptr);
  auto soc_inst = db_->getChip()->findChipInst("soc_inst");
  auto region_inst = soc_inst->findChipRegionInst("r1");
  EXPECT_EQ(region_inst->getChipBumpInsts().size(), 2);
  auto bump_inst = *region_inst->getChipBumpInsts().begin();
  EXPECT_EQ(bump_inst->getChipBump(), bump);
  EXPECT_EQ(bump_inst->getChipRegionInst(), region_inst);
}

}  // namespace
}  // namespace odb
