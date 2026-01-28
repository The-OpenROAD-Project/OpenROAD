#include <string>

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
    // Create a top chip
    top_chip_
        = dbChip::create(db_.get(), nullptr, "TopChip", dbChip::ChipType::HIER);
    top_chip_->setWidth(10000);
    top_chip_->setHeight(10000);
    top_chip_->setThickness(1000);
    // Create master chips
    chip1_ = dbChip::create(db_.get(), tech_, "Chip1", dbChip::ChipType::DIE);
    chip1_->setWidth(2000);
    chip1_->setHeight(2000);
    chip1_->setThickness(500);

    chip2_ = dbChip::create(db_.get(), tech_, "Chip2", dbChip::ChipType::DIE);
    chip2_->setWidth(1500);
    chip2_->setHeight(1500);
    chip2_->setThickness(500);

    chip3_ = dbChip::create(db_.get(), tech_, "Chip3", dbChip::ChipType::DIE);
    chip3_->setWidth(1000);
    chip3_->setHeight(1000);
    chip3_->setThickness(500);
  }

  dbTech* tech_;
  dbChip* top_chip_;
  dbChip* chip1_;
  dbChip* chip2_;
  dbChip* chip3_;
};

TEST_F(CheckerFixture, test_overlapping_chips)
{
  // Create two overlapping chip instances
  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  // Place inst2 overlapping with inst1
  inst2->setLoc(Point3D(1000, 1000, 0));  // Overlaps with inst1
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  // Run checker
  ThreeDBlox three_dblox(&logger_, db_.get());
  three_dblox.check();

  // Verify markers were created
  auto category = top_chip_->findMarkerCategory("3DBlox");
  ASSERT_NE(category, nullptr);

  auto overlapping_category = category->findMarkerCategory("Overlapping chips");
  ASSERT_NE(overlapping_category, nullptr);

  auto markers = overlapping_category->getMarkers();
  EXPECT_EQ(markers.size(), 1);

  if (!markers.empty()) {
    auto marker = *markers.begin();
    auto sources = marker->getSources();
    EXPECT_EQ(sources.size(), 2);
    EXPECT_EQ(marker->getBBox(), odb::Rect(1000, 1000, 2000, 2000));
    // Verify both chip instances are in the sources
    bool found_inst1 = false;
    bool found_inst2 = false;
    for (auto src : sources) {
      if (src->getObjectType() == dbObjectType::dbChipInstObj) {
        auto chip_inst = static_cast<dbChipInst*>(src);
        if (chip_inst->getName() == "inst1") {
          found_inst1 = true;
        }
        if (chip_inst->getName() == "inst2") {
          found_inst2 = true;
        }
      }
    }
    EXPECT_TRUE(found_inst1);
    EXPECT_TRUE(found_inst2);
  }
}

TEST_F(CheckerFixture, test_floating_chips)
{
  // Create three chip instances: two connected, one floating
  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  // Place inst2 touching inst1 (connected)
  inst2->setLoc(Point3D(0, 0, 500));  // Stacked on top of inst1
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst3 = dbChipInst::create(top_chip_, chip3_, "inst3");
  // Place inst3 far away (floating)
  inst3->setLoc(Point3D(5000, 5000, 0));
  inst3->setOrient(dbOrientType3D(dbOrientType::R0, false));

  // Run checker
  ThreeDBlox three_dblox(&logger_, db_.get());
  three_dblox.check();

  // Verify markers were created
  auto category = top_chip_->findMarkerCategory("3DBlox");
  ASSERT_NE(category, nullptr);

  auto floating_category = category->findMarkerCategory("Floating chips");
  ASSERT_NE(floating_category, nullptr);

  auto markers = floating_category->getMarkers();
  EXPECT_EQ(markers.size(), 1);

  if (!markers.empty()) {
    auto marker = *markers.begin();
    auto sources = marker->getSources();
    EXPECT_EQ(sources.size(), 1);
    // Verify the floating chip is inst3
    if (!sources.empty()) {
      auto src = *sources.begin();
      EXPECT_EQ(src->getObjectType(), dbObjectType::dbChipInstObj);
      auto chip_inst = static_cast<dbChipInst*>(src);
      EXPECT_EQ(chip_inst->getName(), "inst3");
    }
  }
}

TEST_F(CheckerFixture, test_no_violations)
{
  // Create two non-overlapping, connected chip instances
  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  // Place inst2 touching but not overlapping inst1
  inst2->setLoc(Point3D(0, 0, 500));  // Stacked on top
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  // Run checker
  ThreeDBlox three_dblox(&logger_, db_.get());
  three_dblox.check();

  // Verify no violation markers were created
  auto category = top_chip_->findMarkerCategory("3DBlox");
  ASSERT_NE(category, nullptr);

  auto overlapping_category = category->findMarkerCategory("Overlapping chips");
  if (overlapping_category != nullptr) {
    auto markers = overlapping_category->getMarkers();
    EXPECT_EQ(markers.size(), 0);
  }

  auto floating_category = category->findMarkerCategory("Floating chips");
  if (floating_category != nullptr) {
    auto markers = floating_category->getMarkers();
    EXPECT_EQ(markers.size(), 0);
  }
}

TEST_F(CheckerFixture, test_multiple_violations)
{
  // Create a complex scenario with both overlapping and floating chips
  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  // Overlapping with inst1
  inst2->setLoc(Point3D(500, 500, 0));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst3 = dbChipInst::create(top_chip_, chip3_, "inst3");
  // Also overlapping with inst1
  inst3->setLoc(Point3D(1000, 1000, 0));
  inst3->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst4 = dbChipInst::create(top_chip_, chip3_, "inst4");
  // Floating chip
  inst4->setLoc(Point3D(7000, 7000, 0));
  inst4->setOrient(dbOrientType3D(dbOrientType::R0, false));

  // Run checker
  ThreeDBlox three_dblox(&logger_, db_.get());
  three_dblox.check();

  // Verify markers were created
  auto category = top_chip_->findMarkerCategory("3DBlox");
  ASSERT_NE(category, nullptr);

  // Check overlapping chips
  auto overlapping_category = category->findMarkerCategory("Overlapping chips");
  ASSERT_NE(overlapping_category, nullptr);
  auto overlapping_markers = overlapping_category->getMarkers();
  EXPECT_GT(overlapping_markers.size(), 0);

  // Check floating chips
  auto floating_category = category->findMarkerCategory("Floating chips");
  ASSERT_NE(floating_category, nullptr);
  auto floating_markers = floating_category->getMarkers();
  EXPECT_EQ(floating_markers.size(), 1);
}

}  // namespace
}  // namespace odb
