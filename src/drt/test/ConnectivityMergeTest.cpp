// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>

#include "AbstractGraphicsFactory.h"
#include "db/obj/frNet.h"
#include "db/obj/frShape.h"
#include "dr/FlexDR_conn.h"
#include "drt/TritonRoute.h"
#include "dst/Distributed.h"
#include "frDesign.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"
#include "tst/db_fixture.h"
#include "utl/ServiceRegistry.h"

namespace drt {
namespace {

class NoGraphicsFactory : public AbstractGraphicsFactory
{
 public:
  void reset(frDebugSettings* settings,
             frDesign* design,
             odb::dbDatabase* db,
             utl::Logger* logger,
             RouterConfiguration* router_cfg) override
  {
  }
  bool guiActive() override { return false; }
  std::unique_ptr<AbstractDRGraphics> makeUniqueDRGraphics() override
  {
    return nullptr;
  }
  std::unique_ptr<AbstractTAGraphics> makeUniqueTAGraphics() override
  {
    return nullptr;
  }
  std::unique_ptr<AbstractPAGraphics> makeUniquePAGraphics() override
  {
    return nullptr;
  }
};

// A net with a pin at each end of one metal2 track, routed by two overlapping
// segments on that track. The connectivity checker merges overlapping
// segments into one; a pin stays connected only through a segment end that
// is truncated and lies on the pin, so the merged segment must keep the
// truncated end (or begin) at a pin even when the other segment has an
// extended one at the same coordinate.
class ConnectivityMergeTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    tech_ = loadTechLef("ng45",
                        "_main/src/drt/test/Nangate45/Nangate45_tech.lef");
    ASSERT_NE(tech_, nullptr);
    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech_);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDefUnits(tech_->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 20000, 20000));
    for (odb::dbTechLayer* layer : tech_->getLayers()) {
      if (layer->getType() != odb::dbTechLayerType::ROUTING) {
        continue;
      }
      odb::dbTrackGrid* grid = odb::dbTrackGrid::create(block_, layer);
      grid->addGridPatternX(layer->getPitchX() / 2,
                            20000 / layer->getPitchX(),
                            layer->getPitchX());
      grid->addGridPatternY(layer->getPitchY() / 2,
                            20000 / layer->getPitchY(),
                            layer->getPitchY());
    }
    block_->setMinRoutingLayer(2);
    block_->setMaxRoutingLayer(3);

    metal2_ = tech_->findLayer("metal2");
    // Instance pins: a pin the checker cannot reach is printed, and a block
    // pin (frBTerm) has no stream operator of its own.
    odb::dbLib* lib = odb::dbLib::create(db_.get(), "pins", tech_, ',');
    odb::dbMaster* master = odb::dbMaster::create(lib, "PIN");
    master->setType(odb::dbMasterType::BLOCK);
    master->setWidth(200);
    master->setHeight(400);
    odb::dbMTerm* mterm = odb::dbMTerm::create(
        master, "P", odb::dbIoType::INOUT, odb::dbSigType::SIGNAL);
    odb::dbBox::create(odb::dbMPin::create(mterm), metal2_, 0, 0, 200, 400);
    master->setFrozen();
    makeInst(block_,
             master,
             "u_lo",
             {.location = odb::Point(kX - 100, kLow - 200),
              .status = odb::dbPlacementStatus::FIRM,
              .iterms = {{.net_name = "a", .term_name = "P"}}});
    makeInst(block_,
             master,
             "u_hi",
             {.location = odb::Point(kX - 100, kHigh - 200),
              .status = odb::dbPlacementStatus::FIRM,
              .iterms = {{.net_name = "a", .term_name = "P"}}});
    net_ = block_->findNet("a");
  }

  // A metal2 segment from y0 to y1 on the track; an end is truncated when its
  // flag is set (DEF extension 0) and extended otherwise.
  static void addSegment(odb::dbWireEncoder& encoder,
                         odb::dbTechLayer* layer,
                         int y0,
                         bool trunc0,
                         int y1,
                         bool trunc1)
  {
    encoder.newPath(layer, odb::dbWireType::ROUTED);
    trunc0 ? encoder.addPoint(kX, y0, 0) : encoder.addPoint(kX, y0);
    trunc1 ? encoder.addPoint(kX, y1, 0) : encoder.addPoint(kX, y1);
  }

  // Reads the design into the router, runs the connectivity check on the
  // net and returns the net's path segments left after it.
  frNet* check()
  {
    registry_ = std::make_unique<utl::ServiceRegistry>(&logger_);
    dist_ = std::make_unique<dst::Distributed>(&logger_);
    router_ = std::make_unique<TritonRoute>(
        db_.get(), &logger_, registry_.get(), dist_.get(), nullptr);
    router_->initGraphics(std::make_unique<NoGraphicsFactory>());
    ParamStruct params;
    params.verbose = 0;
    router_->setParams(params);
    router_->pinAccess();  // reads the design, the net's wires included
    frDesign* design = router_->getDesign();
    design->getRegionQuery()->initDRObj();
    frNet* net = design->getTopBlock()->findNet("a");
    EXPECT_NE(net, nullptr);
    EXPECT_EQ(net->getShapes().size(), 2);
    net->setModified(true);
    FlexDRConnectivityChecker checker(
        router_.get(), &logger_, router_->getRouterConfiguration(), nullptr);
    EXPECT_NO_THROW(checker.check(0));
    return net;
  }

  static void expectOneTruncatedSegment(frNet* net)
  {
    ASSERT_EQ(net->getShapes().size(), 1);
    auto ps = static_cast<frPathSeg*>(net->getShapes().front().get());
    EXPECT_EQ(ps->getBeginPoint(), odb::Point(kX, kLow));
    EXPECT_EQ(ps->getEndPoint(), odb::Point(kX, kHigh));
    EXPECT_TRUE(ps->isBeginTruncated());
    EXPECT_TRUE(ps->isEndTruncated());
  }

  static constexpr int kX = 3990;  // a metal2 track (pitch 380, offset 190)
  static constexpr int kLow = 1200;
  static constexpr int kMid = 3000;
  static constexpr int kHigh = 5000;

  odb::dbTech* tech_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  odb::dbTechLayer* metal2_ = nullptr;
  odb::dbNet* net_ = nullptr;
  // the router keeps pointers to the registry and dist, so they outlive it
  std::unique_ptr<utl::ServiceRegistry> registry_;
  std::unique_ptr<dst::Distributed> dist_;
  std::unique_ptr<TritonRoute> router_;
};

// The pin segment's truncated end and a route segment's extended end meet at
// the same point on the upper pin: the merged end stays truncated.
TEST_F(ConnectivityMergeTest, TruncatedEndSurvivesAnEqualExtendedEnd)
{
  odb::dbWire* wire = odb::dbWire::create(net_);
  odb::dbWireEncoder encoder;
  encoder.begin(wire);
  addSegment(encoder, metal2_, kLow, true, kHigh, true);
  addSegment(encoder, metal2_, kMid, false, kHigh, false);
  encoder.end();

  expectOneTruncatedSegment(check());
}

// A shorter segment starts at the lower pin with an extended begin, and the
// pin segment starts there too with a truncated one: the merged begin stays
// truncated.
TEST_F(ConnectivityMergeTest, TruncatedBeginAtTheMergedLowEndSurvives)
{
  odb::dbWire* wire = odb::dbWire::create(net_);
  odb::dbWireEncoder encoder;
  encoder.begin(wire);
  addSegment(encoder, metal2_, kLow, false, kMid, false);
  addSegment(encoder, metal2_, kLow, true, kHigh, true);
  encoder.end();

  expectOneTruncatedSegment(check());
}

}  // namespace
}  // namespace drt
