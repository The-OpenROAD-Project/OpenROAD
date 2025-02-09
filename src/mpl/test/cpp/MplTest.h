#include "../../src/hier_rtlmp.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace mpl {

class MplTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    logger_ = std::make_unique<utl::Logger>();
    db_ = utl::UniquePtrWithDeleter<odb::dbDatabase>(odb::dbDatabase::create(),
                                                     &odb::dbDatabase::destroy);
    db_->setLogger(logger_.get());

    odb::dbTech::create(db_.get(), "tech");
    odb::dbLib::create(db_.get(), "lib", db_->getTech(), ',');

    odb::dbChip::create(db_.get());

    odb::dbBlock::create(db_->getChip(), "block");
    db_->getChip()->getBlock()->setDieArea(
        odb::Rect(0, 0, die_width_, die_height_));
  }

  std::unique_ptr<utl::Logger> logger_;
  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  const int die_width_ = 500000;
  const int die_height_ = 500000;
};

}  // namespace mpl