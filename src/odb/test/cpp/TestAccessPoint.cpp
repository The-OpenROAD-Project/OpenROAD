#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace odb {
namespace {

TEST_F(SimpleDbFixture, test_default)
{
  createSimpleDB();
  auto block = db_->getChip()->getBlock();
  auto and2 = db_->findMaster("and2");
  auto term = and2->findMTerm("a");
  auto layer = db_->getTech()->findLayer("L1");
  auto pin = dbMPin::create(term);
  auto ap = dbAccessPoint::create(block, pin, 0);
  auto inst = dbInst::create(db_->getChip()->getBlock(), and2, "i1");
  auto iterm = inst->getITerm(term);
  ap->addSegment(Rect(10, 20, 30, 40), true, false);
  ap->setPoint(Point(10, 250));
  ap->setLayer(layer);
  ap->setHighType(dbAccessType::HalfGrid);
  ap->setAccess(true, dbDirection::DOWN);
  iterm->setAccessPoint(pin, ap);
  std::filesystem::create_directory("results");
  std::string path = "results/TestAccessPointDbRW";
  std::ofstream write;
  write.exceptions(std::ifstream::failbit | std::ifstream::badbit
                   | std::ios::eofbit);
  write.open(path, std::ios::binary);
  db_->write(write);

  dbDatabase* db2 = dbDatabase::create();
  std::ifstream read;
  read.exceptions(std::ifstream::failbit | std::ifstream::badbit
                  | std::ios::eofbit);
  read.open(path.c_str(), std::ios::binary);
  db2->read(read);
  auto aps = db2->getChip()
                 ->getBlock()
                 ->findInst("i1")
                 ->findITerm("a")
                 ->getPrefAccessPoints();
  EXPECT_EQ(aps.size(), 1);
  ap = aps[0];
  EXPECT_EQ(ap->getPoint().x(), 10);
  EXPECT_EQ(ap->getPoint().y(), 250);
  EXPECT_EQ(ap->getMPin()->getMTerm()->getName(), "a");
  EXPECT_EQ(ap->getLayer()->getName(), "L1");
  EXPECT_TRUE(ap->hasAccess());
  EXPECT_EQ(ap->getHighType(), dbAccessType::HalfGrid);
  EXPECT_TRUE(ap->hasAccess(dbDirection::DOWN));
  auto path_segs = ap->getSegments();
  EXPECT_EQ(path_segs.size(), 1);
  EXPECT_EQ(std::get<0>(path_segs[0]), Rect(10, 20, 30, 40));
  EXPECT_EQ(std::get<1>(path_segs[0]), true);
  EXPECT_EQ(std::get<2>(path_segs[0]), false);
  std::vector<dbDirection> dirs;
  ap->getAccesses(dirs);
  EXPECT_EQ(dirs.size(), 1);
  EXPECT_EQ(dirs[0], dbDirection::DOWN);
  odb::dbAccessPoint::destroy(ap);
  aps = db2->getChip()
            ->getBlock()
            ->findInst("i1")
            ->findITerm("a")
            ->getPrefAccessPoints();
  EXPECT_EQ(aps.size(), 0);
  dbDatabase::destroy(db2);
}

TEST_F(SimpleDbFixture, SwapMasterClearsAndInvalidatesAccessPoints)
{
  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();
  dbLib* lib = db_->findLib("lib1");
  ASSERT_NE(lib, nullptr);
  dbMaster* old_master = db_->findMaster("and2");
  ASSERT_NE(old_master, nullptr);
  dbMaster* new_master
      = createMaster2X1(lib, "and2_x4", 2000, 1000, "a", "b", "o");
  ASSERT_NE(new_master, nullptr);
  dbMTerm* old_term = old_master->findMTerm("a");
  ASSERT_NE(old_term, nullptr);
  dbMTerm* new_term = new_master->findMTerm("a");
  ASSERT_NE(new_term, nullptr);

  // Create APs on both masters so a stale pin-access index would be visible.
  dbMPin* old_pin = dbMPin::create(old_term);
  ASSERT_NE(old_pin, nullptr);
  dbAccessPoint* old_ap = dbAccessPoint::create(block, old_pin, 0);
  ASSERT_NE(old_ap, nullptr);
  dbMPin* new_pin = dbMPin::create(new_term);
  ASSERT_NE(new_pin, nullptr);
  dbAccessPoint* new_ap = dbAccessPoint::create(block, new_pin, 0);
  ASSERT_NE(new_ap, nullptr);

  dbInst* inst = dbInst::create(block, old_master, "i1");
  ASSERT_NE(inst, nullptr);
  dbITerm* iterm = inst->getITerm(old_term);
  ASSERT_NE(iterm, nullptr);
  inst->setPinAccessIdx(0);
  iterm->setAccessPoint(old_pin, old_ap);
  ASSERT_EQ(iterm->getPrefAccessPoints().size(), 1);
  ASSERT_EQ(iterm->getAccessPoints().at(old_pin).size(), 1);

  ASSERT_TRUE(inst->swapMaster(new_master));
  dbITerm* swapped_iterm = inst->findITerm("a");
  ASSERT_NE(swapped_iterm, nullptr);

  EXPECT_EQ(inst->getPinAccessIdx(), static_cast<uint32_t>(-1));
  EXPECT_TRUE(swapped_iterm->getPrefAccessPoints().empty());
  EXPECT_TRUE(swapped_iterm->getAccessPoints().empty());
}

TEST_F(SimpleDbFixture, BlockClearUnlinksMasterAccessPoints)
{
  createSimpleDB();
  auto block = db_->getChip()->getBlock();
  auto master = db_->findMaster("and2");
  auto term = master->findMTerm("a");
  auto pin = dbMPin::create(term);
  auto ap = dbAccessPoint::create(block, pin, 0);
  dbAccessPoint::create(block, pin, 0);
  auto replacement = dbAccessPoint::create(block, pin, 2);
  for (const auto* name : {"i1", "i2"}) {
    auto inst = dbInst::create(block, master, name);
    inst->setPinAccessIdx(0);
    inst->getITerm(term)->setAccessPoint(pin, ap);
    ASSERT_EQ(inst->getITerm(term)->getPrefAccessPoints().size(), 1);
  }
  for (const auto* name : {"replaced", "cleared"}) {
    auto inst = dbInst::create(block, master, name);
    auto iterm = inst->getITerm(term);
    iterm->setAccessPoint(pin, ap);
    iterm->setAccessPoint(pin, replacement);
    if (std::string(name) == "cleared") {
      iterm->setAccessPoint(pin, nullptr);
    }
    dbInst::destroy(inst);
  }
  auto net = dbNet::create(block, "n1");
  auto port = dbBTerm::create(net, "IN1");
  auto bpin = dbBPin::create(port);
  dbAccessPoint::create(bpin);
  ASSERT_EQ(block->getAccessPoints().size(), 4);
  ASSERT_EQ(pin->getPinAccess().at(0).size(), 2);
  ASSERT_EQ(pin->getPinAccess().at(2).size(), 1);

  block->clear();

  EXPECT_EQ(db_->getChip()->getBlock(), block);
  EXPECT_TRUE(block->getAccessPoints().empty());
  EXPECT_TRUE(pin->getPinAccess().empty());

  auto inst = dbInst::create(block, master, "i1");
  auto fresh_ap = dbAccessPoint::create(block, pin, 0);
  inst->setPinAccessIdx(0);
  inst->getITerm(term)->setAccessPoint(pin, fresh_ap);
  const auto fresh_access = pin->getPinAccess();
  ASSERT_EQ(fresh_access.size(), 1);
  ASSERT_EQ(fresh_access.at(0).size(), 1);
  EXPECT_EQ(fresh_access.at(0).front(), fresh_ap);
  const auto preferred = inst->getITerm(term)->getPrefAccessPoints();
  ASSERT_EQ(preferred.size(), 1);
  EXPECT_EQ(preferred.front(), fresh_ap);
}

TEST_F(SimpleDbFixture, BlockClearIgnoresDeletedAccessPointOwners)
{
  createSimpleDB();
  auto block = db_->getChip()->getBlock();
  auto master = db_->findMaster("and2");
  auto pin = dbMPin::create(master->findMTerm("a"));
  dbAccessPoint::create(block, pin, 0);
  dbMaster::destroy(master);

  auto lib = dbLib::create(db_.get(), "removed_lib", db_->getTech(), ',');
  auto removed_master
      = createMaster1X1(lib, "removed_master", 1000, 1000, "a", "o");
  auto removed_pin = dbMPin::create(removed_master->findMTerm("a"));
  dbAccessPoint::create(block, removed_pin, 0);
  dbLib::destroy(lib);
  ASSERT_EQ(block->getAccessPoints().size(), 2);

  block->clear();

  EXPECT_TRUE(block->getAccessPoints().empty());
}

}  // namespace
}  // namespace odb
