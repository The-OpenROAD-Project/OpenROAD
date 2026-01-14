#include <exception>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

TEST_F(SimpleDbFixture, test_delete_module)
{
  createSimpleDB();

  auto and2 = db_->findMaster("and2");
  ASSERT_NE(and2, nullptr);

  auto inst = dbInst::create(db_->getChip()->getBlock(), and2, "i1");

  // Can't delete while there is an instance of the master
  EXPECT_THROW(dbMaster::destroy(and2), std::exception);

  dbInst::destroy(inst);
  dbMaster::destroy(and2);  // now we can

  // It is gone
  for (auto lib : db_->getLibs()) {
    for (auto master : lib->getMasters()) {
      EXPECT_NE(master, and2);
    }
  }
}

}  // namespace
}  // namespace odb
