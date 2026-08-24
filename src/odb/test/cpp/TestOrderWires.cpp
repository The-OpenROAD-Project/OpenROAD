// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "gtest/gtest.h"
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

  dbMaster* createBumpMaster()
  {
    dbLib* lib = dbLib::create(db_.get(), "bump_lib", db_->getTech(), ',');
    dbTechLayer* metal1 = db_->getTech()->findLayer("metal1");

    dbMaster* bump_master = dbMaster::create(lib, "bump_pad");
    bump_master->setWidth(4000);
    bump_master->setHeight(4000);
    bump_master->setType(dbMasterType::COVER_BUMP);

    dbMTerm* bump_mterm = dbMTerm::create(
        bump_master, "PAD", dbIoType::INOUT, dbSigType::SIGNAL);
    dbMPin* bump_mpin = dbMPin::create(bump_mterm);
    dbBox::create(bump_mpin, metal1, 0, 0, 4000, 4000);

    bump_master->setFrozen();

    return bump_master;
  }

  dbBlock* block_;
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

// A bterm has the exact same geometry as a bump pad pin of its net, so
// the bterm is the terminal that actually exercises the logical connection:
//
//  receiver                         bump
//                                   +-------------------------+
//  +-----------+                    | PAD pin (the bterm pin  |
//  |         A o--------------------o has the same box)       |
//  +-----------+       wire         |                         |
//                                   +-------------------------+
//
// The wire should be anchored at the bterm, not the bump pin.
TEST_F(TestOrderWires, UseBTermInsteadOfBumpIterm)
{
  dbTechLayer* metal1 = db_->getTech()->findLayer("metal1");

  dbInst* receiver = makeInst(block_,
                              db_->findMaster("INV_X1"),
                              "receiver",
                              {.location = {10000, 10000},
                               .status = dbPlacementStatus::PLACED,
                               .iterms = {{"net", "A"}}});

  dbITerm* receiver_iterm = receiver->findITerm("A");
  int receiver_x, receiver_y;
  receiver_iterm->getAvgXY(&receiver_x, &receiver_y);

  // Align the bump pad y center with the receiver pin so a single horizontal
  // segment connects them.
  dbMaster* bump_master = createBumpMaster();
  const int bump_y = receiver_y - bump_master->getHeight() / 2;
  dbInst* bump = makeInst(block_,
                          bump_master,
                          "bump",
                          {.location = {50000, bump_y},
                           .status = dbPlacementStatus::PLACED,
                           .iterms = {{"net", "PAD"}}});

  const Rect bump_pin_shape = bump->findITerm("PAD")->getBBox();
  dbBTerm* bterm = makeBTerm(block_,
                             "net",
                             {.io_type = dbIoType::INPUT,
                              .bpins = {{.layer_name = "metal1",
                                         .rect = bump_pin_shape,
                                         .status = dbPlacementStatus::FIRM}}});

  dbNet* net = block_->findNet("net");
  const int pad_x = bump_pin_shape.xCenter();
  const int pad_y = bump_pin_shape.yCenter();

  dbWire* wire = dbWire::create(net);
  dbWireEncoder encoder;
  encoder.begin(wire);
  encoder.newPath(metal1, dbWireType::ROUTED);
  encoder.addPoint(receiver_x, receiver_y);
  encoder.addPoint(pad_x, pad_y);
  encoder.end();

  orderWires(&logger_, block_);
  EXPECT_TRUE(net->isWireOrdered());

  // Despite the name, isDisconnected does not mean the net lost a connection:
  // it only tells that the ordered wire failed to reach every terminal. A
  // replaced bump pin must not be counted as such a terminal.
  EXPECT_FALSE(net->isDisconnected());

  dbWireDecoder decoder;
  decoder.begin(net->getWire());
  EXPECT_EQ(decoder.next(), dbWireDecoder::PATH);

  // Operation codes of the bterm.
  EXPECT_EQ(decoder.next(), dbWireDecoder::POINT);
  int x, y;
  decoder.getPoint(x, y);
  EXPECT_EQ(x, pad_x);
  EXPECT_EQ(y, pad_y);
  EXPECT_EQ(decoder.next(), dbWireDecoder::BTERM);
  EXPECT_EQ(decoder.getBTerm(), bterm);

  // Operation codes of the receiver's terminal.
  EXPECT_EQ(decoder.next(), dbWireDecoder::POINT);
  decoder.getPoint(x, y);
  EXPECT_EQ(x, receiver_x);
  EXPECT_EQ(y, receiver_y);
  EXPECT_EQ(decoder.next(), dbWireDecoder::ITERM);
  EXPECT_EQ(decoder.getITerm(), receiver_iterm);

  EXPECT_EQ(decoder.next(), dbWireDecoder::END_DECODE);
}

}  // namespace odb
