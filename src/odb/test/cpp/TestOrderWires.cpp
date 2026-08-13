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

}  // namespace odb
