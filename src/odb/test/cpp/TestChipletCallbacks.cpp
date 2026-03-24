// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

// Tests for dbChipletCallBackObj callback infrastructure.
// Verifies that chiplet-level mutations fire the correct callbacks.

#include <algorithm>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbChipletCallBackObj.h"
#include "odb/geom.h"
#include "tst/fixture.h"

namespace odb {
namespace {

// Test callback that records events as strings
class TestChipletCbk : public dbChipletCallBackObj
{
 public:
  std::vector<std::string> events;

  void clearEvents() { events.clear(); }
  bool hasEvent(const std::string& e) const
  {
    return std::find(events.begin(), events.end(), e) != events.end();
  }

  // dbChipInst
  void inDbChipInstCreate(dbChipInst* inst) override
  {
    events.push_back("ChipInstCreate:" + inst->getName());
  }
  void inDbChipInstDestroy(dbChipInst* inst) override
  {
    events.push_back("ChipInstDestroy:" + inst->getName());
  }
  void inDbChipInstPreModify(dbChipInst* inst) override
  {
    events.push_back("ChipInstPreModify:" + inst->getName());
  }
  void inDbChipInstPostModify(dbChipInst* inst) override
  {
    events.push_back("ChipInstPostModify:" + inst->getName());
  }

  // dbChip
  void inDbChipPreModify(dbChip* chip) override
  {
    events.push_back("ChipPreModify:" + std::string(chip->getName()));
  }
  void inDbChipPostModify(dbChip* chip) override
  {
    events.push_back("ChipPostModify:" + std::string(chip->getName()));
  }

  // dbChipConn
  void inDbChipConnCreate(dbChipConn* conn) override
  {
    events.push_back("ChipConnCreate:" + conn->getName());
  }
  void inDbChipConnDestroy(dbChipConn* conn) override
  {
    events.push_back("ChipConnDestroy:" + conn->getName());
  }
  void inDbChipConnPreModify(dbChipConn* conn) override
  {
    events.push_back("ChipConnPreModify:" + conn->getName());
  }
  void inDbChipConnPostModify(dbChipConn* conn) override
  {
    events.push_back("ChipConnPostModify:" + conn->getName());
  }

  // dbChipNet
  void inDbChipNetCreate(dbChipNet* net) override
  {
    events.push_back("ChipNetCreate:" + net->getName());
  }
  void inDbChipNetDestroy(dbChipNet* net) override
  {
    events.push_back("ChipNetDestroy:" + net->getName());
  }
  void inDbChipNetAddBumpInst(dbChipNet* net,
                              dbChipBumpInst*,
                              const std::vector<dbChipInst*>&) override
  {
    events.push_back("ChipNetAddBumpInst:" + net->getName());
  }

  // dbChipRegion
  void inDbChipRegionCreate(dbChipRegion* region) override
  {
    events.push_back("ChipRegionCreate:" + region->getName());
  }
  void inDbChipRegionPreModify(dbChipRegion*) override
  {
    events.emplace_back("ChipRegionPreModify");
  }
  void inDbChipRegionPostModify(dbChipRegion*) override
  {
    events.emplace_back("ChipRegionPostModify");
  }

  // dbChipBump
  void inDbChipBumpCreate(dbChipBump*) override
  {
    events.emplace_back("ChipBumpCreate");
  }
};

class ChipletCallbackFixture : public tst::Fixture
{
 protected:
  ChipletCallbackFixture()
  {
    tech_ = dbTech::create(db_.get(), "tech");
    top_chip_
        = dbChip::create(db_.get(), nullptr, "TopChip", dbChip::ChipType::HIER);
    die_chip_
        = dbChip::create(db_.get(), tech_, "DieChip", dbChip::ChipType::DIE);
    die_chip_->setWidth(2000);
    die_chip_->setHeight(2000);
    die_chip_->setThickness(500);

    auto region = dbChipRegion::create(
        die_chip_, "front", dbChipRegion::Side::FRONT, nullptr);
    region->setBox(Rect(0, 0, 2000, 2000));
  }

  dbTech* tech_ = nullptr;
  dbChip* top_chip_ = nullptr;
  dbChip* die_chip_ = nullptr;
  TestChipletCbk cbk_;
};

// ---- Registration tests ----

TEST_F(ChipletCallbackFixture, callback_not_fired_before_registration)
{
  // No registration — callback should not fire
  dbChipInst::create(top_chip_, die_chip_, "inst1");
  EXPECT_TRUE(cbk_.events.empty());
}

TEST_F(ChipletCallbackFixture, callback_fired_after_registration)
{
  cbk_.addOwner(db_.get());
  dbChipInst::create(top_chip_, die_chip_, "inst1");
  ASSERT_FALSE(cbk_.events.empty());
  EXPECT_EQ(cbk_.events.back(), "ChipInstCreate:inst1");
  cbk_.removeOwner();
}

TEST_F(ChipletCallbackFixture, callback_not_fired_after_unregistration)
{
  cbk_.addOwner(db_.get());
  cbk_.removeOwner();
  dbChipInst::create(top_chip_, die_chip_, "inst1");
  EXPECT_TRUE(cbk_.events.empty());
}

// ---- ChipInst lifecycle tests ----

TEST_F(ChipletCallbackFixture, chip_inst_create_fires_callback)
{
  cbk_.addOwner(db_.get());
  dbChipInst* inst = dbChipInst::create(top_chip_, die_chip_, "inst1");
  ASSERT_NE(inst, nullptr);

  // Create fires: region create callbacks (from master) + inst create
  EXPECT_TRUE(cbk_.hasEvent("ChipInstCreate:inst1"));
}

TEST_F(ChipletCallbackFixture, chip_inst_destroy_fires_callback)
{
  cbk_.addOwner(db_.get());
  dbChipInst* inst = dbChipInst::create(top_chip_, die_chip_, "inst1");
  cbk_.clearEvents();

  dbChipInst::destroy(inst);
  ASSERT_FALSE(cbk_.events.empty());
  EXPECT_EQ(cbk_.events[0], "ChipInstDestroy:inst1");
}

// ---- ChipInst property change tests (generated setters via "notify") ----

TEST_F(ChipletCallbackFixture, chip_inst_set_orient_fires_pre_post)
{
  cbk_.addOwner(db_.get());
  dbChipInst* inst = dbChipInst::create(top_chip_, die_chip_, "inst1");
  cbk_.clearEvents();

  inst->setOrient(dbOrientType3D(dbOrientType::R90, false));

  ASSERT_EQ(cbk_.events.size(), 2);
  EXPECT_EQ(cbk_.events[0], "ChipInstPreModify:inst1");
  EXPECT_EQ(cbk_.events[1], "ChipInstPostModify:inst1");
}

TEST_F(ChipletCallbackFixture, chip_inst_set_loc_fires_pre_post)
{
  cbk_.addOwner(db_.get());
  dbChipInst* inst = dbChipInst::create(top_chip_, die_chip_, "inst1");
  inst->setOrient(dbOrientType3D(dbOrientType::R0, false));
  cbk_.clearEvents();

  // setLoc() calls generated setOrigin() which fires callbacks
  inst->setLoc(Point3D(100, 200, 300));

  ASSERT_EQ(cbk_.events.size(), 2);
  EXPECT_EQ(cbk_.events[0], "ChipInstPreModify:inst1");
  EXPECT_EQ(cbk_.events[1], "ChipInstPostModify:inst1");
}

// ---- Chip geometry tests (generated setters via "notify") ----

TEST_F(ChipletCallbackFixture, chip_set_width_fires_pre_post)
{
  cbk_.addOwner(db_.get());
  // Clear events from constructor setWidth/setHeight/setThickness calls
  cbk_.clearEvents();

  die_chip_->setWidth(3000);

  ASSERT_EQ(cbk_.events.size(), 2);
  EXPECT_EQ(cbk_.events[0], "ChipPreModify:DieChip");
  EXPECT_EQ(cbk_.events[1], "ChipPostModify:DieChip");
}

TEST_F(ChipletCallbackFixture, chip_set_seal_ring_fires_pre_post)
{
  cbk_.addOwner(db_.get());
  cbk_.clearEvents();

  die_chip_->setSealRingEast(100);

  ASSERT_EQ(cbk_.events.size(), 2);
  EXPECT_EQ(cbk_.events[0], "ChipPreModify:DieChip");
  EXPECT_EQ(cbk_.events[1], "ChipPostModify:DieChip");
}

TEST_F(ChipletCallbackFixture, chip_shrink_does_not_fire_callback)
{
  cbk_.addOwner(db_.get());
  cbk_.clearEvents();

  // shrink_ does NOT have "notify" flag — no callback expected
  die_chip_->setShrink(0.5f);

  EXPECT_TRUE(cbk_.events.empty());
}

// ---- Connection tests ----

TEST_F(ChipletCallbackFixture, chip_conn_create_destroy_fires_callbacks)
{
  cbk_.addOwner(db_.get());
  dbChipInst* inst1 = dbChipInst::create(top_chip_, die_chip_, "inst1");
  dbChipInst* inst2 = dbChipInst::create(top_chip_, die_chip_, "inst2");
  auto* ri1 = inst1->findChipRegionInst("front");
  auto* ri2 = inst2->findChipRegionInst("front");
  cbk_.clearEvents();

  dbChipConn* conn
      = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  ASSERT_NE(conn, nullptr);
  EXPECT_TRUE(cbk_.hasEvent("ChipConnCreate:c1"));

  cbk_.clearEvents();
  dbChipConn::destroy(conn);
  ASSERT_FALSE(cbk_.events.empty());
  EXPECT_EQ(cbk_.events[0], "ChipConnDestroy:c1");
}

TEST_F(ChipletCallbackFixture, chip_conn_set_thickness_fires_pre_post)
{
  cbk_.addOwner(db_.get());
  dbChipInst* inst1 = dbChipInst::create(top_chip_, die_chip_, "inst1");
  dbChipInst* inst2 = dbChipInst::create(top_chip_, die_chip_, "inst2");
  auto* ri1 = inst1->findChipRegionInst("front");
  auto* ri2 = inst2->findChipRegionInst("front");
  dbChipConn* conn
      = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  cbk_.clearEvents();

  conn->setThickness(100);

  ASSERT_EQ(cbk_.events.size(), 2);
  EXPECT_EQ(cbk_.events[0], "ChipConnPreModify:c1");
  EXPECT_EQ(cbk_.events[1], "ChipConnPostModify:c1");
}

// ---- Net tests ----

TEST_F(ChipletCallbackFixture, chip_net_create_destroy_fires_callbacks)
{
  cbk_.addOwner(db_.get());
  cbk_.clearEvents();

  dbChipNet* net = dbChipNet::create(top_chip_, "net1");
  ASSERT_NE(net, nullptr);
  ASSERT_FALSE(cbk_.events.empty());
  EXPECT_EQ(cbk_.events.back(), "ChipNetCreate:net1");

  cbk_.clearEvents();
  dbChipNet::destroy(net);
  ASSERT_FALSE(cbk_.events.empty());
  EXPECT_EQ(cbk_.events[0], "ChipNetDestroy:net1");
}

// ---- Region tests ----

TEST_F(ChipletCallbackFixture, chip_region_set_box_fires_pre_post)
{
  cbk_.addOwner(db_.get());
  cbk_.clearEvents();

  auto region = die_chip_->findChipRegion("front");
  ASSERT_NE(region, nullptr);

  region->setBox(Rect(10, 10, 1990, 1990));

  ASSERT_EQ(cbk_.events.size(), 2);
  EXPECT_EQ(cbk_.events[0], "ChipRegionPreModify");
  EXPECT_EQ(cbk_.events[1], "ChipRegionPostModify");
}

// ---- Multiple callbacks test ----

TEST_F(ChipletCallbackFixture, multiple_callbacks_both_receive_events)
{
  TestChipletCbk cbk2;
  cbk_.addOwner(db_.get());
  cbk2.addOwner(db_.get());

  dbChipInst::create(top_chip_, die_chip_, "inst1");

  // Both callbacks should receive the create event
  EXPECT_TRUE(cbk_.hasEvent("ChipInstCreate:inst1"));
  EXPECT_TRUE(cbk2.hasEvent("ChipInstCreate:inst1"));

  cbk2.removeOwner();
}

}  // namespace
}  // namespace odb
