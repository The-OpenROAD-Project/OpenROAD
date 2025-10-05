#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "env.h"
#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

TEST(AccessPoint, test_default)
{
  dbDatabase* db;
  db = createSimpleDB();
  auto block = db->getChip()->getBlock();
  auto and2 = db->findMaster("and2");
  auto term = and2->findMTerm("a");
  auto layer = db->getTech()->findLayer("L1");
  auto pin = dbMPin::create(term);
  auto ap = dbAccessPoint::create(block, pin, 0);
  auto inst = dbInst::create(db->getChip()->getBlock(), and2, "i1");
  auto iterm = inst->getITerm(term);
  ap->addSegment(Rect(10, 20, 30, 40), true, false);
  ap->setPoint(Point(10, 250));
  ap->setLayer(layer);
  ap->setHighType(dbAccessType::HalfGrid);
  ap->setAccess(true, dbDirection::DOWN);
  iterm->setAccessPoint(pin, ap);
  std::string path = testTmpPath("results", "TestAccessPointDbRW");
  std::ofstream write;
  write.exceptions(std::ifstream::failbit | std::ifstream::badbit
                   | std::ios::eofbit);
  write.open(path, std::ios::binary);
  db->write(write);
  dbDatabase::destroy(db);

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

}  // namespace
}  // namespace odb
