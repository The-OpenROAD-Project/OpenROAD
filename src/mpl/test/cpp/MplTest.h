// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <memory>

#include "../../src/hier_rtlmp.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "tst/fixture.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace mpl {

class MplTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    odb::dbTech::create(db_.get(), "tech");
    odb::dbLib::create(db_.get(), "lib", db_->getTech(), ',');

    odb::dbChip::create(db_.get(), db_->getTech());

    odb::dbBlock::create(db_->getChip(), "block");
    db_->getChip()->getBlock()->setDieArea(
        odb::Rect(0, 0, die_width_, die_height_));
  }

  const int die_width_ = 500000;
  const int die_height_ = 500000;
};

}  // namespace mpl
