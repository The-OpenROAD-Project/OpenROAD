#include <memory>

#include "dpl/Opendp.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace dpl {

class OpendpTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    lib_ = loadTechAndLib(
        "tech", "isPlacedTestLibName", "sky130hd/sky130_fd_sc_hd_merged.lef");

    chip_ = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip_, "top");
    block_->setDefUnits(lib_->getTech()->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
  }

  odb::dbLib* lib_;
  odb::dbChip* chip_;
  odb::dbBlock* block_;
};

}  // namespace dpl
