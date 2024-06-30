#define BOOST_TEST_MODULE TestMaster
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <string>

#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

BOOST_AUTO_TEST_SUITE(test_suite)

struct F_DEFAULT
{
  F_DEFAULT()
  {
    db = createSimpleDB();
    block = db->getChip()->getBlock();
    lib = db->findLib("lib1");
  }
  ~F_DEFAULT() { dbDatabase::destroy(db); }
  dbDatabase* db;
  dbLib* lib;
  dbBlock* block;
};

BOOST_FIXTURE_TEST_CASE(test_delete_module, F_DEFAULT)
{
  auto and2 = db->findMaster("and2");
  BOOST_TEST(and2 != nullptr);

  auto inst = dbInst::create(db->getChip()->getBlock(), and2, "i1");

  // Can't delete while there is an instance of the master
  BOOST_CHECK_THROW(dbMaster::destroy(and2), std::exception);

  dbInst::destroy(inst);
  dbMaster::destroy(and2);  // now we can

  // It is gone
  for (auto lib : db->getLibs()) {
    for (auto master : lib->getMasters()) {
      BOOST_TEST(master != and2);
    }
  }
}
BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb
