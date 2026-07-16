// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include <algorithm>
#include <string>
#include <vector>

#include "Test3DBloxCheckerFixture.h"
#include "gtest/gtest.h"
#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace odb {

namespace {

class AlignmentMarkersFixture : public CheckerFixture
{
 protected:
  AlignmentMarkersFixture()
  {
    dbTechLayer::create(tech_, "layer1", dbTechLayerType::ROUTING);

    dbBlock::create(chip1_, "block1");
    dbBlock::create(chip2_, "block2");

    lib_ = dbLib::create(db_.get(), "align_lib", tech_, ',');
    marker_master_ = makeMarkerMaster("ALIGN_MARK");
  }

  dbMaster* makeMarkerMaster(const char* name)
  {
    dbMaster* m = dbMaster::create(lib_, name);
    m->setWidth(100);
    m->setHeight(100);
    // Center the master's placement bbox on its origin so the bbox center is
    // invariant under orient changes. Tests that vary orient rely on this.
    m->setOrigin(50, 50);
    m->setType(dbMasterType::CORE);
    dbMTerm::create(m, "pin", dbIoType::INOUT, dbSigType::SIGNAL);
    m->setFrozen();
    return m;
  }

  // Create a self-symmetric rule (master_a == master_b == marker_master_) so
  // markers of marker_master_ are subject to alignment checking with the given
  // tolerance.
  dbAlignmentMarkerRule* createRule(int tolerance)
  {
    return createRule(marker_master_, marker_master_, tolerance);
  }

  dbAlignmentMarkerRule* createRule(dbMaster* master_a,
                                    dbMaster* master_b,
                                    int tolerance)
  {
    auto* rule = dbAlignmentMarkerRule::create(master_a, master_b);
    rule->setTolerance(tolerance);
    return rule;
  }

  dbInst* placeMarker(dbChip* chip,
                      const char* name,
                      int x,
                      int y,
                      dbMaster* master = nullptr,
                      dbOrientType orient = dbOrientType::R0)
  {
    dbBlock* block = chip->getBlock();
    dbInst* inst
        = dbInst::create(block, master ? master : marker_master_, name);
    inst->setOrient(orient);
    inst->setOrigin(x, y);
    inst->setPlacementStatus(dbPlacementStatus::PLACED);
    return inst;
  }

  // chip1 at z=[0,500], chip2 stacked on top at z=[500,1000], bonded via a
  // dbChipConn between chip1's front region and chip2's back region. The
  // alignment-marker check is connection-aware, so the connection is required
  // for markers on the two chips to be compared.
  void stackChips()
  {
    auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
    inst1->setLoc(Point3D(0, 0, 0));
    inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

    auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
    inst2->setLoc(Point3D(0, 0, 500));
    inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

    auto* ri1 = inst1->findChipRegionInst("r1_fr");
    auto* ri2 = inst2->findChipRegionInst("r2_bk");
    auto* conn
        = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
    conn->setThickness(0);
  }

  dbLib* lib_;
  dbMaster* marker_master_;

  static constexpr const char* alignment_markers_category = "Alignment Markers";
};

TEST_F(AlignmentMarkersFixture, exactAlignmentZeroTolerance)
{
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 500, 500);
  stackChips();
  createRule(0);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 0);
}

TEST_F(AlignmentMarkersFixture, withinToleranceNoViolation)
{
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 520, 510);  // offset ~22 units
  stackChips();
  createRule(50);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 0);
}

TEST_F(AlignmentMarkersFixture, beyondToleranceViolation)
{
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 700, 700);  // offset ~283 units
  stackChips();
  createRule(50);

  check();

  // Neither marker has a valid counterpart -> two violations.
  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 2);
}

TEST_F(AlignmentMarkersFixture, orphanMarker)
{
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip1_, "m2", 800, 800);
  placeMarker(chip2_, "m1", 500, 500);
  stackChips();
  createRule(10);

  check();

  // m2 on chip1 has no counterpart -> one violation.
  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 1);
}

TEST_F(AlignmentMarkersFixture, zeroToleranceRejectsNonExactMatch)
{
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 501, 500);  // off by 1 unit
  stackChips();
  createRule(0);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 2);
}

TEST_F(AlignmentMarkersFixture, noRuleIsNoOp)
{
  // No dbAlignmentMarkerRule referencing the master, so its instances are not
  // treated as alignment markers and mismatched placements are ignored.
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 999, 999);
  stackChips();

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 0);
}

TEST_F(AlignmentMarkersFixture, emptyOrientationListAllowsAny)
{
  // Empty allowed-orientation list means "no orientation constraint".
  placeMarker(chip1_, "m1", 500, 500, nullptr, dbOrientType::R0);
  placeMarker(chip2_, "m1", 500, 500, nullptr, dbOrientType::R90);
  stackChips();
  createRule(0);  // default: getRelativeOrientations() is empty

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 0);
}

TEST_F(AlignmentMarkersFixture, relativeOrientationMatch)
{
  // Pair has relative orient R0 (both markers R0); rule allows R0 -> pass.
  placeMarker(chip1_, "m1", 500, 500, nullptr, dbOrientType::R0);
  placeMarker(chip2_, "m1", 500, 500, nullptr, dbOrientType::R0);
  stackChips();
  auto* rule = createRule(0);
  rule->addRelativeOrientation(dbOrientType::R0);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 0);
}

TEST_F(AlignmentMarkersFixture, relativeOrientationMismatchViolates)
{
  // Pair has relative orient R90 (R0^-1 * R90 = R90); rule only allows R0.
  placeMarker(chip1_, "m1", 500, 500, nullptr, dbOrientType::R0);
  placeMarker(chip2_, "m1", 500, 500, nullptr, dbOrientType::R90);
  stackChips();
  auto* rule = createRule(0);
  rule->addRelativeOrientation(dbOrientType::R0);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 1);
}

TEST_F(AlignmentMarkersFixture, selfSymmetricRuleClosesUnderInverse)
{
  // Self-symmetric rule (master_a == master_b): allowed list [R90] should
  // implicitly accept R270 too, since the relative orientation between two
  // markers of the same master is direction-ambiguous (R270 is R90's inverse).
  placeMarker(chip1_, "m1", 500, 500, nullptr, dbOrientType::R0);
  placeMarker(chip2_, "m1", 500, 500, nullptr, dbOrientType::R270);
  stackChips();
  auto* rule = createRule(0);
  rule->addRelativeOrientation(dbOrientType::R90);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 0);
}

TEST_F(AlignmentMarkersFixture, selfSymmetricRuleStillRejectsUnrelatedOrient)
{
  // Same self-symmetric rule [R90]; closure adds R270 but R180 is still
  // disallowed.
  placeMarker(chip1_, "m1", 500, 500, nullptr, dbOrientType::R0);
  placeMarker(chip2_, "m1", 500, 500, nullptr, dbOrientType::R180);
  stackChips();
  auto* rule = createRule(0);
  rule->addRelativeOrientation(dbOrientType::R90);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 1);
}

TEST_F(AlignmentMarkersFixture, rulePropertyRoundTrip)
{
  dbMaster* m_a = makeMarkerMaster("MA");
  dbMaster* m_b = makeMarkerMaster("MB");
  auto* rule = dbAlignmentMarkerRule::create(m_a, m_b);
  rule->setTolerance(42);
  rule->setRelativeOrientations({dbOrientType::R0, dbOrientType::MX});

  EXPECT_EQ(rule->getMasterA(), m_a);
  EXPECT_EQ(rule->getMasterB(), m_b);
  EXPECT_EQ(rule->getTolerance(), 42);
  // Asymmetric rule -> no inverse closure, list is preserved verbatim.
  auto orients = rule->getRelativeOrientations();
  EXPECT_EQ(orients.size(), 2);
  EXPECT_EQ(orients[0], dbOrientType::R0);
  EXPECT_EQ(orients[1], dbOrientType::MX);
}

TEST_F(AlignmentMarkersFixture, selfSymmetricRuleGetterClosesList)
{
  auto* rule = dbAlignmentMarkerRule::create(marker_master_, marker_master_);
  rule->addRelativeOrientation(dbOrientType::R90);

  auto orients = rule->getRelativeOrientations();
  // R90's inverse is R270; closure must expose both.
  EXPECT_EQ(orients.size(), 2);
  EXPECT_NE(std::ranges::find(orients, dbOrientType(dbOrientType::R90)),
            orients.end());
  EXPECT_NE(std::ranges::find(orients, dbOrientType(dbOrientType::R270)),
            orients.end());
}

}  // namespace

}  // namespace odb
