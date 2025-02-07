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

TEST_F(TestSnapper, ManufacturingGrid)
{
  tech_->setManufacturingGrid(10);

  odb::dbMaster* macro_master = odb::dbMaster::create(lib_, "macro_master");
  macro_master->setFrozen();

  odb::dbInst* macro = odb::dbInst::create(block_, macro_master, "macro");
  macro->setOrigin(1014, 1019);
  snapper_->setMacro(macro);
  snapper_->snapMacro();
  EXPECT_EQ(macro->getOrigin().x(), 1010);
  EXPECT_EQ(macro->getOrigin().y(), 1020);
}

TEST_F(TestSnapper, SingleLayer)
{
  tech_->setManufacturingGrid(1);

  const int track_pitch = 36;
  odb::dbTechLayer* horizontal_layer
      = odb::dbTechLayer::create(tech_, "H1", odb::dbTechLayerType::DEFAULT);
  horizontal_layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track = odb::dbTrackGrid::create(block_, horizontal_layer);
  track->addGridPatternY(0, die_height_ / track_pitch, track_pitch);

  odb::dbMaster* macro_master = odb::dbMaster::create(lib_, "macro_master");
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

  odb::dbInst* macro = odb::dbInst::create(block_, macro_master, "macro");
  macro->setOrigin(1500, 1500);

  snapper_->setMacro(macro);
  snapper_->snapMacro();

  EXPECT_EQ(macro->getOrigin().x(), 1500);
  EXPECT_EQ(macro->getOrigin().y(), 1487);
}

TEST_F(TestSnapper, DoubleLayer)
{
  tech_->setManufacturingGrid(1);

  const int track1_pitch = 36;
  odb::dbTechLayer* horizontal_layer1
      = odb::dbTechLayer::create(tech_, "H1", odb::dbTechLayerType::DEFAULT);
  horizontal_layer1->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track1
      = odb::dbTrackGrid::create(block_, horizontal_layer1);
  track1->addGridPatternY(0, die_height_ / track1_pitch, track1_pitch);

  const int track2_pitch = 48;
  odb::dbTechLayer* horizontal_layer2
      = odb::dbTechLayer::create(tech_, "H2", odb::dbTechLayerType::DEFAULT);
  horizontal_layer2->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track2
      = odb::dbTrackGrid::create(block_, horizontal_layer2);
  track2->addGridPatternY(0, die_height_ / track2_pitch, track2_pitch);

  odb::dbMaster* macro_master = odb::dbMaster::create(lib_, "macro_master");
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

  odb::dbInst* macro = odb::dbInst::create(block_, macro_master, "macro");
  macro->setOrigin(1500, 1500);

  snapper_->setMacro(macro);
  snapper_->snapMacro();

  EXPECT_EQ(macro->getOrigin().x(), 1500);
  EXPECT_EQ(macro->getOrigin().y(), 1559);
}

}  // namespace
}  // namespace mpl
