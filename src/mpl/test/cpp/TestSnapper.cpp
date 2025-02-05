#include <memory>

#include "MplTest.h"

namespace mpl {
namespace {

class TestSnapper : public MplTest
{
 protected:
  void SetUp() override
  {
    MplTest::SetUp();
    snapper_ = std::make_unique<Snapper>(logger_.get());
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
  // 1487 (origin) + 25 (pin center) equals 1512 which is
  // a multiple of 36 (pin is aligned to track)
  EXPECT_EQ(macro->getOrigin().y(), 1487);
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
  // 1559 (origin) + 25 (pin2 center) equals 1632 which is
  // a multiple of 48 (pin is aligned to track2)
  EXPECT_EQ(macro->getOrigin().y(), 1559);
}

TEST_F(TestSnapper, MultiPattern)
{
  tech_->setManufacturingGrid(1);

  odb::dbTechLayer* h1 = odb::dbTechLayer::create(
      tech_.get(), "H1", odb::dbTechLayerType::DEFAULT);
  h1->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track1 = odb::dbTrackGrid::create(block_.get(), h1);
  track1->addGridPatternY(0, 1000 / 3, 3);
  track1->addGridPatternY(0, 1000 / 5, 5);

  odb::dbMaster* master1 = odb::dbMaster::create(lib_.get(), "master1");
  master1->setHeight(100);
  master1->setWidth(100);
  master1->setType(odb::dbMasterType::BLOCK);

  odb::dbMTerm* mterm1 = odb::dbMTerm::create(
      master1, "pin1", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin1 = odb::dbMPin::create(mterm1);
  odb::dbBox::create(mpin1, h1, 0, 0, 10, 10);

  odb::dbMTerm* mterm2 = odb::dbMTerm::create(
      master1, "pin2", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin2 = odb::dbMPin::create(mterm2);
  odb::dbBox::create(mpin2, h1, 0, 10, 10, 20);

  master1->setFrozen();

  odb::dbInst* inst = odb::dbInst::create(block_.get(), master1, "macro1");
  inst->setOrigin(10, 119);

  snapper_->setMacro(inst);
  snapper_->snapMacro();

  EXPECT_EQ(inst->getOrigin().x(), 10);
  EXPECT_EQ(inst->getOrigin().y(), 115);
}

TEST_F(TestSnapper, MultiLayerMultiPattern)
{
  tech_->setManufacturingGrid(1);

  odb::dbTechLayer* h1 = odb::dbTechLayer::create(
      tech_.get(), "H1", odb::dbTechLayerType::DEFAULT);
  h1->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track1 = odb::dbTrackGrid::create(block_.get(), h1);
  track1->addGridPatternY(0, 1000 / 3, 3);
  track1->addGridPatternY(0, 1000 / 11, 11);

  odb::dbTechLayer* h2 = odb::dbTechLayer::create(
      tech_.get(), "H2", odb::dbTechLayerType::DEFAULT);
  h2->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track2 = odb::dbTrackGrid::create(block_.get(), h2);
  track2->addGridPatternY(0, 1000 / 7, 7);
  track2->addGridPatternY(0, 1000 / 13, 13);

  odb::dbMaster* master1 = odb::dbMaster::create(lib_.get(), "master1");
  master1->setHeight(100);
  master1->setWidth(100);
  master1->setType(odb::dbMasterType::BLOCK);

  odb::dbMTerm* mterm1 = odb::dbMTerm::create(
      master1, "pin1", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin1 = odb::dbMPin::create(mterm1);
  odb::dbBox::create(mpin1, h1, 0, 0, 10, 10);

  odb::dbMTerm* mterm2 = odb::dbMTerm::create(
      master1, "pin2", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin2 = odb::dbMPin::create(mterm2);
  odb::dbBox::create(mpin2, h1, 0, 10, 10, 20);

  odb::dbMTerm* mterm3 = odb::dbMTerm::create(
      master1, "pin3", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin3 = odb::dbMPin::create(mterm3);
  odb::dbBox::create(mpin3, h2, 0, 0, 10, 10);

  odb::dbMTerm* mterm4 = odb::dbMTerm::create(
      master1, "pin4", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin4 = odb::dbMPin::create(mterm4);
  odb::dbBox::create(mpin4, h2, 0, 10, 10, 20);

  master1->setFrozen();

  odb::dbInst* inst = odb::dbInst::create(block_.get(), master1, "macro1");
  inst->setOrigin(10, 119);

  snapper_->setMacro(inst);
  snapper_->snapMacro();

  EXPECT_EQ(inst->getOrigin().x(), 10);
  EXPECT_EQ(inst->getOrigin().y(), 121);
}

}  // namespace
}  // namespace mpl
