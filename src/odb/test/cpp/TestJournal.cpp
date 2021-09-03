#define BOOST_TEST_MODULE TestJournal
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "db.h"
#include "helper.cpp"
#include <string>

using namespace odb;
using namespace std;
BOOST_AUTO_TEST_SUITE(test_suite)

struct F_DEFAULT {
  F_DEFAULT()
  {
    db        = createSimpleDB();
    block     = db->getChip()->getBlock();
    lib       = db->findLib("lib1");
  }
  ~F_DEFAULT()
  {
    dbDatabase::destroy(db);
  }
  dbDatabase* db;
  dbLib*      lib;
  dbBlock*    block;
  std::string journal_path = std::string(std::getenv("BASE_DIR")) + "/results/db_jounal.eco";
};
BOOST_FIXTURE_TEST_CASE(test_write, F_DEFAULT)
{
  db->beginEco(block);
  auto i1 = odb::dbInst::create(block, lib->findMaster("and2"), "i1");
  auto i2 = odb::dbInst::create(block, lib->findMaster("and2"), "i2");
  auto n1 = odb::dbNet::create(block, "n1");
  auto n2 = odb::dbNet::create(block, "n2");
  odb::dbBTerm::create(n1, "b1");
  auto b2 = odb::dbBTerm::create(n1, "b2");
  auto b3 = odb::dbBTerm::create(n1, "b3");
  odb::dbITerm::connect(i1->findITerm("a"), n1);
  b2->disconnect();
  i1->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  i1->swapMaster(lib->findMaster("or2"));
  odb::dbBTerm::destroy(b3);
  odb::dbInst::destroy(i2);
  odb::dbNet::destroy(n2);
  db->endEco(block);
  db->writeEco(block, journal_path.c_str());
}
BOOST_FIXTURE_TEST_CASE(test_read, F_DEFAULT)
{
  db->beginEco(block);
  db->readEco(block,journal_path.c_str());
  db->commitEco(block);
  auto i1 = block->findInst("i1");
  auto n1 = block->findNet("n1");
  auto b1 = block->findBTerm("b1");
  auto b2 = block->findBTerm("b2");
  // test journal redo CREATE_OBJECT
  BOOST_TEST(i1 != nullptr);
  BOOST_TEST(n1 != nullptr);
  BOOST_TEST(b1 != nullptr);
  BOOST_TEST(b2 != nullptr);
  // test journal redo DELETE_OBJECT
  BOOST_TEST(block->findBTerm("i2") == nullptr);
  BOOST_TEST(block->findBTerm("b3") == nullptr);
  BOOST_TEST(block->findBTerm("n2") == nullptr);
  // test journal redo UPDATE_FIELD
  BOOST_TEST(i1->getPlacementStatus() == dbPlacementStatus::PLACED);
  // test journal redo SWAP_OBJECT
  BOOST_TEST(i1->getMaster()->getName() == "or2");
  // test journal redo CONNECT_OBJECT
  BOOST_TEST(b1->getNet() == n1);
  BOOST_TEST(i1->findITerm("a")->getNet() == n1);
  // test journal redo DISCONNECT_OBJECT
  BOOST_TEST(b2->getNet() == nullptr);
}
BOOST_AUTO_TEST_SUITE_END()
