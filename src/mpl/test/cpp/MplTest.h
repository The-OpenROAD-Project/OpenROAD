#include "../../src/hier_rtlmp.h"
#include "gtest/gtest.h"
#include "mpl/rtl_mp.h"
#include "odb/db.h"
#include "odb/util.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace mpl {

class MplTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    db_ = utl::UniquePtrWithDeleter<odb::dbDatabase>(odb::dbDatabase::create(),
                                                     &odb::dbDatabase::destroy);
    db_->setLogger(&logger_);

    tech_ = utl::UniquePtrWithDeleter<odb::dbTech>(
        odb::dbTech::create(db_.get(), "tech"), &odb::dbTech::destroy);
    lib_ = utl::UniquePtrWithDeleter<odb::dbLib>(
        odb::dbLib::create(db_.get(), "lib", tech_.get(), ','),
        &odb::dbLib::destroy);
    master_ = utl::UniquePtrWithDeleter<odb::dbMaster>(
        odb::dbMaster::create(lib_.get(), "master"), &odb::dbMaster::destroy);
    master_->setType(odb::dbMasterType::CORE);

    chip_ = utl::UniquePtrWithDeleter<odb::dbChip>(
        odb::dbChip::create(db_.get()), &odb::dbChip::destroy);
    // odb::dbBlock::destroy has overloads, so we need to specify which one we
    // are using
    void (*block_destroy)(odb::dbBlock*) = &odb::dbBlock::destroy;
    block_ = utl::UniquePtrWithDeleter<odb::dbBlock>(
        odb::dbBlock::create(chip_.get(), "block"), block_destroy);

    block_->setDieArea(odb::Rect(0, 0, dimension_, dimension_));
    master_->setWidth(dimension_);
    master_->setHeight(dimension_);
    master_->setFrozen();
  }

  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  utl::UniquePtrWithDeleter<odb::dbTech> tech_;
  utl::UniquePtrWithDeleter<odb::dbLib> lib_;
  utl::UniquePtrWithDeleter<odb::dbMaster> master_;
  utl::UniquePtrWithDeleter<odb::dbChip> chip_;
  utl::UniquePtrWithDeleter<odb::dbBlock> block_;
  utl::Logger logger_;
  unsigned int dimension_ = 500000;
};

}  // namespace mpl