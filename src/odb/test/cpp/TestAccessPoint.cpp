#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "../../src/db/dbAccessPoint.h"
#include "../../src/db/dbMPin.h"
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

TEST_F(SimpleDbFixture, clear_mpin_access_points_is_idempotent)
{
  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();
  dbMaster* master = db_->findMaster("and2");
  dbMTerm* term = master->findMTerm("a");
  dbMPin* pin = dbMPin::create(term);
  dbAccessPoint* ap = dbAccessPoint::create(block, pin, 0);
  dbInst* inst = dbInst::create(block, master, "i1");
  dbITerm* iterm = inst->getITerm(term);
  iterm->setAccessPoint(pin, ap);

  master->clearPinAccess(0);
  master->clearPinAccess(0);

  EXPECT_TRUE(iterm->getPrefAccessPoints().empty());
}

TEST_F(SimpleDbFixture, clear_mpin_access_points_removes_duplicate_references)
{
  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();
  dbMaster* master = db_->findMaster("and2");
  dbMTerm* term = master->findMTerm("a");
  dbMPin* pin = dbMPin::create(term);
  dbAccessPoint* ap = dbAccessPoint::create(block, pin, 0);
  _dbMPin* pin_impl = static_cast<_dbMPin*>(pin->getImpl());

  // Reproduce duplicate AP bookkeeping that can be left by router pin access
  // updates before a master pin-access slot is cleared.
  pin_impl->aps_[0].push_back(ap->getImpl()->getOID());
  ASSERT_EQ(pin_impl->aps_[0].size(), 2);

  master->clearPinAccess(0);

  EXPECT_TRUE(pin_impl->aps_[0].empty());
}

TEST_F(SimpleDbFixture, destroy_inst_removes_access_point_back_references)
{
  createSimpleDB();
  dbBlock* block = db_->getChip()->getBlock();
  dbMaster* master = db_->findMaster("and2");
  dbMTerm* term = master->findMTerm("a");
  dbMPin* pin = dbMPin::create(term);
  dbAccessPoint* ap = dbAccessPoint::create(block, pin, 1);
  dbInst* inst = dbInst::create(block, master, "i1");
  dbITerm* iterm = inst->getITerm(term);
  iterm->setAccessPoint(pin, ap);
  inst->setPinAccessIdx(0);
  _dbAccessPoint* ap_impl = static_cast<_dbAccessPoint*>(ap->getImpl());
  ASSERT_EQ(ap_impl->iterms_.size(), 1);

  dbInst::destroy(inst);

  EXPECT_TRUE(ap_impl->iterms_.empty());
}

}  // namespace
}  // namespace odb
