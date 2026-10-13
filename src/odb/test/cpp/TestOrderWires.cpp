// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "gtest/gtest.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/wOrder.h"
#include "tst/fixture.h"

namespace odb {

class TestOrderWires : public tst::Fixture
{
 protected:
  struct InputBumpNet
  {
    dbNet* net;
    dbITerm* receiver_iterm;
    dbITerm* bump_iterm;
    int receiver_x;
    int receiver_y;
    int bump_x;
    int bump_y;
  };

  void SetUp() override
  {
    loadTechAndLib(
        "tech", "Nangate45.lef", "_main/test/Nangate45/Nangate45.lef");
    dbChip* chip = dbChip::create(db_.get(), db_->getTech());
    block_ = dbBlock::create(chip, "top");
  }

  InputBumpNet buildInputBumpNet()
  {
    InputBumpNet bump_net;

    dbInst* bump = makeInst(block_,
                            db_->findMaster("INV_X1"),
                            "bump",
                            {.location = {50000, 10000},
                             .status = dbPlacementStatus::PLACED,
                             .iterms = {{"net", "A"}}});

    // Note that here we could use any type of logical cell.
    dbInst* receiver = makeInst(block_,
                                db_->findMaster("INV_X1"),
                                "receiver",
                                {.location = {10000, 10000},
                                 .status = dbPlacementStatus::PLACED,
                                 .iterms = {{"net", "A"}}});

    // The bterm associated to the bump has no geometry.
    dbBTerm* bterm
        = makeBTerm(block_, "net", {.io_type = dbIoType::INPUT, .bpins = {}});

    bump_net.net = block_->findNet("net");
    bump_net.receiver_iterm = receiver->findITerm("A");
    bump_net.bump_iterm = bump->findITerm("A");

    dbChipRegion* chip_region = dbChipRegion::create(
        db_->getChip(), "R1", dbChipRegion::Side::FRONT, nullptr);
    dbChipBump* chip_bump = dbChipBump::create(chip_region, bump);
    chip_bump->setNet(bump_net.net);
    chip_bump->setBTerm(bterm);

    bump_net.receiver_iterm->getAvgXY(&bump_net.receiver_x,
                                      &bump_net.receiver_y);
    bump_net.bump_iterm->getAvgXY(&bump_net.bump_x, &bump_net.bump_y);

    return bump_net;
  }

  // A driver and a receiver 40 µm apart.
  void buildDriverReceiverNet()
  {
    makeInst(block_,
             db_->findMaster("INV_X1"),
             "driver",
             {.location = {10000, 10000},
              .status = dbPlacementStatus::PLACED,
              .iterms = {{"net", "ZN"}}});
    makeInst(block_,
             db_->findMaster("INV_X1"),
             "receiver",
             {.location = {50000, 10000},
              .status = dbPlacementStatus::PLACED,
              .iterms = {{"net", "A"}}});

    net_ = block_->findNet("net");
  }

  //   (driver_x, above_y)                        (receiver_x, above_y)
  //         +--------------- path 1 ----------------+
  //         |                                       |
  //       [ZN]                                     [A]
  //       driver                               receiver
  //         |                                       |
  //         +--------------- path 2 ----------------+
  //   (driver_x, below_y)                        (receiver_x, below_y)
  void createRingWire()
  {
    dbITerm* driver = block_->findInst("driver")->findITerm("ZN");
    dbITerm* receiver = block_->findInst("receiver")->findITerm("A");

    int driver_x, driver_y, receiver_x, receiver_y;
    driver->getAvgXY(&driver_x, &driver_y);
    receiver->getAvgXY(&receiver_x, &receiver_y);

    const int above_y = driver_y + 8000;
    const int below_y = driver_y - 8000;

    dbTechLayer* metal1 = db_->getTech()->findLayer("metal1");
    wire_ = dbWire::create(net_);
    dbWireEncoder encoder;
    encoder.begin(wire_);

    encoder.newPath(metal1, dbWireType::ROUTED);
    encoder.addPoint(driver_x, driver_y);
    encoder.addPoint(driver_x, above_y);
    encoder.addPoint(receiver_x, above_y);
    encoder.addPoint(receiver_x, receiver_y);

    encoder.newPath(metal1, dbWireType::ROUTED);
    encoder.addPoint(driver_x, driver_y);
    encoder.addPoint(driver_x, below_y);
    encoder.addPoint(receiver_x, below_y);
    encoder.addPoint(receiver_x, receiver_y);
    encoder.end();
  }

  //                      (meet_x, meet_y + 8000)
  //                               |
  //                               | path 3
  //                               |
  //      +------- path 1 ---------+----------- path 2 -----------+
  //      |                (meet_x, meet_y)                       |
  //    [ZN]                                                     [A]
  //    driver                                               receiver
  void createThreePathsWire()
  {
    dbITerm* driver = block_->findInst("driver")->findITerm("ZN");
    dbITerm* receiver = block_->findInst("receiver")->findITerm("A");

    int driver_x, driver_y, receiver_x, receiver_y;
    driver->getAvgXY(&driver_x, &driver_y);
    receiver->getAvgXY(&receiver_x, &receiver_y);

    const int meet_x = (driver_x + receiver_x) / 2;
    const int meet_y = driver_y + 4000;

    dbTechLayer* metal1 = db_->getTech()->findLayer("metal1");
    wire_ = dbWire::create(net_);
    dbWireEncoder encoder;
    encoder.begin(wire_);

    encoder.newPath(metal1, dbWireType::ROUTED);
    encoder.addPoint(driver_x, driver_y);
    encoder.addPoint(driver_x, meet_y);
    encoder.addPoint(meet_x, meet_y);

    encoder.newPath(metal1, dbWireType::ROUTED);
    encoder.addPoint(receiver_x, receiver_y);
    encoder.addPoint(receiver_x, meet_y);
    encoder.addPoint(meet_x, meet_y);

    encoder.newPath(metal1, dbWireType::ROUTED);
    encoder.addPoint(meet_x, meet_y);
    encoder.addPoint(meet_x, meet_y + 8000);
    encoder.end();
  }

  PtrMap<dbITerm, int> itermMarkersInWire(dbWire* wire)
  {
    // Start every terminal at zero, so one that the wire never marks shows
    // up as a count of 0 rather than as a missing key.
    PtrMap<dbITerm, int> markers;
    for (dbITerm* iterm : net_->getITerms()) {
      markers[iterm] = 0;
    }

    dbWireDecoder decoder;
    decoder.begin(wire);
    for (dbWireDecoder::OpCode opcode = decoder.next();
         opcode != dbWireDecoder::END_DECODE;
         opcode = decoder.next()) {
      if (opcode == dbWireDecoder::ITERM) {
        markers[decoder.getITerm()]++;
      }
    }

    return markers;
  }

  void testNormalizedConnectivity()
  {
    EXPECT_TRUE(net_->isWireOrdered());

    const PtrMap<dbITerm, int> markers = itermMarkersInWire(wire_);
    for (dbITerm* iterm : net_->getITerms()) {
      EXPECT_EQ(markers.at(iterm), 1) << iterm->getName();
    }
  }

  dbBlock* block_;
  dbNet* net_{nullptr};
  dbWire* wire_{nullptr};
};

TEST_F(TestOrderWires, InputBumpNetWithPatchAtTheBeginning)
{
  const InputBumpNet bump_net = buildInputBumpNet();
  dbTechLayer* metal1 = db_->getTech()->findLayer("metal1");
  dbWire* wire = dbWire::create(bump_net.net);

  dbWireEncoder encoder;
  encoder.begin(wire);

  // First path: a lone patch RECT, making a wire point without edges.
  encoder.newPath(metal1, dbWireType::ROUTED);
  encoder.addPoint(bump_net.bump_x, bump_net.bump_y);
  encoder.addRect(-70, -70, 70, 70);

  // Second path: a real segment from the receiver pin to the bump pad.
  encoder.newPath(metal1, dbWireType::ROUTED);
  encoder.addPoint(bump_net.receiver_x, bump_net.receiver_y);
  encoder.addPoint(bump_net.bump_x, bump_net.bump_y);
  encoder.end();

  orderWires(&logger_, block_);
  EXPECT_TRUE(bump_net.net->isWireOrdered());

  dbWireDecoder decoder;
  decoder.begin(bump_net.net->getWire());
  EXPECT_EQ(decoder.next(), dbWireDecoder::PATH);

  // Operation codes of the bump pin.
  EXPECT_EQ(decoder.next(), dbWireDecoder::POINT);
  int x, y;
  decoder.getPoint(x, y);
  EXPECT_EQ(x, bump_net.bump_x);
  EXPECT_EQ(y, bump_net.bump_y);
  EXPECT_EQ(decoder.next(), dbWireDecoder::ITERM);
  EXPECT_EQ(decoder.getITerm(), bump_net.bump_iterm);

  // Operation codes of the receiver.
  EXPECT_EQ(decoder.next(), dbWireDecoder::POINT);
  decoder.getPoint(x, y);
  EXPECT_EQ(x, bump_net.receiver_x);
  EXPECT_EQ(y, bump_net.receiver_y);
  EXPECT_EQ(decoder.next(), dbWireDecoder::ITERM);
  EXPECT_EQ(decoder.getITerm(), bump_net.receiver_iterm);

  EXPECT_EQ(decoder.next(), dbWireDecoder::END_DECODE);
}

// One short at each pin; together with the path edges they close one loop.
// Wire loop removal has to break it by dropping a short, and the wire must
// still reach both pins.
TEST_F(TestOrderWires, RingOfTwoPaths)
{
  buildDriverReceiverNet();
  createRingWire();

  orderWires(&logger_, block_);
  testNormalizedConnectivity();
}

// Three paths meet at one point, creating three shorts (one per pair) which
// close a loop on their own before any path edge is in the graph. Short loop
// removal should break it.
TEST_F(TestOrderWires, ThreePathsMeetingAtOnePoint)
{
  buildDriverReceiverNet();
  createThreePathsWire();

  orderWires(&logger_, block_);
  testNormalizedConnectivity();
}

}  // namespace odb
