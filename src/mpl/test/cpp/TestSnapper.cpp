#include "MplTest.h"

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

TEST_F(TestSnapper, ManufacturingGrid)
{
  tech_->setManufacturingGrid(10);

  odb::dbInst* inst
      = odb::dbInst::create(block_.get(), master_.get(), "macro1");
  inst->setOrigin(14, 19);
  snapper_->setMacro(inst);
  snapper_->snapMacro();
  EXPECT_EQ(inst->getOrigin().x(), 10);
  EXPECT_EQ(inst->getOrigin().y(), 20);
}

TEST_F(TestSnapper, SingleLayer)
{
  tech_->setManufacturingGrid(5);

  odb::dbTechLayer* h1 = odb::dbTechLayer::create(
      tech_.get(), "H1", odb::dbTechLayerType::DEFAULT);
  h1->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track1 = odb::dbTrackGrid::create(block_.get(), h1);
  track1->addGridPatternY(0, 100, 10);

  odb::dbMaster* master1 = odb::dbMaster::create(lib_.get(), "master1");
  master1->setHeight(100);
  master1->setWidth(100);
  master1->setType(odb::dbMasterType::BLOCK);

  odb::dbMTerm* mterm = odb::dbMTerm::create(
      master1, "pin", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin = odb::dbMPin::create(mterm);
  odb::dbBox::create(mpin, h1, 0, 0, 10, 10);

  master1->setFrozen();

  odb::dbInst* inst = odb::dbInst::create(block_.get(), master1, "macro1");
  inst->setOrigin(9, 119);

  snapper_->setMacro(inst);
  snapper_->snapMacro();

  EXPECT_EQ(inst->getOrigin().x(), 10);
  EXPECT_EQ(inst->getOrigin().y(), 115);
}

TEST_F(TestSnapper, MultiLayer)
{
  tech_->setManufacturingGrid(1);

  odb::dbTechLayer* h1 = odb::dbTechLayer::create(
      tech_.get(), "H1", odb::dbTechLayerType::DEFAULT);
  h1->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track1 = odb::dbTrackGrid::create(block_.get(), h1);
  track1->addGridPatternY(0, 1000 / 5, 5);

  odb::dbTechLayer* h2 = odb::dbTechLayer::create(
      tech_.get(), "H2", odb::dbTechLayerType::DEFAULT);
  h2->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track2 = odb::dbTrackGrid::create(block_.get(), h2);
  track2->addGridPatternY(0, 1000 / 7, 7);

  odb::dbTechLayer* h3 = odb::dbTechLayer::create(
      tech_.get(), "H3", odb::dbTechLayerType::DEFAULT);
  h3->setDirection(odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* track3 = odb::dbTrackGrid::create(block_.get(), h3);
  track3->addGridPatternY(0, 1000 / 11, 11);

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
  odb::dbBox::create(mpin2, h2, 0, 10, 10, 20);

  odb::dbMTerm* mterm3 = odb::dbMTerm::create(
      master1, "pin3", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMPin* mpin3 = odb::dbMPin::create(mterm3);
  odb::dbBox::create(mpin3, h3, 0, 20, 10, 30);

  master1->setFrozen();

  odb::dbInst* inst = odb::dbInst::create(block_.get(), master1, "macro1");
  inst->setOrigin(10, 119);

  snapper_->setMacro(inst);
  snapper_->snapMacro();

  EXPECT_EQ(inst->getOrigin().x(), 10);
  EXPECT_EQ(inst->getOrigin().y(), 195);
}

}  // namespace
}  // namespace mpl
