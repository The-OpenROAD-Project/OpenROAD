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

    tech_ = odb::dbTech::create(db_.get(), "tech");
    lib_ = odb::dbLib::create(db_.get(), "lib", tech_, ',');

    chip_ = odb::dbChip::create(db_.get());

    block_ = odb::dbBlock::create(chip_, "block");
    block_->setDieArea(odb::Rect(0, 0, die_width_, die_height_));
  }

  std::unique_ptr<utl::Logger> logger_;
  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  odb::dbTech* tech_;
  odb::dbLib* lib_;
  odb::dbChip* chip_;
  odb::dbBlock* block_;
  const int die_width_ = 500000;
  const int die_height_ = 500000;
};

}  // namespace mpl