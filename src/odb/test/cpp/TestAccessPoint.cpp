#define BOOST_TEST_MODULE TestAccessPoint
#include <boost/test/included/unit_test.hpp>
#include <fstream>

#include "env.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

BOOST_AUTO_TEST_SUITE(test_suite)

BOOST_AUTO_TEST_CASE(test_default)
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
  BOOST_TEST(aps.size() == 1);
  ap = aps[0];
  BOOST_TEST(ap->getPoint().x() == 10);
  BOOST_TEST(ap->getPoint().y() == 250);
  BOOST_TEST(ap->getMPin()->getMTerm()->getName() == "a");
  BOOST_TEST(ap->getLayer()->getName() == "L1");
  BOOST_TEST(ap->hasAccess());
  BOOST_TEST(ap->getHighType() == dbAccessType::HalfGrid);
  BOOST_TEST(ap->hasAccess(dbDirection::DOWN));
  auto path_segs = ap->getSegments();
  BOOST_TEST(path_segs.size() == 1);
  BOOST_TEST(std::get<0>(path_segs[0]) == Rect(10, 20, 30, 40));
  BOOST_TEST(std::get<1>(path_segs[0]) == true);
  BOOST_TEST(std::get<2>(path_segs[0]) == false);
  std::vector<dbDirection> dirs;
  ap->getAccesses(dirs);
  BOOST_TEST(dirs.size() == 1);
  BOOST_TEST(dirs[0] == dbDirection::DOWN);
  odb::dbAccessPoint::destroy(ap);
  aps = db2->getChip()
            ->getBlock()
            ->findInst("i1")
            ->findITerm("a")
            ->getPrefAccessPoints();
  BOOST_TEST(aps.size() == 0);
  dbDatabase::destroy(db2);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
