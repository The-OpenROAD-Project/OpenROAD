// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <stdexcept>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/wOrder.h"
#include "tst/fixture.h"

namespace odb {

class TestOrderWires : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    loadTechAndLib(
        "tech", "Nangate45.lef", "_main/test/Nangate45/Nangate45.lef");
    dbChip* chip = dbChip::create(db_.get(), db_->getTech());
    block_ = dbBlock::create(chip, "top");
  }

  // A face-to-face style net: a receiver instance pin, a bump pad iterm
  // (special, as a SPECIALNETS connection reads in), and an unplaced
  // driver bterm carrying no geometry.  The BUMP_ASSIGNMENT net property
  // marks the net as tied to the bump.
  void buildBumpNet()
  {
    dbMaster* inv = db_->findMaster("INV_X1");

    dbInst* recv = makeInst(block_,
                            inv,
                            "recv",
                            {.location = {10000, 10000},
                             .status = dbPlacementStatus::PLACED,
                             .iterms = {{"n1", "A"}}});
    dbInst* bump = makeInst(block_,
                            inv,
                            "bump",
                            {.location = {50000, 10000},
                             .status = dbPlacementStatus::PLACED,
                             .iterms = {{"n1", "A"}}});

    makeBTerm(block_, "n1", {.io_type = dbIoType::INPUT, .bpins = {}});

    net_ = block_->findNet("n1");
    recv_iterm_ = recv->findITerm("A");
    bump_iterm_ = bump->findITerm("A");
    bump_iterm_->setSpecial();
    dbStringProperty::create(net_, "BUMP_ASSIGNMENT", "ASSIGNED");

    ASSERT_TRUE(recv_iterm_->getAvgXY(&recv_x_, &recv_y_));
    ASSERT_TRUE(bump_iterm_->getAvgXY(&bump_x_, &bump_y_));
    ASSERT_EQ(recv_y_, bump_y_);
  }

  bool wireHasITerm(dbWire* wire, dbITerm* iterm)
  {
    dbWireDecoder decoder;
    decoder.begin(wire);
    for (dbWireDecoder::OpCode op = decoder.next();
         op != dbWireDecoder::END_DECODE;
         op = decoder.next()) {
      if (op == dbWireDecoder::ITERM && decoder.getITerm() == iterm) {
        return true;
      }
    }
    return false;
  }

  dbBlock* block_;
  dbNet* net_;
  dbITerm* recv_iterm_;
  dbITerm* bump_iterm_;
  int recv_x_;
  int recv_y_;
  int bump_x_;
  int bump_y_;
};

// A net driven by an unplaced bterm gives the tree walk no driver anchor,
// making it start at the wire's first point.  If the first path holds only
// a patch RECT that point has no graph edges, and the walk used to abort
// (ODB-0395) without stamping any terminal markers into the wire, leaving
// extraction with no terminal connections.  On a bump-assigned net the
// walk must anchor at the bump pad iterm instead and stamp the reachable
// terms.
TEST_F(TestOrderWires, RectFirstBumpNetWithUnplacedDriverPin)
{
  buildBumpNet();
  dbTechLayer* metal1 = db_->getTech()->findLayer("metal1");

  dbWire* wire = dbWire::create(net_);
  dbWireEncoder encoder;
  encoder.begin(wire);
  // First path: a lone patch RECT, making a wire point without edges.
  encoder.newPath(metal1, dbWireType::ROUTED);
  encoder.addPoint(bump_x_, bump_y_);
  encoder.addRect(-70, -70, 70, 70);
  // Second path: a real segment from the receiver pin to the bump pad.
  encoder.newPath(metal1, dbWireType::ROUTED);
  encoder.addPoint(recv_x_, recv_y_);
  encoder.addPoint(bump_x_, bump_y_);
  encoder.end();

  const int warnings_before = logger_.getWarningCount();
  orderWires(&logger_, block_);

  EXPECT_TRUE(net_->isWireOrdered());
  EXPECT_EQ(logger_.getWarningCount(), warnings_before);
  EXPECT_TRUE(wireHasITerm(net_->getWire(), recv_iterm_));
  EXPECT_TRUE(wireHasITerm(net_->getWire(), bump_iterm_));
}

// On a bump-assigned net whose driver is a bterm with no pin geometry,
// the walk must be rooted at the bump pad iterm: the signal physically
// enters the die there.
TEST_F(TestOrderWires, BumpAssignedNetRootsTreeAtBumpIterm)
{
  buildBumpNet();
  dbTechLayer* metal1 = db_->getTech()->findLayer("metal1");

  dbWire* wire = dbWire::create(net_);
  dbWireEncoder encoder;
  encoder.begin(wire);
  encoder.newPath(metal1, dbWireType::ROUTED);
  encoder.addPoint(recv_x_, recv_y_);
  encoder.addPoint(bump_x_, bump_y_);
  encoder.end();

  orderWires(&logger_, block_);

  // The rewritten wire must begin at the bump pin, marker included.
  dbWireDecoder decoder;
  decoder.begin(net_->getWire());
  EXPECT_EQ(decoder.next(), dbWireDecoder::PATH);
  EXPECT_EQ(decoder.next(), dbWireDecoder::POINT);
  int x, y;
  decoder.getPoint(x, y);
  EXPECT_EQ(x, bump_x_);
  EXPECT_EQ(y, bump_y_);
  EXPECT_EQ(decoder.next(), dbWireDecoder::ITERM);
  EXPECT_EQ(decoder.getITerm(), bump_iterm_);
}

// Without a bump assignment nothing anchors the walk of a net driven by a
// bterm with no geometry, and a wire whose first path holds only a patch
// RECT makes it start at a point with no reachable segment.  Ordering
// must fail loudly (ODB-0395) and must not leave the net marked as
// ordered.
TEST_F(TestOrderWires, UnanchoredNetWithUnreachableStartPointFailsLoudly)
{
  dbTechLayer* metal1 = db_->getTech()->findLayer("metal1");
  dbMaster* inv = db_->findMaster("INV_X1");

  dbInst* recv = makeInst(block_,
                          inv,
                          "recv",
                          {.location = {10000, 10000},
                           .status = dbPlacementStatus::PLACED,
                           .iterms = {{"n1", "A"}}});

  makeBTerm(block_, "n1", {.io_type = dbIoType::INPUT, .bpins = {}});

  dbNet* net = block_->findNet("n1");
  int recv_x, recv_y;
  ASSERT_TRUE(recv->findITerm("A")->getAvgXY(&recv_x, &recv_y));

  dbWire* wire = dbWire::create(net);
  dbWireEncoder encoder;
  encoder.begin(wire);
  encoder.newPath(metal1, dbWireType::ROUTED);
  encoder.addPoint(recv_x, recv_y + 2000);
  encoder.addRect(-70, -70, 70, 70);
  encoder.newPath(metal1, dbWireType::ROUTED);
  encoder.addPoint(recv_x, recv_y);
  encoder.addPoint(recv_x, recv_y + 2000);
  encoder.end();

  EXPECT_THROW(orderWires(&logger_, block_), std::runtime_error);
  EXPECT_FALSE(net->isWireOrdered());
}

}  // namespace odb
