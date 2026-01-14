// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <memory>

#include "odb/db.h"
#include "tst/fixture.h"

namespace tst {
class Sky130Fixture : public tst::Fixture
{
 protected:
  Sky130Fixture()
  {
    lib_ = loadTechAndLib(
        "sky130", "sky130", "_main/test/sky130hd/sky130_fd_sc_hd_merged.lef");

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
