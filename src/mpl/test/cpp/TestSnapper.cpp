// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <array>
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

// Keeps a macro inside a core that has no legal manufacturing-grid coordinate.
TEST_F(TestSnapper, ManufacturingGridPreservesCoreContainedOrigin)
{
  db_->getTech()->setManufacturingGrid(10);
  odb::dbBlock* block = db_->getChip()->getBlock();
  block->setCoreArea(odb::Rect(5, 5, 105, 105));

  odb::dbMaster* macro_master
      = odb::dbMaster::create(db_->findLib("lib"), "macro_master");
  macro_master->setWidth(100);
  macro_master->setHeight(100);
  macro_master->setType(odb::dbMasterType::BLOCK);
  macro_master->setFrozen();

  odb::dbInst* macro = odb::dbInst::create(block, macro_master, "macro");
  macro->setOrigin(5, 5);
  snapper_->setMacro(macro);
  snapper_->snapMacro();

  EXPECT_EQ(macro->getOrigin().x(), 5);
  EXPECT_EQ(macro->getOrigin().y(), 5);
}

class TestSnapperCoreBoundary : public TestSnapper,
                                public testing::WithParamInterface<int>
{
 protected:
  odb::dbInst* makeMacro(bool pins, int pin_min = 4, int height = 100)
  {
    db_->getTech()->setManufacturingGrid(10);
    auto* block = db_->getChip()->getBlock();
    auto* master = odb::dbMaster::create(db_->findLib("lib"), "macro_master");
    master->setWidth(100);
    master->setHeight(height);
    master->setType(odb::dbMasterType::BLOCK);
    if (pins) {
      for (const auto direction :
           {odb::dbTechLayerDir::VERTICAL, odb::dbTechLayerDir::HORIZONTAL}) {
        const char* name
            = direction == odb::dbTechLayerDir::VERTICAL ? "V" : "H";
        auto* layer = odb::dbTechLayer::create(
            db_->getTech(), name, odb::dbTechLayerType::DEFAULT);
        layer->setDirection(direction);
        auto* grid = odb::dbTrackGrid::create(block, layer);
        if (direction == odb::dbTechLayerDir::VERTICAL) {
          grid->addGridPatternX(0, 22, 10);
        } else {
          grid->addGridPatternY(0, 22, 10);
        }
        auto* term = odb::dbMTerm::create(
            master, name, odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
        auto* pin = odb::dbMPin::create(term);
        odb::dbBox::create(
            pin, layer, pin_min, pin_min, pin_min + 2, pin_min + 2);
      }
    }
    master->setFrozen();
    auto* macro = odb::dbInst::create(block, master, "macro");
    const std::array<odb::dbOrientType, 8> orientations{
        odb::dbOrientType::R0,
        odb::dbOrientType::R90,
        odb::dbOrientType::R180,
        odb::dbOrientType::R270,
        odb::dbOrientType::MX,
        odb::dbOrientType::MY,
        odb::dbOrientType::MXR90,
        odb::dbOrientType::MYR90};
    macro->setOrient(orientations[GetParam()]);
    snapper_->setMacro(macro);
    return macro;
  }
};

TEST_P(TestSnapperCoreBoundary, ManufacturingRoundingAtCoreEdges)
{
  auto* macro = makeMacro(false);
  auto* block = db_->getChip()->getBlock();
  const odb::Rect core(5, 5, 205, 205);
  block->setCoreArea(core);
  for (const int x : {5, 105}) {
    for (const int y : {5, 105}) {
      SCOPED_TRACE(testing::Message() << "location=" << x << "," << y);
      macro->setLocation(x, y);
      ASSERT_TRUE(core.contains(macro->getBBox()->getBox()));
      snapper_->snapMacro();
      const auto& box = macro->getBBox()->getBox();
      EXPECT_TRUE(core.contains(box));
      EXPECT_EQ(box.xMin(), x == 5 ? 10 : 100);
      EXPECT_EQ(box.yMin(), y == 5 ? 10 : 100);
    }
  }
}

TEST_P(TestSnapperCoreBoundary, NoManufacturingGridCoordinatePreservesOrigin)
{
  auto* macro = makeMacro(false);
  const odb::Rect core(5, 5, 105, 105);
  macro->getBlock()->setCoreArea(core);
  macro->setLocation(5, 5);
  const auto origin = macro->getOrigin();
  snapper_->snapMacro();
  EXPECT_EQ(macro->getOrigin(), origin);
  EXPECT_EQ(macro->getBBox()->getBox(), core);
}

TEST_P(TestSnapperCoreBoundary, TrackCandidateIncludesManufacturingRounding)
{
  auto* macro = makeMacro(true);
  const odb::Rect core(5, 5, 205, 205);
  macro->getBlock()->setCoreArea(core);
  macro->setLocation(105, 105);
  ASSERT_TRUE(core.contains(macro->getBBox()->getBox()));
  snapper_->snapMacro();
  const auto& box = macro->getBBox()->getBox();
  EXPECT_TRUE(core.contains(box));
  EXPECT_EQ(box.xMin(), 100);
  EXPECT_EQ(box.yMin(), 100);
}

TEST_P(TestSnapperCoreBoundary, NoLegalTrackCandidatePreservesOrigin)
{
  auto* macro = makeMacro(true);
  const odb::Rect core(5, 5, 105, 105);
  macro->getBlock()->setCoreArea(core);
  macro->setLocation(5, 5);
  const auto origin = macro->getOrigin();
  snapper_->snapMacro();
  EXPECT_EQ(macro->getOrigin(), origin);
  EXPECT_EQ(macro->getBBox()->getBox(), core);
}

TEST_P(TestSnapperCoreBoundary, NonsquareRotatedMacroStaysInsideCore)
{
  auto* macro = makeMacro(true, 4, 60);
  const odb::Rect core(5, 5, 205, 205);
  macro->getBlock()->setCoreArea(core);
  const int x = core.xMax() - macro->getBBox()->getBox().dx();
  const int y = core.yMax() - macro->getBBox()->getBox().dy();
  macro->setLocation(x, y);
  ASSERT_TRUE(core.contains(macro->getBBox()->getBox()));
  snapper_->snapMacro();
  const auto& box = macro->getBBox()->getBox();
  EXPECT_TRUE(core.contains(box));
  EXPECT_EQ(box.xMin(), x - 5);
  EXPECT_EQ(box.yMin(), y - 5);
}

TEST_P(TestSnapperCoreBoundary, NoLegalCandidateStillChecksRequiredGrid)
{
  auto* macro = makeMacro(true, 0);
  const odb::Rect core(5, 5, 105, 105);
  macro->getBlock()->setCoreArea(core);
  macro->setLocation(5, 5);
  db_->getTech()->findLayer("H")->setRightWayOnGridOnly(true);
  db_->getTech()->findLayer("V")->setRightWayOnGridOnly(true);
  const auto origin = macro->getOrigin();
  try {
    snapper_->snapMacro();
    FAIL() << "Expected an unsnappable RightWayOnGridOnly pin to fail";
  } catch (const std::exception& error) {
    EXPECT_STREQ(error.what(), "MPL-0005");
  }
  EXPECT_EQ(macro->getOrigin(), origin);
  EXPECT_EQ(macro->getBBox()->getBox(), core);
}

INSTANTIATE_TEST_SUITE_P(AllOrientations,
                         TestSnapperCoreBoundary,
                         testing::Range(0, 8));

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

}  // namespace
}  // namespace mpl
