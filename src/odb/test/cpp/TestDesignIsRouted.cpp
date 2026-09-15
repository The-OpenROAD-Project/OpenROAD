// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "tst/fixture.h"

namespace odb {

// Unit tests for dbBlock::designIsRouted(): a design is "routed" when every
// signal (non-special) net with more than one pin either has routing
// (wires/vias) or is connected by abutment.  Special nets are ignored.
class TestDesignIsRouted : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    loadTechAndLib(
        "tech", "Nangate45.lef", "_main/test/Nangate45/Nangate45.lef");
    dbChip* chip = dbChip::create(db_.get(), db_->getTech());
    block_ = dbBlock::create(chip, "top");
  }

  // Create a 2-pin signal net connecting the output of one inverter to the
  // input of another (pin_count == 2, so it must be routed to pass).
  dbNet* makeMultiPinNet(const std::string& name)
  {
    dbNet* net = dbNet::create(block_, name.c_str());
    dbMaster* inv = db_->findMaster("INV_X1");
    dbInst* i0 = dbInst::create(block_, inv, (name + "_i0").c_str());
    dbInst* i1 = dbInst::create(block_, inv, (name + "_i1").c_str());
    i0->findITerm("ZN")->connect(net);
    i1->findITerm("A")->connect(net);
    return net;
  }

  // Attach a trivial metal1 wire so the net counts as routed.
  void routeNet(dbNet* net)
  {
    dbTechLayer* metal1 = db_->getTech()->findLayer("metal1");
    dbWire* wire = dbWire::create(net);
    dbWireEncoder encoder;
    encoder.begin(wire);
    encoder.newPath(metal1, dbWireType::ROUTED);
    encoder.addPoint(0, 0);
    encoder.addPoint(0, 1000);
    encoder.end();
  }

  dbBlock* block_;
};

// A design whose multi-pin signal net carries wires is fully routed.
TEST_F(TestDesignIsRouted, RoutedSignalNet)
{
  dbNet* net = makeMultiPinNet("n1");
  routeNet(net);

  EXPECT_TRUE(block_->designIsRouted(/*verbose=*/false));
}

// Special (power/ground) nets are skipped entirely: an unrouted special net
// does not make the design unrouted.
TEST_F(TestDesignIsRouted, SpecialNetsAreIgnored)
{
  dbNet* net = makeMultiPinNet("vdd");
  net->setSpecial();
  // Intentionally left unrouted.

  EXPECT_TRUE(block_->designIsRouted(/*verbose=*/false));
}

// A mix of an (ignored) special net and a routed signal net is fully routed.
TEST_F(TestDesignIsRouted, MixedSpecialAndRoutedSignalNets)
{
  dbNet* special = makeMultiPinNet("vdd");
  special->setSpecial();

  dbNet* signal = makeMultiPinNet("n1");
  routeNet(signal);

  EXPECT_TRUE(block_->designIsRouted(/*verbose=*/false));
}

// A net with a single pin needs no routing and never makes a design unrouted.
TEST_F(TestDesignIsRouted, SinglePinNetIsRouted)
{
  dbNet* net = dbNet::create(block_, "n1");
  dbInst* inst = dbInst::create(block_, db_->findMaster("INV_X1"), "i0");
  inst->findITerm("A")->connect(net);

  EXPECT_TRUE(block_->designIsRouted(/*verbose=*/false));
}

// A design with no nets is trivially routed.
TEST_F(TestDesignIsRouted, EmptyDesignIsRouted)
{
  EXPECT_TRUE(block_->designIsRouted(/*verbose=*/false));
}

// A multi-pin signal net with no wires (and no abutment) is unrouted.
TEST_F(TestDesignIsRouted, UnroutedSignalNetFails)
{
  makeMultiPinNet("n1");

  EXPECT_FALSE(block_->designIsRouted(/*verbose=*/false));
}

// An unrouted signal net is detected even when ignored special nets coexist.
TEST_F(TestDesignIsRouted, UnroutedSignalNetWithSpecialNetsFails)
{
  dbNet* special = makeMultiPinNet("vdd");
  special->setSpecial();

  makeMultiPinNet("n1");  // unrouted signal net

  EXPECT_FALSE(block_->designIsRouted(/*verbose=*/false));
}

// Non-verbose mode reports an unrouted design without emitting any warning.
TEST_F(TestDesignIsRouted, NonVerboseEmitsNoWarning)
{
  makeMultiPinNet("n1");
  makeMultiPinNet("n2");

  const int warnings_before = logger_.getWarningCount();
  EXPECT_FALSE(block_->designIsRouted(/*verbose=*/false));
  EXPECT_EQ(logger_.getWarningCount(), warnings_before);
}

// Verbose mode returns false and warns once per unrouted net (ODB-0232),
// scanning all nets rather than short-circuiting.
TEST_F(TestDesignIsRouted, VerboseWarnsForEachUnroutedNet)
{
  makeMultiPinNet("n1");
  makeMultiPinNet("n2");

  const int warnings_before = logger_.getWarningCount();
  EXPECT_FALSE(block_->designIsRouted(/*verbose=*/true));
  EXPECT_EQ(logger_.getWarningCount(), warnings_before + 2);
}

}  // namespace odb
