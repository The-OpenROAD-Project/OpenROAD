#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

TEST_F(SimpleDbFixture, test_default)
{
  createSimpleDB();
  auto block = db_->getChip()->getBlock();
  auto l1 = db_->getTech()->findLayer("L1");
  dbGCellGrid* grid = dbGCellGrid::create(block);
  grid->addGridPatternX(0, 20, 10);  // 0 10 20 30 40 50 ... 190
  grid->addGridPatternY(0, 20, 10);  // 0 10 20 30 40 50 ... 190

  EXPECT_EQ(grid->getXIdx(-1), 0);
  EXPECT_EQ(grid->getXIdx(0), 0);
  EXPECT_EQ(grid->getXIdx(5), 0);
  EXPECT_EQ(grid->getXIdx(15), 1);
  EXPECT_EQ(grid->getXIdx(210), 19);
  EXPECT_EQ(grid->getCapacity(l1, 0, 0), 0);
  grid->setCapacity(l1, 0, 0, 20);
  EXPECT_EQ(grid->getCapacity(l1, 0, 0), 20);
  grid->addGridPatternX(30, 20, 10);
  EXPECT_EQ(grid->getCapacity(l1, 0, 0), 0);
}

}  // namespace
}  // namespace odb
