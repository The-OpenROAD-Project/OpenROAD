// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/geom.h"
#include "tst/fixture.h"

namespace odb {
namespace {

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
    ThreeDBlox three_dblox(&logger_, db_.get());
    three_dblox.check();
  }

  std::vector<dbMarker*> getFloatingMarkers()
  {
    auto category = top_chip_->findMarkerCategory("3DBlox");
    if (!category) {
      return {};
    }
    auto float_cat = category->findMarkerCategory("Floating chips");
    if (!float_cat) {
      return {};
    }

    std::vector<dbMarker*> markers;
    for (auto* m : float_cat->getMarkers()) {
      markers.push_back(m);
    }
    return markers;
  }

  std::vector<dbMarker*> getOverlappingMarkers()
  {
    auto category = top_chip_->findMarkerCategory("3DBlox");
    if (!category) {
      return {};
    }
    auto overlap_cat = category->findMarkerCategory("Overlapping chips");
    if (!overlap_cat) {
      return {};
    }

    std::vector<dbMarker*> markers;
    for (auto* m : overlap_cat->getMarkers()) {
      markers.push_back(m);
    }
    return markers;
  }

  dbTech* tech_;
  dbChip* top_chip_;
  dbChip* chip1_;
  dbChip* chip2_;
};

TEST_F(CheckerFixture, test_no_violations)
{
  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));  // Stacked on top
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");

  auto* conn1 = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn1->setThickness(0);

  check();
  EXPECT_TRUE(getFloatingMarkers().empty());
  EXPECT_TRUE(getOverlappingMarkers().empty());
}

TEST_F(CheckerFixture, test_overlapping_chips)
{
  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(500, 500, 0));  // Overlaps in 3D
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  check();
  auto markers = getOverlappingMarkers();
  EXPECT_EQ(markers.size(), 1);

  if (!markers.empty()) {
    auto sources = markers[0]->getSources();
    EXPECT_EQ(sources.size(), 2);
  }
}

TEST_F(CheckerFixture, test_single_floating_chip)
{
  // inst1 and inst2 connected, inst3 floating
  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst3 = dbChipInst::create(top_chip_, chip1_, "inst3");
  inst3->setLoc(Point3D(5000, 5000, 0));  // Floating
  inst3->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn1 = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn1->setThickness(0);

  check();
  auto markers = getFloatingMarkers();
  EXPECT_EQ(markers.size(), 1);

  if (!markers.empty()) {
    auto sources = markers[0]->getSources();
    EXPECT_EQ(sources.size(), 1);
  }
}

TEST_F(CheckerFixture, test_multiple_floating_groups)
{
  // Group 1: inst1 -> inst2 (Connected)
  // Group 2: inst3 (Floating)
  // Group 3: inst4 (Floating)

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst3 = dbChipInst::create(top_chip_, chip1_, "inst3");
  inst3->setLoc(Point3D(5000, 5000, 0));
  inst3->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst4 = dbChipInst::create(top_chip_, chip2_, "inst4");
  inst4->setLoc(Point3D(7000, 7000, 0));
  inst4->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn1 = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn1->setThickness(0);

  check();
  auto markers = getFloatingMarkers();
  EXPECT_EQ(markers.size(), 2);
}

TEST_F(CheckerFixture, test_connectivity_gap)
{
  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(100);  // Expect gap of 100

  // Case 1: Valid connection
  // inst1 top is at 500. inst2 bot should be at 500 + 100 = 600
  inst2->setLoc(Point3D(0, 0, 600));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  check();
  EXPECT_TRUE(getFloatingMarkers().empty());

  // Case 2: Broken connection (gap too large)
  // Move inst2 to 800
  inst2->setLoc(Point3D(0, 0, 800));
  check();
  EXPECT_EQ(getFloatingMarkers().size(), 1);
}

}  // namespace
}  // namespace odb
