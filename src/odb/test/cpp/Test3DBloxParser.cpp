#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "helper.h"
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
  EXPECT_EQ(db_->getDbuPerMicron(), 2000);

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
  EXPECT_EQ(regions.size(), 2);

  auto region = chip->findChipRegion("back_reg");
  ASSERT_NE(region, nullptr);
  EXPECT_EQ(region->getSide(), dbChipRegion::Side::BACK);

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
  EXPECT_EQ(connection->getThickness(), 0);
  connection = *itr;
  EXPECT_EQ(connection->getName(), "soc_to_virtual");
  EXPECT_EQ(connection->getTopRegion()->getChipInst()->getName(), "soc_inst");
  EXPECT_EQ(connection->getTopRegionPath().size(), 1);
  EXPECT_EQ(connection->getTopRegionPath()[0]->getName(), "soc_inst");
  EXPECT_EQ(connection->getTopRegion()->getChipRegion()->getName(), "back_reg");
  EXPECT_EQ(connection->getBottomRegionPath().size(), 0);
  EXPECT_EQ(connection->getBottomRegion(), nullptr);
  EXPECT_EQ(connection->getThickness(), 0.0);
}

TEST_F(DbvFixture, test_bump_map_parser)
{
  auto region = db_->findChip("SoC")->findChipRegion("back_reg");
  ASSERT_NE(region, nullptr);
  EXPECT_EQ(region->getChipBumps().size(), 2);
  auto bump = *region->getChipBumps().begin();
  EXPECT_EQ(bump->getInst()->getName(), "bump1");
  EXPECT_EQ(bump->getInst()->getMaster()->getName(), "BUMP");
  const double bump_size2 = 29.0 / 2;
  EXPECT_EQ(bump->getInst()->getOrigin().x(),
            (100.0 - bump_size2) * db_->getDbuPerMicron());
  EXPECT_EQ(bump->getInst()->getOrigin().y(),
            (200.0 - bump_size2) * db_->getDbuPerMicron());
  EXPECT_EQ(bump->getNet(), nullptr);
  EXPECT_EQ(bump->getBTerm(), nullptr);
  auto soc_inst = db_->getChip()->findChipInst("soc_inst");
  auto region_inst = soc_inst->findChipRegionInst("back_reg");
  ASSERT_NE(region_inst, nullptr);
  EXPECT_EQ(region_inst->getChipBumpInsts().size(), 2);
  auto bump_inst = *region_inst->getChipBumpInsts().begin();
  EXPECT_EQ(bump_inst->getChipBump(), bump);
  EXPECT_EQ(bump_inst->getChipRegionInst(), region_inst);
}

TEST_F(SimpleDbFixture, test_bump_map_reader)
{
  createSimpleDB();

  db_->setDbuPerMicron(1000);

  // Create BUMP master
  dbLib* lib = db_->findLib("lib1");
  dbTechLayer* bot_layer
      = dbTechLayer::create(lib->getTech(), "BOT", dbTechLayerType::ROUTING);
  dbTechLayer* layer
      = dbTechLayer::create(lib->getTech(), "TOP", dbTechLayerType::ROUTING);
  dbMaster* master = dbMaster::create(lib, "BUMP");
  master->setWidth(10000);
  master->setHeight(10000);
  master->setOrigin(5000, 5000);
  master->setType(dbMasterType::COVER_BUMP);
  dbMTerm* term
      = dbMTerm::create(master, "PAD", dbIoType::INOUT, dbSigType::SIGNAL);
  dbMPin* bot_pin = dbMPin::create(term);
  dbBox::create(bot_pin, bot_layer, -1000, -1000, 1000, 1000);
  dbMPin* pin = dbMPin::create(term);
  dbBox::create(pin, layer, -2000, -2000, 2000, 2000);
  master->setFrozen();

  // Create BTerms
  dbBlock* block = db_->getChip()->getBlock();
  dbBTerm* SIG1 = dbBTerm::create(dbNet::create(block, "SIG1"), "SIG1");
  dbBTerm* SIG2 = dbBTerm::create(dbNet::create(block, "SIG2"), "SIG2");
  block->setDieArea(Rect(0, 0, 500, 500));

  EXPECT_EQ(block->getInsts().size(), 0);

  ThreeDBlox parser(&logger_, db_.get());
  std::string path = getFilePath(prefix + "data/example1.bmap");
  parser.readBMap(path);

  // Check bumps were created
  EXPECT_EQ(block->getInsts().size(), 2);
  dbInst* inst1 = block->findInst("bump1");
  EXPECT_EQ(inst1->getBBox()->getBox().xCenter(), 100 * 1000);
  EXPECT_EQ(inst1->getBBox()->getBox().yCenter(), 200 * 1000);
  dbInst* inst2 = block->findInst("bump2");
  EXPECT_EQ(inst2->getBBox()->getBox().xCenter(), 150 * 1000);
  EXPECT_EQ(inst2->getBBox()->getBox().yCenter(), 200 * 1000);

  // Check that BPins where added
  EXPECT_EQ(SIG1->getBPins().size(), 1);
  EXPECT_EQ(SIG2->getBPins().size(), 1);
  EXPECT_EQ(SIG1->getBPins().begin()->getBoxes().size(), 1);
  EXPECT_EQ(SIG2->getBPins().begin()->getBoxes().size(), 1);

  // Check bPin shape and layer
  dbBox* sig1_box = *SIG1->getBPins().begin()->getBoxes().begin();
  dbBox* sig2_box = *SIG2->getBPins().begin()->getBoxes().begin();
  EXPECT_EQ(sig1_box->getTechLayer(), layer);
  EXPECT_EQ(sig1_box->getBox().xMin(), 98000);
  EXPECT_EQ(sig1_box->getBox().yMin(), 198000);
  EXPECT_EQ(sig1_box->getBox().xMax(), 102000);
  EXPECT_EQ(sig1_box->getBox().yMax(), 202000);
  EXPECT_EQ(sig2_box->getTechLayer(), layer);
  EXPECT_EQ(sig2_box->getBox().xMin(), 148000);
  EXPECT_EQ(sig2_box->getBox().yMin(), 198000);
  EXPECT_EQ(sig2_box->getBox().xMax(), 152000);
  EXPECT_EQ(sig2_box->getBox().yMax(), 202000);
}

TEST_F(SimpleDbFixture, test_bump_map_reader_rounding)
{
  createSimpleDB();

  db_->setDbuPerMicron(1000);

  // Create BUMP master
  dbLib* lib = db_->findLib("lib1");
  dbTechLayer* bot_layer
      = dbTechLayer::create(lib->getTech(), "BOT", dbTechLayerType::ROUTING);
  dbTechLayer* layer
      = dbTechLayer::create(lib->getTech(), "TOP", dbTechLayerType::ROUTING);
  dbMaster* master = dbMaster::create(lib, "BUMP");
  master->setWidth(10000);
  master->setHeight(10000);
  master->setOrigin(5000, 5000);
  master->setType(dbMasterType::COVER_BUMP);
  dbMTerm* term
      = dbMTerm::create(master, "PAD", dbIoType::INOUT, dbSigType::SIGNAL);
  dbMPin* bot_pin = dbMPin::create(term);
  dbBox::create(bot_pin, bot_layer, -1000, -1000, 1000, 1000);
  dbMPin* pin = dbMPin::create(term);
  dbBox::create(pin, layer, -2000, -2000, 2000, 2000);
  master->setFrozen();

  dbBlock* block = db_->getChip()->getBlock();
  block->setDieArea(Rect(0, 0, 500, 500));

  EXPECT_EQ(block->getInsts().size(), 0);

  ThreeDBlox parser(&logger_, db_.get());
  std::string path = getFilePath(prefix + "data/example4.bmap");
  parser.readBMap(path);

  // Check bumps were created
  EXPECT_EQ(block->getInsts().size(), 2);
  dbInst* inst1 = block->findInst("bump1");
  EXPECT_EQ(inst1->getBBox()->getBox().xCenter(), 1036800);
  EXPECT_EQ(inst1->getBBox()->getBox().yCenter(), 5991840);
  dbInst* inst2 = block->findInst("bump2");
  EXPECT_EQ(inst2->getBBox()->getBox().xCenter(), 1041120);
  EXPECT_EQ(inst2->getBBox()->getBox().yCenter(), 5991840);
}

TEST_F(SimpleDbFixture, test_bump_map_reader_netsonly)
{
  createSimpleDB();

  db_->setDbuPerMicron(1000);

  // Create BUMP master
  dbLib* lib = db_->findLib("lib1");
  dbTechLayer* bot_layer
      = dbTechLayer::create(lib->getTech(), "BOT", dbTechLayerType::ROUTING);
  dbTechLayer* layer
      = dbTechLayer::create(lib->getTech(), "TOP", dbTechLayerType::ROUTING);
  dbMaster* master = dbMaster::create(lib, "BUMP");
  master->setWidth(10000);
  master->setHeight(10000);
  master->setOrigin(5000, 5000);
  master->setType(dbMasterType::COVER_BUMP);
  dbMTerm* term
      = dbMTerm::create(master, "PAD", dbIoType::INOUT, dbSigType::SIGNAL);
  dbMPin* bot_pin = dbMPin::create(term);
  dbBox::create(bot_pin, bot_layer, -1000, -1000, 1000, 1000);
  dbMPin* pin = dbMPin::create(term);
  dbBox::create(pin, layer, -2000, -2000, 2000, 2000);
  master->setFrozen();

  // Create BTerms
  dbBlock* block = db_->getChip()->getBlock();
  dbBTerm* SIG1 = dbBTerm::create(dbNet::create(block, "SIG1"), "SIG1");
  dbBTerm* SIG2 = dbBTerm::create(dbNet::create(block, "SIG2"), "SIG2");
  block->setDieArea(Rect(0, 0, 500, 500));

  EXPECT_EQ(block->getInsts().size(), 0);

  ThreeDBlox parser(&logger_, db_.get());
  std::string path = getFilePath(prefix + "data/example2.bmap");
  parser.readBMap(path);

  // Check bumps were created
  EXPECT_EQ(block->getInsts().size(), 2);
  dbInst* inst1 = block->findInst("bump1");
  EXPECT_EQ(inst1->getBBox()->getBox().xCenter(), 100 * 1000);
  EXPECT_EQ(inst1->getBBox()->getBox().yCenter(), 200 * 1000);
  dbInst* inst2 = block->findInst("bump2");
  EXPECT_EQ(inst2->getBBox()->getBox().xCenter(), 150 * 1000);
  EXPECT_EQ(inst2->getBBox()->getBox().yCenter(), 200 * 1000);

  // Check that BPins where added
  EXPECT_EQ(SIG1->getBPins().size(), 1);
  EXPECT_EQ(SIG2->getBPins().size(), 1);
  EXPECT_EQ(SIG1->getBPins().begin()->getBoxes().size(), 1);
  EXPECT_EQ(SIG2->getBPins().begin()->getBoxes().size(), 1);

  // Check bPin shape and layer
  dbBox* sig1_box = *SIG1->getBPins().begin()->getBoxes().begin();
  dbBox* sig2_box = *SIG2->getBPins().begin()->getBoxes().begin();
  EXPECT_EQ(sig1_box->getTechLayer(), layer);
  EXPECT_EQ(sig1_box->getBox().xMin(), 98000);
  EXPECT_EQ(sig1_box->getBox().yMin(), 198000);
  EXPECT_EQ(sig1_box->getBox().xMax(), 102000);
  EXPECT_EQ(sig1_box->getBox().yMax(), 202000);
  EXPECT_EQ(sig2_box->getTechLayer(), layer);
  EXPECT_EQ(sig2_box->getBox().xMin(), 148000);
  EXPECT_EQ(sig2_box->getBox().yMin(), 198000);
  EXPECT_EQ(sig2_box->getBox().xMax(), 152000);
  EXPECT_EQ(sig2_box->getBox().yMax(), 202000);
}

TEST_F(SimpleDbFixture, test_bump_map_reader_multiple_bterms)
{
  createSimpleDB();

  db_->setDbuPerMicron(1000);

  // Create BUMP master
  dbLib* lib = db_->findLib("lib1");
  dbTechLayer* bot_layer
      = dbTechLayer::create(lib->getTech(), "BOT", dbTechLayerType::ROUTING);
  dbTechLayer* layer
      = dbTechLayer::create(lib->getTech(), "TOP", dbTechLayerType::ROUTING);
  dbMaster* master = dbMaster::create(lib, "BUMP");
  master->setWidth(10000);
  master->setHeight(10000);
  master->setOrigin(5000, 5000);
  master->setType(dbMasterType::COVER_BUMP);
  dbMTerm* term
      = dbMTerm::create(master, "PAD", dbIoType::INOUT, dbSigType::SIGNAL);
  dbMPin* bot_pin = dbMPin::create(term);
  dbBox::create(bot_pin, bot_layer, -1000, -1000, 1000, 1000);
  dbMPin* pin = dbMPin::create(term);
  dbBox::create(pin, layer, -2000, -2000, 2000, 2000);
  master->setFrozen();

  // Create BTerms
  dbBlock* block = db_->getChip()->getBlock();
  dbNet* net = dbNet::create(block, "SIG1");
  dbBTerm::create(net, "TERM1");
  dbBTerm::create(net, "TERM2");
  block->setDieArea(Rect(0, 0, 500, 500));

  EXPECT_EQ(block->getInsts().size(), 0);

  ThreeDBlox parser(&logger_, db_.get());
  std::string path = getFilePath(prefix + "data/example3.bmap");
  EXPECT_THROW(parser.readBMap(path), std::runtime_error);
}

TEST_F(SimpleDbFixture, test_bump_map_reader_no_bterms)
{
  createSimpleDB();

  db_->setDbuPerMicron(1000);

  // Create BUMP master
  dbLib* lib = db_->findLib("lib1");
  dbTechLayer* bot_layer
      = dbTechLayer::create(lib->getTech(), "BOT", dbTechLayerType::ROUTING);
  dbTechLayer* layer
      = dbTechLayer::create(lib->getTech(), "TOP", dbTechLayerType::ROUTING);
  dbMaster* master = dbMaster::create(lib, "BUMP");
  master->setWidth(10000);
  master->setHeight(10000);
  master->setOrigin(5000, 5000);
  master->setType(dbMasterType::COVER_BUMP);
  dbMTerm* term
      = dbMTerm::create(master, "PAD", dbIoType::INOUT, dbSigType::SIGNAL);
  dbMPin* bot_pin = dbMPin::create(term);
  dbBox::create(bot_pin, bot_layer, -1000, -1000, 1000, 1000);
  dbMPin* pin = dbMPin::create(term);
  dbBox::create(pin, layer, -2000, -2000, 2000, 2000);
  master->setFrozen();

  // Create BTerms
  dbBlock* block = db_->getChip()->getBlock();
  dbNet::create(block, "SIG1");
  block->setDieArea(Rect(0, 0, 500, 500));

  EXPECT_EQ(block->getInsts().size(), 0);

  ThreeDBlox parser(&logger_, db_.get());
  std::string path = getFilePath(prefix + "data/example3.bmap");
  EXPECT_THROW(parser.readBMap(path), std::runtime_error);
}

TEST_F(SimpleDbFixture, test_bterm_get_chip_bump)
{
  createSimpleDB();

  dbChip* chip = db_->getChip();
  dbBlock* block = chip->getBlock();

  // Create a chip region on the chip
  dbChipRegion* region = dbChipRegion::create(
      chip, "bump_region", dbChipRegion::Side::BACK, nullptr);
  ASSERT_NE(region, nullptr);

  // Create an inst to back the bump
  dbLib* lib = db_->findLib("lib1");
  dbMaster* bump_master = dbMaster::create(lib, "BUMP_CELL");
  bump_master->setType(dbMasterType::COVER_BUMP);
  bump_master->setFrozen();
  dbInst* bump_inst = dbInst::create(block, bump_master, "bump1");
  ASSERT_NE(bump_inst, nullptr);

  // Create a chip bump
  dbChipBump* bump = dbChipBump::create(region, bump_inst);
  ASSERT_NE(bump, nullptr);

  // Create bterms: one that will be associated with the bump, one that won't
  dbBTerm* sig1 = dbBTerm::create(dbNet::create(block, "SIG1"), "SIG1");
  dbBTerm* sig2 = dbBTerm::create(dbNet::create(block, "SIG2"), "SIG2");
  ASSERT_NE(sig1, nullptr);
  ASSERT_NE(sig2, nullptr);

  // Before setBTerm: both bterms return nullptr for getChipBump
  EXPECT_EQ(sig1->getChipBump(), nullptr);
  EXPECT_EQ(sig2->getChipBump(), nullptr);

  // Associate sig1 with the bump
  bump->setBTerm(sig1);

  // After setBTerm: sig1 returns the bump, sig2 still returns nullptr
  EXPECT_EQ(sig1->getChipBump(), bump);
  EXPECT_EQ(sig2->getChipBump(), nullptr);

  // Verify the reverse link is also consistent
  EXPECT_EQ(bump->getBTerm(), sig1);
  EXPECT_EQ(bump->getChipRegion(), region);
}

// ---------------------------------------------------------------------------
// Path Assertion Parser Tests
// ---------------------------------------------------------------------------

class PathAssertionFixture : public tst::Fixture
{
 protected:
  ThreeDBlox parser_{&logger_, db_.get()};
};

TEST_F(PathAssertionFixture, test_path_assertions_from_file)
{
  std::string path = getFilePath(prefix + "data/path_assertions.3dbx");
  parser_.readDbx(path);

  dbChip* chip = db_->findChip("TopDesign");
  ASSERT_NE(chip, nullptr);
  auto chip_paths = chip->getChipPaths();
  ASSERT_EQ(chip_paths.size(), 2);

  // Build a map for lookup by name
  std::map<std::string, dbChipPath*> paths_by_name;
  for (auto* p : chip_paths) {
    paths_by_name[p->getName()] = p;
  }

  // Path1: 3 entries, one negated
  ASSERT_EQ(paths_by_name.count("Path1"), 1u);
  const std::vector<dbChipPath::Entry> e1
      = paths_by_name["Path1"]->getEntries();
  ASSERT_EQ(e1.size(), 3u);
  ASSERT_NE(e1[0].region, nullptr);
  ASSERT_EQ(e1[0].chip_inst_path.size(), 1u);
  EXPECT_EQ(e1[0].chip_inst_path[0]->getName(), "soc_inst");
  EXPECT_EQ(e1[0].region->getChipRegion()->getName(), "front_reg");
  EXPECT_FALSE(e1[0].negated);
  ASSERT_NE(e1[1].region, nullptr);
  ASSERT_EQ(e1[1].chip_inst_path.size(), 1u);
  EXPECT_EQ(e1[1].chip_inst_path[0]->getName(), "soc_inst_duplicate");
  EXPECT_EQ(e1[1].region->getChipRegion()->getName(), "back_reg");
  EXPECT_TRUE(e1[1].negated);
  ASSERT_NE(e1[2].region, nullptr);
  ASSERT_EQ(e1[2].chip_inst_path.size(), 1u);
  EXPECT_EQ(e1[2].chip_inst_path[0]->getName(), "soc_inst_duplicate");
  EXPECT_EQ(e1[2].region->getChipRegion()->getName(), "front_reg");
  EXPECT_FALSE(e1[2].negated);

  // Path2: 2 entries, none negated
  ASSERT_EQ(paths_by_name.count("Path2"), 1u);
  const std::vector<dbChipPath::Entry> e2
      = paths_by_name["Path2"]->getEntries();
  ASSERT_EQ(e2.size(), 2u);
  ASSERT_NE(e2[0].region, nullptr);
  ASSERT_EQ(e2[0].chip_inst_path.size(), 1u);
  EXPECT_EQ(e2[0].chip_inst_path[0]->getName(), "soc_inst");
  EXPECT_EQ(e2[0].region->getChipRegion()->getName(), "back_reg");
  EXPECT_FALSE(e2[0].negated);
  ASSERT_NE(e2[1].region, nullptr);
  ASSERT_EQ(e2[1].chip_inst_path.size(), 1u);
  EXPECT_EQ(e2[1].chip_inst_path[0]->getName(), "soc_inst_duplicate");
  EXPECT_EQ(e2[1].region->getChipRegion()->getName(), "back_reg");
  EXPECT_FALSE(e2[1].negated);
}

TEST_F(PathAssertionFixture, test_path_assertions_no_path_block)
{
  std::string path = getFilePath(prefix + "data/example.3dbx");
  parser_.readDbx(path);

  dbChip* chip = db_->findChip("TopDesign");
  ASSERT_NE(chip, nullptr);
  EXPECT_EQ(chip->getChipPaths().size(), 0u);
}

// Test that chip_inst_path correctly disambiguates the same
// dbChipRegionInst in a folded model with multiple hierarchy levels.
//
// Hierarchy:
//   TopDesign (HIER)
//     ├── hier1 (SubHier, HIER)
//     │     └── die_inst (LeafChip, DIE) → region r1
//     └── hier2 (SubHier, HIER)
//           └── die_inst (LeafChip, DIE) → region r1
//
// hier1/die_inst and hier2/die_inst share the same dbChipRegionInst
// for r1, so chip_inst_path is required to distinguish them.
TEST_F(PathAssertionFixture, test_path_assertions_folded_model)
{
  // Build hierarchy programmatically
  dbTech* tech = dbTech::create(db_.get(), "test_tech");
  dbChip* leaf_chip
      = dbChip::create(db_.get(), tech, "LeafChip", dbChip::ChipType::DIE);
  ASSERT_NE(leaf_chip, nullptr);
  leaf_chip->setWidth(100);
  leaf_chip->setHeight(100);
  leaf_chip->setThickness(50);
  dbChipRegion::create(leaf_chip, "r1", dbChipRegion::Side::FRONT, nullptr);

  dbChip* sub_hier
      = dbChip::create(db_.get(), nullptr, "SubHier", dbChip::ChipType::HIER);
  ASSERT_NE(sub_hier, nullptr);
  dbChipInst* die_inst = dbChipInst::create(sub_hier, leaf_chip, "die_inst");
  ASSERT_NE(die_inst, nullptr);

  dbChip* top
      = dbChip::create(db_.get(), nullptr, "Top", dbChip::ChipType::HIER);
  ASSERT_NE(top, nullptr);
  dbChipInst* hier1 = dbChipInst::create(top, sub_hier, "hier1");
  dbChipInst* hier2 = dbChipInst::create(top, sub_hier, "hier2");
  ASSERT_NE(hier1, nullptr);
  ASSERT_NE(hier2, nullptr);

  // Both hier1/die_inst and hier2/die_inst share the same dbChipRegionInst
  dbChipRegionInst* region_inst = die_inst->findChipRegionInst("r1");
  ASSERT_NE(region_inst, nullptr);

  // Create a path assertion with entries for both hierarchical paths
  dbChipPath* chip_path = dbChipPath::create(top, "Path1");
  chip_path->addEntry({hier1, die_inst}, region_inst, false);
  chip_path->addEntry({hier2, die_inst}, region_inst, true);

  // Verify entries
  std::vector<dbChipPath::Entry> entries = chip_path->getEntries();
  ASSERT_EQ(entries.size(), 2u);

  // Both entries point to the same region inst (folded model)
  EXPECT_EQ(entries[0].region, entries[1].region);
  EXPECT_EQ(entries[0].region, region_inst);

  // But they have different chip_inst_path to disambiguate
  ASSERT_EQ(entries[0].chip_inst_path.size(), 2u);
  EXPECT_EQ(entries[0].chip_inst_path[0]->getName(), "hier1");
  EXPECT_EQ(entries[0].chip_inst_path[1]->getName(), "die_inst");
  EXPECT_FALSE(entries[0].negated);

  ASSERT_EQ(entries[1].chip_inst_path.size(), 2u);
  EXPECT_EQ(entries[1].chip_inst_path[0]->getName(), "hier2");
  EXPECT_EQ(entries[1].chip_inst_path[1]->getName(), "die_inst");
  EXPECT_TRUE(entries[1].negated);
}

}  // namespace
}  // namespace odb
