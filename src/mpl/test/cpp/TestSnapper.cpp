// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <exception>
#include <memory>
#include <string>

#include "MplTest.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "snapper.h"

namespace mpl {
namespace {

class TestSnapper : public MplTest
{
 protected:
  void SetUp() override
  {
    MplTest::SetUp();
    snapper_ = std::make_unique<Snapper>(&logger_);
  }

  // A core row of site_width x site_height sites with its origin at
  // (row_x, row_y).
  void createCoreRow(const int site_width,
                     const int site_height,
                     const int row_x,
                     const int row_y)
  {
    odb::dbSite* site = odb::dbSite::create(db_->findLib("lib"), "core");
    site->setWidth(site_width);
    site->setHeight(site_height);
    site->setClass(odb::dbSiteClass::CORE);
    odb::dbRow::create(db_->getChip()->getBlock(),
                       "row",
                       site,
                       row_x,
                       row_y,
                       odb::dbOrientType::R0,
                       odb::dbRowDir::HORIZONTAL,
                       die_width_ / site_width - 1,
                       site_width);
  }

  // A 10000 x 10000 block macro with one signal pin spanning
  // [0, pin_size] in both directions on the given layer, if any.
  odb::dbInst* createMacro(odb::dbTechLayer* pin_layer = nullptr,
                           const int pin_size = 0)
  {
    odb::dbMaster* macro_master
        = odb::dbMaster::create(db_->findLib("lib"), "macro_master");
    macro_master->setHeight(10000);
    macro_master->setWidth(10000);
    macro_master->setType(odb::dbMasterType::BLOCK);
    if (pin_layer != nullptr) {
      odb::dbMTerm* mterm = odb::dbMTerm::create(
          macro_master, "pin", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
      odb::dbMPin* mpin = odb::dbMPin::create(mterm);
      odb::dbBox::create(mpin, pin_layer, 0, 0, pin_size, pin_size);
    }
    macro_master->setFrozen();

    return odb::dbInst::create(
        db_->getChip()->getBlock(), macro_master, "macro");
  }

  odb::dbTechLayer* createHorizontalLayer(const int track_pitch)
  {
    odb::dbTechLayer* layer = odb::dbTechLayer::create(
        db_->getTech(), "H1", odb::dbTechLayerType::DEFAULT);
    layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);
    odb::dbTrackGrid* track
        = odb::dbTrackGrid::create(db_->getChip()->getBlock(), layer);
    track->addGridPatternY(0, die_height_ / track_pitch, track_pitch);
    return layer;
  }

  std::unique_ptr<Snapper> snapper_;
};

// Snaps both directions to the manucfaturing grid, choosing the closest
// position
TEST_F(TestSnapper, ManufacturingGrid)
{
  db_->getTech()->setManufacturingGrid(10);

  odb::dbMaster* macro_master
      = odb::dbMaster::create(db_->findLib("lib"), "macro_master");
  macro_master->setFrozen();

  odb::dbInst* macro
      = odb::dbInst::create(db_->getChip()->getBlock(), macro_master, "macro");
  macro->setOrigin(1014, 1019);
  snapper_->setMacro(macro);
  snapper_->snapMacro();
  // Closest grid position to 1014
  EXPECT_EQ(macro->getOrigin().x(), 1010);
  // Closest grid position to 1019
  EXPECT_EQ(macro->getOrigin().y(), 1020);
}

// Snaps a macro using one horizontal layer, aligning one pin
TEST_F(TestSnapper, SingleLayer)
{
  db_->getTech()->setManufacturingGrid(1);

  const int track_pitch = 36;
  odb::dbTechLayer* horizontal_layer = odb::dbTechLayer::create(
      db_->getTech(), "H1", odb::dbTechLayerType::DEFAULT);
  horizontal_layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track
      = odb::dbTrackGrid::create(db_->getChip()->getBlock(), horizontal_layer);
  track->addGridPatternY(0, die_height_ / track_pitch, track_pitch);

  odb::dbMaster* macro_master
      = odb::dbMaster::create(db_->findLib("lib"), "macro_master");
  macro_master->setHeight(10000);
  macro_master->setWidth(10000);
  macro_master->setType(odb::dbMasterType::BLOCK);

  const int pin_height = 20;
  const int pin_width = 50;

  const int pin_x = 0;
  const int pin_y = 0;
  odb::dbMTerm* mterm = odb::dbMTerm::create(
      macro_master, "pin", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin = odb::dbMPin::create(mterm);
  odb::dbBox::create(mpin,
                     horizontal_layer,
                     pin_x,
                     pin_y,
                     pin_x + pin_height,
                     pin_y + pin_width);

  macro_master->setFrozen();

  odb::dbInst* macro
      = odb::dbInst::create(db_->getChip()->getBlock(), macro_master, "macro");
  macro->setOrigin(1500, 1500);

  snapper_->setMacro(macro);
  snapper_->snapMacro();

  // Snapped to manufacturing grid
  EXPECT_EQ(macro->getOrigin().x(), 1500);
  // 1523 (origin) + 25 (pin center) equals 1548 which is
  // a multiple of 36 (pin is aligned to track)
  EXPECT_EQ(macro->getOrigin().y(), 1523);
}

// Snaps a macro with two pins, with pins in different layers but same direction
TEST_F(TestSnapper, DoubleLayer)
{
  db_->getTech()->setManufacturingGrid(1);

  const int track1_pitch = 36;
  odb::dbTechLayer* horizontal_layer1 = odb::dbTechLayer::create(
      db_->getTech(), "H1", odb::dbTechLayerType::DEFAULT);
  horizontal_layer1->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track1
      = odb::dbTrackGrid::create(db_->getChip()->getBlock(), horizontal_layer1);
  track1->addGridPatternY(0, die_height_ / track1_pitch, track1_pitch);

  const int track2_pitch = 48;
  odb::dbTechLayer* horizontal_layer2 = odb::dbTechLayer::create(
      db_->getTech(), "H2", odb::dbTechLayerType::DEFAULT);
  horizontal_layer2->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track2
      = odb::dbTrackGrid::create(db_->getChip()->getBlock(), horizontal_layer2);
  track2->addGridPatternY(0, die_height_ / track2_pitch, track2_pitch);

  odb::dbMaster* macro_master
      = odb::dbMaster::create(db_->findLib("lib"), "macro_master");
  macro_master->setHeight(10000);
  macro_master->setWidth(10000);
  macro_master->setType(odb::dbMasterType::BLOCK);

  const int pin_height = 20;
  const int pin_width = 50;

  const int pin1_x = 0;
  const int pin1_y = 0;
  odb::dbMTerm* mterm1 = odb::dbMTerm::create(
      macro_master, "pin1", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin1 = odb::dbMPin::create(mterm1);
  odb::dbBox::create(mpin1,
                     horizontal_layer1,
                     pin1_x,
                     pin1_y,
                     pin1_x + pin_height,
                     pin1_y + pin_width);

  const int pin2_x = 0;
  const int pin2_y = 48;
  odb::dbMTerm* mterm2 = odb::dbMTerm::create(
      macro_master, "pin2", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin2 = odb::dbMPin::create(mterm2);
  odb::dbBox::create(mpin2,
                     horizontal_layer2,
                     pin2_x,
                     pin2_y,
                     pin2_x + pin_height,
                     pin2_y + pin_width);

  macro_master->setFrozen();

  odb::dbInst* macro
      = odb::dbInst::create(db_->getChip()->getBlock(), macro_master, "macro");
  macro->setOrigin(1500, 1500);

  snapper_->setMacro(macro);
  snapper_->snapMacro();

  // Snapped to manufacturing grid
  EXPECT_EQ(macro->getOrigin().x(), 1500);
  // 1559 (origin) + 25 (pin1 center) equals 1584 which is
  // a multiple of 36 (pin is aligned to track1)
  // 1559 (origin) + 73 (pin2 center) equals 1632 which is
  // a multiple of 48 (pin is aligned to track2)
  EXPECT_EQ(macro->getOrigin().y(), 1559);
}

TEST_F(TestSnapper, MultiPitchPattern)
{
  db_->getTech()->setManufacturingGrid(1);

  const int track_pitch1 = 36;
  const int track_pitch2 = 48;

  odb::dbTechLayer* horizontal_layer = odb::dbTechLayer::create(
      db_->getTech(), "H1", odb::dbTechLayerType::DEFAULT);
  horizontal_layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track
      = odb::dbTrackGrid::create(db_->getChip()->getBlock(), horizontal_layer);
  track->addGridPatternY(0, die_height_ / track_pitch1, track_pitch1);
  track->addGridPatternY(0, die_height_ / track_pitch2, track_pitch2);

  odb::dbMaster* macro_master
      = odb::dbMaster::create(db_->findLib("lib"), "macro_master");
  macro_master->setHeight(10000);
  macro_master->setWidth(10000);
  macro_master->setType(odb::dbMasterType::BLOCK);

  const int pin_height = 20;
  const int pin_width = 50;

  const int pin1_x = 0;
  const int pin1_y = 0;
  // Pins 1 and 2 follow the first pattern
  odb::dbMTerm* mterm1 = odb::dbMTerm::create(
      macro_master, "pin1", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin1 = odb::dbMPin::create(mterm1);
  odb::dbBox::create(mpin1,
                     horizontal_layer,
                     pin1_x,
                     pin1_y,
                     pin1_x + pin_height,
                     pin1_y + pin_width);

  const int pin2_x = 0;
  const int pin2_y = 72;
  odb::dbMTerm* mterm2 = odb::dbMTerm::create(
      macro_master, "pin2", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin2 = odb::dbMPin::create(mterm2);
  odb::dbBox::create(mpin2,
                     horizontal_layer,
                     pin2_x,
                     pin2_y,
                     pin2_x + pin_height,
                     pin2_y + pin_width);

  // Pins 3 and 4 follow the second pattern
  const int pin3_x = 0;
  const int pin3_y = 216;
  odb::dbMTerm* mterm3 = odb::dbMTerm::create(
      macro_master, "pin3", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin3 = odb::dbMPin::create(mterm3);
  odb::dbBox::create(mpin3,
                     horizontal_layer,
                     pin3_x,
                     pin3_y,
                     pin3_x + pin_height,
                     pin3_y + pin_width);

  const int pin4_x = 0;
  const int pin4_y = 312;
  odb::dbMTerm* mterm4 = odb::dbMTerm::create(
      macro_master, "pin4", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin4 = odb::dbMPin::create(mterm4);
  odb::dbBox::create(mpin4,
                     horizontal_layer,
                     pin4_x,
                     pin4_y,
                     pin4_x + pin_height,
                     pin4_y + pin_width);

  macro_master->setFrozen();

  odb::dbInst* macro
      = odb::dbInst::create(db_->getChip()->getBlock(), macro_master, "macro");
  macro->setOrigin(1500, 1500);

  snapper_->setMacro(macro);
  snapper_->snapMacro();

  EXPECT_EQ(macro->getOrigin().x(), 1500);
  // 1487 (origin) + 25 (pin1 center) equals 1512 which is
  // a multiple of 36 (pin is aligned to pattern1)
  // 1487 (origin) + 241 (pin3 center) equals 1728 which is
  // a multiple of 48 (pin is aligned to pattern2)
  EXPECT_EQ(macro->getOrigin().y(), 1487);
}

TEST_F(TestSnapper, MultiOriginPattern)
{
  db_->getTech()->setManufacturingGrid(1);

  const int track_pitch = 36;
  const int track_origin1 = 5;
  const int track_origin2 = 7;

  odb::dbTechLayer* horizontal_layer = odb::dbTechLayer::create(
      db_->getTech(), "H1", odb::dbTechLayerType::DEFAULT);
  horizontal_layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track
      = odb::dbTrackGrid::create(db_->getChip()->getBlock(), horizontal_layer);
  track->addGridPatternY(track_origin1, die_height_ / track_pitch, track_pitch);
  track->addGridPatternY(track_origin2, die_height_ / track_pitch, track_pitch);

  odb::dbMaster* macro_master
      = odb::dbMaster::create(db_->findLib("lib"), "macro_master");
  macro_master->setHeight(10000);
  macro_master->setWidth(10000);
  macro_master->setType(odb::dbMasterType::BLOCK);

  const int pin_height = 20;
  const int pin_width = 50;

  const int pin1_x = 0;
  const int pin1_y = 0;
  // Pins 1 and 2 follow the first pattern
  odb::dbMTerm* mterm1 = odb::dbMTerm::create(
      macro_master, "pin1", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin1 = odb::dbMPin::create(mterm1);
  odb::dbBox::create(mpin1,
                     horizontal_layer,
                     pin1_x,
                     pin1_y,
                     pin1_x + pin_height,
                     pin1_y + pin_width);

  const int pin2_x = 0;
  const int pin2_y = 72;
  odb::dbMTerm* mterm2 = odb::dbMTerm::create(
      macro_master, "pin2", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin2 = odb::dbMPin::create(mterm2);
  odb::dbBox::create(mpin2,
                     horizontal_layer,
                     pin2_x,
                     pin2_y,
                     pin2_x + pin_height,
                     pin2_y + pin_width);

  // Pins 3 and 4 follow the second pattern
  const int pin3_x = 0;
  const int pin3_y = 146;
  odb::dbMTerm* mterm3 = odb::dbMTerm::create(
      macro_master, "pin3", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin3 = odb::dbMPin::create(mterm3);
  odb::dbBox::create(mpin3,
                     horizontal_layer,
                     pin3_x,
                     pin3_y,
                     pin3_x + pin_height,
                     pin3_y + pin_width);

  const int pin4_x = 0;
  const int pin4_y = 218;
  odb::dbMTerm* mterm4 = odb::dbMTerm::create(
      macro_master, "pin4", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin4 = odb::dbMPin::create(mterm4);
  odb::dbBox::create(mpin4,
                     horizontal_layer,
                     pin4_x,
                     pin4_y,
                     pin4_x + pin_height,
                     pin4_y + pin_width);

  macro_master->setFrozen();

  odb::dbInst* macro
      = odb::dbInst::create(db_->getChip()->getBlock(), macro_master, "macro");
  macro->setOrigin(1500, 1500);

  snapper_->setMacro(macro);
  snapper_->snapMacro();

  EXPECT_EQ(macro->getOrigin().x(), 1500);
  // 1528 (origin) + 25 (pin1 center) - 5 (pattern1 origin) equals 1548 which is
  // a multiple of 36 (pin is aligned to pattern1)
  // 1528 (origin) + 171 (pin3 center) - 7 (pattern2 origin) equals 1692 which
  // is a multiple of 36 (pin is aligned to pattern2)
  EXPECT_EQ(macro->getOrigin().y(), 1528);
}

// Snaps a macro using one horizontal layer, aligning one pin while the other is
// unalignable
TEST_F(TestSnapper, SingleLayerWithUnalignablePin)
{
  db_->getTech()->setManufacturingGrid(1);

  const int track_pitch = 36;
  odb::dbTechLayer* horizontal_layer = odb::dbTechLayer::create(
      db_->getTech(), "H1", odb::dbTechLayerType::DEFAULT);
  horizontal_layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track
      = odb::dbTrackGrid::create(db_->getChip()->getBlock(), horizontal_layer);
  track->addGridPatternY(0, die_height_ / track_pitch, track_pitch);

  odb::dbMaster* macro_master
      = odb::dbMaster::create(db_->findLib("lib"), "macro_master");
  macro_master->setHeight(10000);
  macro_master->setWidth(10000);
  macro_master->setType(odb::dbMasterType::BLOCK);

  const int pin_height = 20;
  const int pin_width = 50;

  const int pin1_x = 0;
  const int pin1_y = 0;
  odb::dbMTerm* mterm1 = odb::dbMTerm::create(
      macro_master, "pin1", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin1 = odb::dbMPin::create(mterm1);
  odb::dbBox::create(mpin1,
                     horizontal_layer,
                     pin1_x,
                     pin1_y,
                     pin1_x + pin_height,
                     pin1_y + pin_width);

  const int pin2_x = 0;
  const int pin2_y = 50;
  odb::dbMTerm* mterm2 = odb::dbMTerm::create(
      macro_master, "pin2", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin2 = odb::dbMPin::create(mterm2);
  odb::dbBox::create(mpin2,
                     horizontal_layer,
                     pin2_x,
                     pin2_y,
                     pin2_x + pin_height,
                     pin2_y + pin_width);

  macro_master->setFrozen();

  odb::dbInst* macro
      = odb::dbInst::create(db_->getChip()->getBlock(), macro_master, "macro");
  macro->setOrigin(1500, 1500);

  snapper_->setMacro(macro);

  testing::internal::CaptureStdout();
  snapper_->snapMacro();
  std::string snap_warning = testing::internal::GetCapturedStdout();

  std::string expected_warning
      = "[WARNING MPL-0002] Could not align all pins of the macro macro to the "
        "track-grid. 1 out of 2 pins were aligned.\n";
  // Snapped to manufacturing grid
  EXPECT_EQ(macro->getOrigin().x(), 1500);
  // 1523 (origin) + 25 (pin center) equals 1548 which is
  // a multiple of 36 (pin is aligned to track)
  EXPECT_EQ(macro->getOrigin().y(), 1523);
  EXPECT_EQ(snap_warning, expected_warning);
}

// Snaps a macro using one horizontal layer, aligning one pin while the other is
// unalignable and the layer is RightWayOnlyGrid, which should throw an
// exception if a pin is unaligned
TEST_F(TestSnapper, SingleLayerWithUnalignablePinThrowsException)
{
  db_->getTech()->setManufacturingGrid(1);

  const int track_pitch = 36;
  odb::dbTechLayer* horizontal_layer = odb::dbTechLayer::create(
      db_->getTech(), "H1", odb::dbTechLayerType::DEFAULT);
  horizontal_layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  horizontal_layer->setRightWayOnGridOnly(true);

  odb::dbTrackGrid* track
      = odb::dbTrackGrid::create(db_->getChip()->getBlock(), horizontal_layer);
  track->addGridPatternY(0, die_height_ / track_pitch, track_pitch);

  odb::dbMaster* macro_master
      = odb::dbMaster::create(db_->findLib("lib"), "macro_master");
  macro_master->setHeight(10000);
  macro_master->setWidth(10000);
  macro_master->setType(odb::dbMasterType::BLOCK);

  const int pin_height = 20;
  const int pin_width = 50;

  const int pin1_x = 0;
  const int pin1_y = 0;
  odb::dbMTerm* mterm1 = odb::dbMTerm::create(
      macro_master, "pin1", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin1 = odb::dbMPin::create(mterm1);
  odb::dbBox::create(mpin1,
                     horizontal_layer,
                     pin1_x,
                     pin1_y,
                     pin1_x + pin_height,
                     pin1_y + pin_width);

  const int pin2_x = 0;
  const int pin2_y = 50;
  odb::dbMTerm* mterm2 = odb::dbMTerm::create(
      macro_master, "pin2", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin2 = odb::dbMPin::create(mterm2);
  odb::dbBox::create(mpin2,
                     horizontal_layer,
                     pin2_x,
                     pin2_y,
                     pin2_x + pin_height,
                     pin2_y + pin_width);

  macro_master->setFrozen();

  odb::dbInst* macro
      = odb::dbInst::create(db_->getChip()->getBlock(), macro_master, "macro");
  macro->setOrigin(1500, 1500);

  snapper_->setMacro(macro);

  try {
    snapper_->snapMacro();
    FAIL() << "Expected exception MPL-0005";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "MPL-0005");
  } catch (...) {
    FAIL() << "Unexpected exception (other than MPL-0005)";
  }
}

// With core rows, both directions snap to the closest position on the site
// grid instead of the manufacturing grid
TEST_F(TestSnapper, SiteGrid)
{
  db_->getTech()->setManufacturingGrid(1);
  createCoreRow(54, 270, 540, 270);

  odb::dbInst* macro = createMacro();
  macro->setOrigin(1014, 1019);
  snapper_->setMacro(macro);
  snapper_->snapMacro();

  // Closest multiple of 54 to 1014
  EXPECT_EQ(macro->getOrigin().x(), 1026);
  // Closest multiple of 270 to 1019
  EXPECT_EQ(macro->getOrigin().y(), 1080);
}

// The site grid is measured from the rows' origin: an unmirrored macro's own
// grid lines up with it at a whole number of sites, a mirrored one's at twice
// the rows' offset from the grid
TEST_F(TestSnapper, SiteGridMirrored)
{
  db_->getTech()->setManufacturingGrid(1);
  createCoreRow(54, 270, 10, 0);

  odb::dbInst* macro = createMacro();
  macro->setOrigin(1014, 1080);
  snapper_->setMacro(macro);
  snapper_->snapMacro();
  // Closest multiple of 54 to 1014
  EXPECT_EQ(macro->getOrigin().x(), 1026);

  // Mirrored, the origin is the macro's right edge
  macro->setOrient(odb::dbOrientType::MY);
  macro->setOrigin(11014, 1080);
  snapper_->snapMacro();
  // Closest position to 11014 that is 20 (twice the rows' offset of 10) past a
  // multiple of 54
  EXPECT_EQ(macro->getOrigin().x(), 11036);
}

// Among the site positions, the closest one that aligns the pin to the track
// is chosen
TEST_F(TestSnapper, SiteGridWithTrack)
{
  db_->getTech()->setManufacturingGrid(1);
  createCoreRow(54, 270, 0, 0);
  odb::dbTechLayer* layer = createHorizontalLayer(36);

  // Pin center at 18 from the origin
  odb::dbInst* macro = createMacro(layer, 36);
  macro->setOrigin(1512, 1500);
  snapper_->setMacro(macro);
  snapper_->snapMacro();

  // 1620 (6 rows) is the closest site position but puts the pin center at
  // 1638, off the track; 1350 (5 rows) puts it at 1368 = 38 * 36
  EXPECT_EQ(macro->getOrigin().y(), 1350);
  // No vertical pins, so the closest multiple of 54
  EXPECT_EQ(macro->getOrigin().x(), 1512);
}

// A macro whose pins can't be aligned from any site position wasn't built on
// the rows, so only its pins are aligned, as without rows
TEST_F(TestSnapper, SiteGridIncompatiblePins)
{
  db_->getTech()->setManufacturingGrid(1);
  createCoreRow(54, 270, 0, 0);
  odb::dbTechLayer* layer = createHorizontalLayer(36);

  // Pin center at 25 from the origin: 270 * k + 25 is never a multiple of 36
  odb::dbInst* macro = createMacro(layer, 50);
  macro->setOrigin(1512, 1500);
  snapper_->setMacro(macro);
  snapper_->snapMacro();

  // 1523 (origin) + 25 (pin center) equals 1548 which is a multiple of 36
  EXPECT_EQ(macro->getOrigin().y(), 1523);
  // No vertical pins, so the closest multiple of 54
  EXPECT_EQ(macro->getOrigin().x(), 1512);
}

// Site positions that would move the macro out of the core are skipped
TEST_F(TestSnapper, SiteGridKeepsMacroInCore)
{
  db_->getTech()->setManufacturingGrid(1);
  createCoreRow(54, 270, 0, 0);
  odb::dbBlock* block = db_->getChip()->getBlock();
  block->setCoreArea(odb::Rect(0, 0, 10800, 10800));

  odb::dbInst* macro = createMacro();
  macro->setOrigin(800, 800);
  snapper_->setMacro(macro);
  snapper_->snapMacro();

  // 810, the closest multiple of 54 and of 270, would put the 10000-wide
  // macro's far edge at 10810, past the core
  EXPECT_EQ(macro->getOrigin().x(), 756);
  EXPECT_EQ(macro->getOrigin().y(), 540);
  EXPECT_TRUE(block->getCoreArea().contains(macro->getBBox()->getBox()));
}

// Site positions that would move the macro onto another macro are skipped
TEST_F(TestSnapper, SiteGridAvoidsOverlap)
{
  db_->getTech()->setManufacturingGrid(1);
  createCoreRow(54, 270, 0, 0);

  odb::dbInst* first = createMacro();
  odb::dbInst* second = odb::dbInst::create(
      db_->getChip()->getBlock(), first->getMaster(), "second");
  first->setOrigin(0, 0);
  first->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  second->setOrigin(10000, 0);
  second->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  snapper_->setMacro(first);
  snapper_->snapMacro();
  snapper_->setMacro(second);
  snapper_->snapMacro();

  // 9990, the closest multiple of 54, would overlap the first macro
  EXPECT_EQ(second->getOrigin().x(), 10044);
  EXPECT_FALSE(
      first->getBBox()->getBox().overlaps(second->getBBox()->getBox()));
}

// Track patterns cover limited ranges: one far from the macro that is
// compatible with the site grid is found past a nearer incompatible one
TEST_F(TestSnapper, SiteGridFiniteTrackPatterns)
{
  db_->getTech()->setManufacturingGrid(1);
  createCoreRow(54, 270, 0, 0);

  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      db_->getTech(), "H1", odb::dbTechLayerType::DEFAULT);
  layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track
      = odb::dbTrackGrid::create(db_->getChip()->getBlock(), layer);
  // Tracks at 0 to 1764, then 2000 to 9164
  track->addGridPatternY(0, 50, 36);
  track->addGridPatternY(2000, 200, 36);

  // Pin center at 18 from the origin
  odb::dbInst* macro = createMacro(layer, 36);
  macro->setOrigin(1512, 3000);
  snapper_->setMacro(macro);
  snapper_->snapMacro();

  // 270 * k + 18 is never 20 past a multiple of 36, as the second pattern's
  // tracks are.  Of the site positions below, 1620 puts the pin at 1638, off
  // the tracks, and 1890 at 1908, between the patterns; 1350 puts it on the
  // first pattern's track at 1368
  EXPECT_EQ(macro->getOrigin().y(), 1350);
}

// With rows of several heights, the smallest site gives the grid
TEST_F(TestSnapper, SiteGridMixedRowHeights)
{
  db_->getTech()->setManufacturingGrid(1);
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbLib* lib = db_->findLib("lib");

  odb::dbSite* double_site = odb::dbSite::create(lib, "double");
  double_site->setWidth(54);
  double_site->setHeight(540);
  double_site->setClass(odb::dbSiteClass::CORE);
  odb::dbRow::create(block,
                     "double_row",
                     double_site,
                     0,
                     0,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     9000,
                     54);

  odb::dbSite* base_site = odb::dbSite::create(lib, "base");
  base_site->setWidth(54);
  base_site->setHeight(270);
  base_site->setClass(odb::dbSiteClass::CORE);
  odb::dbRow::create(block,
                     "base_row",
                     base_site,
                     0,
                     540,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     9000,
                     54);

  odb::dbTechLayer* layer = createHorizontalLayer(36);
  // Pin center at 18 from the origin
  odb::dbInst* macro = createMacro(layer, 36);
  macro->setOrigin(1512, 1500);
  snapper_->setMacro(macro);
  snapper_->snapMacro();

  // 1350 (5 base rows) puts the pin at 1368 = 38 * 36; no multiple of the
  // double row height does
  EXPECT_EQ(macro->getOrigin().y(), 1350);
}

}  // namespace
}  // namespace mpl
