// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

// End-to-end tests for the dynamic UnfoldedModel.
// Verifies that ODB mutations automatically update the model via
// the callback → UnfoldedModelUpdater → surgical method pipeline.

#include <cstddef>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbChipletCallBackObj.h"
#include "odb/geom.h"
#include "odb/unfoldedModel.h"
#include "tst/fixture.h"

namespace odb {
namespace {

// Mixin that provides shared model-inspection helpers.
// Multiple-inherited alongside tst::Fixture by every fixture that
// owns a const UnfoldedModel* model_.
class ModelHelper
{
 protected:
  const UnfoldedModel* model_ = nullptr;

  int countValidChips() const
  {
    int count = 0;
    for (const auto& chip : model_->getUnfilteredChips()) {
      if (chip.isValid()) {
        count++;
      }
    }
    return count;
  }

  int countValidConnections() const
  {
    int count = 0;
    for (const auto& conn : model_->getUnfilteredConnections()) {
      if (conn.isValid()) {
        count++;
      }
    }
    return count;
  }

  int countValidNets() const
  {
    int count = 0;
    for (const auto& net : model_->getUnfilteredNets()) {
      if (net.isValid()) {
        count++;
      }
    }
    return count;
  }

  const UnfoldedChip* findValidChip(const std::string& name) const
  {
    for (const auto& chip : model_->getUnfilteredChips()) {
      if (chip.isValid() && chip.name == name) {
        return &chip;
      }
    }
    return nullptr;
  }
};

class SurgicalUpdateFixture : public tst::Fixture, public ModelHelper
{
 protected:
  SurgicalUpdateFixture()
  {
    tech_ = dbTech::create(db_.get(), "tech");
    top_chip_
        = dbChip::create(db_.get(), nullptr, "TopChip", dbChip::ChipType::HIER);

    die1_ = dbChip::create(db_.get(), tech_, "Die1", dbChip::ChipType::DIE);
    die1_->setWidth(2000);
    die1_->setHeight(2000);
    die1_->setThickness(500);

    die2_ = dbChip::create(db_.get(), tech_, "Die2", dbChip::ChipType::DIE);
    die2_->setWidth(1500);
    die2_->setHeight(1500);
    die2_->setThickness(400);

    dbChipRegion::create(die1_, "d1_front", dbChipRegion::Side::FRONT, nullptr)
        ->setBox(Rect(0, 0, 2000, 2000));

    dbChipRegion::create(die2_, "d2_back", dbChipRegion::Side::BACK, nullptr)
        ->setBox(Rect(0, 0, 1500, 1500));

    // Create two instances and build model
    inst1_ = dbChipInst::create(top_chip_, die1_, "inst1");
    inst1_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    inst1_->setLoc(Point3D(0, 0, 0));

    inst2_ = dbChipInst::create(top_chip_, die2_, "inst2");
    inst2_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    inst2_->setLoc(Point3D(0, 0, 500));

    // Build the initial model (registers the updater callback)
    db_->setTopChip(top_chip_);
    db_->constructUnfoldedModel();
    model_ = db_->getUnfoldedModel();
    EXPECT_NE(model_, nullptr);
  }

  dbTech* tech_ = nullptr;
  dbChip* top_chip_ = nullptr;
  dbChip* die1_ = nullptr;
  dbChip* die2_ = nullptr;
  dbChipInst* inst1_ = nullptr;
  dbChipInst* inst2_ = nullptr;
};

// ---- Baseline test ----

TEST_F(SurgicalUpdateFixture, initial_model_has_two_chips)
{
  EXPECT_EQ(countValidChips(), 2);
  ASSERT_NE(findValidChip("inst1"), nullptr);
  ASSERT_NE(findValidChip("inst2"), nullptr);
}

// ---- addChipInst ----

TEST_F(SurgicalUpdateFixture, adding_chip_inst_updates_model)
{
  EXPECT_EQ(countValidChips(), 2);

  dbChipInst* inst3 = dbChipInst::create(top_chip_, die1_, "inst3");
  inst3->setOrient(dbOrientType3D(dbOrientType::R0, false));
  inst3->setLoc(Point3D(3000, 0, 0));

  EXPECT_EQ(countValidChips(), 3);
  const UnfoldedChip* uf_inst3 = findValidChip("inst3");
  ASSERT_NE(uf_inst3, nullptr);
  EXPECT_EQ(uf_inst3->chip_inst_path.back(), inst3);
}

// ---- removeChipInst ----

TEST_F(SurgicalUpdateFixture, destroying_chip_inst_tombstones_in_model)
{
  EXPECT_EQ(countValidChips(), 2);

  dbChipInst::destroy(inst2_);

  EXPECT_EQ(countValidChips(), 1);
  EXPECT_NE(findValidChip("inst1"), nullptr);
  EXPECT_EQ(findValidChip("inst2"), nullptr);
}

// ---- delta transform (translation) ----

TEST_F(SurgicalUpdateFixture, moving_chip_inst_updates_positions)
{
  const UnfoldedChip* uf1 = findValidChip("inst1");
  ASSERT_NE(uf1, nullptr);

  int old_x = uf1->cuboid.xMin();

  // Move inst1 by 500 in X
  inst1_->setLoc(Point3D(500, 0, 0));

  // The model should have been updated via delta transform
  int new_x = uf1->cuboid.xMin();
  EXPECT_EQ(new_x, old_x + 500);
}

// ---- chip geometry change ----

TEST_F(SurgicalUpdateFixture, changing_chip_width_updates_all_instances)
{
  const UnfoldedChip* uf1 = findValidChip("inst1");
  ASSERT_NE(uf1, nullptr);

  int old_width = uf1->cuboid.xMax() - uf1->cuboid.xMin();

  // Change the chip type's width
  die1_->setWidth(3000);

  // The unfolded chip should reflect the new geometry
  int new_width = uf1->cuboid.xMax() - uf1->cuboid.xMin();
  EXPECT_EQ(new_width, 3000);
  EXPECT_NE(new_width, old_width);
}

// ---- connection add/remove ----

TEST_F(SurgicalUpdateFixture, adding_connection_updates_model)
{
  EXPECT_EQ(countValidConnections(), 0);

  auto* ri1 = inst1_->findChipRegionInst("d1_front");
  auto* ri2 = inst2_->findChipRegionInst("d2_back");
  ASSERT_NE(ri1, nullptr);
  ASSERT_NE(ri2, nullptr);

  dbChipConn* conn
      = dbChipConn::create("c1", top_chip_, {inst1_}, ri1, {inst2_}, ri2);
  conn->setThickness(0);

  EXPECT_EQ(countValidConnections(), 1);
}

TEST_F(SurgicalUpdateFixture, destroying_connection_tombstones_in_model)
{
  auto* ri1 = inst1_->findChipRegionInst("d1_front");
  auto* ri2 = inst2_->findChipRegionInst("d2_back");
  dbChipConn* conn
      = dbChipConn::create("c1", top_chip_, {inst1_}, ri1, {inst2_}, ri2);

  EXPECT_EQ(countValidConnections(), 1);

  dbChipConn::destroy(conn);

  EXPECT_EQ(countValidConnections(), 0);
}

// ---- net add/remove ----

TEST_F(SurgicalUpdateFixture, adding_net_updates_model)
{
  EXPECT_EQ(countValidNets(), 0);

  dbChipNet::create(top_chip_, "net1");

  EXPECT_EQ(countValidNets(), 1);
}

TEST_F(SurgicalUpdateFixture, destroying_net_tombstones_in_model)
{
  dbChipNet* net = dbChipNet::create(top_chip_, "net1");
  EXPECT_EQ(countValidNets(), 1);

  dbChipNet::destroy(net);
  EXPECT_EQ(countValidNets(), 0);
}

// ---- destroying chip inst cleans up connections ----

TEST_F(SurgicalUpdateFixture, destroying_inst_tombstones_its_connections)
{
  auto* ri1 = inst1_->findChipRegionInst("d1_front");
  auto* ri2 = inst2_->findChipRegionInst("d2_back");
  dbChipConn::create("c1", top_chip_, {inst1_}, ri1, {inst2_}, ri2);
  EXPECT_EQ(countValidConnections(), 1);

  // Destroying inst2 should also tombstone the connection
  dbChipInst::destroy(inst2_);

  EXPECT_EQ(countValidConnections(), 0);
  EXPECT_EQ(countValidChips(), 1);
}

// ---- thread safety: shared_lock read while model exists ----

TEST_F(SurgicalUpdateFixture, read_guard_provides_thread_safe_access)
{
  UnfoldedModel::ReadGuard guard(model_);
  EXPECT_TRUE(static_cast<bool>(guard));
  EXPECT_EQ(guard->getUnfilteredChips().size(),
            model_->getUnfilteredChips().size());
}

TEST_F(SurgicalUpdateFixture, read_guard_handles_nullptr)
{
  UnfoldedModel::ReadGuard guard(nullptr);
  EXPECT_FALSE(static_cast<bool>(guard));
}

// ====================================================================
// Initial-build coverage
//
// Verify that connections and nets that already exist on the ODB chip
// when constructUnfoldedModel() is called are correctly picked up by
// the unfoldConnections / unfoldNets build-time paths (not via
// surgical callbacks).
// ====================================================================

// Fixture that creates connection and net BEFORE calling constructUnfoldedModel,
// so the initial build paths (unfoldConnections/unfoldNets) are exercised.
class InitialBuildWithConnsFixture : public tst::Fixture, public ModelHelper
{
 protected:
  InitialBuildWithConnsFixture()
  {
    tech_ = dbTech::create(db_.get(), "tech");
    top_chip_
        = dbChip::create(db_.get(), nullptr, "TopChip", dbChip::ChipType::HIER);

    die1_ = dbChip::create(db_.get(), tech_, "Die1", dbChip::ChipType::DIE);
    die1_->setWidth(2000);
    die1_->setHeight(2000);
    die1_->setThickness(500);

    die2_ = dbChip::create(db_.get(), tech_, "Die2", dbChip::ChipType::DIE);
    die2_->setWidth(1500);
    die2_->setHeight(1500);
    die2_->setThickness(400);

    dbChipRegion::create(die1_, "d1_front", dbChipRegion::Side::FRONT, nullptr)
        ->setBox(Rect(0, 0, 2000, 2000));
    dbChipRegion::create(die2_, "d2_back", dbChipRegion::Side::BACK, nullptr)
        ->setBox(Rect(0, 0, 1500, 1500));

    inst1_ = dbChipInst::create(top_chip_, die1_, "inst1");
    inst1_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    inst1_->setLoc(Point3D(0, 0, 0));

    inst2_ = dbChipInst::create(top_chip_, die2_, "inst2");
    inst2_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    inst2_->setLoc(Point3D(0, 0, 500));

    dbChipRegionInst* ri1 = inst1_->findChipRegionInst("d1_front");
    dbChipRegionInst* ri2 = inst2_->findChipRegionInst("d2_back");
    EXPECT_NE(ri1, nullptr);
    EXPECT_NE(ri2, nullptr);

    // Create connection and net BEFORE model construction to exercise the
    // unfoldConnections / unfoldNets initial-build code paths.
    conn_ = dbChipConn::create("pre_conn", top_chip_, {inst1_}, ri1, {inst2_}, ri2);
    conn_->setThickness(0);
    net_ = dbChipNet::create(top_chip_, "pre_net");

    db_->setTopChip(top_chip_);
    db_->constructUnfoldedModel();
    model_ = db_->getUnfoldedModel();
    EXPECT_NE(model_, nullptr);
  }

  dbTech* tech_ = nullptr;
  dbChip* top_chip_ = nullptr;
  dbChip* die1_ = nullptr;
  dbChip* die2_ = nullptr;
  dbChipInst* inst1_ = nullptr;
  dbChipInst* inst2_ = nullptr;
  dbChipConn* conn_ = nullptr;
  dbChipNet* net_ = nullptr;
};

// The connection created before construction must appear in the model.
TEST_F(InitialBuildWithConnsFixture, initial_build_unfolds_preexisting_connection)
{
  // One connection must be visible through the filtered view
  EXPECT_EQ(
      std::ranges::distance(model_->getConnections()), 1);

  for (const auto& conn : model_->getConnections()) {
    EXPECT_EQ(conn.connection, conn_);
    // Both region endpoints must be resolved
    EXPECT_NE(conn.top_region, nullptr);
    EXPECT_NE(conn.bottom_region, nullptr);
    // Top-level connection: parent_path is empty
    EXPECT_TRUE(conn.parent_path.empty());
  }
}

// The net created before construction must appear in the model.
TEST_F(InitialBuildWithConnsFixture, initial_build_unfolds_preexisting_net)
{
  // One net must be visible through the filtered view
  EXPECT_EQ(
      std::ranges::distance(model_->getNets()), 1);

  for (const auto& net : model_->getNets()) {
    EXPECT_EQ(net.chip_net, net_);
    // Top-level net: parent_path is empty
    EXPECT_TRUE(net.parent_path.empty());
  }
}

// ====================================================================
// Multi-Instance Hierarchy Bug tests
//
// When a HIER sub-chip is instantiated twice, surgical methods must
// unfold objects into BOTH contexts, not just the first one found.
// ====================================================================

// Fixture that builds a two-level hierarchy:
//   TopChip (HIER)
//     ├── sub_inst_a → SubHier (HIER)
//     │                  └── leaf_inst → LeafDie (DIE)
//     └── sub_inst_b → SubHier (HIER)
//                        └── leaf_inst → LeafDie (DIE)
//
// The SubHier chip is instantiated TWICE. Any addition to SubHier
// must appear under both sub_inst_a and sub_inst_b.
class MultiInstHierFixture : public tst::Fixture, public ModelHelper
{
 protected:
  MultiInstHierFixture()
  {
    tech_ = dbTech::create(db_.get(), "tech");

    top_chip_
        = dbChip::create(db_.get(), nullptr, "Top", dbChip::ChipType::HIER);

    sub_hier_
        = dbChip::create(db_.get(), nullptr, "SubHier", dbChip::ChipType::HIER);

    leaf_die_
        = dbChip::create(db_.get(), tech_, "LeafDie", dbChip::ChipType::DIE);
    leaf_die_->setWidth(1000);
    leaf_die_->setHeight(1000);
    leaf_die_->setThickness(200);

    leaf_die2_
        = dbChip::create(db_.get(), tech_, "LeafDie2", dbChip::ChipType::DIE);
    leaf_die2_->setWidth(800);
    leaf_die2_->setHeight(800);
    leaf_die2_->setThickness(150);

    // Create regions on the leaf dies
    dbChipRegion::create(
        leaf_die_, "lf_front", dbChipRegion::Side::FRONT, nullptr)
        ->setBox(Rect(0, 0, 1000, 1000));
    dbChipRegion::create(
        leaf_die_, "lf_back", dbChipRegion::Side::BACK, nullptr)
        ->setBox(Rect(0, 0, 1000, 1000));

    dbChipRegion::create(
        leaf_die2_, "lf2_front", dbChipRegion::Side::FRONT, nullptr)
        ->setBox(Rect(0, 0, 800, 800));

    // Build the initial hierarchy: SubHier contains one leaf instance
    leaf_in_sub_ = dbChipInst::create(sub_hier_, leaf_die_, "leaf_inst");
    leaf_in_sub_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    leaf_in_sub_->setLoc(Point3D(0, 0, 0));

    // Instantiate SubHier twice in TopChip
    sub_inst_a_ = dbChipInst::create(top_chip_, sub_hier_, "sub_inst_a");
    sub_inst_a_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    sub_inst_a_->setLoc(Point3D(0, 0, 0));

    sub_inst_b_ = dbChipInst::create(top_chip_, sub_hier_, "sub_inst_b");
    sub_inst_b_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    sub_inst_b_->setLoc(Point3D(2000, 0, 0));

    // Build the model
    db_->setTopChip(top_chip_);
    db_->constructUnfoldedModel();
    model_ = db_->getUnfoldedModel();
    EXPECT_NE(model_, nullptr);
  }

  int countValidBumpsInNets() const
  {
    int count = 0;
    for (const auto& net : model_->getUnfilteredNets()) {
      if (net.isValid()) {
        count += static_cast<int>(net.connected_bumps.size());
      }
    }
    return count;
  }

  dbTech* tech_ = nullptr;
  dbChip* top_chip_ = nullptr;
  dbChip* sub_hier_ = nullptr;
  dbChip* leaf_die_ = nullptr;
  dbChip* leaf_die2_ = nullptr;
  dbChipInst* leaf_in_sub_ = nullptr;
  dbChipInst* sub_inst_a_ = nullptr;
  dbChipInst* sub_inst_b_ = nullptr;
};

// Baseline: initial model unfolds leaf_inst under both sub_inst_a and
// sub_inst_b
TEST_F(MultiInstHierFixture, baseline_two_unfolded_leaves)
{
  // SubHier has one leaf → 2 unfolded chips (one per parent context)
  EXPECT_EQ(countValidChips(), 2);
  ASSERT_NE(findValidChip("sub_inst_a/leaf_inst"), nullptr);
  ASSERT_NE(findValidChip("sub_inst_b/leaf_inst"), nullptr);
}

// BUG: addChipInst only adds to the first parent path.
// Adding a new chip inst to SubHier should create unfolded chips
// under BOTH sub_inst_a and sub_inst_b.
TEST_F(MultiInstHierFixture, add_chip_inst_unfolds_in_all_parent_contexts)
{
  EXPECT_EQ(countValidChips(), 2);

  // Dynamically add a second leaf to SubHier
  dbChipInst* leaf2 = dbChipInst::create(sub_hier_, leaf_die2_, "leaf_inst2");
  leaf2->setOrient(dbOrientType3D(dbOrientType::R0, false));
  leaf2->setLoc(Point3D(0, 0, 200));

  // Should now have 4 unfolded chips: 2 original + 2 new (one per parent)
  EXPECT_EQ(countValidChips(), 4);
  EXPECT_NE(findValidChip("sub_inst_a/leaf_inst2"), nullptr);
  EXPECT_NE(findValidChip("sub_inst_b/leaf_inst2"), nullptr);
}

// BUG: addConnection only resolves paths under the first parent.
// A connection defined in SubHier should be unfolded in both contexts.
TEST_F(MultiInstHierFixture, add_connection_unfolds_in_all_parent_contexts)
{
  EXPECT_EQ(countValidConnections(), 0);

  // Create a connection inside SubHier between leaf_inst regions
  auto* ri_front = leaf_in_sub_->findChipRegionInst("lf_front");
  auto* ri_back = leaf_in_sub_->findChipRegionInst("lf_back");
  ASSERT_NE(ri_front, nullptr);
  ASSERT_NE(ri_back, nullptr);

  dbChipConn* conn = dbChipConn::create(
      "c1", sub_hier_, {leaf_in_sub_}, ri_front, {leaf_in_sub_}, ri_back);
  conn->setThickness(0);

  // The connection should appear in both contexts
  EXPECT_EQ(countValidConnections(), 2);
}

// BUG: addNet only resolves bump references under the first parent.
TEST_F(MultiInstHierFixture, add_net_unfolds_in_all_parent_contexts)
{
  EXPECT_EQ(countValidNets(), 0);

  // Create a net on SubHier (no bump insts yet, just structural)
  dbChipNet::create(sub_hier_, "net1");

  // The net should appear once per parent context
  EXPECT_EQ(countValidNets(), 2);
}

// ====================================================================
// Phase 1: Vector reallocation pointer invalidation
// ====================================================================

TEST_F(SurgicalUpdateFixture, vector_realloc_does_not_invalidate_net_pointers)
{
  // Add many nets to force std::vector reallocation.
  // If unfolded_nets_ is a vector, the pointer stored for the first net
  // becomes dangling after reallocation.
  dbChipNet* first_net = dbChipNet::create(top_chip_, "net_0");
  EXPECT_EQ(countValidNets(), 1);

  // Record the address of the first unfolded net
  const UnfoldedNet* first_ptr = nullptr;
  for (const auto& n : model_->getUnfilteredNets()) {
    if (n.isValid() && n.chip_net == first_net) {
      first_ptr = &n;
      break;
    }
  }
  ASSERT_NE(first_ptr, nullptr);

  // Add enough nets to force reallocation (typical initial capacity ~1-8)
  for (int i = 1; i <= 200; i++) {
    dbChipNet::create(top_chip_, "net_" + std::to_string(i));
  }
  EXPECT_EQ(countValidNets(), 201);

  // The first net must still be at the same address (pointer-stable).
  // With std::vector this would be a dangling pointer after reallocation.
  const UnfoldedNet* check_ptr = nullptr;
  for (const auto& n : model_->getUnfilteredNets()) {
    if (n.isValid() && n.chip_net == first_net) {
      check_ptr = &n;
      break;
    }
  }
  EXPECT_EQ(check_ptr, first_ptr);
}

TEST_F(SurgicalUpdateFixture, vector_realloc_does_not_invalidate_conn_pointers)
{
  auto* ri1 = inst1_->findChipRegionInst("d1_front");
  auto* ri2 = inst2_->findChipRegionInst("d2_back");
  ASSERT_NE(ri1, nullptr);
  ASSERT_NE(ri2, nullptr);

  dbChipConn* first_conn
      = dbChipConn::create("c_0", top_chip_, {inst1_}, ri1, {inst2_}, ri2);
  EXPECT_EQ(countValidConnections(), 1);

  const UnfoldedConnection* first_ptr = nullptr;
  for (const auto& c : model_->getUnfilteredConnections()) {
    if (c.isValid() && c.connection == first_conn) {
      first_ptr = &c;
      break;
    }
  }
  ASSERT_NE(first_ptr, nullptr);

  // Add many more connections to force reallocation
  for (int i = 1; i <= 200; i++) {
    dbChipConn::create(
        "c_" + std::to_string(i), top_chip_, {inst1_}, ri1, {inst2_}, ri2);
  }
  EXPECT_EQ(countValidConnections(), 201);

  // First connection pointer must still be valid
  const UnfoldedConnection* check_ptr = nullptr;
  for (const auto& c : model_->getUnfilteredConnections()) {
    if (c.isValid() && c.connection == first_conn) {
      check_ptr = &c;
      break;
    }
  }
  EXPECT_EQ(check_ptr, first_ptr);
}

// ====================================================================
// Phase 2: conn_map_ / net_map_ overwrite on hierarchical sub-chips
// ====================================================================

TEST_F(MultiInstHierFixture, remove_net_cleans_all_hierarchical_instances)
{
  // Create a net on SubHier — should unfold into 2 contexts
  dbChipNet* net = dbChipNet::create(sub_hier_, "shared_net");
  EXPECT_EQ(countValidNets(), 2);

  // Remove the net — ALL unfolded instances must be tombstoned
  dbChipNet::destroy(net);
  EXPECT_EQ(countValidNets(), 0);
}

TEST_F(MultiInstHierFixture, remove_conn_cleans_all_hierarchical_instances)
{
  auto* ri_front = leaf_in_sub_->findChipRegionInst("lf_front");
  auto* ri_back = leaf_in_sub_->findChipRegionInst("lf_back");

  dbChipConn* conn = dbChipConn::create("shared_conn",
                                        sub_hier_,
                                        {leaf_in_sub_},
                                        ri_front,
                                        {leaf_in_sub_},
                                        ri_back);
  conn->setThickness(0);
  EXPECT_EQ(countValidConnections(), 2);

  // Remove the connection — ALL unfolded instances must be tombstoned
  dbChipConn::destroy(conn);
  EXPECT_EQ(countValidConnections(), 0);
}

// After explicitly removing a connection, then removing one of the HIER
// parent instances, the model must remain consistent: no extra tombstones,
// correct chip and connection counts, and compact() produces a clean result.
TEST_F(MultiInstHierFixture, remove_conn_before_inst_leaves_model_consistent)
{
  auto* ri_front = leaf_in_sub_->findChipRegionInst("lf_front");
  auto* ri_back = leaf_in_sub_->findChipRegionInst("lf_back");

  dbChipConn* conn = dbChipConn::create("shared_conn",
                                        sub_hier_,
                                        {leaf_in_sub_},
                                        ri_front,
                                        {leaf_in_sub_},
                                        ri_back);
  conn->setThickness(0);
  EXPECT_EQ(countValidConnections(), 2);

  // Explicitly remove the connection — 2 tombstones in unfiltered deque
  dbChipConn::destroy(conn);
  EXPECT_EQ(countValidConnections(), 0);
  EXPECT_EQ(model_->getUnfilteredConnections().size(), 2u);

  // Now remove one HIER instance — must not produce extra tombstones or
  // corrupt the connection count (inst_to_conns_ must be clean)
  dbChipInst::destroy(sub_inst_a_);
  EXPECT_EQ(countValidChips(), 1);
  EXPECT_EQ(countValidConnections(), 0);
  EXPECT_EQ(model_->getUnfilteredConnections().size(), 2u);

  // compact() must produce a clean model
  const_cast<UnfoldedModel*>(model_)->compact();
  EXPECT_EQ(model_->getUnfilteredConnections().size(), 0u);
  EXPECT_EQ(model_->getUnfilteredChips().size(), 1u);
}

// After explicitly removing a net, then removing one of the HIER
// parent instances, the model must remain consistent.
TEST_F(MultiInstHierFixture, remove_net_before_inst_leaves_model_consistent)
{
  dbChipNet* net = dbChipNet::create(sub_hier_, "shared_net");
  EXPECT_EQ(countValidNets(), 2);

  // Explicitly remove the net — 2 tombstones in unfiltered deque
  dbChipNet::destroy(net);
  EXPECT_EQ(countValidNets(), 0);
  EXPECT_EQ(model_->getUnfilteredNets().size(), 2u);

  // Remove one HIER instance — must not produce extra tombstones or
  // corrupt the net count (inst_to_nets_ must be clean)
  dbChipInst::destroy(sub_inst_a_);
  EXPECT_EQ(countValidChips(), 1);
  EXPECT_EQ(countValidNets(), 0);
  EXPECT_EQ(model_->getUnfilteredNets().size(), 2u);

  // compact() must produce a clean model
  const_cast<UnfoldedModel*>(model_)->compact();
  EXPECT_EQ(model_->getUnfilteredNets().size(), 0u);
  EXPECT_EQ(model_->getUnfilteredChips().size(), 1u);
}

// ====================================================================
// Phase 3a: Translation delta math under rotated parent
// ====================================================================

TEST_F(SurgicalUpdateFixture, region_geometry_update_fixes_bump_z)
{
  // updateRegionGeometry (triggered by setBox) must also update bump Z.
  // Create a second region definition on die2_ with FRONT side, and add a
  // new bump-bearing region that we can modify.
  // Instead we use the existing d2_back region on inst2_, which is BOTTOM.
  // Its surface Z = cuboid.zMin() = inst2 z-origin = 500.

  const UnfoldedChip* uf2 = findValidChip("inst2");
  ASSERT_NE(uf2, nullptr);

  // Find the BOTTOM region on inst2
  const UnfoldedRegion* back_region = nullptr;
  for (const auto& r : uf2->getRegions()) {
    if (r.isValid() && r.isBottom()) {
      back_region = &r;
      break;
    }
  }
  ASSERT_NE(back_region, nullptr);

  // For BOTTOM region, surface Z = cuboid.zMin()
  int initial_z = back_region->getSurfaceZ();
  EXPECT_EQ(initial_z, 500);  // inst2 at z=500, BACK z_min_=0 → global zMin=500

  // Now change the region's box. The cuboid Y range changes which also
  // changes the transformed cuboid. But more importantly for our test:
  // bump Z values must be refreshed to getSurfaceZ() after the change.
  // We use die2_'s BACK region. We can change its box which triggers
  // updateRegionGeometry.
  dbChipRegion* d2_back_region = nullptr;
  for (auto* r : die2_->getChipRegions()) {
    if (r->getName() == "d2_back") {
      d2_back_region = r;
      break;
    }
  }
  ASSERT_NE(d2_back_region, nullptr);

  // Shrink the box — this triggers updateRegionGeometry callback.
  // The Z doesn't change here (BACK Z is still 0 in local coordinates),
  // but the cuboid's XY changes, and bumps should not be left stale.
  // For a more direct Z test, we verify the bump Z gets recalculated
  // from getSurfaceZ() after the region cuboid update.
  d2_back_region->setBox(Rect(100, 100, 1400, 1400));

  // After the update, the region cuboid should reflect the new box
  int new_xmin = back_region->cuboid.xMin();
  EXPECT_EQ(new_xmin, 100);  // Transformed: inst2 at x=0 → 0+100 = 100

  // Surface Z should still be consistent with cuboid.zMin()
  int new_z = back_region->getSurfaceZ();
  EXPECT_EQ(new_z, back_region->cuboid.zMin());
}

// ====================================================================
// Adversarial Bug Tests
//
// These tests assert the CORRECT expected behavior.  Each will FAIL
// against the current implementation, proving the bug exists.
// ====================================================================

// ---------- EmptyHierFixture ----------
// A HIER sub-chip that starts with NO children at model-construction
// time.  Used to test collectParentContexts when chip_path_map_ has
// no leaf paths going through the HIER type.
class EmptyHierFixture : public tst::Fixture, public ModelHelper
{
 protected:
  EmptyHierFixture()
  {
    tech_ = dbTech::create(db_.get(), "tech");
    top_chip_
        = dbChip::create(db_.get(), nullptr, "Top", dbChip::ChipType::HIER);

    empty_hier_ = dbChip::create(
        db_.get(), nullptr, "EmptyHier", dbChip::ChipType::HIER);

    leaf_die_
        = dbChip::create(db_.get(), tech_, "LeafDie", dbChip::ChipType::DIE);
    leaf_die_->setWidth(1000);
    leaf_die_->setHeight(1000);
    leaf_die_->setThickness(200);

    dbChipRegion::create(
        leaf_die_, "lf_front", dbChipRegion::Side::FRONT, nullptr)
        ->setBox(Rect(0, 0, 1000, 1000));

    // EmptyHier has NO children — deliberately empty at construction
    inst_a_ = dbChipInst::create(top_chip_, empty_hier_, "inst_a");
    inst_a_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    inst_a_->setLoc(Point3D(0, 0, 0));

    inst_b_ = dbChipInst::create(top_chip_, empty_hier_, "inst_b");
    inst_b_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    inst_b_->setLoc(Point3D(2000, 0, 0));

    db_->setTopChip(top_chip_);
    db_->constructUnfoldedModel();
    model_ = db_->getUnfoldedModel();
    EXPECT_NE(model_, nullptr);
  }

  dbTech* tech_ = nullptr;
  dbChip* top_chip_ = nullptr;
  dbChip* empty_hier_ = nullptr;
  dbChip* leaf_die_ = nullptr;
  dbChipInst* inst_a_ = nullptr;
  dbChipInst* inst_b_ = nullptr;
};

// ---- BUG: collectParentContexts fallback on initially-empty HIER ----
//
// collectParentContexts() discovers HIER parent contexts by scanning
// chip_path_map_ for leaf paths whose prefix includes an instance of
// the target chip type.  When the HIER has ZERO leaf descendants
// (never had children, or all children were removed), chip_path_map_
// has no relevant entries.  The fallback returns {{}, identity} — the
// TOP-LEVEL context — so new children are unfolded at the wrong
// hierarchy level.
TEST_F(EmptyHierFixture, first_child_to_empty_hier_placed_at_wrong_level)
{
  // Initially empty HIER → no unfolded chips
  EXPECT_EQ(countValidChips(), 0);

  // Add the first leaf to EmptyHier
  dbChipInst* leaf = dbChipInst::create(empty_hier_, leaf_die_, "leaf");
  leaf->setOrient(dbOrientType3D(dbOrientType::R0, false));
  leaf->setLoc(Point3D(0, 0, 0));

  // Expected: 2 unfolded chips — inst_a/leaf and inst_b/leaf
  // BUG: collectParentContexts falls back to top-level → creates 1 chip
  //      named "leaf" instead of two chips under each HIER instance
  EXPECT_EQ(countValidChips(), 2);
  EXPECT_NE(findValidChip("inst_a/leaf"), nullptr);
  EXPECT_NE(findValidChip("inst_b/leaf"), nullptr);
}

// ---- BUG: Same fallback, triggered by removing all leaves first ----
//
// Start with SubHier containing one leaf.  After destroying that leaf,
// chip_path_map_ loses all entries referencing SubHier.  Re-adding a
// leaf hits the same empty-path fallback.
TEST_F(MultiInstHierFixture, readd_leaf_after_removing_all_goes_to_wrong_level)
{
  EXPECT_EQ(countValidChips(), 2);

  // Remove the ONLY leaf from SubHier — tombstones both contexts
  dbChipInst::destroy(leaf_in_sub_);
  EXPECT_EQ(countValidChips(), 0);

  // Add a new leaf to the now-empty SubHier
  dbChipInst* new_leaf = dbChipInst::create(sub_hier_, leaf_die2_, "new_leaf");
  new_leaf->setOrient(dbOrientType3D(dbOrientType::R0, false));
  new_leaf->setLoc(Point3D(0, 0, 0));

  // Expected: 2 unfolded chips (one per HIER context)
  // BUG: 1 chip named "new_leaf" placed directly under top
  EXPECT_EQ(countValidChips(), 2);
  EXPECT_NE(findValidChip("sub_inst_a/new_leaf"), nullptr);
  EXPECT_NE(findValidChip("sub_inst_b/new_leaf"), nullptr);
}

// ---- BUG: Net on empty HIER also goes to wrong context ----
TEST_F(EmptyHierFixture, net_on_empty_hier_placed_at_wrong_level)
{
  EXPECT_EQ(countValidNets(), 0);

  // Create a net on the (currently empty) HIER sub-chip
  dbChipNet::create(empty_hier_, "net1");

  // Expected: 2 unfolded nets — one per HIER instance context
  // BUG: collectParentContexts fallback → 1 net at top level
  EXPECT_EQ(countValidNets(), 2);
}

// ---- BUG: Connection on empty HIER goes to wrong context ----
TEST_F(EmptyHierFixture, connection_on_empty_hier_at_wrong_level)
{
  // Add two leaves so we have regions to connect
  dbChip* leaf_die2
      = dbChip::create(db_.get(), tech_, "LeafDie2", dbChip::ChipType::DIE);
  leaf_die2->setWidth(800);
  leaf_die2->setHeight(800);
  leaf_die2->setThickness(150);
  dbChipRegion::create(leaf_die2, "lf2_back", dbChipRegion::Side::BACK, nullptr)
      ->setBox(Rect(0, 0, 800, 800));

  dbChipInst* leaf1 = dbChipInst::create(empty_hier_, leaf_die_, "leaf1");
  leaf1->setOrient(dbOrientType3D(dbOrientType::R0, false));
  leaf1->setLoc(Point3D(0, 0, 0));

  dbChipInst* leaf2 = dbChipInst::create(empty_hier_, leaf_die2, "leaf2");
  leaf2->setOrient(dbOrientType3D(dbOrientType::R0, false));
  leaf2->setLoc(Point3D(0, 0, 200));

  // Due to the context fallback bug, leaves are at the wrong level.
  // Now create a connection between them on EmptyHier.
  auto* ri_front = leaf1->findChipRegionInst("lf_front");
  auto* ri_back = leaf2->findChipRegionInst("lf2_back");
  ASSERT_NE(ri_front, nullptr);
  ASSERT_NE(ri_back, nullptr);

  dbChipConn* conn = dbChipConn::create(
      "c1", empty_hier_, {leaf1}, ri_front, {leaf2}, ri_back);
  conn->setThickness(0);

  // Expected: 2 connections (one per HIER context)
  // BUG: leaves are at wrong level → at most 1 connection resolves
  EXPECT_EQ(countValidConnections(), 2);
}

// ---- BUG: removeChipInst doesn't tombstone orphaned nets ----
//
// When a HIER instance is destroyed, removeChipInst tombstones the
// leaf chips, regions, and bumps.  It cleans stale bumps from nets.
// But it does NOT tombstone the unfolded nets that were created in the
// destroyed context.  Those nets remain isValid()=true with no backing
// chips or bumps.
TEST_F(MultiInstHierFixture, orphaned_nets_after_removing_hier_instance)
{
  // Create a net on SubHier — unfolded into 2 contexts
  dbChipNet::create(sub_hier_, "net1");
  EXPECT_EQ(countValidNets(), 2);

  // Destroy one HIER instance (sub_inst_a)
  dbChipInst::destroy(sub_inst_a_);
  EXPECT_EQ(countValidChips(), 1);

  // The unfolded net in sub_inst_a's context is now orphaned — its
  // parent chips are tombstoned but the net itself is still valid.
  // Expected: 1 valid net (only sub_inst_b context survives)
  // BUG: 2 valid nets remain (orphaned net is never tombstoned)
  EXPECT_EQ(countValidNets(), 1);
}

// ---- BUG: Removing the shared leaf orphans nets in ALL contexts ----
TEST_F(MultiInstHierFixture, orphaned_nets_after_removing_shared_leaf)
{
  // Create a net on SubHier — 2 unfolded nets
  dbChipNet::create(sub_hier_, "net1");
  EXPECT_EQ(countValidNets(), 2);

  // Remove the shared leaf — both contexts lose their chips
  dbChipInst::destroy(leaf_in_sub_);
  EXPECT_EQ(countValidChips(), 0);

  // Both unfolded nets are orphaned — no chips or bumps remain.
  // Expected: 0 valid nets
  // BUG: 2 valid nets remain (isValid() never set to false)
  EXPECT_EQ(countValidNets(), 0);
}

// ---- BUG: Tombstone accumulation — chip deque grows without bound ----
//
// Each add/remove cycle leaves a tombstoned (isValid()=false) entry in
// the deque.  No compaction or reclaim mechanism exists.  After N
// cycles the deque has N extra dead entries that slow down linear scans
// (updateRegionGeometry, removeChipInst connection cleanup, etc.) and
// waste memory.
TEST_F(SurgicalUpdateFixture, chip_deque_grows_without_bound)
{
  const size_t initial_total = model_->getUnfilteredChips().size();
  EXPECT_EQ(initial_total, 2u);

  // 100 add/remove cycles — tombstones accumulate (by design)
  for (int i = 0; i < 100; i++) {
    dbChipInst* tmp
        = dbChipInst::create(top_chip_, die1_, "tmp_" + std::to_string(i));
    tmp->setOrient(dbOrientType3D(dbOrientType::R0, false));
    tmp->setLoc(Point3D(i * 3000, 0, 0));
    dbChipInst::destroy(tmp);
  }

  // Valid count is stable
  EXPECT_EQ(countValidChips(), 2);

  // Tombstones accumulate until explicit compact()
  EXPECT_GT(model_->getUnfilteredChips().size(), initial_total);

  // After compact(), deque is clean
  const_cast<UnfoldedModel*>(model_)->compact();
  EXPECT_LE(model_->getUnfilteredChips().size(), initial_total);
}

TEST_F(SurgicalUpdateFixture, net_deque_grows_without_bound)
{
  for (int i = 0; i < 50; i++) {
    dbChipNet* net = dbChipNet::create(top_chip_, "net_" + std::to_string(i));
    dbChipNet::destroy(net);
  }

  EXPECT_EQ(countValidNets(), 0);

  // Tombstones accumulate until explicit compact()
  EXPECT_EQ(model_->getUnfilteredNets().size(), 50u);

  const_cast<UnfoldedModel*>(model_)->compact();
  EXPECT_EQ(model_->getUnfilteredNets().size(), 0u);
}

TEST_F(SurgicalUpdateFixture, connection_deque_grows_without_bound)
{
  auto* ri1 = inst1_->findChipRegionInst("d1_front");
  auto* ri2 = inst2_->findChipRegionInst("d2_back");
  ASSERT_NE(ri1, nullptr);
  ASSERT_NE(ri2, nullptr);

  for (int i = 0; i < 50; i++) {
    dbChipConn* conn = dbChipConn::create(
        "conn_" + std::to_string(i), top_chip_, {inst1_}, ri1, {inst2_}, ri2);
    conn->setThickness(0);
    dbChipConn::destroy(conn);
  }

  EXPECT_EQ(countValidConnections(), 0);

  // Tombstones accumulate until explicit compact()
  EXPECT_EQ(model_->getUnfilteredConnections().size(), 50u);

  const_cast<UnfoldedModel*>(model_)->compact();
  EXPECT_EQ(model_->getUnfilteredConnections().size(), 0u);
}

// ---------- BumpHierFixture ----------
// Multi-instance hierarchy WITH physical bumps.  Used to test the
// addBumpInstToNet cross-contamination bug.
//
// Hierarchy:
//   Top (HIER)
//     ├── sub_inst_a → SubHier (HIER)
//     │     └── leaf_inst → LeafDie (DIE) → lf_front region → bump
//     └── sub_inst_b → SubHier (HIER)
//           └── leaf_inst → LeafDie (DIE) → lf_front region → bump
class BumpHierFixture : public tst::Fixture, public ModelHelper
{
 protected:
  BumpHierFixture()
  {
    tech_ = dbTech::create(db_.get(), "tech");
    dbTechLayer::create(tech_, "layer1", dbTechLayerType::ROUTING);

    top_chip_
        = dbChip::create(db_.get(), nullptr, "Top", dbChip::ChipType::HIER);
    sub_hier_
        = dbChip::create(db_.get(), nullptr, "SubHier", dbChip::ChipType::HIER);

    leaf_die_
        = dbChip::create(db_.get(), tech_, "LeafDie", dbChip::ChipType::DIE);
    leaf_die_->setWidth(1000);
    leaf_die_->setHeight(1000);
    leaf_die_->setThickness(200);

    // Create block on the leaf die for physical bump instances
    dbBlock::create(leaf_die_, "block");

    // Lib + bump master
    lib_ = dbLib::create(db_.get(), "bump_lib", tech_, ',');
    bump_master_ = dbMaster::create(lib_, "bump_pad");
    bump_master_->setWidth(100);
    bump_master_->setHeight(100);
    bump_master_->setType(dbMasterType::CORE);
    dbMTerm::create(bump_master_, "pin", dbIoType::INOUT, dbSigType::SIGNAL);
    bump_master_->setFrozen();

    // Create a region on the leaf die with one bump
    lf_front_ = dbChipRegion::create(
        leaf_die_, "lf_front", dbChipRegion::Side::FRONT, nullptr);
    lf_front_->setBox(Rect(0, 0, 1000, 1000));

    dbBlock* block = leaf_die_->getBlock();
    dbInst* pad_inst = dbInst::create(block, bump_master_, "pad0");
    pad_inst->setOrigin(500, 500);
    pad_inst->setPlacementStatus(dbPlacementStatus::PLACED);
    dbChipBump::create(lf_front_, pad_inst);

    // Build the hierarchy
    leaf_in_sub_ = dbChipInst::create(sub_hier_, leaf_die_, "leaf_inst");
    leaf_in_sub_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    leaf_in_sub_->setLoc(Point3D(0, 0, 0));

    sub_inst_a_ = dbChipInst::create(top_chip_, sub_hier_, "sub_inst_a");
    sub_inst_a_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    sub_inst_a_->setLoc(Point3D(0, 0, 0));

    sub_inst_b_ = dbChipInst::create(top_chip_, sub_hier_, "sub_inst_b");
    sub_inst_b_->setOrient(dbOrientType3D(dbOrientType::R0, false));
    sub_inst_b_->setLoc(Point3D(2000, 0, 0));

    db_->setTopChip(top_chip_);
    db_->constructUnfoldedModel();
    model_ = db_->getUnfoldedModel();
    EXPECT_NE(model_, nullptr);
  }

  int countTotalBumpsInValidNets() const
  {
    int count = 0;
    for (const auto& net : model_->getUnfilteredNets()) {
      if (net.isValid()) {
        count += static_cast<int>(net.connected_bumps.size());
      }
    }
    return count;
  }

  int countBumpsInRegions() const
  {
    int count = 0;
    for (const auto& chip : model_->getUnfilteredChips()) {
      if (!chip.isValid()) {
        continue;
      }
      for (const auto& region : chip.getRegions()) {
        auto bumps = region.getBumps();
        std::vector<UnfoldedBump> valid_bumps(bumps.begin(), bumps.end());
        count += static_cast<int>(valid_bumps.size());
      }
    }
    return count;
  }

  dbTech* tech_ = nullptr;
  dbLib* lib_ = nullptr;
  dbMaster* bump_master_ = nullptr;
  dbChip* top_chip_ = nullptr;
  dbChip* sub_hier_ = nullptr;
  dbChip* leaf_die_ = nullptr;
  dbChipRegion* lf_front_ = nullptr;
  dbChipInst* leaf_in_sub_ = nullptr;
  dbChipInst* sub_inst_a_ = nullptr;
  dbChipInst* sub_inst_b_ = nullptr;
};

// ---- BUG: addBumpInstToNet cross-contaminates hierarchy contexts ----
//
// addBumpInstToNet() iterates ALL entries in bump_inst_map_ for each
// matching unfolded net.  In a multi-instance hierarchy, the same
// dbChipBumpInst* appears in multiple map entries with different path
// keys (one per context).  The method adds ALL matching bumps to ALL
// matching nets, instead of scoping each net to its own hierarchy
// context.
//
// Result: each unfolded net gets N bumps instead of 1, where N is the
// number of parent HIER instances.
TEST_F(BumpHierFixture, addBumpInstToNet_cross_contaminates_contexts)
{
  // Verify bump infrastructure: 2 unfolded bumps (one per context)
  EXPECT_EQ(countBumpsInRegions(), 2);

  // Create a net on SubHier — should unfold into 2 contexts
  dbChipNet* net = dbChipNet::create(sub_hier_, "net1");
  EXPECT_EQ(countValidNets(), 2);

  // Add the bump_inst to the net
  auto* ri = leaf_in_sub_->findChipRegionInst("lf_front");
  ASSERT_NE(ri, nullptr);
  auto bump_insts = ri->getChipBumpInsts();
  ASSERT_EQ(bump_insts.size(), 1u);
  dbChipBumpInst* bump_inst = *bump_insts.begin();

  // addBumpInst triggers callback → addBumpInstToNet
  net->addBumpInst(bump_inst, {leaf_in_sub_});

  // Expected: 2 total bump references — 1 per unfolded net
  // BUG: addBumpInstToNet scans ALL bump_inst_map_ entries matching
  //      the bump_inst pointer.  Both contexts match, so each of the
  //      2 unfolded nets gets 2 bumps → 4 total.
  EXPECT_EQ(countTotalBumpsInValidNets(), 2);
}

// ---- BUG: Each unfolded net should have exactly 1 bump ----
//
// Stricter per-net check: verifies that bumps are scoped to their
// own hierarchy context and don't leak across.
TEST_F(BumpHierFixture, each_unfolded_net_gets_only_its_own_bump)
{
  dbChipNet* net = dbChipNet::create(sub_hier_, "net1");

  auto* ri = leaf_in_sub_->findChipRegionInst("lf_front");
  ASSERT_NE(ri, nullptr);
  dbChipBumpInst* bump_inst = *ri->getChipBumpInsts().begin();
  net->addBumpInst(bump_inst, {leaf_in_sub_});

  // Check per-net bump count
  for (const auto& uf_net : model_->getUnfilteredNets()) {
    if (!uf_net.isValid() || uf_net.chip_net != net) {
      continue;
    }
    // Expected: exactly 1 bump per unfolded net
    // BUG: 2 bumps per net (cross-contamination from other context)
    EXPECT_EQ(uf_net.connected_bumps.size(), 1u);
  }
}

// ---- BUG: After remove+readd, transform recompute visits stale data ----
//
// After removing a HIER instance and re-creating it, the new instance's
// subtree should be fully independent.  Moving the shared leaf should
// update positions in both contexts correctly.
TEST_F(MultiInstHierFixture, remove_then_readd_transform_consistency)
{
  EXPECT_EQ(countValidChips(), 2);

  // Remove sub_inst_a and its subtree
  dbChipInst::destroy(sub_inst_a_);
  EXPECT_EQ(countValidChips(), 1);

  // Re-create an instance of SubHier in TopChip
  dbChipInst* sub_inst_a2
      = dbChipInst::create(top_chip_, sub_hier_, "sub_inst_a2");
  sub_inst_a2->setOrient(dbOrientType3D(dbOrientType::R0, false));
  sub_inst_a2->setLoc(Point3D(500, 0, 0));

  // Should have 2 valid chips again
  EXPECT_EQ(countValidChips(), 2);

  const UnfoldedChip* chip_a2 = findValidChip("sub_inst_a2/leaf_inst");
  ASSERT_NE(chip_a2, nullptr);

  // sub_inst_a2 is at x=500, leaf_inst at x=0 within SubHier
  // Expected global xMin = 500
  EXPECT_EQ(chip_a2->cuboid.xMin(), 500);

  // sub_inst_b's leaf should still be at x=2000
  const UnfoldedChip* chip_b = findValidChip("sub_inst_b/leaf_inst");
  ASSERT_NE(chip_b, nullptr);
  EXPECT_EQ(chip_b->cuboid.xMin(), 2000);
}

// ---- BUG: Connections also go to wrong context after leaf removal ----
//
// After removing all leaves from SubHier and re-adding two new ones,
// connections defined on SubHier between the new leaves are placed
// at the wrong hierarchy level due to the collectParentContexts bug.
TEST_F(MultiInstHierFixture, conn_after_removing_all_leaves_wrong_context)
{
  EXPECT_EQ(countValidChips(), 2);

  // Add a second leaf to SubHier so we have two regions to connect
  dbChip* leaf_die3
      = dbChip::create(db_.get(), tech_, "LeafDie3", dbChip::ChipType::DIE);
  leaf_die3->setWidth(800);
  leaf_die3->setHeight(800);
  leaf_die3->setThickness(150);
  dbChipRegion::create(leaf_die3, "lf3_back", dbChipRegion::Side::BACK, nullptr)
      ->setBox(Rect(0, 0, 800, 800));

  dbChipInst* leaf2 = dbChipInst::create(sub_hier_, leaf_die3, "leaf_inst2");
  leaf2->setOrient(dbOrientType3D(dbOrientType::R0, false));
  leaf2->setLoc(Point3D(0, 0, 200));

  EXPECT_EQ(countValidChips(), 4);

  // Remove BOTH leaves — empties SubHier from chip_path_map_
  dbChipInst::destroy(leaf_in_sub_);
  dbChipInst::destroy(leaf2);
  EXPECT_EQ(countValidChips(), 0);

  // Re-add two leaves
  dbChipInst* new_leaf1 = dbChipInst::create(sub_hier_, leaf_die_, "new_leaf1");
  new_leaf1->setOrient(dbOrientType3D(dbOrientType::R0, false));
  new_leaf1->setLoc(Point3D(0, 0, 0));

  dbChipInst* new_leaf2 = dbChipInst::create(sub_hier_, leaf_die3, "new_leaf2");
  new_leaf2->setOrient(dbOrientType3D(dbOrientType::R0, false));
  new_leaf2->setLoc(Point3D(0, 0, 200));

  auto* ri_front = new_leaf1->findChipRegionInst("lf_front");
  auto* ri_back = new_leaf2->findChipRegionInst("lf3_back");
  ASSERT_NE(ri_front, nullptr);
  ASSERT_NE(ri_back, nullptr);

  dbChipConn* conn = dbChipConn::create(
      "c1", sub_hier_, {new_leaf1}, ri_front, {new_leaf2}, ri_back);
  conn->setThickness(0);

  // Expected: 2 connections (one per HIER context)
  // BUG: leaves are at the wrong level → connections resolve incorrectly
  EXPECT_EQ(countValidConnections(), 2);
}

// ====================================================================
// Bug: Data Race in UnfoldedModelUpdater::inDbChipInstPreModify
//
// inDbChipInstPreModify writes to saved_transforms_ (unordered_map)
// WITHOUT acquiring any lock (unfoldedModelUpdater.cpp:33).
// inDbChipInstPostModify reads/erases from the SAME map UNDER the
// model's unique_lock (unfoldedModelUpdater.cpp:38).
//
// Concurrent setLoc calls from multiple threads trigger concurrent
// PreModify callbacks, all racing to mutate saved_transforms_.
// std::unordered_map is not thread-safe for concurrent writes — even
// to different keys — because insertions can trigger a rehash.
//
// Under ThreadSanitizer (TSan) this test reliably reports the race.
// Without TSan the race may silently corrupt the map, causing some
// delta transforms to be lost (positions become stale/wrong).
// ====================================================================

TEST_F(SurgicalUpdateFixture, data_race_in_premodify_saved_transforms)
{
  // Create extra instances so each thread operates on its own
  dbChipInst* inst3 = dbChipInst::create(top_chip_, die1_, "inst3");
  inst3->setOrient(dbOrientType3D(dbOrientType::R0, false));
  inst3->setLoc(Point3D(3000, 0, 0));

  dbChipInst* inst4 = dbChipInst::create(top_chip_, die1_, "inst4");
  inst4->setOrient(dbOrientType3D(dbOrientType::R0, false));
  inst4->setLoc(Point3D(6000, 0, 0));

  const int iterations = 500;

  // Each thread moves its own instance many times, then sets a known
  // final position.  All threads concurrently write to the SAME
  // saved_transforms_ map inside inDbChipInstPreModify (no lock).
  auto move_instance = [](dbChipInst* inst, int final_x) {
    for (int i = 0; i < iterations; i++) {
      inst->setLoc(Point3D(final_x + i, i, 0));
    }
    // Final known position
    inst->setLoc(Point3D(final_x, 0, 0));
  };

  std::thread t1(move_instance, inst1_, 100);
  std::thread t2(move_instance, inst2_, 200);
  std::thread t3(move_instance, inst3, 300);
  std::thread t4(move_instance, inst4, 400);

  t1.join();
  t2.join();
  t3.join();
  t4.join();

  // If saved_transforms_ was corrupted by the data race, some delta
  // transforms would be lost and positions would be stale/wrong.
  const UnfoldedChip* uf1 = findValidChip("inst1");
  const UnfoldedChip* uf2 = findValidChip("inst2");
  const UnfoldedChip* uf3 = findValidChip("inst3");
  const UnfoldedChip* uf4 = findValidChip("inst4");

  ASSERT_NE(uf1, nullptr);
  ASSERT_NE(uf2, nullptr);
  ASSERT_NE(uf3, nullptr);
  ASSERT_NE(uf4, nullptr);

  EXPECT_EQ(uf1->cuboid.xMin(), 100);
  EXPECT_EQ(uf2->cuboid.xMin(), 200);
  EXPECT_EQ(uf3->cuboid.xMin(), 300);
  EXPECT_EQ(uf4->cuboid.xMin(), 400);
}

// ====================================================================
// Bug: Missing nullptr guard on getDatabase() in generated setters
//
// Generated setters with the "notify" flag (impl.cpp.jinja:152-153):
//
//   _dbDatabase* _notifyDb = obj->getDatabase();
//   for (auto _notifyCb : _notifyDb->chiplet_callbacks_) { ... }
//
// No nullptr check on _notifyDb.  For objects created through the
// public API, getDatabase() walks the intact owner chain and always
// succeeds.  But the code is STRUCTURALLY unsafe: if the owner chain
// is ever broken (table corruption, custom allocations, mid-destroy
// access), getDatabase() returns nullptr and the next line segfaults.
//
// These tests verify the happy path works and document the risk.
// ====================================================================

class NotifyNullCheckFixture : public tst::Fixture
{
 protected:
  NotifyNullCheckFixture()
  {
    tech_ = dbTech::create(db_.get(), "tech");
    top_chip_
        = dbChip::create(db_.get(), nullptr, "TopChip", dbChip::ChipType::HIER);

    die1_ = dbChip::create(db_.get(), tech_, "Die1", dbChip::ChipType::DIE);
  }

  dbTech* tech_ = nullptr;
  dbChip* top_chip_ = nullptr;
  dbChip* die1_ = nullptr;
};

// Verify setters with "notify" flag work when NO callbacks are
// registered (empty chiplet_callbacks_ list).  The generated code
// dereferences getDatabase() and iterates the list — with an empty
// list the loop body never executes, so this is a no-op.
// If getDatabase() returned nullptr, we'd crash here.
TEST_F(NotifyNullCheckFixture, notify_setter_no_callbacks_registered)
{
  // No constructUnfoldedModel() — no callbacks exist yet.
  // setWidth triggers the generated notify path.
  die1_->setWidth(2000);
  EXPECT_EQ(die1_->getWidth(), 2000);

  die1_->setHeight(3000);
  EXPECT_EQ(die1_->getHeight(), 3000);

  die1_->setThickness(500);
  EXPECT_EQ(die1_->getThickness(), 500);
}

// Verify setters correctly fire callbacks when registered.
TEST_F(NotifyNullCheckFixture, notify_setter_fires_registered_callback)
{
  die1_->setWidth(1000);
  die1_->setHeight(1000);
  die1_->setThickness(200);

  dbChipRegion::create(die1_, "front", dbChipRegion::Side::FRONT, nullptr)
      ->setBox(Rect(0, 0, 1000, 1000));

  dbChipInst* inst = dbChipInst::create(top_chip_, die1_, "inst1");
  inst->setOrient(dbOrientType3D(dbOrientType::R0, false));
  inst->setLoc(Point3D(0, 0, 0));

  db_->setTopChip(top_chip_);
  db_->constructUnfoldedModel();
  const UnfoldedModel* model = db_->getUnfoldedModel();
  ASSERT_NE(model, nullptr);

  // setWidth fires notify → updateChipGeometry callback updates model
  die1_->setWidth(2000);

  for (const auto& chip : model->getUnfilteredChips()) {
    if (chip.isValid()) {
      int width = chip.cuboid.xMax() - chip.cuboid.xMin();
      EXPECT_EQ(width, 2000);
    }
  }
}

// ====================================================================
// Bug: Iterator invalidation when callback deregisters during notify
//
// Generated setters iterate chiplet_callbacks_ (std::list) with a
// range-based for loop WITHOUT copying the list first:
//
//   for (auto _notifyCb : _notifyDb->chiplet_callbacks_) {
//     _notifyCb->inDb...PreModify(...);
//   }
//
// If a callback calls removeOwner() during its invocation, it calls
// std::list::remove(this), erasing itself from the list.  This
// invalidates the iterator the range-based for loop is using.
// ++iterator on an invalidated std::list iterator is undefined
// behavior (use-after-free on the erased list node).
//
// Under AddressSanitizer (ASan) this is detected as use-after-free.
// Without ASan, the freed node's next pointer usually hasn't been
// overwritten yet, so ++iterator appears to work — but it's still UB.
//
// These tests verify that callbacks AFTER the self-deregistering one
// still fire.  They serve as a regression canary: if the std::list
// allocator changes (or ASan is enabled), the UB manifests as either
// a crash or a missed callback.
// ====================================================================

// Callback that deregisters itself when fired
class SelfDeregisteringCallback : public dbChipletCallBackObj
{
 public:
  void inDbChipPreModify(dbChip*) override
  {
    fired_ = true;
    // removeOwner() calls chiplet_callbacks_.remove(this), mutating
    // the list that the generated for-loop is actively iterating.
    removeOwner();
  }
  bool fired_ = false;
};

// Callback that records whether it was invoked
class FiringTracker : public dbChipletCallBackObj
{
 public:
  void inDbChipPreModify(dbChip*) override { fired_ = true; }
  bool fired_ = false;
};

TEST_F(SurgicalUpdateFixture, iterator_invalidation_single_self_deregistration)
{
  // Registration order determines iteration order (std::list push_back).
  // The UnfoldedModelUpdater was registered first by constructUnfoldedModel.
  FiringTracker tracker_before;
  tracker_before.addOwner(db_.get());

  SelfDeregisteringCallback suicide;
  suicide.addOwner(db_.get());

  FiringTracker tracker_after;
  tracker_after.addOwner(db_.get());

  // setWidth fires inDbChipPreModify for each callback in list order.
  // When suicide's turn comes, it calls removeOwner() which erases
  // itself from the list.  The range-based for loop's iterator now
  // points to the freed list node.  ++iterator is UB.
  die1_->setWidth(9999);

  EXPECT_TRUE(tracker_before.fired_);
  EXPECT_TRUE(suicide.fired_);

  // If the iterator was invalidated, tracker_after may not fire.
  // Under ASan this is detected as use-after-free.
  // Without ASan, this usually passes "by accident" (freed node's
  // next pointer still intact), but the code has real UB.
  EXPECT_TRUE(tracker_after.fired_)
      << "Callback after self-deregistration did not fire — "
         "iterator invalidation in range-based for over std::list";
}

// Stronger variant: multiple consecutive self-deregistrations increase
// the chance of detecting the UB (more freed nodes to traverse).
TEST_F(SurgicalUpdateFixture,
       iterator_invalidation_multiple_self_deregistrations)
{
  FiringTracker tracker_before;
  tracker_before.addOwner(db_.get());

  SelfDeregisteringCallback suicide1;
  suicide1.addOwner(db_.get());

  SelfDeregisteringCallback suicide2;
  suicide2.addOwner(db_.get());

  SelfDeregisteringCallback suicide3;
  suicide3.addOwner(db_.get());

  FiringTracker tracker_after;
  tracker_after.addOwner(db_.get());

  die1_->setWidth(8888);

  EXPECT_TRUE(tracker_before.fired_);
  EXPECT_TRUE(suicide1.fired_);

  // After suicide1 deregisters, the iterator is already invalid.
  // Whether suicide2, suicide3, and tracker_after fire depends on
  // whether the freed nodes' memory has been reused.
  EXPECT_TRUE(tracker_after.fired_)
      << "Multiple self-deregistrations during iteration — "
         "increased chance of iterator invalidation detection";
}

// ====================================================================
// Compaction Strategy Tests
//
// Evaluate the correctness, performance characteristics, and edge
// cases of the tombstone compaction strategy.
// ====================================================================

// --- Problem 1: Eager compact() on every removeNet is O(N) per call ---
//
// If removeNet calls compact() unconditionally, removing N nets from a
// model with M live chips costs O(N * (M + N)) because each compact()
// rebuilds all maps and cross-references.  This should be amortized.
// --- Explicit compaction: tombstones accumulate, compact() cleans ---

TEST_F(SurgicalUpdateFixture, batch_net_removal_accumulates_tombstones)
{
  std::vector<dbChipNet*> nets;
  nets.reserve(200);
  for (int i = 0; i < 200; i++) {
    nets.push_back(dbChipNet::create(top_chip_, "net_" + std::to_string(i)));
  }
  EXPECT_EQ(countValidNets(), 200);

  for (int i = 0; i < 200; i++) {
    dbChipNet::destroy(nets[i]);
  }

  // Tombstones accumulate — no synchronous compaction
  EXPECT_EQ(countValidNets(), 0);
  EXPECT_EQ(model_->getUnfilteredNets().size(), 200u);

  // Explicit compact cleans everything
  const_cast<UnfoldedModel*>(model_)->compact();
  EXPECT_EQ(model_->getUnfilteredNets().size(), 0u);
}

TEST_F(SurgicalUpdateFixture, compact_cleans_mixed_tombstones)
{
  auto* ri1 = inst1_->findChipRegionInst("d1_front");
  auto* ri2 = inst2_->findChipRegionInst("d2_back");

  // Create and destroy nets and connections
  for (int i = 0; i < 30; i++) {
    dbChipNet::destroy(
        dbChipNet::create(top_chip_, "net_" + std::to_string(i)));
    dbChipConn* conn = dbChipConn::create(
        "c_" + std::to_string(i), top_chip_, {inst1_}, ri1, {inst2_}, ri2);
    dbChipConn::destroy(conn);
  }

  // Tombstones accumulated
  EXPECT_EQ(model_->getUnfilteredNets().size(), 30u);
  EXPECT_EQ(model_->getUnfilteredConnections().size(), 30u);

  const_cast<UnfoldedModel*>(model_)->compact();
  EXPECT_EQ(model_->getUnfilteredNets().size(), 0u);
  EXPECT_EQ(model_->getUnfilteredConnections().size(), 0u);
}

// --- Correctness: compact preserves cross-references ---
TEST_F(SurgicalUpdateFixture, compact_preserves_connection_region_pointers)
{
  auto* ri1 = inst1_->findChipRegionInst("d1_front");
  auto* ri2 = inst2_->findChipRegionInst("d2_back");
  ASSERT_NE(ri1, nullptr);
  ASSERT_NE(ri2, nullptr);

  dbChipConn* conn1
      = dbChipConn::create("c1", top_chip_, {inst1_}, ri1, {inst2_}, ri2);
  conn1->setThickness(0);
  dbChipConn* conn2
      = dbChipConn::create("c2", top_chip_, {inst1_}, ri1, {inst2_}, ri2);
  conn2->setThickness(0);
  EXPECT_EQ(countValidConnections(), 2);

  dbChipConn::destroy(conn1);
  EXPECT_EQ(countValidConnections(), 1);

  // Compact with a tombstone present
  const_cast<UnfoldedModel*>(model_)->compact();

  // Surviving connection's region pointers must still be valid
  for (const auto& conn : model_->getConnections()) {
    ASSERT_NE(conn.top_region, nullptr);
    ASSERT_NE(conn.bottom_region, nullptr);
    EXPECT_EQ(conn.top_region->region_inst, ri1);
    EXPECT_EQ(conn.bottom_region->region_inst, ri2);
  }
}

// --- Correctness: interleaved add/remove + explicit compact ---
TEST_F(SurgicalUpdateFixture, interleaved_add_remove_stays_consistent)
{
  std::vector<dbChipNet*> live_nets;
  for (int i = 0; i < 100; i++) {
    for (int j = 0; j < 3; j++) {
      live_nets.push_back(
          dbChipNet::create(top_chip_, "net_" + std::to_string(i * 3 + j)));
    }
    dbChipNet::destroy(live_nets.front());
    live_nets.erase(live_nets.begin());

    // Invariant: valid count == live_nets.size()
    EXPECT_EQ(countValidNets(), static_cast<int>(live_nets.size()));
    // Tombstones accumulate between compactions
    EXPECT_GE(model_->getUnfilteredNets().size(), live_nets.size());
  }

  // Clean up remaining nets
  while (!live_nets.empty()) {
    dbChipNet::destroy(live_nets.back());
    live_nets.pop_back();
  }
  EXPECT_EQ(countValidNets(), 0);

  // Tombstones remain until compact
  EXPECT_GT(model_->getUnfilteredNets().size(), 0u);
  const_cast<UnfoldedModel*>(model_)->compact();
  EXPECT_EQ(model_->getUnfilteredNets().size(), 0u);
}

// --- Chip tombstone count diagnostic ---
TEST_F(SurgicalUpdateFixture, chip_tombstone_count_tracks_dead_chips)
{
  EXPECT_EQ(model_->getChipTombstoneCount(), 0u);

  dbChipInst* tmp = dbChipInst::create(top_chip_, die1_, "tmp_chip");
  tmp->setOrient(dbOrientType3D(dbOrientType::R0, false));
  tmp->setLoc(Point3D(9000, 0, 0));

  dbChipInst::destroy(tmp);
  EXPECT_EQ(model_->getChipTombstoneCount(), 1u);

  const_cast<UnfoldedModel*>(model_)->compact();
  EXPECT_EQ(model_->getChipTombstoneCount(), 0u);
}

// ---- WriteGuard deadlock detection ----

TEST_F(SurgicalUpdateFixture, write_guard_detects_deadlock_with_read_guard)
{
  // Hold a read guard on the current thread
  UnfoldedModel::ReadGuard rguard(model_);
  // Attempting a write guard on the same thread must throw (logger->error)
  EXPECT_ANY_THROW({
    UnfoldedModel::WriteGuard wguard(const_cast<UnfoldedModel*>(model_));
  });
}

}  // namespace
}  // namespace odb
