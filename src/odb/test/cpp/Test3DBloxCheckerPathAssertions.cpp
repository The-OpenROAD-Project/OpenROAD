// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include <string>
#include <variant>

#include "Test3DBloxCheckerFixture.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace odb {
namespace {
// ---------------------------------------------------------------------------
// Path Assertion Tests
//
// These tests exercise Checker::checkPathAssertions, which validates
// user-specified path assertions using BFS on the region routing graph.
// Graph nodes are UnfoldedRegion objects; edges come from UnfoldedConnection.
//
// Non-negated entries are must-touch regions (must be mutually reachable).
// Negated entries are do-not-touch regions (blocked during BFS traversal).
// ---------------------------------------------------------------------------

// 1. Connected must-touch regions → no violation.
TEST_F(CheckerFixture, test_path_assertion_connected)
{
  auto* inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  auto* chip_path = dbChipPath::create(top_chip_, "Path1");
  chip_path->addEntry({inst1}, ri1, false);
  chip_path->addEntry({inst2}, ri2, false);

  check();
  EXPECT_TRUE(getMarkers(path_assertions_category).empty());
}

// 2. Disconnected must-touch regions (no connection) → one violation.
TEST_F(CheckerFixture, test_path_assertion_disconnected)
{
  auto* inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  // No connection between inst1 and inst2.
  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");

  auto* chip_path = dbChipPath::create(top_chip_, "Path1");
  chip_path->addEntry({inst1}, ri1, false);
  chip_path->addEntry({inst2}, ri2, false);

  check();
  auto markers = getMarkers(path_assertions_category);
  ASSERT_EQ(markers.size(), 1);
  EXPECT_NE(markers[0]->getComment().find("Path1"), std::string::npos);

  // ri2 is unreachable from ri1 (no connection) — it must appear as a source
  // and its region cuboid must be recorded as the marker shape.
  auto sources = markers[0]->getSources();
  ASSERT_EQ(sources.size(), 1u);
  EXPECT_EQ(sources.count(ri2), 1u);

  auto shapes = markers[0]->getShapes();
  ASSERT_EQ(shapes.size(), 1u);
  EXPECT_TRUE(std::holds_alternative<Cuboid>(shapes[0]));
}

// 3. Do-not-touch blocks the only path → violation.
//    inst1.r1_fr --c1--> inst2.r2_bk <-(blackbox clique)-> inst2.r2_fr --c2-->
//    inst3.r2_bk The intra-chip clique of inst2 (blackbox) connects r2_bk and
//    r2_fr. Blocking r2_bk cuts the only entry point into inst2, making
//    inst3.r2_bk unreachable (r2_fr is only reachable through the blocked
//    r2_bk).
TEST_F(CheckerFixture, test_path_assertion_do_not_touch_blocks)
{
  auto* inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* inst3 = dbChipInst::create(top_chip_, chip2_, "inst3");
  inst3->setLoc(Point3D(0, 0, 1000));
  inst3->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2_bk = inst2->findChipRegionInst("r2_bk");
  auto* ri2_fr = inst2->findChipRegionInst("r2_fr");
  auto* ri3_bk = inst3->findChipRegionInst("r2_bk");

  auto* c1 = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2_bk);
  c1->setThickness(0);
  auto* c2
      = dbChipConn::create("c2", top_chip_, {inst2}, ri2_fr, {inst3}, ri3_bk);
  c2->setThickness(0);

  auto* chip_path = dbChipPath::create(top_chip_, "Path1");
  chip_path->addEntry({inst1}, ri1, false);
  chip_path->addEntry({inst2}, ri2_bk, true);  // do-not-touch
  chip_path->addEntry({inst3}, ri3_bk, false);

  check();
  auto markers = getMarkers(path_assertions_category);
  ASSERT_EQ(markers.size(), 1);
  EXPECT_NE(markers[0]->getComment().find("Path1"), std::string::npos);

  // ri3_bk is unreachable because ri2_bk (the only bridge) is blocked.
  // ri2_bk itself is a do-not-touch entry, not a must-touch, so it must NOT
  // appear as a source — only ri3_bk should.
  auto sources = markers[0]->getSources();
  ASSERT_EQ(sources.size(), 1u);
  EXPECT_EQ(sources.count(ri3_bk), 1u);
  EXPECT_EQ(sources.count(ri2_bk), 0u);

  auto shapes = markers[0]->getShapes();
  ASSERT_EQ(shapes.size(), 1u);
  EXPECT_TRUE(std::holds_alternative<Cuboid>(shapes[0]));
}

// 4. Do-not-touch on a non-critical region → no violation.
//    Same topology as test 3 but blocking r2_fr (not on the path) instead.
TEST_F(CheckerFixture, test_path_assertion_do_not_touch_non_critical)
{
  auto* inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2_bk = inst2->findChipRegionInst("r2_bk");
  auto* ri2_fr = inst2->findChipRegionInst("r2_fr");

  auto* conn
      = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2_bk);
  conn->setThickness(0);

  // r2_fr is not on the path from r1_fr to r2_bk, so blocking it is harmless.
  auto* chip_path = dbChipPath::create(top_chip_, "Path1");
  chip_path->addEntry({inst1}, ri1, false);
  chip_path->addEntry({inst2}, ri2_fr, true);  // do-not-touch (not on path)
  chip_path->addEntry({inst2}, ri2_bk, false);

  check();
  EXPECT_TRUE(getMarkers(path_assertions_category).empty());
}

// 5. No path assertions → no markers.
TEST_F(CheckerFixture, test_path_assertion_empty)
{
  auto* inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  check();
  EXPECT_TRUE(getMarkers(path_assertions_category).empty());
}

}  // namespace
}  // namespace odb
