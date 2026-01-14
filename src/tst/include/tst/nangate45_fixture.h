// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <memory>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "tst/fixture.h"

namespace tst {

class Nangate45Fixture : public tst::Fixture
{
 protected:
  Nangate45Fixture()
  {
    lib_ = loadTechAndLib("ng45", "ng45", "_main/test/Nangate45/Nangate45.lef");
    chip_ = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip_, "top");
    block_->setDefUnits(lib_->getTech()->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
  }

  odb::dbLib* lib_;
  odb::dbChip* chip_;
  odb::dbBlock* block_;
};

}  // namespace tst
