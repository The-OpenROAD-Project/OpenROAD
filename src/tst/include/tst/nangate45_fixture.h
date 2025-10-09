// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <memory>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "tst/fixture.h"

namespace odb {

class Nangate45Fixture : public tst::Fixture
{
 protected:
  void SetUp() override
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

}  // namespace odb
