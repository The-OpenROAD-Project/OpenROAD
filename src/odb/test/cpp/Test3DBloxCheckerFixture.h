// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"
#include "tst/fixture.h"

namespace odb {
class CheckerFixture : public tst::Fixture
{
 protected:
  CheckerFixture()
  {
    tech_ = dbTech::create(db_.get(), "tech");
    top_chip_
        = dbChip::create(db_.get(), nullptr, "TopChip", dbChip::ChipType::HIER);

    // Create master chips
    chip1_ = dbChip::create(db_.get(), tech_, "Chip1", dbChip::ChipType::DIE);
    chip1_->setWidth(2000);
    chip1_->setHeight(2000);
    chip1_->setThickness(500);

    chip2_ = dbChip::create(db_.get(), tech_, "Chip2", dbChip::ChipType::DIE);
    chip2_->setWidth(1500);
    chip2_->setHeight(1500);
    chip2_->setThickness(500);

    // Create regions on master chips
    auto r1_fr = dbChipRegion::create(
        chip1_, "r1_fr", dbChipRegion::Side::FRONT, nullptr);
    r1_fr->setBox(Rect(0, 0, 2000, 2000));

    auto r2_bk = dbChipRegion::create(
        chip2_, "r2_bk", dbChipRegion::Side::BACK, nullptr);
    r2_bk->setBox(Rect(0, 0, 1500, 1500));

    auto r2_fr = dbChipRegion::create(
        chip2_, "r2_fr", dbChipRegion::Side::FRONT, nullptr);
    r2_fr->setBox(Rect(0, 0, 1500, 1500));
  }

  void check()
  {
    db_->setTopChip(top_chip_);
    db_->constructUnfoldedModel();
    ThreeDBlox three_dblox(&logger_, db_.get());
    three_dblox.check();
  }

  std::vector<dbMarker*> getMarkers(const char* category_name)
  {
    auto top_cat = top_chip_->findMarkerCategory("3DBlox");
    if (!top_cat) {
      return {};
    }
    auto cat = top_cat->findMarkerCategory(category_name);
    if (!cat) {
      return {};
    }

    std::vector<dbMarker*> markers;
    for (auto* m : cat->getMarkers()) {
      markers.push_back(m);
    }
    return markers;
  }

  dbTech* tech_;
  dbChip* top_chip_;
  dbChip* chip1_;
  dbChip* chip2_;

  static constexpr const char* floating_chips_category = "Floating chips";
  static constexpr const char* overlapping_chips_category = "Overlapping chips";
  static constexpr const char* unused_internal_ext_category
      = "Unused internal_ext";
  static constexpr const char* connected_regions_category
      = "Connection regions";
  static constexpr const char* logical_connectivity_category
      = "Logical Connectivity";
  static constexpr const char* bump_alignment_category = "Bump Alignment";
};

}  // namespace odb
