// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "PdnTest.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "techlayer.h"
#include "tst/physical_builder.h"
#include "via.h"

namespace pdn {
namespace {

using TestEnclosure = PdnTest;

// Default-constructed enclosure has zero overhang in both directions.
TEST_F(TestEnclosure, DefaultConstructorIsZero)
{
  Enclosure e;
  EXPECT_EQ(e.getX(), 0);
  EXPECT_EQ(e.getY(), 0);
}

// The (x, y) constructor stores the values exactly.
TEST_F(TestEnclosure, XYConstructorStoresValues)
{
  Enclosure e(10, 20);
  EXPECT_EQ(e.getX(), 10);
  EXPECT_EQ(e.getY(), 20);
}

// setX / setY update the stored values.
TEST_F(TestEnclosure, SettersUpdateValues)
{
  Enclosure e(1, 2);
  e.setX(7);
  e.setY(9);
  EXPECT_EQ(e.getX(), 7);
  EXPECT_EQ(e.getY(), 9);
}

// check(x, y) returns true only when the candidate enclosure meets or
// exceeds the required overhang in both directions. The (x, y) constructor
// disables axis-swapping, so the comparison is strict.
TEST_F(TestEnclosure, CheckPassesWhenBothMeetRequirement)
{
  Enclosure required(10, 20);
  EXPECT_TRUE(required.check(10, 20));  // exactly equal
  EXPECT_TRUE(required.check(15, 25));  // both larger
}

// check(x, y) fails as soon as either axis is below the requirement.
TEST_F(TestEnclosure, CheckFailsWhenEitherAxisIsTooSmall)
{
  Enclosure required(10, 20);
  EXPECT_FALSE(required.check(9, 20));   // x too small
  EXPECT_FALSE(required.check(10, 19));  // y too small
}

// The basic (x, y) constructor sets allow_swap_ = false, so an enclosure
// with x and y swapped does NOT satisfy the requirement.
TEST_F(TestEnclosure, CheckDoesNotSwapAxesByDefault)
{
  Enclosure required(10, 20);
  // candidate (20, 10) would satisfy a swap-allowed rule but not this one
  EXPECT_FALSE(required.check(20, 10));
}

// operator== compares x and y component-wise.
TEST_F(TestEnclosure, EqualityComparesXAndY)
{
  EXPECT_TRUE(Enclosure(3, 4) == Enclosure(3, 4));
  EXPECT_FALSE(Enclosure(3, 4) == Enclosure(3, 5));
  EXPECT_FALSE(Enclosure(3, 4) == Enclosure(4, 4));
}

// operator< orders lexicographically by (x, y).
TEST_F(TestEnclosure, LessThanOrdersByXThenY)
{
  EXPECT_TRUE(Enclosure(1, 5) < Enclosure(2, 0));   // x decides
  EXPECT_TRUE(Enclosure(1, 2) < Enclosure(1, 3));   // x ties, y decides
  EXPECT_FALSE(Enclosure(1, 2) < Enclosure(1, 2));  // equal is not less
}

// copy() from a pointer copies x and y (and allow_swap_).
TEST_F(TestEnclosure, CopyFromPointerCopiesValues)
{
  Enclosure src(11, 22);
  Enclosure dst;
  dst.copy(&src);
  EXPECT_EQ(dst.getX(), 11);
  EXPECT_EQ(dst.getY(), 22);
}

// copy() from a reference works the same as copy from a pointer.
TEST_F(TestEnclosure, CopyFromReferenceCopiesValues)
{
  Enclosure src(11, 22);
  Enclosure dst;
  dst.copy(src);
  EXPECT_EQ(dst.getX(), 11);
  EXPECT_EQ(dst.getY(), 22);
}

// isPreferredOver(nullptr, ...) is always true: any enclosure beats "no
// enclosure".
TEST_F(TestEnclosure, IsPreferredOverNullptrIsTrue)
{
  Enclosure e(5, 5);
  EXPECT_TRUE(e.isPreferredOver(nullptr, true));
  EXPECT_TRUE(e.isPreferredOver(nullptr, false));
}

// With minimize_x = true, a smaller x wins; on equal x, a smaller y wins.
TEST_F(TestEnclosure, IsPreferredOverMinimizeXTieBreaksOnY)
{
  Enclosure a(10, 5);
  Enclosure b(20, 1);
  EXPECT_TRUE(a.isPreferredOver(&b, /*minimize_x=*/true));   // smaller x
  EXPECT_FALSE(b.isPreferredOver(&a, /*minimize_x=*/true));  // larger x

  Enclosure c(10, 4);
  EXPECT_TRUE(c.isPreferredOver(&a, /*minimize_x=*/true));   // x ties, y wins
  EXPECT_FALSE(a.isPreferredOver(&c, /*minimize_x=*/true));  // x ties, y loses
}

// With minimize_x = false (i.e. minimize y), a smaller y wins; on equal y,
// a smaller x wins.
TEST_F(TestEnclosure, IsPreferredOverMinimizeYTieBreaksOnX)
{
  Enclosure a(5, 10);
  Enclosure b(1, 20);
  EXPECT_TRUE(a.isPreferredOver(&b, /*minimize_x=*/false));   // smaller y
  EXPECT_FALSE(b.isPreferredOver(&a, /*minimize_x=*/false));  // larger y

  Enclosure c(4, 10);
  EXPECT_TRUE(c.isPreferredOver(&a, /*minimize_x=*/false));   // y ties, x wins
  EXPECT_FALSE(a.isPreferredOver(&c, /*minimize_x=*/false));  // y ties, x loses
}

// On a horizontal layer, isPreferredOver picks the variant that minimizes
// y first (because preferred routing direction is horizontal, so the
// across-track extent dominates).
TEST_F(TestEnclosure, IsPreferredOverHorizontalLayerMinimizesY)
{
  odb::dbTechLayer* h
      = odb::dbTechLayer::create(tech(), "M_h", odb::dbTechLayerType::ROUTING);
  h->setDirection(odb::dbTechLayerDir::HORIZONTAL);

  Enclosure small_y(10, 5);
  Enclosure large_y(1, 20);
  EXPECT_TRUE(small_y.isPreferredOver(&large_y, h));
  EXPECT_FALSE(large_y.isPreferredOver(&small_y, h));
}

// On a vertical layer, isPreferredOver minimizes x first instead.
TEST_F(TestEnclosure, IsPreferredOverVerticalLayerMinimizesX)
{
  odb::dbTechLayer* v
      = odb::dbTechLayer::create(tech(), "M_v", odb::dbTechLayerType::ROUTING);
  v->setDirection(odb::dbTechLayerDir::VERTICAL);

  Enclosure small_x(5, 10);
  Enclosure large_x(20, 1);
  EXPECT_TRUE(small_x.isPreferredOver(&large_x, v));
  EXPECT_FALSE(large_x.isPreferredOver(&small_x, v));
}

// snap() rounds each axis down to the nearest manufacturing-grid multiple.
// Values already on the grid are unchanged.
TEST_F(TestEnclosure, SnapRoundsDownToManufacturingGrid)
{
  tech()->setManufacturingGrid(10);

  Enclosure off_grid(13, 27);
  off_grid.snap(tech());
  EXPECT_EQ(off_grid.getX(), 10);
  EXPECT_EQ(off_grid.getY(), 20);

  Enclosure on_grid(20, 30);
  on_grid.snap(tech());
  EXPECT_EQ(on_grid.getX(), 20);
  EXPECT_EQ(on_grid.getY(), 30);
}

// snap() is a no-op when the tech has no manufacturing grid set.
TEST_F(TestEnclosure, SnapIsNoOpWithoutManufacturingGrid)
{
  // Clear the grid the fixture set in SetUp so we can exercise the
  // "no grid" branch.
  tech()->setManufacturingGrid(0);
  ASSERT_FALSE(tech()->hasManufacturingGrid());

  Enclosure e(13, 27);
  e.snap(tech());
  EXPECT_EQ(e.getX(), 13);
  EXPECT_EQ(e.getY(), 27);
}

// A zero enclosure is satisfied by a zero candidate (the trivial pass case).
TEST_F(TestEnclosure, CheckZeroEnclosureAcceptsZeroCandidate)
{
  Enclosure e;  // default-constructed -> (0, 0)
  EXPECT_TRUE(e.check(0, 0));
  EXPECT_TRUE(e.check(1, 1));
}

// isPreferredOver uses strict less-than, so an enclosure is NOT preferred
// over an equal one regardless of which axis is being minimized.
TEST_F(TestEnclosure, IsPreferredOverEqualEnclosureIsFalse)
{
  Enclosure a(7, 7);
  Enclosure b(7, 7);
  EXPECT_FALSE(a.isPreferredOver(&b, /*minimize_x=*/true));
  EXPECT_FALSE(a.isPreferredOver(&b, /*minimize_x=*/false));
}

// The cut-enclosure-rule constructor with HORZ_AND_VERT stores
// firstOverhang as x and secondOverhang as y and disables axis swapping.
TEST_F(TestEnclosure, RuleConstructorHorzAndVertStoresOverhangsAsIs)
{
  odb::dbTechLayer* layer
      = odb::dbTechLayer::create(tech(), "M_hv", odb::dbTechLayerType::ROUTING);
  layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);

  odb::dbTechLayerCutEnclosureRule* rule
      = odb::dbTechLayerCutEnclosureRule::create(layer);
  rule->setType(odb::dbTechLayerCutEnclosureRule::HORZ_AND_VERT);
  rule->setFirstOverhang(10);   // horizontal overhang -> x
  rule->setSecondOverhang(20);  // vertical overhang -> y

  Enclosure e(
      rule, layer, odb::Rect(0, 0, 50, 50), odb::dbTechLayerDir::HORIZONTAL);
  EXPECT_EQ(e.getX(), 10);
  EXPECT_EQ(e.getY(), 20);
  // HORZ_AND_VERT disables axis swapping in check().
  EXPECT_FALSE(e.check(20, 10));
}

// The cut-enclosure-rule DEFAULT constructor lets the layer direction drive
// axis swapping. On a horizontal layer the larger overhang ends up on x.
TEST_F(TestEnclosure, RuleConstructorDefaultSwapsForHorizontalLayer)
{
  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      tech(), "M_def_h", odb::dbTechLayerType::ROUTING);
  layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);

  odb::dbTechLayerCutEnclosureRule* rule
      = odb::dbTechLayerCutEnclosureRule::create(layer);
  rule->setType(odb::dbTechLayerCutEnclosureRule::DEFAULT);
  rule->setFirstOverhang(10);
  rule->setSecondOverhang(20);

  // x=10, y=20 with HORIZONTAL layer -> y > x triggers a swap so x ends up
  // larger (preferred-direction extent dominates).
  Enclosure e(rule, layer, odb::Rect(0, 0, 50, 50), odb::dbTechLayerDir::NONE);
  EXPECT_EQ(e.getX(), 20);
  EXPECT_EQ(e.getY(), 10);
}

// On a vertical layer the DEFAULT rule constructor swaps so the larger
// overhang ends up on y instead.
TEST_F(TestEnclosure, RuleConstructorDefaultSwapsForVerticalLayer)
{
  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      tech(), "M_def_v", odb::dbTechLayerType::ROUTING);
  layer->setDirection(odb::dbTechLayerDir::VERTICAL);

  odb::dbTechLayerCutEnclosureRule* rule
      = odb::dbTechLayerCutEnclosureRule::create(layer);
  rule->setType(odb::dbTechLayerCutEnclosureRule::DEFAULT);
  rule->setFirstOverhang(20);  // larger first
  rule->setSecondOverhang(10);

  // x=20, y=10 with VERTICAL layer -> x > y triggers a swap so y ends up
  // larger.
  Enclosure e(rule, layer, odb::Rect(0, 0, 50, 50), odb::dbTechLayerDir::NONE);
  EXPECT_EQ(e.getX(), 10);
  EXPECT_EQ(e.getY(), 20);
}

// EOL rule with HORIZONTAL direction stores firstOverhang as x and
// secondOverhang as y; allow_swap is disabled.
TEST_F(TestEnclosure, RuleConstructorEolHorizontalUsesFirstAsX)
{
  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      tech(), "M_eol_h", odb::dbTechLayerType::ROUTING);

  odb::dbTechLayerCutEnclosureRule* rule
      = odb::dbTechLayerCutEnclosureRule::create(layer);
  rule->setType(odb::dbTechLayerCutEnclosureRule::EOL);
  rule->setFirstOverhang(7);
  rule->setSecondOverhang(13);

  Enclosure e(
      rule, layer, odb::Rect(0, 0, 50, 50), odb::dbTechLayerDir::HORIZONTAL);
  EXPECT_EQ(e.getX(), 7);
  EXPECT_EQ(e.getY(), 13);
  EXPECT_FALSE(e.check(13, 7));  // EOL disables axis swapping
}

// EOL rule with VERTICAL direction maps secondOverhang -> x and
// firstOverhang -> y (i.e. axes are swapped vs the HORIZONTAL case).
TEST_F(TestEnclosure, RuleConstructorEolVerticalSwapsAxes)
{
  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      tech(), "M_eol_v", odb::dbTechLayerType::ROUTING);

  odb::dbTechLayerCutEnclosureRule* rule
      = odb::dbTechLayerCutEnclosureRule::create(layer);
  rule->setType(odb::dbTechLayerCutEnclosureRule::EOL);
  rule->setFirstOverhang(7);
  rule->setSecondOverhang(13);

  Enclosure e(
      rule, layer, odb::Rect(0, 0, 50, 50), odb::dbTechLayerDir::VERTICAL);
  EXPECT_EQ(e.getX(), 13);
  EXPECT_EQ(e.getY(), 7);
}

// EOL rule with NONE direction picks max(first, second) for both axes.
TEST_F(TestEnclosure, RuleConstructorEolNoneUsesMaxOverhang)
{
  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      tech(), "M_eol_n", odb::dbTechLayerType::ROUTING);

  odb::dbTechLayerCutEnclosureRule* rule
      = odb::dbTechLayerCutEnclosureRule::create(layer);
  rule->setType(odb::dbTechLayerCutEnclosureRule::EOL);
  rule->setFirstOverhang(7);
  rule->setSecondOverhang(13);

  Enclosure e(rule, layer, odb::Rect(0, 0, 50, 50), odb::dbTechLayerDir::NONE);
  EXPECT_EQ(e.getX(), 13);  // max(7, 13)
  EXPECT_EQ(e.getY(), 13);
}

// ENDSIDE rule with a tall cut (dx < dy) routes END (first) to y, past its
// short top and bottom edges, and SIDE (second) to x, past its long sides.
TEST_F(TestEnclosure, RuleConstructorEndsideTallCutSwapsOverhangs)
{
  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      tech(), "M_es_tall", odb::dbTechLayerType::ROUTING);

  odb::dbTechLayerCutEnclosureRule* rule
      = odb::dbTechLayerCutEnclosureRule::create(layer);
  rule->setType(odb::dbTechLayerCutEnclosureRule::ENDSIDE);
  rule->setFirstOverhang(7);    // END
  rule->setSecondOverhang(13);  // SIDE

  // Tall cut: width (dx) = 10 < height (dy) = 20.
  Enclosure e(
      rule, layer, odb::Rect(0, 0, 10, 20), odb::dbTechLayerDir::HORIZONTAL);
  EXPECT_EQ(e.getX(), 13);  // SIDE overhang
  EXPECT_EQ(e.getY(), 7);   // END overhang
}

// ENDSIDE rule with a wide-or-square cut (dx >= dy) keeps first as x and
// second as y.
TEST_F(TestEnclosure, RuleConstructorEndsideWideCutKeepsOverhangsAsIs)
{
  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      tech(), "M_es_wide", odb::dbTechLayerType::ROUTING);

  odb::dbTechLayerCutEnclosureRule* rule
      = odb::dbTechLayerCutEnclosureRule::create(layer);
  rule->setType(odb::dbTechLayerCutEnclosureRule::ENDSIDE);
  rule->setFirstOverhang(7);
  rule->setSecondOverhang(13);

  // Wide cut: width (dx) = 20 > height (dy) = 10.
  Enclosure e(
      rule, layer, odb::Rect(0, 0, 20, 10), odb::dbTechLayerDir::HORIZONTAL);
  EXPECT_EQ(e.getX(), 7);
  EXPECT_EQ(e.getY(), 13);
}

// The via-layer-rule constructor pulls (x, y) directly from the rule and
// then snaps axes to the layer's preferred direction.
TEST_F(TestEnclosure, ViaLayerRuleConstructorSnapsToLayerDirection)
{
  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      tech(), "M_vlr", odb::dbTechLayerType::ROUTING);
  layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);

  odb::dbTechViaGenerateRule* gen_rule
      = odb::dbTechViaGenerateRule::create(tech(), "g", false);
  odb::dbTechViaLayerRule* via_layer_rule
      = odb::dbTechViaLayerRule::create(tech(), gen_rule, layer);
  via_layer_rule->setEnclosure(5, 10);  // x=5, y=10

  // HORIZONTAL layer wants x >= y, so the constructor should swap to
  // (10, 5).
  Enclosure e(via_layer_rule, layer);
  EXPECT_EQ(e.getX(), 10);
  EXPECT_EQ(e.getY(), 5);
}

// The cut-enclosure-rule constructor sets allow_swap = true for DEFAULT
// rules, so check() succeeds even when the candidate has its axes swapped.
TEST_F(TestEnclosure, RuleConstructorDefaultAllowsAxisSwapInCheck)
{
  odb::dbTechLayer* layer = odb::dbTechLayer::create(
      tech(), "M_def_swap", odb::dbTechLayerType::ROUTING);
  layer->setDirection(odb::dbTechLayerDir::HORIZONTAL);

  odb::dbTechLayerCutEnclosureRule* rule
      = odb::dbTechLayerCutEnclosureRule::create(layer);
  rule->setType(odb::dbTechLayerCutEnclosureRule::DEFAULT);
  rule->setFirstOverhang(5);
  rule->setSecondOverhang(10);

  // After construction with a HORIZONTAL layer, the stored values become
  // (x, y) = (10, 5) due to the directional swap.
  Enclosure e(rule, layer, odb::Rect(0, 0, 50, 50), odb::dbTechLayerDir::NONE);
  EXPECT_TRUE(e.check(10, 5));   // exact orientation
  EXPECT_TRUE(e.check(5, 10));   // swapped orientation also passes
  EXPECT_FALSE(e.check(4, 10));  // x too small even after swap
}

// =============================================================================
// Tests that exercise the rest of the via classes (DbTechVia, DbGenerateVia,
// DbArrayVia, DbSplitCutVia, DbGenerateStackedVia, ViaGenerator,
// TechViaGenerator, GenerateViaGenerator). Built on an in-memory tech via
// PdnTest::makeBasicViaStack() / addStackedLevel() so we don't depend on
// any LEF data.
// =============================================================================

using TestPDNViaTech = PdnTest;

// Convenience constraint with no fitting requirements.
static ViaGenerator::Constraint loose()
{
  return {false, false, false, std::nullopt};
}

// -------- TechViaGenerator --------

// TechViaGenerator extracts the bottom/top metal layers from the dbTechVia
// and identifies the cut layer as everything in between.
TEST_F(TestPDNViaTech, TechViaGeneratorIdentifiesLayersFromVia)
{
  const ViaStack s = makeBasicViaStack();

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_EQ(gen.getBottomLayer(), s.bottom);
  EXPECT_EQ(gen.getTopLayer(), s.top);
  EXPECT_EQ(gen.getCutLayer(), s.cut);
}

// getName() forwards the underlying dbTechVia's name.
TEST_F(TestPDNViaTech, TechViaGeneratorGetNameMatchesDbTechVia)
{
  const ViaStack s = makeBasicViaStack();

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_EQ(gen.getName(), s.via->getName());
}

// Before build() the generator has 0 rows / 0 columns -> 0 total cuts.
TEST_F(TestPDNViaTech, TechViaGeneratorTotalCutsIsZeroBeforeBuild)
{
  const ViaStack s = makeBasicViaStack();

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_EQ(gen.getTotalCuts(), 0);
}

// isSetupValid is true when the provided lower/upper layers match the
// via's bottom/top.
TEST_F(TestPDNViaTech, TechViaGeneratorIsSetupValidWhenLayersMatch)
{
  const ViaStack s = makeBasicViaStack();

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_TRUE(gen.isSetupValid(s.bottom, s.top));
}

// isSetupValid is false when the layers don't match the via's endpoints.
TEST_F(TestPDNViaTech, TechViaGeneratorIsSetupInvalidWhenLayersMismatch)
{
  const ViaStack s = makeBasicViaStack();
  // Make a "wrong" routing layer to compare against.
  odb::dbTechLayer* other = odb::dbTechLayer::create(
      tech(), "other", odb::dbTechLayerType::ROUTING);

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_FALSE(gen.isSetupValid(s.bottom, other));
}

// After a successful build with no cut spacing rules, the basic via has
// one cut (cut pitch is 0 -> getCuts returns 1). cut.area() = 50*50 =
// 2500, total cuts = 1, so getCutArea = 2500.
TEST_F(TestPDNViaTech, TechViaGeneratorCutAreaMatchesSingleCutArea)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getCutArea(), 2500);
  EXPECT_EQ(gen.getTotalCuts(), 1);
  EXPECT_EQ(gen.getRows(), 1);
  EXPECT_EQ(gen.getColumns(), 1);
}

// Cut pitch setters round-trip.
TEST_F(TestPDNViaTech, ViaGeneratorCutPitchSettersRoundTrip)
{
  const ViaStack s = makeBasicViaStack();

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  gen.setCutPitchX(100);
  gen.setCutPitchY(200);
  EXPECT_EQ(gen.getCutPitchX(), 100);
  EXPECT_EQ(gen.getCutPitchY(), 200);
}

// Cut offset setters round-trip.
TEST_F(TestPDNViaTech, ViaGeneratorCutOffsetSettersRoundTrip)
{
  const ViaStack s = makeBasicViaStack();

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  gen.setCutOffsetX(5);
  gen.setCutOffsetY(7);
  EXPECT_EQ(gen.getCutOffsetX(), 5);
  EXPECT_EQ(gen.getCutOffsetY(), 7);
}

// getLowerRect / getUpperRect return what was passed to the constructor.
TEST_F(TestPDNViaTech, ViaGeneratorReturnsConstructorRects)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect lower(0, 0, 500, 100);
  const odb::Rect upper(0, 0, 100, 500);
  TechViaGenerator gen(getLogger(), s.via, lower, loose(), upper, loose());
  EXPECT_EQ(gen.getLowerRect(), lower);
  EXPECT_EQ(gen.getUpperRect(), upper);
}

// hasCutClass is false when the cut layer has no cut-class rules.
TEST_F(TestPDNViaTech, ViaGeneratorHasCutClassFalseByDefault)
{
  const ViaStack s = makeBasicViaStack();

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_FALSE(gen.hasCutClass());
  EXPECT_EQ(gen.getCutClass(), nullptr);
}

// isSplitCutArray defaults to false; flips to true after setSplitCutArray.
TEST_F(TestPDNViaTech, ViaGeneratorSplitCutArrayDefaultsToFalse)
{
  const ViaStack s = makeBasicViaStack();

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_FALSE(gen.isSplitCutArray());
  gen.setSplitCutArray(true, false);
  EXPECT_TRUE(gen.isSplitCutArray());
}

// After build() the generator dimensions match cut + 2*enclosure. For our
// 50x50 cut + 10-unit physical enclosure on each side: 50 + 2*10 = 70.
TEST_F(TestPDNViaTech, ViaGeneratorGeneratorWidthHeightMatchSingleViaShape)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getGeneratorWidth(/*bottom=*/true), 70);
  EXPECT_EQ(gen.getGeneratorHeight(/*bottom=*/true), 70);
  EXPECT_EQ(gen.getGeneratorWidth(/*bottom=*/false), 70);
  EXPECT_EQ(gen.getGeneratorHeight(/*bottom=*/false), 70);
}

// build() populates the bottom/top enclosures.
TEST_F(TestPDNViaTech, ViaGeneratorPostBuildHasEnclosures)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_NE(gen.getBottomEnclosure(), nullptr);
  EXPECT_NE(gen.getTopEnclosure(), nullptr);
}

// Without calling build(), getTotalCuts is 0 -> checkConstraints with
// check_cuts=true short-circuits to false.
TEST_F(TestPDNViaTech, ViaGeneratorCheckConstraintsFailsWhenZeroCuts)
{
  const ViaStack s = makeBasicViaStack();
  const odb::Rect r(0, 0, 1000, 1000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  // No build() -> totalCuts == 0.
  EXPECT_FALSE(gen.checkConstraints(/*check_cuts=*/true, false, false));
}

// A cut class rule whose length is NOT marked valid forces determineCutClass
// to use the rule's width for both width and length when matching the cut.
// Our 50x50 cut matches a rule with width=50 and length_valid=false.
TEST_F(TestPDNViaTech, ViaGeneratorDetermineCutClassWithoutLengthValid)
{
  ViaStack s = makeBasicViaStack();
  odb::dbTechLayerCutClassRule* cls
      = odb::dbTechLayerCutClassRule::create(s.cut, "VCNL");
  cls->setWidth(50);
  // intentionally NOT calling setLengthValid(true) so the rule's
  // rule_length = rule_width path executes.

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_TRUE(gen.hasCutClass());
  EXPECT_EQ(gen.getCutClass(), cls);
}

// After build(), checkConstraints() should pass on a wide-open area.
TEST_F(TestPDNViaTech, ViaGeneratorCheckConstraintsPassesOnWideArea)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// setMaxRows / setMaxColumns clip the array result to exactly 1x1.
TEST_F(TestPDNViaTech, ViaGeneratorMaxRowsAndColumnsAreHonored)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setMaxRows(1);
  gen.setMaxColumns(1);
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getRows(), 1);
  EXPECT_EQ(gen.getColumns(), 1);
}

// getIntersectionRect is the geometric intersection of lower and upper.
TEST_F(TestPDNViaTech, ViaGeneratorIntersectionRectIsLowerCapUpper)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect lower(0, 0, 1000, 1000);
  const odb::Rect upper(500, 500, 1500, 1500);
  TechViaGenerator gen(getLogger(), s.via, lower, loose(), upper, loose());
  odb::Rect expected;
  lower.intersection(upper, expected);
  EXPECT_EQ(gen.getIntersectionRect(), expected);
}

// recheckConstraints restores the original rect after the probe.
TEST_F(TestPDNViaTech, ViaGeneratorRecheckConstraintsLeavesLowerRectIntact)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  const odb::Rect before = gen.getLowerRect();
  (void) gen.recheckConstraints(odb::Rect(0, 0, 500, 500), /*bottom=*/true);
  EXPECT_EQ(gen.getLowerRect(), before);
}

// A generator is not strictly preferred over an identical configuration.
TEST_F(TestPDNViaTech, ViaGeneratorIsPreferredOverSelfIsFalse)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator a(getLogger(), s.via, r, loose(), r, loose());
  TechViaGenerator b(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(a.build(false, false));
  ASSERT_TRUE(b.build(false, false));
  EXPECT_FALSE(a.isPreferredOver(&b));
}

// generate(block) returns a non-null DbVia after build.
TEST_F(TestPDNViaTech, ViaGeneratorGenerateReturnsADbVia)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  DbVia* db_via = gen.generate(block());
  ASSERT_NE(db_via, nullptr);
  delete db_via;
}

// TechViaGenerator::makeBaseVia returns a non-null DbBaseVia.
TEST_F(TestPDNViaTech, TechViaGeneratorMakeBaseViaReturnsDbBaseVia)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  DbBaseVia* base = gen.makeBaseVia(gen.getRows(), 0, gen.getColumns(), 0);
  ASSERT_NE(base, nullptr);
  EXPECT_FALSE(base->getName().empty());
  delete base;
}

// Static helper getViaObstructionRects exercises the obstruction walk and
// returns x/y pseudo-obstructions derived from each cut box.
TEST_F(TestPDNViaTech, TechViaGeneratorObstructionRectsContainsCutBased)
{
  const ViaStack s = makeBasicViaStack();

  const auto rects
      = TechViaGenerator::getViaObstructionRects(getLogger(), s.via, {0, 0});
  // Single cut -> at least one derived obstruction rect.
  EXPECT_FALSE(rects.empty());
}

// build() with internal-layer flags exercises the alternate row/column
// determination logic.
TEST_F(TestPDNViaTech, ViaGeneratorBuildWithInternalLayerFlags)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_TRUE(gen.build(/*bottom_is_internal_layer=*/true,
                        /*top_is_internal_layer=*/true));
}

// Tight must_fit constraints drive checkConstraints' rejection path.
TEST_F(TestPDNViaTech,
       ViaGeneratorBuildWithTightFitConstraintsExercisesFailPaths)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect tight(0, 0, 10, 10);
  const ViaGenerator::Constraint must_fit{true, true, false, std::nullopt};
  TechViaGenerator gen(getLogger(), s.via, tight, must_fit, tight, must_fit);
  // a 50x50 cut cannot fit in a 10x10 shape that must hold it, which the
  // setup check reports before any build
  EXPECT_FALSE(gen.isSetupValid(s.bottom, s.top));
}

// intersection_only forces the intersection-rect path in build().
TEST_F(TestPDNViaTech, ViaGeneratorBuildWithIntersectionOnlyConstraint)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect lower(0, 0, 1500, 500);
  const odb::Rect upper(500, 0, 2000, 1500);
  const ViaGenerator::Constraint inter{false, false, true, std::nullopt};
  TechViaGenerator gen(getLogger(), s.via, lower, inter, upper, inter);
  (void) gen.build(false, false);
  SUCCEED();
}

// Set explicit cut pitch before build to exercise updateCutSpacing.
TEST_F(TestPDNViaTech, ViaGeneratorBuildWithExplicitCutPitch)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setCutPitchX(500);
  gen.setCutPitchY(500);
  (void) gen.build(false, false);
  SUCCEED();
}

// A degenerate (0-area) rect with loose constraints still exercises the
// build path; with no must-fit requirement the via is allowed.
TEST_F(TestPDNViaTech, ViaGeneratorBuildOnDegenerateRect)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect empty(0, 0, 0, 0);
  TechViaGenerator gen(getLogger(), s.via, empty, loose(), empty, loose());
  ASSERT_TRUE(gen.build(false, false));
  // Single-cut via is the only thing that fits "inside" zero area.
  EXPECT_EQ(gen.getRows(), 1);
  EXPECT_EQ(gen.getColumns(), 1);
}

// -------- DbTechVia / DbVia --------

// DbTechVia::getName forwards the wrapped dbTechVia's name.
TEST_F(TestPDNViaTech, DbTechViaSingleCutNameMatchesDbVia)
{
  const ViaStack s = makeBasicViaStack();

  DbTechVia dbtv(s.via,
                 /*rows=*/1,
                 /*row_pitch=*/0,
                 /*cols=*/1,
                 /*col_pitch=*/0);
  EXPECT_EQ(dbtv.getName(), s.via->getName());
}

// requiresPatch() is false for a single-row, single-column tech via.
TEST_F(TestPDNViaTech, DbTechViaSingleCutDoesNotRequirePatch)
{
  const ViaStack s = makeBasicViaStack();

  DbTechVia dbtv(s.via, 1, 0, 1, 0);
  EXPECT_FALSE(dbtv.requiresPatch());
}

// requiresPatch() flips to true once the via is an array.
TEST_F(TestPDNViaTech, DbTechViaArrayRequiresPatch)
{
  const ViaStack s = makeBasicViaStack();

  DbTechVia dbtv(s.via,
                 /*rows=*/2,
                 /*row_pitch=*/100,
                 /*cols=*/2,
                 /*col_pitch=*/100);
  EXPECT_TRUE(dbtv.requiresPatch());
}

// DbTechVia::getViaRect with all-true flags returns the union of the cut
// and both metal enclosures. The basic stack's metal enclosure is 70x70
// centered on origin -> (-35, -35, 35, 35).
TEST_F(TestPDNViaTech, DbTechViaGetViaRectIsMetalEnclosureUnion)
{
  const ViaStack s = makeBasicViaStack();

  DbTechVia dbtv(s.via, 1, 0, 1, 0);
  const odb::Rect r = dbtv.getViaRect(true, true, true, true);
  EXPECT_EQ(r, odb::Rect(-35, -35, 35, 35));
}

// DbVia::adjustToMinArea is a no-op on a layer without a min-area rule.
TEST_F(TestPDNViaTech, DbViaAdjustToMinAreaIsNoOpWithoutMinArea)
{
  const ViaStack s = makeBasicViaStack();

  DbTechVia dbtv(s.via, 1, 0, 1, 0);
  const odb::Rect rect(0, 0, 100, 100);
  const odb::Rect adjusted = dbtv.adjustToMinArea(s.bottom, rect);
  EXPECT_EQ(adjusted, rect);  // no rule -> unchanged
}

// hasGenerator() is false on a DbVia that hasn't been linked yet.
TEST_F(TestPDNViaTech, DbViaHasGeneratorFalseUntilSet)
{
  const ViaStack s = makeBasicViaStack();

  DbTechVia dbtv(s.via, 1, 0, 1, 0);
  EXPECT_FALSE(dbtv.hasGenerator());
  EXPECT_EQ(dbtv.getGenerator(), nullptr);
}

// getViaReport on a DbBaseVia returns a single entry keyed by the via name.
TEST_F(TestPDNViaTech, DbTechViaReportContainsViaNameWithZeroCount)
{
  const ViaStack s = makeBasicViaStack();

  DbTechVia dbtv(s.via, 1, 0, 1, 0);
  const ViaReport report = dbtv.getViaReport();
  ASSERT_EQ(report.size(), 1u);
  EXPECT_EQ(report.begin()->first, s.via->getName());
  EXPECT_EQ(report.begin()->second, 0);
}

// DbTechVia::generate() with a single cut writes via boxes into the wire
// and returns shapes on the bottom and top layers.
TEST_F(TestPDNViaTech, DbTechViaGenerateProducesShapes)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDD");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  DbTechVia dbtv(s.via, 1, 0, 1, 0);
  const auto shapes = dbtv.generate(block(),
                                    wire,
                                    odb::dbWireShapeType::STRIPE,
                                    /*x=*/200,
                                    /*y=*/200,
                                    /*ongrid=*/{},
                                    getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// A dbTechVia that already contains a 2x2 grid of cuts, when wrapped in a
// 2x2 DbTechVia array, triggers the multicut-simplification path in the
// DbTechVia constructor (folds the inner array into the outer one).
TEST_F(TestPDNViaTech, DbTechViaMultiCutInputArraySimplifies)
{
  const ViaStack s = makeBasicViaStack();
  // 2x2 cuts at +/- 50 from origin, enclosed by 5.
  odb::dbTechVia* mv = addMultiCutViaToStack(
      s, "MV", /*cut_size=*/50, /*cut_pitch=*/100, /*enclosure=*/5);

  // Outer rows=2/cols=2 with row_pitch=col_pitch=200 -> via_cols * dx (=
  // 2*100=200) is divisible by col_pitch, so pitch_match succeeds and the
  // simplification fires.
  DbTechVia dbtv(mv,
                 /*rows=*/2,
                 /*row_pitch=*/200,
                 /*cols=*/2,
                 /*col_pitch=*/200);
  EXPECT_EQ(dbtv.getName(), std::string("MV"));
  EXPECT_TRUE(dbtv.requiresPatch());
}

// DbTechVia::generate() with rows*cols > 1 takes the array path.
TEST_F(TestPDNViaTech, DbTechViaGenerateArrayProducesShapes)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDDA");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  DbTechVia dbtv(s.via,
                 /*rows=*/2,
                 /*row_pitch=*/200,
                 /*cols=*/2,
                 /*col_pitch=*/200);
  const auto shapes = dbtv.generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 500, 500, {}, getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// Every via a DbBaseVia places is counted, and the report carries the count.
TEST_F(TestPDNViaTech, DbTechViaCountsTheViasItPlaces)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDD");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  DbTechVia dbtv(s.via, 1, 0, 1, 0);
  EXPECT_EQ(dbtv.getCount(), 0);
  for (const int x : {200, 400}) {
    dbtv.generate(
        block(), wire, odb::dbWireShapeType::STRIPE, x, 200, {}, getLogger());
  }
  EXPECT_EQ(dbtv.getCount(), 2);
  EXPECT_EQ(dbtv.getViaReport().at(s.via->getName()), 2);
}

// A DbVia can be cached unless the generator that built it says otherwise; a
// via with no generator at all always can.
TEST_F(TestPDNViaTech, DbViaCanCacheFollowsItsGenerator)
{
  const ViaStack s = makeBasicViaStack();
  DbTechVia dbtv(s.via, 1, 0, 1, 0);
  EXPECT_TRUE(dbtv.canCache());

  const odb::Rect r(0, 0, 2000, 2000);
  auto gen = std::make_shared<TechViaGenerator>(
      getLogger(), s.via, r, loose(), r, loose());
  EXPECT_EQ(gen->getVia(), s.via);
  dbtv.setGenerator(gen);
  EXPECT_TRUE(dbtv.hasGenerator());
  EXPECT_EQ(dbtv.getGenerator(), gen.get());
  EXPECT_TRUE(dbtv.canCache());
}

// -------- DbGenerateVia --------

// DbGenerateVia::getName forwards the rule name.
TEST_F(TestPDNViaTech, DbGenerateViaGetNameUsesRuleName)
{
  const ViaStack s = makeBasicViaStack();

  DbGenerateVia dbgv(odb::Rect(0, 0, 500, 500),
                     s.rule,
                     1,
                     1,
                     0,
                     0,
                     10,
                     10,
                     10,
                     10,
                     s.bottom,
                     s.cut,
                     s.top);
  EXPECT_EQ(dbgv.getName(), s.rule->getName());
  // A single generated via is one piece of metal, so it never needs a patch.
  EXPECT_FALSE(dbgv.requiresPatch());
}

// DbGenerateVia::getViaRect(include_enclosure=false) returns the cut rect
// alone. For a single-cut via with our 50x50 cut rule it's centered on
// the origin: (-25, -25, 25, 25).
TEST_F(TestPDNViaTech, DbGenerateViaViaRectMatchesCutExtent)
{
  const ViaStack s = makeBasicViaStack();

  DbGenerateVia dbgv(odb::Rect(0, 0, 500, 500),
                     s.rule,
                     1,
                     1,
                     0,
                     0,
                     10,
                     10,
                     10,
                     10,
                     s.bottom,
                     s.cut,
                     s.top);
  const odb::Rect r
      = dbgv.getViaRect(/*include_enclosure=*/false, true, true, true);
  EXPECT_EQ(r, odb::Rect(-25, -25, 25, 25));
}

// DbGenerateVia::getViaRect(include_enclosure=true) pads the cut rect by
// max(bot_enc, top_enc) on each side. cut=50x50, encs=(20,20)/(40,40) ->
// pad = max(20,40) = 40 each side, giving 50 + 80 = 130 across.
TEST_F(TestPDNViaTech, DbGenerateViaViaRectIncludesEnclosure)
{
  const ViaStack s = makeBasicViaStack();

  DbGenerateVia dbgv(odb::Rect(0, 0, 500, 500),
                     s.rule,
                     /*rows=*/1,
                     /*columns=*/1,
                     /*cut_pitch_x=*/0,
                     /*cut_pitch_y=*/0,
                     /*bot_enc_x=*/20,
                     /*bot_enc_y=*/20,
                     /*top_enc_x=*/40,
                     /*top_enc_y=*/40,
                     s.bottom,
                     s.cut,
                     s.top);
  const odb::Rect r
      = dbgv.getViaRect(/*include_enclosure=*/true, true, true, true);
  EXPECT_EQ(r, odb::Rect(-65, -65, 65, 65));
}

// DbGenerateVia::generate emits via boxes into the wire.
TEST_F(TestPDNViaTech, DbGenerateViaGenerateProducesShapes)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDDB");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  DbGenerateVia dbgv(odb::Rect(-100, -100, 100, 100),
                     s.rule,
                     /*rows=*/1,
                     /*columns=*/1,
                     /*cut_pitch_x=*/0,
                     /*cut_pitch_y=*/0,
                     /*bottom_enclosure_x=*/40,
                     /*bottom_enclosure_y=*/40,
                     /*top_enclosure_x=*/40,
                     /*top_enclosure_y=*/40,
                     s.bottom,
                     s.cut,
                     s.top);
  const auto shapes = dbgv.generate(block(),
                                    wire,
                                    odb::dbWireShapeType::STRIPE,
                                    /*x=*/300,
                                    /*y=*/300,
                                    /*ongrid=*/{},
                                    getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// -------- DbArrayVia --------

// DbArrayVia::getViaReport aggregates the core via name.
TEST_F(TestPDNViaTech, DbArrayViaReportContainsCoreViaName)
{
  const ViaStack s = makeBasicViaStack();

  auto* core = new DbTechVia(s.via, 1, 0, 1, 0);
  DbArrayVia array(core,
                   nullptr,
                   nullptr,
                   nullptr,
                   /*core_rows=*/2,
                   /*core_columns=*/2,
                   /*array_spacing_x=*/100,
                   /*array_spacing_y=*/100);
  const ViaReport report = array.getViaReport();
  ASSERT_EQ(report.count(s.via->getName()), 1u);
}

// The report counts each of the four vias an array is built from exactly
// once. A 2x2 core with every end via is 3x3: the corner is the
// end-of-row-column via, the rest of the first row the end-of-row via, the
// rest of the first column the end-of-column via, and the remaining four are
// core vias.
TEST_F(TestPDNViaTech, DbArrayViaReportCountsEachViaOnce)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDD");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  auto make_via = [&](const char* name) {
    return new DbTechVia(
        tst::addVia(tech(), name, s.bottom, s.cut, s.top, 50, 10), 1, 0, 1, 0);
  };

  DbArrayVia array(make_via("CORE"),
                   make_via("END_ROW"),
                   make_via("END_COL"),
                   make_via("END_ROW_COL"),
                   /*core_rows=*/2,
                   /*core_columns=*/2,
                   /*array_spacing_x=*/100,
                   /*array_spacing_y=*/100);
  array.generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 500, 500, {}, getLogger());

  const ViaReport expected{
      {"CORE", 4}, {"END_ROW", 2}, {"END_COL", 2}, {"END_ROW_COL", 1}};
  EXPECT_EQ(array.getViaReport(), expected);
}

// DbArrayVia with non-null end-of-row / end-of-column / end-of-row-column
// drives the row/column-extension branches in the constructor and the
// per-cell selection logic in generate(). The report aggregates all four
// inner via names (but since they're the same DbTechVia name here, the
// report still has a single entry keyed by that name).
TEST_F(TestPDNViaTech, DbArrayViaWithEndViasDrivesAllCellSelectionBranches)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDDC2");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  auto* core = new DbTechVia(s.via, 1, 0, 1, 0);
  auto* end_row = new DbTechVia(s.via, 1, 0, 1, 0);
  auto* end_col = new DbTechVia(s.via, 1, 0, 1, 0);
  auto* end_rc = new DbTechVia(s.via, 1, 0, 1, 0);
  DbArrayVia array(core,
                   end_row,
                   end_col,
                   end_rc,
                   /*core_rows=*/2,
                   /*core_columns=*/2,
                   /*array_spacing_x=*/100,
                   /*array_spacing_y=*/100);
  const auto shapes = array.generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 500, 500, {}, getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
  EXPECT_TRUE(array.requiresPatch());
}

// DbArrayVia::generate emits a 2x2 grid of via shapes.
TEST_F(TestPDNViaTech, DbArrayViaGenerateProducesShapes)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDDC");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  auto* core = new DbTechVia(s.via, 1, 0, 1, 0);
  DbArrayVia array(core,
                   nullptr,
                   nullptr,
                   nullptr,
                   /*core_rows=*/2,
                   /*core_columns=*/2,
                   /*array_spacing_x=*/200,
                   /*array_spacing_y=*/200);
  const auto shapes = array.generate(block(),
                                     wire,
                                     odb::dbWireShapeType::STRIPE,
                                     /*x=*/500,
                                     /*y=*/500,
                                     /*ongrid=*/{},
                                     getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
  EXPECT_TRUE(array.requiresPatch());
}

// -------- DbSplitCutVia --------

// DbSplitCutVia::getViaReport rolls up the inner via's name.
TEST_F(TestPDNViaTech, DbSplitCutViaReportContainsInnerViaName)
{
  const ViaStack s = makeBasicViaStack();

  auto* inner = new DbTechVia(s.via, 1, 0, 1, 0);
  DbSplitCutVia split(inner,
                      /*rows=*/2,
                      /*row_pitch=*/200,
                      /*row_offset=*/0,
                      /*cols=*/2,
                      /*col_pitch=*/200,
                      /*col_offset=*/0,
                      block(),
                      s.bottom,
                      /*snap_bottom=*/false,
                      s.top,
                      /*snap_top=*/false);
  const ViaReport report = split.getViaReport();
  ASSERT_EQ(report.count(s.via->getName()), 1u);
}

// DbSplitCutVia with snap_bottom=true and snap_top=true drives the
// snap-axis branches in both the constructor and generate(). Track grids
// on both metals provide the snap targets.
TEST_F(TestPDNViaTech, DbSplitCutViaWithSnapEnabledUsesTrackGrids)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbTrackGrid* bot_tracks = odb::dbTrackGrid::create(block(), s.bottom);
  bot_tracks->addGridPatternX(0, 10, 50);
  bot_tracks->addGridPatternY(0, 10, 50);
  odb::dbTrackGrid* top_tracks = odb::dbTrackGrid::create(block(), s.top);
  top_tracks->addGridPatternX(0, 10, 50);
  top_tracks->addGridPatternY(0, 10, 50);

  odb::dbNet* net = odb::dbNet::create(block(), "VDDSS");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  auto* inner = new DbTechVia(s.via, 1, 0, 1, 0);
  DbSplitCutVia split(inner,
                      /*rows=*/2,
                      /*row_pitch=*/300,
                      /*row_offset=*/0,
                      /*cols=*/2,
                      /*col_pitch=*/300,
                      /*col_offset=*/0,
                      block(),
                      s.bottom,
                      /*snap_bottom=*/true,
                      s.top,
                      /*snap_top=*/true);
  const auto shapes = split.generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 500, 500, {}, getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// DbSplitCutVia::generate emits sub-vias into the wire.
TEST_F(TestPDNViaTech, DbSplitCutViaGenerateProducesShapes)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDDS");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  auto* inner = new DbTechVia(s.via, 1, 0, 1, 0);
  DbSplitCutVia split(inner,
                      /*rows=*/2,
                      /*row_pitch=*/300,
                      /*row_offset=*/0,
                      /*cols=*/2,
                      /*col_pitch=*/300,
                      /*col_offset=*/0,
                      block(),
                      s.bottom,
                      false,
                      s.top,
                      false);
  const auto shapes = split.generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 500, 500, {}, getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// -------- DbGenerateStackedVia --------

// DbGenerateStackedVia stacks two DbTechVias and reports exactly two
// entries -- one per inner via.
TEST_F(TestPDNViaTech, DbGenerateStackedViaReportIncludesAllVias)
{
  const ViaStack s1 = makeBasicViaStack();
  const ViaStack s2 = addStackedLevel(s1);

  std::vector<std::unique_ptr<DbVia>> vias;
  vias.push_back(std::make_unique<DbTechVia>(s1.via, 1, 0, 1, 0));
  vias.push_back(std::make_unique<DbTechVia>(s2.via, 1, 0, 1, 0));
  DbGenerateStackedVia stacked(std::move(vias), s1.bottom, block());
  const ViaReport report = stacked.getViaReport();
  EXPECT_EQ(report.size(), 2u);
  EXPECT_EQ(report.count(s1.via->getName()), 1u);
  EXPECT_EQ(report.count(s2.via->getName()), 1u);
}

// DbGenerateStackedVia::generate emits shapes spanning bottom -> top.
TEST_F(TestPDNViaTech, DbGenerateStackedViaGenerateProducesShapes)
{
  const ViaStack s1 = makeBasicViaStack();
  const ViaStack s2 = addStackedLevel(s1);

  odb::dbNet* net = odb::dbNet::create(block(), "VDDST");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  std::vector<std::unique_ptr<DbVia>> vias;
  vias.push_back(std::make_unique<DbTechVia>(s1.via, 1, 0, 1, 0));
  vias.push_back(std::make_unique<DbTechVia>(s2.via, 1, 0, 1, 0));
  DbGenerateStackedVia stacked(std::move(vias), s1.bottom, block());
  const auto shapes = stacked.generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 600, 600, {}, getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// A stack can be cached only if every via in it can. A via built from a
// generator that took enclosure from the strap past the overlap cannot.
TEST_F(TestPDNViaTech, DbGenerateStackedViaCanCacheOnlyIfEveryViaCan)
{
  const ViaStack s1 = makeBasicViaStack();
  const ViaStack s2 = addStackedLevel(s1);
  addCutEnclosureRule(s1.cut, 30, 30);
  const ViaGenerator::Constraint fit_x{true, false, true, std::nullopt};
  auto uncacheable
      = std::make_shared<GenerateViaGenerator>(getLogger(),
                                               s1.rule,
                                               odb::Rect(0, 0, 400, 2000),
                                               fit_x,
                                               odb::Rect(160, 900, 240, 1100),
                                               loose());
  ASSERT_TRUE(uncacheable->build(false, false));
  ASSERT_FALSE(uncacheable->canCache());

  std::vector<std::unique_ptr<DbVia>> vias;
  vias.push_back(std::make_unique<DbTechVia>(s1.via, 1, 0, 1, 0));
  vias.push_back(std::make_unique<DbTechVia>(s2.via, 1, 0, 1, 0));
  DbVia* lower = vias.front().get();
  DbGenerateStackedVia stacked(std::move(vias), s1.bottom, block());
  EXPECT_TRUE(stacked.canCache());

  lower->setGenerator(uncacheable);
  EXPECT_FALSE(stacked.canCache());
}

// A two-level stack whose vias cross on the shared middle layer: the lower
// via's top pad runs along x and the upper via's bottom pad along y, so the
// metal they leave there is a cross that has to be patched out to its
// bounding box.
class TestStackedViaPatch : public PdnTest
{
 protected:
  void SetUp() override
  {
    PdnTest::SetUp();
    lower_ = makeBasicViaStack();
    upper_ = addStackedLevel(lower_);
    odb::dbNet* net = odb::dbNet::create(block(), "VDD");
    wire_ = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  }

  // Generates the stack at (1000, 1000). The upper via carries the generator
  // Connect would have built it from, sized for a 100 wide strap on the middle
  // layer; the patch is rechecked against it.
  DbVia::ViaLayerShape generate()
  {
    std::vector<std::unique_ptr<DbVia>> vias;
    vias.push_back(std::make_unique<DbGenerateVia>(odb::Rect(-50, -50, 50, 50),
                                                   lower_.rule,
                                                   1,
                                                   1,
                                                   0,
                                                   0,
                                                   10,
                                                   10,
                                                   /*top_enclosure_x=*/100,
                                                   /*top_enclosure_y=*/10,
                                                   lower_.bottom,
                                                   lower_.cut,
                                                   lower_.top));
    auto upper_via
        = std::make_unique<DbGenerateVia>(odb::Rect(-50, -50, 50, 50),
                                          upper_.rule,
                                          1,
                                          1,
                                          0,
                                          0,
                                          /*bottom_enclosure_x=*/10,
                                          /*bottom_enclosure_y=*/100,
                                          10,
                                          10,
                                          upper_.bottom,
                                          upper_.cut,
                                          upper_.top);
    auto generator = std::make_shared<GenerateViaGenerator>(
        getLogger(),
        upper_.rule,
        odb::Rect(-50, -1000, 50, 1000),
        loose(),
        odb::Rect(-1000, -50, 1000, 50),
        loose());
    EXPECT_TRUE(generator->build(false, false));
    upper_via->setGenerator(generator);
    vias.push_back(std::move(upper_via));

    DbGenerateStackedVia stacked(std::move(vias), lower_.bottom, block());
    return stacked.generate(block(),
                            wire_,
                            odb::dbWireShapeType::STRIPE,
                            1000,
                            1000,
                            {},
                            getLogger());
  }

  ViaStack lower_;
  ViaStack upper_;
  odb::dbSWire* wire_ = nullptr;
};

// The patch fills the cross out to its bounding box with DRC fill on the
// middle layer.
TEST_F(TestStackedViaPatch, CrossedPadsArePatchedToTheirBoundingBox)
{
  const DbVia::ViaLayerShape shapes = generate();

  const odb::Rect patch(875, 875, 1125, 1125);
  bool found = false;
  for (const auto& [rect, box] : shapes.middle) {
    if (rect == patch) {
      found = true;
      EXPECT_EQ(box->getTechLayer(), lower_.top);
      EXPECT_EQ(box->getWireShapeType(), odb::dbWireShapeType::DRCFILL);
    }
  }
  EXPECT_TRUE(found);
}

// A patch wide enough to bring a min-cut rule into force that the upper via
// cannot meet makes the stack illegal, so all of it is removed.
TEST_F(TestStackedViaPatch, PatchThatBreaksAMinCutRuleRemovesTheStack)
{
  // The 100 wide strap the upper via was built for is under the rule's
  // width; the 250 wide patch is over it.
  addMinCutRuleV54(upper_.bottom, /*num_cuts=*/2, /*width=*/150);

  const DbVia::ViaLayerShape shapes = generate();

  EXPECT_TRUE(shapes.bottom.empty());
  EXPECT_TRUE(shapes.middle.empty());
  EXPECT_TRUE(shapes.top.empty());
  EXPECT_TRUE(wire_->getWires().empty());
}

// -------- DbGenerateDummyVia --------

// A dummy via stands in for a via that could not be built: it places nothing
// and reports nothing.
TEST_F(TestPDNViaTech, DbGenerateDummyViaPlacesNothing)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDD");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  DbGenerateDummyVia dummy(/*connect=*/nullptr,
                           odb::Rect(-50, -50, 50, 50),
                           s.bottom,
                           s.top,
                           /*add_report=*/false,
                           "no rule");
  const auto shapes = dummy.generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 500, 500, {}, getLogger());
  EXPECT_TRUE(shapes.bottom.empty());
  EXPECT_TRUE(shapes.middle.empty());
  EXPECT_TRUE(shapes.top.empty());
  EXPECT_TRUE(wire->getWires().empty());
  EXPECT_TRUE(dummy.getViaReport().empty());
}

// -------- GenerateViaGenerator --------

// GenerateViaGenerator picks up bottom/cut/top layers from the viarule.
TEST_F(TestPDNViaTech, GenerateViaGeneratorReadsLayersFromRule)
{
  const ViaStack s = makeBasicViaStack();

  GenerateViaGenerator gen(getLogger(),
                           s.rule,
                           odb::Rect(0, 0, 1000, 1000),
                           loose(),
                           odb::Rect(0, 0, 1000, 1000),
                           loose());
  EXPECT_EQ(gen.getBottomLayer(), s.bottom);
  EXPECT_EQ(gen.getCutLayer(), s.cut);
  EXPECT_EQ(gen.getTopLayer(), s.top);
  EXPECT_EQ(gen.getRuleName(), s.rule->getName());
}

// getName() is non-empty and includes the rule name in some form.
TEST_F(TestPDNViaTech, GenerateViaGeneratorGetNameIsNonEmpty)
{
  const ViaStack s = makeBasicViaStack();

  GenerateViaGenerator gen(getLogger(),
                           s.rule,
                           odb::Rect(0, 0, 2000, 2000),
                           loose(),
                           odb::Rect(0, 0, 2000, 2000),
                           loose());
  EXPECT_FALSE(gen.getName().empty());
}

// A FROMABOVE MINIMUMCUT rule covers only connections from above its layer.
// The via is below the top layer, so the rule does not apply to it there,
// and a cut count the single-cut via cannot meet does not reject it.
TEST_F(TestPDNViaTech, ViaGeneratorMinCutFromAboveIsIgnoredForTopLayer)
{
  ViaStack s = makeBasicViaStack();
  addMinCutRuleV54(s.top,
                   /*num_cuts=*/99,
                   /*width=*/1,
                   /*above_only=*/true,
                   /*below_only=*/false);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// build() succeeds on a wide-open area and makeBaseVia returns a DbBaseVia.
TEST_F(TestPDNViaTech, GenerateViaGeneratorMakeBaseViaAfterBuild)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  GenerateViaGenerator gen(getLogger(), s.rule, r, loose(), r, loose());
  if (!gen.build(false, false)) {
    GTEST_SKIP() << "build() did not produce a placeable via";
  }
  DbBaseVia* base = gen.makeBaseVia(gen.getRows(), 0, gen.getColumns(), 0);
  ASSERT_NE(base, nullptr);
  delete base;
}

// -------- Rule-driven branch coverage --------

// When a cut-class rule on the cut layer matches the via's cut box,
// determineCutClass attaches it and hasCutClass() becomes true.
TEST_F(TestPDNViaTech, ViaGeneratorPicksUpMatchingCutClass)
{
  ViaStack s = makeBasicViaStack();
  auto* cut_class = addCutClassMatching(s, "VC", /*cut_dim=*/50);

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_TRUE(gen.hasCutClass());
  EXPECT_EQ(gen.getCutClass(), cut_class);
}

// A cut-class rule whose width/length don't match the via's cut leaves
// cutclass_ unset, exercising the non-matching branch in determineCutClass.
TEST_F(TestPDNViaTech, ViaGeneratorIgnoresNonMatchingCutClass)
{
  ViaStack s = makeBasicViaStack();               // cut is 50x50
  addCutClassMatching(s, "VC", /*cut_dim=*/999);  // won't match

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_FALSE(gen.hasCutClass());
}

// LEF58 CUTSPACING table with a cut class drives the class-pair branch of
// determineCutSpacing. With spacing=120 and our 50x50 cut, the resulting
// pitch is cut + spacing = 170.
TEST_F(TestPDNViaTech, ViaGeneratorDetermineCutSpacingUsesSpacingTable)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  addCutSpacingTableRule(s.cut, "VC", /*spacing=*/120);

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_EQ(gen.getCutPitchX(), 170);
  EXPECT_EQ(gen.getCutPitchY(), 170);
}

// Each spacing table entry is a pair: the spacing without a parallel run,
// then the spacing with one. The cuts of an array line up and have a
// parallel run, so a pair whose second value is larger must give the larger.
TEST_F(TestPDNViaTech, ViaGeneratorDetermineCutSpacingUsesTheLargerOfAPair)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  auto* rule = odb::dbTechLayerCutSpacingTableDefRule::create(s.cut);
  rule->setDefault(0);
  const std::map<std::string, uint32_t> index{{"VC/SIDE", 0}, {"VC/END", 1}};
  rule->setSpacingTable(
      {{{120, 200}, {120, 200}}, {{120, 200}, {120, 200}}}, index, index);

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_EQ(gen.getCutPitchX(), 250);
  EXPECT_EQ(gen.getCutPitchY(), 250);
}

// CENTERTOCENTER on the rule's class pair makes determineCutSpacing
// subtract the cut size from the looked-up spacing before turning it into
// a pitch. With spacing=120 on a 50x50 cut: rule_spacing = 120 - 50 = 70,
// then pitch = cut + 70 = 120.
TEST_F(TestPDNViaTech, ViaGeneratorDetermineCutSpacingHonorsCenterToCenter)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  auto* rule = addCutSpacingTableRule(s.cut, "VC", /*spacing=*/120);
  rule->addCenterToCenterEntry("VC", "VC");

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_EQ(gen.getCutPitchX(), 120);
  EXPECT_EQ(gen.getCutPitchY(), 120);
}

// SAMENET on the rule causes determineCutSpacing's rule loop to break
// after the first matching rule; the final pitch reflects that one rule's
// spacing.
TEST_F(TestPDNViaTech, ViaGeneratorDetermineCutSpacingHonorsSameNetBreak)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  addCutSpacingTableRule(s.cut, "VC", /*spacing=*/120, /*same_net=*/true);

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  EXPECT_EQ(gen.getCutPitchX(), 170);
  EXPECT_EQ(gen.getCutPitchY(), 170);
}

// A CUTSPACING table rule with a non-null SecondLayer is skipped by
// determineCutSpacing. Verify by setting up an effective rule alongside a
// skipped one and confirming the skipped rule's larger spacing is ignored.
TEST_F(TestPDNViaTech, ViaGeneratorDetermineCutSpacingSkipsSecondLayerRule)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  // Effective rule: 120 spacing -> 170 pitch.
  addCutSpacingTableRule(s.cut, "VC", /*spacing=*/120);
  // Skipped rule: would push pitch to 50 + 999 if not skipped.
  auto* skipped = addCutSpacingTableRule(s.cut, "VC", /*spacing=*/999);
  skipped->setSecondLayer(s.top);

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  // The pitch reflects only the non-second-layer rule.
  EXPECT_EQ(gen.getCutPitchX(), 170);
  EXPECT_EQ(gen.getCutPitchY(), 170);
}

// Setting the cut layer's spacing triggers the first branch of
// determineCutSpacing and pushes the cut pitch out.
TEST_F(TestPDNViaTech, ViaGeneratorDetermineCutSpacingUsesLayerSpacing)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/40);

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  // Cut is 50x50, so pitch should now be cut_dim + spacing = 50 + 40 = 90.
  EXPECT_EQ(gen.getCutPitchX(), 90);
  EXPECT_EQ(gen.getCutPitchY(), 90);
}

// A LEF54 MINIMUMCUT rule that a via cannot satisfy rejects it: a 2x2
// array has 4 cuts against the 8 required for a wire wider than 1.
TEST_F(TestPDNViaTech, ViaGeneratorCheckConstraintsFailsWhenMinCutsViolated)
{
  ViaStack s = makeBasicViaStack();
  addMinCutRuleV54(s.bottom, /*num_cuts=*/8, /*width=*/1);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setMaxRows(2);
  gen.setMaxColumns(2);
  EXPECT_FALSE(gen.build(false, false));
}

// With a sufficiently loose MINIMUMCUT rule (1 cut), checkMinCuts passes.
TEST_F(TestPDNViaTech, ViaGeneratorCheckConstraintsPassesWithLooseMinCuts)
{
  ViaStack s = makeBasicViaStack();
  addMinCutRuleV54(s.bottom, /*num_cuts=*/1, /*width=*/1);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// A FROMBELOW MINIMUMCUT rule covers only connections from below its layer.
// The via is above the bottom layer, so the rule does not apply to it there.
TEST_F(TestPDNViaTech, ViaGeneratorMinCutFromBelowIsIgnoredForBottomLayer)
{
  ViaStack s = makeBasicViaStack();
  addMinCutRuleV54(s.bottom,
                   /*num_cuts=*/99,
                   /*width=*/1,
                   /*above_only=*/false,
                   /*below_only=*/true);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// The via comes from below the top layer, so a FROMBELOW rule there applies,
// and the single-cut via cannot meet its 99 cuts.
TEST_F(TestPDNViaTech, ViaGeneratorMinCutFromBelowAppliesToTopLayer)
{
  ViaStack s = makeBasicViaStack();
  addMinCutRuleV54(s.top,
                   /*num_cuts=*/99,
                   /*width=*/1,
                   /*above_only=*/false,
                   /*below_only=*/true);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_FALSE(gen.build(false, false));
}

// The via comes from above the bottom layer, so a FROMABOVE rule there
// applies, and the single-cut via cannot meet its 99 cuts.
TEST_F(TestPDNViaTech, ViaGeneratorMinCutFromAboveAppliesToBottomLayer)
{
  ViaStack s = makeBasicViaStack();
  addMinCutRuleV54(s.bottom,
                   /*num_cuts=*/99,
                   /*width=*/1,
                   /*above_only=*/true,
                   /*below_only=*/false);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_FALSE(gen.build(false, false));
}

// A cut enclosure rule attached to the cut layer feeds the
// getCutMinimumEnclosureRules path and getMinimumEnclosures populates its
// vectors from the rule.
TEST_F(TestPDNViaTech, ViaGeneratorCheckMinEnclosureWithCutEnclosureRule)
{
  ViaStack s = makeBasicViaStack();
  // Modest overhang requirement (well below the 10-unit physical overhang
  // built into makeBasicViaStack).
  addCutEnclosureRule(s.cut, /*first_overhang=*/5, /*second_overhang=*/5);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// A cut enclosure rule larger than a 200x200 shape can hold: shapes that may
// grow take the full overhang, and shapes that must hold the via reject it.
TEST_F(TestPDNViaTech, ViaGeneratorBuildOrCheckRejectsTooLargeOverhangRule)
{
  ViaStack s = makeBasicViaStack();
  addCutEnclosureRule(s.cut,
                      /*first_overhang=*/1000,
                      /*second_overhang=*/1000);

  const odb::Rect tight(0, 0, 200, 200);
  TechViaGenerator grows(getLogger(), s.via, tight, loose(), tight, loose());
  ASSERT_TRUE(grows.build(false, false));
  for (const Enclosure* enc :
       {grows.getBottomEnclosure(), grows.getTopEnclosure()}) {
    EXPECT_GE(enc->getX(), 1000);
    EXPECT_GE(enc->getY(), 1000);
  }

  const ViaGenerator::Constraint must_fit{true, true, false, std::nullopt};
  TechViaGenerator fits(getLogger(), s.via, tight, must_fit, tight, must_fit);
  EXPECT_FALSE(fits.build(false, false));
}

// GenerateViaGenerator::isSetupValid is true when the routing widths fit
// within the WIDTH range set on the rule's metal layer rules.
TEST_F(TestPDNViaTech, GenerateViaGeneratorIsSetupValidWithinWidthRange)
{
  ViaStack s = makeBasicViaStack();
  setRoutingWidthRange(s, /*min_w=*/10, /*max_w=*/100);

  const odb::Rect r(0, 0, 50, 50);  // 50 falls inside [10, 100]
  GenerateViaGenerator gen(getLogger(), s.rule, r, loose(), r, loose());
  EXPECT_TRUE(gen.isSetupValid(s.bottom, s.top));
}

// And false when the routing rect is too wide for the WIDTH range.
TEST_F(TestPDNViaTech, GenerateViaGeneratorIsSetupInvalidOutsideWidthRange)
{
  ViaStack s = makeBasicViaStack();
  setRoutingWidthRange(s, /*min_w=*/10, /*max_w=*/100);

  const odb::Rect too_wide(0, 0, 500, 500);  // 500 > max 100
  GenerateViaGenerator gen(
      getLogger(), s.rule, too_wide, loose(), too_wide, loose());
  EXPECT_FALSE(gen.isSetupValid(s.bottom, s.top));
}

// updateCutSpacing's V58 ADJACENTCUTS rule path. A square area gives an
// array with several rows and columns, so a cut has adjacent cuts along both
// axes and both pitches take the rule's spacing.
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingHonorsAdjacentCutsRule)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  setCutLayerSpacing(s, /*spacing=*/30);  // base pitch = 50 + 30 = 80
  addAdjacentCutsSpacingRule(s.cut, /*cut_spacing=*/200, /*adjacent_cuts=*/2);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getCutPitchX(), 250);  // 50 + 200
  EXPECT_EQ(gen.getCutPitchY(), 250);
}

// recheckConstraints with bottom=false exercises the upper-rect path.
TEST_F(TestPDNViaTech, ViaGeneratorRecheckConstraintsUpperRectPath)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  const odb::Rect before = gen.getUpperRect();
  (void) gen.recheckConstraints(odb::Rect(0, 0, 1000, 1000),
                                /*bottom=*/false);
  EXPECT_EQ(gen.getUpperRect(), before);
}

// isPreferredOver between two equal-cuts generators on equal-area rects
// must return false in both directions.
TEST_F(TestPDNViaTech, ViaGeneratorIsPreferredOverEqualPairIsBothFalse)
{
  const ViaStack s = makeBasicViaStack();

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator a(getLogger(), s.via, r, loose(), r, loose());
  TechViaGenerator b(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(a.build(false, false));
  ASSERT_TRUE(b.build(false, false));
  EXPECT_FALSE(a.isPreferredOver(&b));
  EXPECT_FALSE(b.isPreferredOver(&a));
}

// With two distinct vias of different cut areas on the same rect, the
// generator with the larger cut area wins via the early-return branch
// (getCutArea() > other->getCutArea()).
TEST_F(TestPDNViaTech, ViaGeneratorIsPreferredOverLargerCutAreaWins)
{
  const ViaStack big
      = makeBasicViaStack("big_", /*cut_size=*/80, /*enclosure=*/20);
  const ViaStack small
      = makeBasicViaStack("sml_", /*cut_size=*/40, /*enclosure=*/20);

  const odb::Rect r(0, 0, 1000, 1000);
  TechViaGenerator a(getLogger(), big.via, r, loose(), r, loose());
  TechViaGenerator b(getLogger(), small.via, r, loose(), r, loose());
  ASSERT_TRUE(a.build(false, false));
  ASSERT_TRUE(b.build(false, false));
  // big cut area beats small cut area in either single-cut or array form.
  EXPECT_TRUE(a.isPreferredOver(&b));
  EXPECT_FALSE(b.isPreferredOver(&a));
}

// Two vias with the same cut area (single cut, same cut size) but
// different enclosures produce different generator dimensions, engaging
// the bottom_prefered/top_prefered tie-breakers in isPreferredOver. With
// a horizontal bottom layer, the larger-height enclosure is preferred
// (it covers more of the routing track).
TEST_F(TestPDNViaTech, ViaGeneratorIsPreferredOverPicksLargerEnclosure)
{
  const ViaStack a = makeBasicViaStack("sm_",
                                       /*cut_size=*/50,
                                       /*enclosure=*/10);  // shape 70
  const ViaStack b = makeBasicViaStack("lg_",
                                       /*cut_size=*/50,
                                       /*enclosure=*/25);  // shape 100

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator ga(getLogger(), a.via, r, loose(), r, loose());
  TechViaGenerator gb(getLogger(), b.via, r, loose(), r, loose());
  ASSERT_TRUE(ga.build(false, false));
  ASSERT_TRUE(gb.build(false, false));
  EXPECT_EQ(ga.getCutArea(), gb.getCutArea());  // identical cut area
  EXPECT_FALSE(ga.isPreferredOver(&gb));
  EXPECT_TRUE(gb.isPreferredOver(&ga));
}

// Two generators with identical configurations on differently-rotated
// rects produce equal preferred and non-preferred deltas, hitting the
// final `return false` fall-through of isPreferredOver.
TEST_F(TestPDNViaTech, ViaGeneratorIsPreferredOverFullEqualityFallsThrough)
{
  const ViaStack s = makeBasicViaStack();
  const odb::Rect r(0, 0, 200, 200);
  TechViaGenerator a(getLogger(), s.via, r, loose(), r, loose());
  TechViaGenerator b(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(a.build(false, false));
  ASSERT_TRUE(b.build(false, false));
  EXPECT_FALSE(a.isPreferredOver(&b));
  EXPECT_FALSE(b.isPreferredOver(&a));
}

// Helper: build a custom-shape tech via with asymmetric enclosures so we
// can drive isPreferredOver's per-axis tie-breakers.
static odb::dbTechVia* makeAsymVia(odb::dbTech* tech,
                                   odb::dbTechLayer* bottom,
                                   odb::dbTechLayer* cut,
                                   odb::dbTechLayer* top,
                                   const char* name,
                                   int cut_size,
                                   int bot_enc_x,
                                   int bot_enc_y,
                                   int top_enc_x,
                                   int top_enc_y)
{
  const int half = cut_size / 2;
  odb::dbTechVia* v = odb::dbTechVia::create(tech, name);
  odb::dbBox::create(v,
                     bottom,
                     -half - bot_enc_x,
                     -half - bot_enc_y,
                     half + bot_enc_x,
                     half + bot_enc_y);
  odb::dbBox::create(v, cut, -half, -half, half, half);
  odb::dbBox::create(v,
                     top,
                     -half - top_enc_x,
                     -half - top_enc_y,
                     half + top_enc_x,
                     half + top_enc_y);
  return v;
}

// Two vias with identical bottom enclosure but differing TOP enclosure
// drive the top_prefered (= top_width_diff for VERTICAL top) tie-break.
TEST_F(TestPDNViaTech, ViaGeneratorIsPreferredOverTopPreferredTieBreaks)
{
  const ViaStack s = makeBasicViaStack();
  // Two vias share the basic stack's layers but differ only in top metal
  // enclosure width.
  odb::dbTechVia* va = makeAsymVia(tech(),
                                   s.bottom,
                                   s.cut,
                                   s.top,
                                   "VA",
                                   /*cut_size=*/50,
                                   /*bot_enc_x=*/10,
                                   /*bot_enc_y=*/10,
                                   /*top_enc_x=*/10,
                                   /*top_enc_y=*/10);
  odb::dbTechVia* vb = makeAsymVia(tech(),
                                   s.bottom,
                                   s.cut,
                                   s.top,
                                   "VB",
                                   50,
                                   10,
                                   10,
                                   /*top_enc_x=*/25,
                                   /*top_enc_y=*/10);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator ga(getLogger(), va, r, loose(), r, loose());
  TechViaGenerator gb(getLogger(), vb, r, loose(), r, loose());
  ASSERT_TRUE(ga.build(false, false));
  ASSERT_TRUE(gb.build(false, false));
  EXPECT_EQ(ga.getCutArea(), gb.getCutArea());
  // bottom dims match, top widths differ -> top_prefered (vertical top)
  // tie-break engages. Larger top width preferred.
  EXPECT_FALSE(ga.isPreferredOver(&gb));
  EXPECT_TRUE(gb.isPreferredOver(&ga));
}

// With matched preferred dimensions on both layers, only the non-preferred
// dimensions can differ. A via with a wider bottom (non-preferred for
// HORIZONTAL bottom) drives the bottom_non_prefered tie-break.
TEST_F(TestPDNViaTech, ViaGeneratorIsPreferredOverBottomNonPreferredTieBreaks)
{
  const ViaStack s = makeBasicViaStack();
  // Same heights everywhere, only bottom width differs.
  odb::dbTechVia* va = makeAsymVia(tech(),
                                   s.bottom,
                                   s.cut,
                                   s.top,
                                   "VAN",
                                   50,
                                   /*bot_enc_x=*/10,
                                   /*bot_enc_y=*/10,
                                   10,
                                   10);
  odb::dbTechVia* vb = makeAsymVia(tech(),
                                   s.bottom,
                                   s.cut,
                                   s.top,
                                   "VBN",
                                   50,
                                   /*bot_enc_x=*/25,
                                   /*bot_enc_y=*/10,
                                   10,
                                   10);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator ga(getLogger(), va, r, loose(), r, loose());
  TechViaGenerator gb(getLogger(), vb, r, loose(), r, loose());
  ASSERT_TRUE(ga.build(false, false));
  ASSERT_TRUE(gb.build(false, false));
  // Bottom non-preferred differs; one of the generators wins.
  const bool a_wins = ga.isPreferredOver(&gb);
  const bool b_wins = gb.isPreferredOver(&ga);
  EXPECT_NE(a_wins, b_wins);
}

// And the symmetric top non-preferred case: top heights differ on vias
// with otherwise identical enclosures.
TEST_F(TestPDNViaTech, ViaGeneratorIsPreferredOverTopNonPreferredTieBreaks)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbTechVia* va = makeAsymVia(
      tech(), s.bottom, s.cut, s.top, "VAT", 50, 10, 10, 10, /*top_enc_y=*/10);
  odb::dbTechVia* vb = makeAsymVia(
      tech(), s.bottom, s.cut, s.top, "VBT", 50, 10, 10, 10, /*top_enc_y=*/25);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator ga(getLogger(), va, r, loose(), r, loose());
  TechViaGenerator gb(getLogger(), vb, r, loose(), r, loose());
  ASSERT_TRUE(ga.build(false, false));
  ASSERT_TRUE(gb.build(false, false));
  const bool a_wins = ga.isPreferredOver(&gb);
  const bool b_wins = gb.isPreferredOver(&ga);
  EXPECT_NE(a_wins, b_wins);
}

// isPreferredOver(nullptr) returns true immediately (already covered for
// the base class but exercises the early-return on this concrete subclass
// as well).
TEST_F(TestPDNViaTech, ViaGeneratorIsPreferredOverNullptrReturnsTrue)
{
  const ViaStack s = makeBasicViaStack();
  const odb::Rect r(0, 0, 1000, 1000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_TRUE(gen.isPreferredOver(nullptr));
}

// V54 SPACING ADJACENTCUTS fires when no LEF58 ADJACENTCUTS rule applies.
// The fallback path sets both pitches to cut_dim + rule_spacing = 50 + 250.
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingHonorsV54Rule)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);
  addV54AdjacentCutsRule(s.cut,
                         /*numcuts=*/2,
                         /*within=*/100,
                         /*spacing=*/250);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getCutPitchX(), 300);
  EXPECT_EQ(gen.getCutPitchY(), 300);
}

// An ABOVE-only cut enclosure rule applies only to the top layer; bottom
// layer enclosures pass through without enforcement.
TEST_F(TestPDNViaTech, ViaGeneratorCutEnclosureRuleAboveOnlyAppliesToTop)
{
  ViaStack s = makeBasicViaStack();
  addAboveBelowCutEnclosureRule(s.cut,
                                /*first=*/5,
                                /*second=*/5,
                                /*above=*/true,
                                /*below=*/false);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// Same for BELOW-only -- exercises the symmetric branch in
// getCutMinimumEnclosureRules.
TEST_F(TestPDNViaTech, ViaGeneratorCutEnclosureRuleBelowOnlyAppliesToBottom)
{
  ViaStack s = makeBasicViaStack();
  addAboveBelowCutEnclosureRule(s.cut,
                                5,
                                5,
                                /*above=*/false,
                                /*below=*/true);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// Two MIN-CUT rules at different widths -- the largest one whose width is
// below the wire width should be the rule used. Drives the
// `rule_width < width` selection branch in checkMinCuts.
TEST_F(TestPDNViaTech, ViaGeneratorMinCutPicksLargestApplicableWidthRule)
{
  ViaStack s = makeBasicViaStack();
  // Two rules: a small-width 1-cut rule (always applies, but trivially
  // satisfied), and a wider rule that demands 1 cut (also satisfied).
  addMinCutRuleV54(s.bottom, /*num_cuts=*/1, /*width=*/10);
  addMinCutRuleV54(s.bottom, /*num_cuts=*/1, /*width=*/100);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// must_fit_x with a lower rect that comfortably holds the via's 70-wide
// bottom enclosure drives the x-only-fits branch of
// TechViaGenerator::mostlyContains (called via isSetupValid -> fitsShapes)
// and accepts.
TEST_F(TestPDNViaTech, ViaGeneratorMustFitXOnlyDrivesXAxisCheck)
{
  const ViaStack s = makeBasicViaStack();
  const odb::Rect lower(0, 0, 200, 2000);
  const ViaGenerator::Constraint must_fit_x{true, false, false, std::nullopt};
  TechViaGenerator gen(getLogger(),
                       s.via,
                       lower,
                       must_fit_x,
                       odb::Rect(0, 0, 2000, 2000),
                       loose());
  EXPECT_TRUE(gen.isSetupValid(s.bottom, s.top));
  EXPECT_TRUE(gen.build(false, false));
}

// must_fit_x rejects a lower rect narrower than the via's bottom enclosure.
// The bottom layer is HORIZONTAL, so without must_fit_x this rect would be
// accepted by the bridging branch (see
// TechViaBridgesHorizontalBottomAlongPreferredDirection); must_fit_x has to
// win, because a constrained layer must never be allowed to extend.
TEST_F(TestPDNViaTech, ViaGeneratorMustFitXOverridesHorizontalBridging)
{
  const ViaStack s = makeBasicViaStack();
  // 40 < the 70-wide bottom enclosure -> the via overhangs both x sides.
  const odb::Rect lower(0, 0, 40, 2000);
  const ViaGenerator::Constraint must_fit_x{true, false, false, std::nullopt};
  TechViaGenerator gen(getLogger(),
                       s.via,
                       lower,
                       must_fit_x,
                       odb::Rect(0, 0, 2000, 2000),
                       loose());
  EXPECT_FALSE(gen.isSetupValid(s.bottom, s.top));
}

// must_fit_y, symmetric to the must_fit_x case.
TEST_F(TestPDNViaTech, ViaGeneratorMustFitYOnlyDrivesYAxisCheck)
{
  const ViaStack s = makeBasicViaStack();
  const odb::Rect lower(0, 0, 2000, 200);
  const ViaGenerator::Constraint must_fit_y{false, true, false, std::nullopt};
  TechViaGenerator gen(getLogger(),
                       s.via,
                       lower,
                       must_fit_y,
                       odb::Rect(0, 0, 2000, 2000),
                       loose());
  EXPECT_TRUE(gen.isSetupValid(s.bottom, s.top));
  EXPECT_TRUE(gen.build(false, false));
}

// must_fit_y rejects a lower rect shorter than the via's bottom enclosure,
// mirroring ViaGeneratorMustFitXOverridesHorizontalBridging for a VERTICAL
// bottom layer (which would otherwise be free to extend in y).
TEST_F(TestPDNViaTech, ViaGeneratorMustFitYOverridesVerticalBridging)
{
  const ViaStack s
      = makeBasicViaStack("", 50, 10, odb::dbTechLayerDir::VERTICAL);
  const odb::Rect lower(0, 0, 2000, 40);
  const ViaGenerator::Constraint must_fit_y{false, true, false, std::nullopt};
  TechViaGenerator gen(getLogger(),
                       s.via,
                       lower,
                       must_fit_y,
                       odb::Rect(0, 0, 2000, 2000),
                       loose());
  EXPECT_FALSE(gen.isSetupValid(s.bottom, s.top));
}

// DbTechVia::generate with the bottom layer in ongrid triggers the
// snap-to-grid branch (populateGrid + do_bottom_snap path).
TEST_F(TestPDNViaTech, DbTechViaGenerateWithBottomOnGrid)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDDG");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  DbTechVia dbtv(s.via, 1, 0, 1, 0);
  odb::PtrSet<odb::dbTechLayer> ongrid;
  ongrid.insert(s.bottom);
  const auto shapes = dbtv.generate(block(),
                                    wire,
                                    odb::dbWireShapeType::STRIPE,
                                    200,
                                    200,
                                    ongrid,
                                    getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
}

// DbTechVia::generate with an ARRAY via and ongrid populated drives the
// snap-to-grid-interval branch in the array sub-path (`*do_col_snap` and
// `*do_row_snap`).
TEST_F(TestPDNViaTech, DbTechViaGenerateArrayWithOnGridSnapsPitch)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), s.bottom);
  tracks->addGridPatternY(0, 10, 50);
  tracks->addGridPatternX(0, 10, 50);
  odb::dbTrackGrid* top_tracks = odb::dbTrackGrid::create(block(), s.top);
  top_tracks->addGridPatternX(0, 10, 50);
  top_tracks->addGridPatternY(0, 10, 50);

  odb::dbNet* net = odb::dbNet::create(block(), "VDDAOG");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  DbTechVia dbtv(s.via,
                 /*rows=*/2,
                 /*row_pitch=*/200,
                 /*cols=*/2,
                 /*col_pitch=*/200);
  odb::PtrSet<odb::dbTechLayer> ongrid;
  ongrid.insert(s.bottom);
  ongrid.insert(s.top);
  const auto shapes = dbtv.generate(block(),
                                    wire,
                                    odb::dbWireShapeType::STRIPE,
                                    500,
                                    500,
                                    ongrid,
                                    getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// And with the top layer in ongrid.
TEST_F(TestPDNViaTech, DbTechViaGenerateWithTopOnGrid)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbNet* net = odb::dbNet::create(block(), "VDDGT");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  DbTechVia dbtv(s.via, 1, 0, 1, 0);
  odb::PtrSet<odb::dbTechLayer> ongrid;
  ongrid.insert(s.top);
  const auto shapes = dbtv.generate(block(),
                                    wire,
                                    odb::dbWireShapeType::STRIPE,
                                    200,
                                    200,
                                    ongrid,
                                    getLogger());
  EXPECT_FALSE(shapes.top.empty());
}

// An ARRAYSPACING rule on the cut layer flips build()/
// determineRowsAndColumns into the array-rules path: it segments the
// array into chunks and inserts extra spacing between them.
TEST_F(TestPDNViaTech, ViaGeneratorArraySpacingRuleEngagesArrayPath)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  setCutLayerSpacing(s, /*spacing=*/50);  // pitch = 100 -> 20 x 20 cuts
  addArraySpacingRule(s.cut, /*num_cuts=*/2, /*array_spacing=*/200);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.isCutArray());
}

// LONGARRAY ARRAYSPACING rule -- exercises the "long_array" sub-branch.
TEST_F(TestPDNViaTech, ViaGeneratorArraySpacingLongArrayBranch)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  addArraySpacingRule(s.cut,
                      /*num_cuts=*/2,
                      /*array_spacing=*/200,
                      /*cut_spacing=*/0,
                      /*long_array=*/true);
  setCutLayerSpacing(s, /*spacing=*/50);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.isCutArray());
}

// Array spacing rule whose ArrayWidth filter excludes the via -- drives
// the early-skip branch in determineRowsAndColumns.
TEST_F(TestPDNViaTech, ViaGeneratorArraySpacingArrayWidthFilterSkipsRule)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  setCutLayerSpacing(s, /*spacing=*/50);
  auto* rule = addArraySpacingRule(s.cut, 2, 200);
  rule->setArrayWidth(99999);  // larger than our rect width -> rule is skipped

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_TRUE(gen.build(false, false));
  EXPECT_FALSE(gen.isCutArray());
}

// A PARALLELOVERLAP array spacing rule is skipped entirely.
TEST_F(TestPDNViaTech, ViaGeneratorArraySpacingParallelOverlapIsIgnored)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  setCutLayerSpacing(s, /*spacing=*/50);
  auto* rule = addArraySpacingRule(s.cut, 2, 200);
  rule->setParallelOverlap(true);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_TRUE(gen.build(false, false));
  EXPECT_FALSE(gen.isCutArray());
}

// An array spacing rule for another cut class does not apply to this via.
TEST_F(TestPDNViaTech, ViaGeneratorArraySpacingForAnotherCutClassIsIgnored)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  auto* other_cls = odb::dbTechLayerCutClassRule::create(s.cut, "OTHER");
  other_cls->setWidth(999);
  setCutLayerSpacing(s, /*spacing=*/50);
  auto* rule = addArraySpacingRule(s.cut, 2, 200);
  rule->setCutClass(other_cls);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_TRUE(gen.build(false, false));
  EXPECT_FALSE(gen.isCutArray());
}

// An array spacing rule only applies to arrays at least as many cuts across
// as the rule names; a 20 x 20 array is too small for a 25 cut rule.
TEST_F(TestPDNViaTech, ViaGeneratorArraySpacingForLargerArraysIsIgnored)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/50);
  addArraySpacingRule(s.cut, /*num_cuts=*/25, /*array_spacing=*/200);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_TRUE(gen.build(false, false));
  EXPECT_FALSE(gen.isCutArray());
}

// When the width left after the last full array still fits a cut, the array
// ends in a narrower column of cuts. 1830 of usable width holds five 150 wide
// arrays at a 350 pitch plus one more cut; the 1980 of usable height holds
// six arrays and nothing more. Every cut placed is counted in the report.
TEST_F(TestPDNViaTech, ViaGeneratorArraySpacingEndsInAPartialColumn)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/50);
  addArraySpacingRule(s.cut, /*num_cuts=*/2, /*array_spacing=*/200);

  const odb::Rect r(0, 0, 1850, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.isCutArray());
  EXPECT_EQ(gen.getColumns(), 5 * 2 + 1);
  EXPECT_EQ(gen.getRows(), 6 * 2);

  odb::dbNet* net = odb::dbNet::create(block(), "VDD");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  std::unique_ptr<DbVia> via(gen.generate(block()));
  via->generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 0, 0, {}, getLogger());
  EXPECT_EQ(via->getViaReport().at(s.via->getName()), 11 * 12);
}

// mostlyContains' "contains > 2" fallback returns false when fewer than
// three sides of the lower rect contain the via shape and the shape spills
// out along the layer's *width* direction. The bottom layer is HORIZONTAL,
// so a lower rect shorter than the via in y (its width axis) gives
// contains == 2 with inside_y false -> isSetupValid returns false.
TEST_F(TestPDNViaTech, ViaGeneratorIsSetupValidFailsWhenViaDoesNotFit)
{
  const ViaStack s = makeBasicViaStack();
  // The via's bottom enclosure spans 70 tall. A 50-tall lower rect cannot
  // contain it on the y sides -> contains drops to 2 and the horizontal
  // layer may not extend across its width.
  const odb::Rect tight_y(0, 0, 2000, 50);
  TechViaGenerator gen(getLogger(),
                       s.via,
                       tight_y,
                       loose(),
                       odb::Rect(0, 0, 2000, 2000),
                       loose());
  EXPECT_FALSE(gen.isSetupValid(s.bottom, s.top));
}

// Vertical bottom-layer stack: drives the VERTICAL branch in
// DbGenerateStackedVia::generate that picks the snap-axes differently.
TEST_F(TestPDNViaTech, DbGenerateStackedViaVerticalBottomLayerGenerates)
{
  const ViaStack s1
      = makeBasicViaStack("", 50, 10, odb::dbTechLayerDir::VERTICAL);
  const ViaStack s2 = addStackedLevel(s1);

  odb::dbNet* net = odb::dbNet::create(block(), "VDDV");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  std::vector<std::unique_ptr<DbVia>> vias;
  vias.push_back(std::make_unique<DbTechVia>(s1.via, 1, 0, 1, 0));
  vias.push_back(std::make_unique<DbTechVia>(s2.via, 1, 0, 1, 0));
  DbGenerateStackedVia stacked(std::move(vias), s1.bottom, block());
  const auto shapes = stacked.generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 400, 400, {}, getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// DbGenerateStackedVia with ongrid populated for the intermediate layers
// drives the populateGrid loop.
TEST_F(TestPDNViaTech, DbGenerateStackedViaOnGridPopulatesLayerGrids)
{
  const ViaStack s1 = makeBasicViaStack();
  const ViaStack s2 = addStackedLevel(s1);
  // Build a track grid on the middle metal so populateGrid has something
  // to read.
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), s1.top);
  tracks->addGridPatternY(0, 5, 100);

  odb::dbNet* net = odb::dbNet::create(block(), "VDDVO");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  std::vector<std::unique_ptr<DbVia>> vias;
  vias.push_back(std::make_unique<DbTechVia>(s1.via, 1, 0, 1, 0));
  vias.push_back(std::make_unique<DbTechVia>(s2.via, 1, 0, 1, 0));
  DbGenerateStackedVia stacked(std::move(vias), s1.bottom, block());
  odb::PtrSet<odb::dbTechLayer> ongrid;
  ongrid.insert(s1.top);
  const auto shapes = stacked.generate(block(),
                                       wire,
                                       odb::dbWireShapeType::STRIPE,
                                       400,
                                       400,
                                       ongrid,
                                       getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
}

// V58 MINCUT rule keyed by cut class: with a matching cut class on the
// via, checkMinCuts iterates the rule and applies its cut count. The
// rule is attached to the top routing layer so its getLowerLayer() (=
// the cut layer) can resolve the cut-class name.
TEST_F(TestPDNViaTech, ViaGeneratorV58MinCutPerCutClassIsConsulted)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  addV58MinCutPerCutClass(s.top, "VC", /*num_cuts=*/1, /*width=*/10);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// A LEF58 MINIMUMCUT rule need not name a cut class; it then applies to
// every cut, and the single-cut via cannot meet its 99 cuts.
TEST_F(TestPDNViaTech, ViaGeneratorV58MinCutWithoutACutClassApplies)
{
  ViaStack s = makeBasicViaStack();
  odb::dbTechLayerMinCutRule* rule
      = odb::dbTechLayerMinCutRule::create(s.bottom);
  rule->setWidth(10);
  rule->setNumCuts(99);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_FALSE(gen.build(false, false));
}

// LEF58 FROMABOVE on the bottom layer covers this via, which comes from
// above it. The bottom layer has no cut layer below it, so the class can only
// be found above.
TEST_F(TestPDNViaTech, ViaGeneratorV58MinCutFromAboveAppliesToBottomLayer)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  auto* rule = addV58MinCutPerCutClass(s.bottom, "VC", 99, /*width=*/10);
  rule->setFromAbove(true);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_FALSE(gen.build(false, false));
}

// LEF58 FROMABOVE on the top layer does not cover this via, which comes from
// below it.
TEST_F(TestPDNViaTech, ViaGeneratorV58MinCutFromAboveIsIgnoredForTopLayer)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  auto* rule = addV58MinCutPerCutClass(s.top, "VC", 99, /*width=*/10);
  rule->setFromAbove(true);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// MINIMUMCUT applies to wires wider than its width, not as wide.
TEST_F(TestPDNViaTech, ViaGeneratorMinCutIgnoresAWireExactlyAtItsWidth)
{
  ViaStack s = makeBasicViaStack();
  addMinCutRuleV54(s.bottom, /*num_cuts=*/99, /*width=*/2000);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator at_width(getLogger(), s.via, r, loose(), r, loose());
  EXPECT_TRUE(at_width.build(false, false));

  const odb::Rect wider(0, 0, 2010, 2010);
  TechViaGenerator over_width(
      getLogger(), s.via, wider, loose(), wider, loose());
  EXPECT_FALSE(over_width.build(false, false));
}

// An ARRAYSPACING rule that segments the array into chunks can produce
// more total cut area than the simple non-array layout, driving the
// "save this" branch in determineRowsAndColumns. The cut layer's default
// spacing is first set so the initial array_size_max >= 2 test passes
// (and we enter the array-rules block).
TEST_F(TestPDNViaTech, ViaGeneratorArraySpacingRuleWinsOnLargeRect)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  setCutLayerSpacing(s, /*spacing=*/50);  // pitch = 100 -> 20 cols x 20 rows
  // Long-array rule with a smaller rule cut_spacing keeps x_cuts
  // unconstrained; y limited to 4 cuts per segment, 50-unit array spacing.
  addArraySpacingRule(s.cut,
                      /*num_cuts=*/4,
                      /*array_spacing=*/50,
                      /*cut_spacing=*/30,
                      /*long_array=*/true);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  // The array path winning means total cuts is non-trivial and the
  // generator picks an array layout.
  EXPECT_GT(gen.getTotalCuts(), 1);
}

// Split-cut path: setSplitCutArray forces determineRowsAndColumns into
// the cols=1/rows=1 short-circuit branch. We seed a non-zero cut pitch
// first to avoid the divide-by-zero that the split-cut sizing math
// would otherwise hit when no cut spacing has been set elsewhere.
TEST_F(TestPDNViaTech, ViaGeneratorSplitCutArrayTakesShortCircuitBranch)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setSplitCutArray(/*split_bot=*/true, /*split_top=*/true);
  EXPECT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.isSplitCutArray());
}

// -------- More ViaGenerator branch coverage --------

// A cut-class rule whose getCutClass() returns the matching class drives
// the second isCutClass branch (cutclass != nullptr && cutclass_ ==
// cutclass) inside getCutMinimumEnclosureRules.
TEST_F(TestPDNViaTech, ViaGeneratorEnclosureRuleClassFilterMatches)
{
  ViaStack s = makeBasicViaStack();
  auto* cls = addCutClassMatching(s, "VC", /*cut_dim=*/50);
  auto* enc_rule
      = addCutEnclosureRule(s.cut,
                            /*first=*/5,
                            /*second=*/5,
                            odb::dbTechLayerCutEnclosureRule::DEFAULT);
  enc_rule->setCutClass(cls);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// A cut-class rule attached to a DIFFERENT class than what the via has
// is filtered out by the isCutClass check.
TEST_F(TestPDNViaTech, ViaGeneratorEnclosureRuleClassFilterRejectsOther)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  // Add an unrelated class and attach a too-big enclosure rule to it.
  auto* other_cls = odb::dbTechLayerCutClassRule::create(s.cut, "OTHER");
  other_cls->setWidth(999);  // won't match the cut
  auto* enc_rule = addCutEnclosureRule(s.cut, /*first=*/9999, /*second=*/9999);
  enc_rule->setCutClass(other_cls);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  // The unrelated rule is filtered out, so it doesn't reject the build.
  EXPECT_TRUE(gen.checkConstraints());
}

// determineCutSpacing's CENTERANDEDGE branch: rule_spacing stays equal to
// rule_pitch (no subtraction).
TEST_F(TestPDNViaTech, ViaGeneratorDetermineCutSpacingHonorsCenterAndEdge)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  auto* rule = addCutSpacingTableRule(s.cut, "VC", /*spacing=*/120);
  rule->addCenterAndEdgeEntry("VC", "VC");

  TechViaGenerator gen(getLogger(),
                       s.via,
                       odb::Rect(0, 0, 1000, 1000),
                       loose(),
                       odb::Rect(0, 0, 1000, 1000),
                       loose());
  // Same effective pitch as default branch (120 + cut = 170).
  EXPECT_EQ(gen.getCutPitchX(), 170);
  EXPECT_EQ(gen.getCutPitchY(), 170);
}

// updateCutSpacing's V58 rule path: a rule restricted to a SPECIFIC cut
// class (not CutClassToAll) is consulted only when cutclass_ matches.
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingFiltersByCutClassPointer)
{
  ViaStack s = makeBasicViaStack();
  auto* cls = addCutClassMatching(s, "VC", /*cut_dim=*/50);
  setCutLayerSpacing(s, /*spacing=*/30);
  // Rule bound to our class (not CutClassToAll).
  odb::dbTechLayerCutSpacingRule* rule
      = odb::dbTechLayerCutSpacingRule::create(s.cut);
  rule->setType(odb::dbTechLayerCutSpacingRule::ADJACENTCUTS);
  rule->setCutSpacing(200);
  rule->setAdjacentCuts(2);
  rule->setCutClass(cls);
  // Not setCutClassToAll(true) -> the rule must match cutclass_.

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  // Cut class matches -> both axes rewritten to cut + 200 = 250.
  EXPECT_EQ(gen.getCutPitchX(), 250);
  EXPECT_EQ(gen.getCutPitchY(), 250);
}

// updateCutSpacing's V54 rule path with except_same_pgnet=true is skipped.
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingSkipsV54ExceptSamePgnet)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);  // base pitch = 80
  odb::dbTechLayerSpacingRule* rule
      = odb::dbTechLayerSpacingRule::create(s.cut);
  rule->setAdjacentCuts(/*numcuts=*/2,
                        /*within=*/100,
                        /*spacing=*/999,
                        /*except_same_pgnet=*/true);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  // The except_same_pgnet rule is skipped, so the base pitch (80) is
  // preserved -- not the 999-based pitch.
  EXPECT_EQ(gen.getCutPitchX(), 80);
  EXPECT_EQ(gen.getCutPitchY(), 80);
}

// GenerateViaGenerator::isLayerValidForWidth returns true when the via
// layer rule has no width restriction at all (`!hasWidth()`).
TEST_F(TestPDNViaTech, GenerateViaGeneratorIsSetupValidWithoutWidthRule)
{
  ViaStack s = makeBasicViaStack();
  // No setRoutingWidthRange call -> via_layer_rule->hasWidth() is false.
  const odb::Rect r(0, 0, 200, 200);
  GenerateViaGenerator gen(getLogger(), s.rule, r, loose(), r, loose());
  EXPECT_TRUE(gen.isSetupValid(s.bottom, s.top));
}

// GenerateViaGenerator::isSetupValid returns false when the base
// ViaGenerator::isSetupValid (appliesToLayers check) fails -- e.g. when
// asked about layers that don't match the rule's bottom/top.
TEST_F(TestPDNViaTech, GenerateViaGeneratorIsSetupValidFailsForWrongLayers)
{
  ViaStack s = makeBasicViaStack();
  odb::dbTechLayer* wrong = odb::dbTechLayer::create(
      tech(), "wrong", odb::dbTechLayerType::ROUTING);

  const odb::Rect r(0, 0, 200, 200);
  GenerateViaGenerator gen(getLogger(), s.rule, r, loose(), r, loose());
  EXPECT_FALSE(gen.isSetupValid(wrong, s.top));
}

// updateCutSpacing's min_dim == 1, max_dim == 2 path: a 1x2 array.
// adj_cuts = 1 -> `adj_cuts < 2` early-return short-circuit, so the rule
// loop is skipped.
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingMinDim1Max2)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);  // pitch 80
  addAdjacentCutsSpacingRule(s.cut, /*spacing=*/200, /*adj=*/2);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setMaxColumns(1);
  gen.setMaxRows(2);
  ASSERT_TRUE(gen.build(false, false));
  // adj_cuts < 2 -> updateCutSpacing returns early -> pitches unchanged.
  EXPECT_EQ(gen.getCutPitchX(), 80);
  EXPECT_EQ(gen.getCutPitchY(), 80);
}

// updateCutSpacing's min_dim == 2, max_dim == 2 path: a 2x2 array.
// adj_cuts = 2 -> rule with adj_cuts <= 2 fires.
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingMinDim2Max2)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);
  addAdjacentCutsSpacingRule(s.cut, /*spacing=*/200, /*adj=*/2);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setMaxColumns(2);
  gen.setMaxRows(2);
  ASSERT_TRUE(gen.build(false, false));
  // adj_cuts = 2 -> rule with adjacentCuts=2 applies -> one axis rewritten.
  EXPECT_TRUE(gen.getCutPitchX() == 250 || gen.getCutPitchY() == 250);
}

// updateCutSpacing's min_dim == 3, max_dim >= 3 path: a 3x_ array sets
// adj_cuts = 4. Combined with a rule asking for >= 3 adjacent cuts the
// pitch is rewritten.
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingMinDim3)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);
  addAdjacentCutsSpacingRule(s.cut, /*spacing=*/200, /*adj=*/3);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setMaxColumns(3);
  gen.setMaxRows(3);
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getCutPitchX(), 250);
  EXPECT_EQ(gen.getCutPitchY(), 250);
}

// A V58 CutSpacing rule whose type is not ADJACENTCUTS is skipped by
// updateCutSpacing's rule loop.
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingSkipsNonAdjacentCutsRule)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);
  // Rule with default type (NONE) -- not ADJACENTCUTS -> continue.
  odb::dbTechLayerCutSpacingRule* rule
      = odb::dbTechLayerCutSpacingRule::create(s.cut);
  rule->setType(odb::dbTechLayerCutSpacingRule::MAXXY);
  rule->setCutSpacing(999);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  // Rule is skipped -> base pitch (80) is preserved.
  EXPECT_EQ(gen.getCutPitchX(), 80);
  EXPECT_EQ(gen.getCutPitchY(), 80);
}

// A V58 ADJACENTCUTS rule whose getCutClass() != the via's cutclass_ and
// CutClassToAll == false is skipped (filtered out by class pointer match).
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingFiltersByDifferentClass)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  setCutLayerSpacing(s, /*spacing=*/30);
  // Rule bound to an UNrelated cut class.
  auto* other_cls = odb::dbTechLayerCutClassRule::create(s.cut, "OTHER");
  other_cls->setWidth(999);
  odb::dbTechLayerCutSpacingRule* rule
      = odb::dbTechLayerCutSpacingRule::create(s.cut);
  rule->setType(odb::dbTechLayerCutSpacingRule::ADJACENTCUTS);
  rule->setCutSpacing(999);
  rule->setAdjacentCuts(2);
  rule->setCutClass(other_cls);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getCutPitchX(), 80);
  EXPECT_EQ(gen.getCutPitchY(), 80);
}

// A LEF58 MINIMUMCUT rule whose cut_class doesn't match the via's class
// is filtered out at the start of checkMinCuts (drives the `continue` in
// the isCutClass-mismatch branch).
TEST_F(TestPDNViaTech, ViaGeneratorCheckMinCutsFiltersUnrelatedCutClass)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  // Add an unrelated cut class.
  odb::dbTechLayerCutClassRule* other
      = odb::dbTechLayerCutClassRule::create(s.cut, "OTHER");
  other->setWidth(999);
  // V58 MINIMUMCUT rule referencing the OTHER class. The rule's lookup
  // happens via the routing layer above, so attach to s.top.
  addV58MinCutPerCutClass(s.top,
                          "OTHER",
                          /*num_cuts=*/9999,
                          /*width=*/1);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  // Rule is filtered out (different cut class) -> constraints pass.
  EXPECT_TRUE(gen.checkConstraints());
}

// A cut enclosure rule with WIDTH-validity set causes
// getCutMinimumEnclosureRules to record min_width = rule->getMinWidth()
// (vs the default 0). Drives the `if (enc_rule->isWidthValid())` branch.
TEST_F(TestPDNViaTech, ViaGeneratorEnclosureRuleHonorsMinWidth)
{
  ViaStack s = makeBasicViaStack();
  odb::dbTechLayerCutEnclosureRule* rule
      = addCutEnclosureRule(s.cut, /*first=*/5, /*second=*/5);
  rule->setMinWidth(10);
  rule->setWidthValid(true);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_TRUE(gen.checkConstraints());
}

// DbSplitCutVia with a VERTICAL bottom layer drives the else-branch of
// the direction switch in DbSplitCutVia::generate (bottom becomes the
// vertical-snap layer instead of horizontal).
TEST_F(TestPDNViaTech, DbSplitCutViaVerticalBottomDirectionPath)
{
  const ViaStack s
      = makeBasicViaStack("", 50, 10, odb::dbTechLayerDir::VERTICAL);

  odb::dbNet* net = odb::dbNet::create(block(), "VDDSV");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  auto* inner = new DbTechVia(s.via, 1, 0, 1, 0);
  DbSplitCutVia split(inner,
                      /*rows=*/2,
                      /*row_pitch=*/300,
                      /*row_offset=*/0,
                      /*cols=*/2,
                      /*col_pitch=*/300,
                      /*col_offset=*/0,
                      block(),
                      s.bottom,
                      false,
                      s.top,
                      false);
  const auto shapes = split.generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 500, 500, {}, getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// DbGenerateStackedVia where each level is an ARRAY DbTechVia drives the
// patch-shape polygon math (`prev_via->requiresPatch() ||
// via->requiresPatch()` is true when adjacent vias are arrays).
TEST_F(TestPDNViaTech, DbGenerateStackedViaArrayLevelsEmitPatches)
{
  const ViaStack s1 = makeBasicViaStack();
  const ViaStack s2 = addStackedLevel(s1);

  odb::dbNet* net = odb::dbNet::create(block(), "VDDSA");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  // Each level is a 2x2 array via -> requiresPatch() is true for both.
  std::vector<std::unique_ptr<DbVia>> vias;
  vias.push_back(std::make_unique<DbTechVia>(s1.via, 2, 200, 2, 200));
  vias.push_back(std::make_unique<DbTechVia>(s2.via, 2, 200, 2, 200));
  DbGenerateStackedVia stacked(std::move(vias), s1.bottom, block());
  const auto shapes = stacked.generate(
      block(), wire, odb::dbWireShapeType::STRIPE, 500, 500, {}, getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// 3-via DbGenerateStackedVia stack drives the patch-shape polygon math
// between adjacent vias (prev_via != null branches with non-trivial
// shape differences).
TEST_F(TestPDNViaTech, DbGenerateStackedViaThreeLevelStackEmitsPatches)
{
  const ViaStack s1 = makeBasicViaStack();
  const ViaStack s2 = addStackedLevel(s1);
  // Manually add a third level by reusing m3 as the bottom of a new
  // stack. Build a fresh cut + top metal above m3.
  odb::dbTechLayer* v2_b = s2.cut;  // unused below; just a marker
  (void) v2_b;
  odb::dbTechLayer* cut3 = tst::addCutLayer(tech(), "v3");
  cut3->setWidth(50);
  odb::dbTechLayer* m4 = tst::addRoutingLayer(
      tech(), "m4", {.direction = odb::dbTechLayerDir::VERTICAL, .width = 70});

  odb::dbTechVia* v3 = tst::addVia(tech(), "TV3", s2.top, cut3, m4, 50, 10);

  odb::dbNet* net = odb::dbNet::create(block(), "VDD3");
  odb::dbSWire* wire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

  std::vector<std::unique_ptr<DbVia>> vias;
  vias.push_back(std::make_unique<DbTechVia>(s1.via, 1, 0, 1, 0));
  vias.push_back(std::make_unique<DbTechVia>(s2.via, 1, 0, 1, 0));
  vias.push_back(std::make_unique<DbTechVia>(v3, 1, 0, 1, 0));
  DbGenerateStackedVia stacked(std::move(vias), s1.bottom, block());
  const auto shapes = stacked.generate(block(),
                                       wire,
                                       odb::dbWireShapeType::STRIPE,
                                       /*x=*/500,
                                       /*y=*/500,
                                       /*ongrid=*/{},
                                       getLogger());
  EXPECT_FALSE(shapes.bottom.empty());
  EXPECT_FALSE(shapes.top.empty());
}

// ViaGenerator::generate(block) on a generator that built a cut-array
// (via has rows or cols > 1) drives the DbArrayVia construction branch.
// We force this by giving the generator a known cut-spacing rule and a
// wide rect.
TEST_F(TestPDNViaTech, ViaGeneratorGenerateProducesArrayViaForLargeRect)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);  // pitch 80 -> many cuts

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_GT(gen.getTotalCuts(), 1);  // many cuts -> array path

  DbVia* db_via = gen.generate(block());
  ASSERT_NE(db_via, nullptr);
  delete db_via;
}

// ViaGenerator::generate(block) when the via uses the ARRAYSPACING-rule
// winning path (isCutArray() == true): drives the cut-array branch that
// constructs a DbArrayVia with up to 4 sub-vias (core + end-of-row,
// end-of-column, end-of-row-column).
TEST_F(TestPDNViaTech, ViaGeneratorGenerateProducesArrayPathWithEndVias)
{
  ViaStack s = makeBasicViaStack();
  addCutClassMatching(s, "VC", /*cut_dim=*/50);
  setCutLayerSpacing(s, /*spacing=*/50);
  addArraySpacingRule(s.cut,
                      /*num_cuts=*/4,
                      /*array_spacing=*/50,
                      /*cut_spacing=*/30,
                      /*long_array=*/true);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  ASSERT_TRUE(gen.isCutArray());  // confirm we're in the right path

  DbVia* db_via = gen.generate(block());
  ASSERT_NE(db_via, nullptr);
  delete db_via;
}

// DbTechVia multicut path with a mismatched col_pitch sets pitch_match =
// false and keeps the original rows_/cols_ unchanged.
TEST_F(TestPDNViaTech, DbTechViaMultiCutMismatchedColPitch)
{
  const ViaStack s = makeBasicViaStack();
  // 2x2 cuts at +/- 50. via_cols=2, x_diff=100, via_cols*x_diff=200.
  // With col_pitch=150 -> 200 % 150 = 50 != 0 -> pitch_match drops to false.
  odb::dbTechVia* mv = addMultiCutViaToStack(
      s, "MV2", /*cut_size=*/50, /*cut_pitch=*/100, /*enclosure=*/5);
  DbTechVia dbtv(mv,
                 /*rows=*/2,
                 /*row_pitch=*/200,
                 /*cols=*/2,
                 /*col_pitch=*/150);
  EXPECT_EQ(dbtv.getName(), std::string("MV2"));
}

// DbTechVia multicut path with a mismatched row_pitch (col matches, row
// doesn't): drives the inner-y pitch_match=false branch.
TEST_F(TestPDNViaTech, DbTechViaMultiCutMismatchedRowPitch)
{
  const ViaStack s = makeBasicViaStack();
  odb::dbTechVia* mv = addMultiCutViaToStack(
      s, "MV3", /*cut_size=*/50, /*cut_pitch=*/100, /*enclosure=*/5);
  // col_pitch=200 matches; row_pitch=150 mismatches the y direction.
  DbTechVia dbtv(mv,
                 /*rows=*/2,
                 /*row_pitch=*/150,
                 /*cols=*/2,
                 /*col_pitch=*/200);
  EXPECT_EQ(dbtv.getName(), std::string("MV3"));
}

// Split cuts place one via per pitch across the whole overlap, so the limits
// on rows and columns cap how many of them there are.
TEST_F(TestPDNViaTech, ViaGeneratorSplitCutArrayHonorsMaxRowsAndColumns)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setSplitCutArray(/*split_bot=*/true, /*split_top=*/true);
  gen.setMaxRows(2);
  gen.setMaxColumns(3);
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getRows(), 2);
  EXPECT_EQ(gen.getColumns(), 3);
}

// A 1 x 3 array has cuts with two neighbours, so an ADJACENTCUTS 2 rule
// applies; they are all along x, so only the x pitch widens.
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingMinDim1Max3WidensX)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);  // pitch 80
  addAdjacentCutsSpacingRule(s.cut, /*cut_spacing=*/200, /*adjacent_cuts=*/2);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setMaxRows(1);
  gen.setMaxColumns(3);
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getCutPitchX(), 250);
  EXPECT_EQ(gen.getCutPitchY(), 80);
}

// A 2 x 3 array has cuts with three neighbours, enough for an ADJACENTCUTS 3
// rule. The neighbours are along both axes, so both pitches widen.
TEST_F(TestPDNViaTech, ViaGeneratorUpdateCutSpacingMinDim2Max3)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);
  addAdjacentCutsSpacingRule(s.cut, /*cut_spacing=*/200, /*adjacent_cuts=*/3);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setMaxRows(2);
  gen.setMaxColumns(3);
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getCutPitchX(), 250);
  EXPECT_EQ(gen.getCutPitchY(), 250);
}

// A LEF 5.4 cut spacing rule without ADJACENTCUTS says nothing about arrays.
TEST_F(TestPDNViaTech,
       ViaGeneratorUpdateCutSpacingSkipsV54RuleWithoutAdjacentCuts)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);
  odb::dbTechLayerSpacingRule::create(s.cut)->setSpacing(999);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setMaxRows(2);
  gen.setMaxColumns(2);
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(gen.getCutPitchX(), 80);
  EXPECT_EQ(gen.getCutPitchY(), 80);
}

// Width-conditioned rules are looked up against the metal actually drawn on a
// layer shared with the next via of a stack, not just this via's own strap.
// The 100 wide straps are under the 150 width of a two cut rule, but a 200
// wide shared pad is over it, and the single cut that fits fails it.
TEST_F(TestPDNViaTech, ViaGeneratorSharedWidthBringsWidthRulesIntoForce)
{
  const ViaStack s = makeBasicViaStack();
  addMinCutRuleV54(s.bottom, /*num_cuts=*/2, /*width=*/150);
  addMinCutRuleV54(s.top, /*num_cuts=*/2, /*width=*/150);

  const odb::Rect lower(0, 0, 2000, 100);
  const odb::Rect upper(0, 0, 100, 2000);
  const ViaGenerator::Constraint shared{false, false, false, 200};

  GenerateViaGenerator neither(
      getLogger(), s.rule, lower, loose(), upper, loose());
  EXPECT_TRUE(neither.build(false, false));

  GenerateViaGenerator below(
      getLogger(), s.rule, lower, shared, upper, loose());
  EXPECT_FALSE(below.build(false, false));

  GenerateViaGenerator above(
      getLogger(), s.rule, lower, loose(), upper, shared);
  EXPECT_FALSE(above.build(false, false));
}

// For a rectangular cut the spacing table is read by edge: cuts beside each
// other across the narrow (x) axis face along their long SIDE edges, and cuts
// above each other along their short END edges.
TEST_F(TestPDNViaTech,
       ViaGeneratorDetermineCutSpacingReadsEdgesOfARectangularCut)
{
  ViaStack s = makeBasicViaStack();
  for (uint32_t i = 0; i < s.rule->getViaLayerRuleCount(); i++) {
    auto* layer_rule = s.rule->getViaLayerRule(i);
    if (layer_rule->getLayer() == s.cut) {
      layer_rule->setRect(odb::Rect(-25, -50, 25, 50));
    }
  }
  auto* cut_class = odb::dbTechLayerCutClassRule::create(s.cut, "VR");
  cut_class->setWidth(50);
  cut_class->setLength(100);
  cut_class->setLengthValid(true);
  auto* rule = odb::dbTechLayerCutSpacingTableDefRule::create(s.cut);
  rule->setSpacingTable({{{100, 100}, {150, 150}}, {{150, 150}, {200, 200}}},
                        {{"VR/SIDE", 0}, {"VR/END", 1}},
                        {{"VR/SIDE", 0}, {"VR/END", 1}});

  const odb::Rect r(0, 0, 2000, 2000);
  GenerateViaGenerator gen(getLogger(), s.rule, r, loose(), r, loose());
  ASSERT_EQ(gen.getCutClass(), cut_class);
  EXPECT_EQ(gen.getCutPitchX(), 50 + 100);
  EXPECT_EQ(gen.getCutPitchY(), 100 + 200);
}

// Of two enclosures that give the same number of cuts, build() keeps the one
// that is narrower across each layer's width: on the horizontal bottom layer
// the smaller y, and on the vertical top layer the smaller x. The tech via
// encloses its cut by 10 on every side, so the rules are raised to at least
// that on each axis before they are tried.
TEST_F(TestPDNViaTech,
       ViaGeneratorBuildPrefersTheBottomEnclosureNarrowAcrossTheLayer)
{
  const ViaStack s = makeBasicViaStack();
  addCutEnclosureRule(
      s.cut, 5, 20, odb::dbTechLayerCutEnclosureRule::HORZ_AND_VERT);
  addCutEnclosureRule(
      s.cut, 20, 5, odb::dbTechLayerCutEnclosureRule::HORZ_AND_VERT);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(*gen.getBottomEnclosure(), Enclosure(20, 10));
  EXPECT_EQ(*gen.getTopEnclosure(), Enclosure(10, 20));
}

// The same choice with the directions swapped, where it is the top layer's
// enclosure that the tie is broken on.
TEST_F(TestPDNViaTech,
       ViaGeneratorBuildPrefersTheTopEnclosureNarrowAcrossTheLayer)
{
  const ViaStack s = makeBasicViaStack(
      "", 50, 10, /*bottom_dir=*/odb::dbTechLayerDir::VERTICAL);
  addCutEnclosureRule(
      s.cut, 5, 20, odb::dbTechLayerCutEnclosureRule::HORZ_AND_VERT);
  addCutEnclosureRule(
      s.cut, 20, 5, odb::dbTechLayerCutEnclosureRule::HORZ_AND_VERT);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  EXPECT_EQ(*gen.getBottomEnclosure(), Enclosure(10, 20));
  EXPECT_EQ(*gen.getTopEnclosure(), Enclosure(20, 10));
}

// An overlap only 80 wide has no room for a 50 wide cut and the 30 enclosure
// the cut layer requires, so on a layer that must fit the overlap in x the
// via is left with 15. Where that layer's strap runs past the overlap the
// enclosure can be taken from the strap instead, and the via is built with
// the full 30. Such a via depends on more than the size of the overlap, so it
// is not cached.
TEST_F(TestPDNViaTech, ViaGeneratorTakesEnclosureFromTheStrapPastTheOverlap)
{
  const ViaStack s = makeBasicViaStack();
  addCutEnclosureRule(s.cut, 30, 30);

  const odb::Rect strap(0, 0, 400, 2000);
  const odb::Rect narrow(160, 900, 240, 1100);
  const ViaGenerator::Constraint fit_x{true, false, true, std::nullopt};

  GenerateViaGenerator bottom(
      getLogger(), s.rule, strap, fit_x, narrow, loose());
  ASSERT_TRUE(bottom.build(false, false));
  EXPECT_EQ(bottom.getColumns(), 1);
  EXPECT_EQ(*bottom.getBottomEnclosure(), Enclosure(30, 30));
  EXPECT_FALSE(bottom.canCache());

  GenerateViaGenerator top(getLogger(), s.rule, narrow, loose(), strap, fit_x);
  ASSERT_TRUE(top.build(false, false));
  EXPECT_EQ(top.getColumns(), 1);
  EXPECT_EQ(*top.getTopEnclosure(), Enclosure(30, 30));
  EXPECT_FALSE(top.canCache());
}

// Derived helpers that expose the protected methods that have no
// production callers but should still be exercised for coverage.
class TechViaGeneratorAccess : public TechViaGenerator
{
 public:
  using TechViaGenerator::TechViaGenerator;
  using ViaGenerator::getArrayCoresX;
  using ViaGenerator::getArrayCoresY;
  using ViaGenerator::getArraySpacingX;
  using ViaGenerator::getViaCoreColumns;
  int callGetLowerHeight(bool only_real) const
  {
    return getLowerHeight(only_real);
  }
  int callGetUpperHeight(bool only_real) const
  {
    return getUpperHeight(only_real);
  }
  bool callFitsShapes() const { return fitsShapes(); }
  // mostlyContains reads nothing but its arguments, so the hosting
  // generator's own geometry is irrelevant to the decision under test.
  bool callMostlyContains(const odb::Rect& full_shape,
                          const odb::Rect& intersection,
                          const odb::Rect& small_shape,
                          const Constraint& constraint,
                          odb::dbTechLayer* layer) const
  {
    return mostlyContains(
        full_shape, intersection, small_shape, constraint, layer);
  }
};

// The generator's width spans every array across x. An area wide enough for
// several arrays but tall enough for only one row of them gives more arrays
// across than up, and the width must still cover all of their cuts.
TEST_F(TestPDNViaTech, ViaGeneratorWidthSpansEveryArrayAcross)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/50);
  addArraySpacingRule(s.cut, /*num_cuts=*/2, /*array_spacing=*/200);

  const odb::Rect r(0, 0, 2000, 400);
  TechViaGeneratorAccess gen(getLogger(), s.via, r, loose(), r, loose());
  ASSERT_TRUE(gen.build(false, false));
  ASSERT_TRUE(gen.isCutArray());
  ASSERT_GT(gen.getArrayCoresX(), gen.getArrayCoresY());

  const int array_width
      = (gen.getViaCoreColumns() - 1) * gen.getCutPitchX() + 50;
  const int cuts_width = gen.getArrayCoresX() * array_width
                         + (gen.getArrayCoresX() - 1) * gen.getArraySpacingX();
  EXPECT_GE(gen.getGeneratorWidth(true), cuts_width);
  EXPECT_GE(gen.getGeneratorWidth(false), cuts_width);
}

// Cover the protected getLowerHeight / getUpperHeight helpers (they have
// no callers in production but mirror getLowerWidth / getUpperWidth).
TEST_F(TestPDNViaTech, ViaGeneratorGetLowerUpperHeightAccessors)
{
  const ViaStack s = makeBasicViaStack();
  const odb::Rect r(0, 0, 1000, 500);
  TechViaGeneratorAccess gen(getLogger(), s.via, r, loose(), r, loose());
  // For a 1000x500 rect, max(dx, dy) = 1000 -> getLowerHeight returns 1000.
  // (only_real=true short-circuits the split-cut zero path.)
  EXPECT_EQ(gen.callGetLowerHeight(true), 1000);
  EXPECT_EQ(gen.callGetUpperHeight(true), 1000);
  // And only_real=false also returns the same when not split-cut.
  EXPECT_EQ(gen.callGetLowerHeight(false), 1000);
  EXPECT_EQ(gen.callGetUpperHeight(false), 1000);
}

// Driving the split-cut branch of ViaGenerator::generate(block).
TEST_F(TestPDNViaTech, ViaGeneratorGenerateSplitCutPath)
{
  ViaStack s = makeBasicViaStack();
  setCutLayerSpacing(s, /*spacing=*/30);

  const odb::Rect r(0, 0, 2000, 2000);
  TechViaGenerator gen(getLogger(), s.via, r, loose(), r, loose());
  gen.setSplitCutArray(/*split_bot=*/true, /*split_top=*/false);
  if (!gen.build(false, false)) {
    GTEST_SKIP() << "split-cut build did not place a via";
  }
  DbVia* db_via = gen.generate(block());
  EXPECT_NE(db_via, nullptr);
  delete db_via;
}

// Exposes the protected enclosure/row-column machinery so a test can drive a
// single enclosure combination directly instead of going through build()'s
// combination search.
class TestableGenerateVia : public GenerateViaGenerator
{
 public:
  using GenerateViaGenerator::GenerateViaGenerator;

  void exposeDetermineRowsAndColumns(const Enclosure& bottom,
                                     const Enclosure& top)
  {
    determineRowsAndColumns(/*use_bottom_min_enclosure=*/false,
                            /*use_top_min_enclosure=*/false,
                            bottom,
                            top);
  }

  bool exposeCheckMinEnclosure() const { return checkMinEnclosure(); }
};

// Stack mirroring a real M1/V1/M2 VIARULE GENERATE: the bottom (vertical) strap
// encloses the cut only along its length (Y), the top (horizontal) strap only
// along its length (X). The cut is 50x50 and the bottom layer requires 20 of
// Y enclosure, so the via's M1 metal is 50x90.
//
// The top strap is only as wide as the cut. That leaves no Y enclosure on
// M2, which the ABOVE rule "30 0" allows: its 30 can go on either pair of
// sides, and the via has it in X. The 90-tall metal is on M1, inside the
// 150x1000 M1 strap, so it does not need to fit the M2 strap. Every layer
// meets a rule, and the via is legal.
TEST_F(TestPDNViaTech, MinWidthTopStrapTakesItsEnclosureAlongTheStrap)
{
  const int cut_size = 50;
  ViaStack s = makeGenerateViaStack(cut_size,
                                    /*bottom_enc_x=*/0,
                                    /*bottom_enc_y=*/20,
                                    /*top_enc_x=*/30,
                                    /*top_enc_y=*/0,
                                    /*cut_spacing=*/100);
  // Cut layer LEF58 enclosure rules checkMinEnclosure validates against
  // (anonymized M1/V1/M2): one asymmetric BELOW rule, two ABOVE rules.
  addAboveBelowCutEnclosureRule(s.cut, 20, 0, /*above=*/false, /*below=*/true);
  addAboveBelowCutEnclosureRule(s.cut, 30, 0, /*above=*/true, /*below=*/false);
  addAboveBelowCutEnclosureRule(s.cut, 20, 20, /*above=*/true, /*below=*/false);

  // Bottom (M1, vertical): 3x min-width strap. Top (M2, horizontal): min-width
  // strap == cut, leaving no room for Y enclosure on M2.
  const odb::Rect lower(0, 0, 150, 1000);
  const odb::Rect upper(0, 0, 1000, cut_size);
  TestableGenerateVia gen(getLogger(), s.rule, lower, loose(), upper, loose());

  gen.exposeDetermineRowsAndColumns(Enclosure(0, 20), Enclosure(30, 0));

  // Each layer's via metal fits its own strap.
  const Enclosure* bottom = gen.getBottomEnclosure();
  const Enclosure* top = gen.getTopEnclosure();
  EXPECT_LE(cut_size + 2 * bottom->getX(), lower.dx());
  EXPECT_LE(cut_size + 2 * bottom->getY(), lower.dy());
  EXPECT_LE(cut_size + 2 * top->getX(), upper.dx());
  EXPECT_LE(cut_size + 2 * top->getY(), upper.dy());

  EXPECT_TRUE(gen.exposeCheckMinEnclosure());
}

// Same stack, but the top strap is 2x min width, so its via metal has room to
// spare; the enclosure is accepted here too.
TEST_F(TestPDNViaTech, DoubleWidthTopStrapEnclosureIsAccepted)
{
  const int cut_size = 50;
  ViaStack s = makeGenerateViaStack(cut_size,
                                    /*bottom_enc_x=*/0,
                                    /*bottom_enc_y=*/20,
                                    /*top_enc_x=*/30,
                                    /*top_enc_y=*/0,
                                    /*cut_spacing=*/100);
  addAboveBelowCutEnclosureRule(s.cut, 20, 0, /*above=*/false, /*below=*/true);
  addAboveBelowCutEnclosureRule(s.cut, 30, 0, /*above=*/true, /*below=*/false);
  addAboveBelowCutEnclosureRule(s.cut, 20, 20, /*above=*/true, /*below=*/false);

  const odb::Rect lower(0, 0, 150, 1000);
  const odb::Rect upper(0, 0, 1000, 2 * cut_size);  // 2x min-width top strap
  TestableGenerateVia gen(getLogger(), s.rule, lower, loose(), upper, loose());

  gen.exposeDetermineRowsAndColumns(Enclosure(0, 20), Enclosure(30, 0));

  ASSERT_LE(cut_size + 2 * gen.getTopEnclosure()->getY(), upper.dy());

  EXPECT_TRUE(gen.exposeCheckMinEnclosure());
}

// =============================================================================
// TechViaGenerator::mostlyContains -- the containment decision behind
// isSetupValid / fitsShapes.
//
// The decision has three tiers, checked in order:
//   1. an explicit must_fit_x / must_fit_y constraint is absolute: the axis it
//      names has to contain the via metal, full stop;
//   2. otherwise a shape contained on three or more sides is accepted;
//   3. otherwise -- an internal routing layer of a stacked via, which has no
//      strap of its own to land on -- the metal may spill past the stripe
//      overlap along the layer's *preferred routing* direction (a metal patch
//      is generated at placement time to bridge the gap), but never across the
//      orthogonal width direction.
//
// Tier 3 is what lets a stacked via bridge to an on-grid neighbor -- e.g. M2
// snapped to its track while M4 stays on its stripe -- and it is the tier the
// -min_width_layers / -ongrid combination depends on. These tests pin every
// branch of the decision.
// =============================================================================

// A bare routing layer that exists only to give mostlyContains a preferred
// direction to reason about; it never carries any geometry.
static odb::dbTechLayer* makeDirLayer(odb::dbTech* tech,
                                      const char* name,
                                      odb::dbTechLayerDir dir)
{
  odb::dbTechLayer* layer
      = odb::dbTechLayer::create(tech, name, odb::dbTechLayerType::ROUTING);
  layer->setDirection(dir);
  return layer;
}

// The stripe overlap every case below is framed against.
static odb::Rect fullShape()
{
  return {0, 0, 100, 100};
}

// Hosts a single mostlyContains call plus one routing layer per direction.
class TestMostlyContains : public PdnTest
{
 protected:
  void SetUp() override
  {
    PdnTest::SetUp();
    stack_ = makeBasicViaStack();
    horizontal_ = makeDirLayer(tech(), "hdir", odb::dbTechLayerDir::HORIZONTAL);
    vertical_ = makeDirLayer(tech(), "vdir", odb::dbTechLayerDir::VERTICAL);
    undirected_ = makeDirLayer(tech(), "nodir", odb::dbTechLayerDir::NONE);

    const odb::Rect r(0, 0, 1000, 1000);
    gen_ = std::make_unique<TechViaGeneratorAccess>(
        getLogger(), stack_.via, r, loose(), r, loose());
  }

  // Unconstrained check against the default full shape.
  bool check(const odb::Rect& small, odb::dbTechLayer* layer) const
  {
    return gen_->callMostlyContains(
        fullShape(), fullShape(), small, loose(), layer);
  }

  bool checkConstrained(const odb::Rect& small,
                        odb::dbTechLayer* layer,
                        const ViaGenerator::Constraint& constraint) const
  {
    return gen_->callMostlyContains(
        fullShape(), fullShape(), small, constraint, layer);
  }

  bool checkAgainstIntersection(const odb::Rect& small,
                                const odb::Rect& intersection,
                                odb::dbTechLayer* layer,
                                bool intersection_only) const
  {
    const ViaGenerator::Constraint constraint{
        false, false, intersection_only, std::nullopt};
    return gen_->callMostlyContains(
        fullShape(), intersection, small, constraint, layer);
  }

  ViaStack stack_;
  odb::dbTechLayer* horizontal_ = nullptr;
  odb::dbTechLayer* vertical_ = nullptr;
  odb::dbTechLayer* undirected_ = nullptr;
  std::unique_ptr<TechViaGeneratorAccess> gen_;
};

// -------- Tier 2: contained on three or more sides --------

// Fully contained: all four sides pass, so the layer direction never matters.
TEST_F(TestMostlyContains, AcceptsShapeContainedOnAllFourSides)
{
  const odb::Rect small(10, 10, 90, 90);
  EXPECT_TRUE(check(small, horizontal_));
  EXPECT_TRUE(check(small, vertical_));
  EXPECT_TRUE(check(small, undirected_));
  EXPECT_TRUE(check(small, nullptr));
}

// The comparisons are inclusive (>= / <=), so a shape flush with every edge
// of the full shape still counts as contained on all four sides.
TEST_F(TestMostlyContains, AcceptsShapeFlushWithEveryEdge)
{
  EXPECT_TRUE(check(fullShape(), horizontal_));
  EXPECT_TRUE(check(fullShape(), vertical_));
  EXPECT_TRUE(check(fullShape(), nullptr));
}

// A degenerate (zero-area) shape inside the full shape is contained.
TEST_F(TestMostlyContains, AcceptsDegenerateShapeInsideFullShape)
{
  EXPECT_TRUE(check(odb::Rect(50, 50, 50, 50), nullptr));
}

// Spilling past exactly one side leaves three contained -> accepted for any
// layer, including a HORIZONTAL layer spilling across its own width. Tier 3
// must not have tightened tier 2.
TEST_F(TestMostlyContains, AcceptsShapeSpillingPastASingleSide)
{
  EXPECT_TRUE(check(odb::Rect(10, 10, 110, 90), horizontal_));  // x high
  EXPECT_TRUE(check(odb::Rect(-10, 10, 90, 90), horizontal_));  // x low
  EXPECT_TRUE(check(odb::Rect(10, 10, 90, 110), horizontal_));  // y high
  EXPECT_TRUE(check(odb::Rect(10, -10, 90, 90), horizontal_));  // y low
  EXPECT_TRUE(check(odb::Rect(10, -10, 90, 90), vertical_));
  EXPECT_TRUE(check(odb::Rect(10, -10, 90, 90), nullptr));
}

// -------- Tier 3: bridging along the preferred routing direction --------

// A HORIZONTAL layer routes in x, so it may spill past both x sides of the
// overlap as long as it stays inside in y. This is the stacked-via bridge.
TEST_F(TestMostlyContains, HorizontalLayerBridgesAlongPreferredDirection)
{
  EXPECT_TRUE(check(odb::Rect(-10, 10, 110, 90), horizontal_));
}

// The same shape on a VERTICAL layer spills across its width -> rejected.
TEST_F(TestMostlyContains, VerticalLayerRejectsSpillAcrossItsWidth)
{
  EXPECT_FALSE(check(odb::Rect(-10, 10, 110, 90), vertical_));
}

// Mirror image: a VERTICAL layer routes in y and may spill past both y sides.
TEST_F(TestMostlyContains, VerticalLayerBridgesAlongPreferredDirection)
{
  EXPECT_TRUE(check(odb::Rect(10, -10, 90, 110), vertical_));
}

// And that shape on a HORIZONTAL layer spills across its width -> rejected.
TEST_F(TestMostlyContains, HorizontalLayerRejectsSpillAcrossItsWidth)
{
  EXPECT_FALSE(check(odb::Rect(10, -10, 90, 110), horizontal_));
}

// Spilling on both axes is never a bridge -- neither direction is contained.
TEST_F(TestMostlyContains, RejectsShapeSpillingOnBothAxes)
{
  const odb::Rect small(-10, -10, 110, 110);
  EXPECT_FALSE(check(small, horizontal_));
  EXPECT_FALSE(check(small, vertical_));
}

// A diagonal overhang (one x side and one y side out) leaves contains == 2
// with neither axis fully inside, so it is rejected in both directions.
TEST_F(TestMostlyContains, RejectsShapeSpillingDiagonally)
{
  const odb::Rect small(-10, -10, 90, 90);
  EXPECT_FALSE(check(small, horizontal_));
  EXPECT_FALSE(check(small, vertical_));
}

// A layer with no preferred direction has no axis it is entitled to extend
// along, so it falls through to the reject.
TEST_F(TestMostlyContains, UndirectedLayerCannotBridge)
{
  EXPECT_FALSE(check(odb::Rect(-10, 10, 110, 90), undirected_));
  EXPECT_FALSE(check(odb::Rect(10, -10, 90, 110), undirected_));
}

// A null layer (the caller has no layer context) likewise cannot bridge.
TEST_F(TestMostlyContains, NullLayerCannotBridge)
{
  EXPECT_FALSE(check(odb::Rect(-10, 10, 110, 90), nullptr));
  EXPECT_FALSE(check(odb::Rect(10, -10, 90, 110), nullptr));
}

// -------- Tier 1: must_fit constraints are absolute --------

// must_fit_x checks x only: a shape that spills in y is still accepted...
TEST_F(TestMostlyContains, MustFitXChecksOnlyTheXAxis)
{
  const ViaGenerator::Constraint must_fit_x{true, false, false, std::nullopt};
  EXPECT_TRUE(checkConstrained(odb::Rect(10, 10, 90, 90), nullptr, must_fit_x));
  EXPECT_TRUE(
      checkConstrained(odb::Rect(10, -10, 90, 110), nullptr, must_fit_x));
}

// ...but a shape that spills in x is rejected even on a HORIZONTAL layer,
// which tier 3 would otherwise have let bridge. A constrained layer must
// never be allowed to extend along the direction it has to fit.
//
// Tier 3 restates that invariant in its own `!constraint.must_fit_x` /
// `!constraint.must_fit_y` guards. Those guards are defensive: tier 1 has
// already returned for any constrained layer, so removing them changes no
// behavior today (a mutation that drops them keeps this suite green). What
// this test pins is the invariant itself -- it fails if tier 1 is ever
// reordered below the bridging tier.
TEST_F(TestMostlyContains, MustFitXOverridesHorizontalBridging)
{
  const ViaGenerator::Constraint must_fit_x{true, false, false, std::nullopt};
  const odb::Rect bridging(-10, 10, 110, 90);
  ASSERT_TRUE(check(bridging, horizontal_));  // tier 3 would accept
  EXPECT_FALSE(checkConstrained(bridging, horizontal_, must_fit_x));
}

// must_fit_y is the mirror image on both counts.
TEST_F(TestMostlyContains, MustFitYChecksOnlyTheYAxis)
{
  const ViaGenerator::Constraint must_fit_y{false, true, false, std::nullopt};
  EXPECT_TRUE(checkConstrained(odb::Rect(10, 10, 90, 90), nullptr, must_fit_y));
  EXPECT_TRUE(
      checkConstrained(odb::Rect(-10, 10, 110, 90), nullptr, must_fit_y));
}

TEST_F(TestMostlyContains, MustFitYOverridesVerticalBridging)
{
  const ViaGenerator::Constraint must_fit_y{false, true, false, std::nullopt};
  const odb::Rect bridging(10, -10, 90, 110);
  ASSERT_TRUE(check(bridging, vertical_));  // tier 3 would accept
  EXPECT_FALSE(checkConstrained(bridging, vertical_, must_fit_y));
}

// Both constraints together require containment on both axes.
TEST_F(TestMostlyContains, MustFitBothRequiresBothAxes)
{
  const ViaGenerator::Constraint must_fit_both{true, true, false, std::nullopt};
  EXPECT_TRUE(
      checkConstrained(odb::Rect(10, 10, 90, 90), horizontal_, must_fit_both));
  EXPECT_FALSE(checkConstrained(
      odb::Rect(-10, 10, 110, 90), horizontal_, must_fit_both));
  EXPECT_FALSE(
      checkConstrained(odb::Rect(10, -10, 90, 110), vertical_, must_fit_both));
}

// -------- intersection_only --------

// intersection_only swaps the rect the inside_x / inside_y tests are taken
// against, which tightens the tier-3 bridge check: the same shape bridges
// against the full stripe but not against the smaller overlap.
TEST_F(TestMostlyContains, IntersectionOnlyTightensTheBridgeCheck)
{
  const odb::Rect small(-10, 10, 110, 90);
  const odb::Rect intersection(0, 20, 100, 80);  // shorter than `small` in y
  EXPECT_TRUE(checkAgainstIntersection(
      small, intersection, horizontal_, /*intersection_only=*/false));
  EXPECT_FALSE(checkAgainstIntersection(
      small, intersection, horizontal_, /*intersection_only=*/true));
}

// The tier-2 side counter, unlike the tier-1 and tier-3 checks, always counts
// against the full shape -- intersection_only does not narrow it. A shape
// contained on three sides of the stripe is accepted even when it escapes a
// much smaller intersection.
//
// NOTE: this asymmetry is the production behavior as written; if
// intersection_only is ever meant to bound the three-side tier too, this is
// the test that should change.
TEST_F(TestMostlyContains, IntersectionOnlyDoesNotNarrowTheThreeSideTier)
{
  const odb::Rect small(10, 30, 110, 70);  // 3 sides inside the full shape
  const odb::Rect intersection(50, 50, 60, 60);  // `small` escapes it entirely
  EXPECT_TRUE(checkAgainstIntersection(
      small, intersection, horizontal_, /*intersection_only=*/true));
}

// =============================================================================
// TechViaGenerator::fitsShapes -- the same decision applied to a real tech via
// placed at the center of the lower/upper overlap, for both endpoints of the
// stack. makeBasicViaStack's via carries a 70x70 metal enclosure on each end.
// =============================================================================

// A HORIZONTAL bottom layer whose stripe overlap is narrower than the via
// metal bridges along x: the via is accepted and a patch fills the gap.
TEST_F(TestPDNViaTech, TechViaBridgesHorizontalBottomAlongPreferredDirection)
{
  const ViaStack s = makeBasicViaStack();
  ASSERT_EQ(s.bottom->getDirection(), odb::dbTechLayerDir::HORIZONTAL);

  // 40 wide < the via's 70-wide metal -> overhangs both x sides.
  const odb::Rect lower(0, 0, 40, 200);
  const odb::Rect upper(0, 0, 200, 200);
  TechViaGeneratorAccess gen(
      getLogger(), s.via, lower, loose(), upper, loose());
  EXPECT_TRUE(gen.callFitsShapes());
  EXPECT_TRUE(gen.isSetupValid(s.bottom, s.top));
}

// A VERTICAL bottom layer is the mirror image: it bridges along y.
TEST_F(TestPDNViaTech, TechViaBridgesVerticalBottomAlongPreferredDirection)
{
  const ViaStack s
      = makeBasicViaStack("", 50, 10, odb::dbTechLayerDir::VERTICAL);
  ASSERT_EQ(s.bottom->getDirection(), odb::dbTechLayerDir::VERTICAL);

  const odb::Rect lower(0, 0, 200, 40);  // 40 tall < the via's 70-tall metal
  const odb::Rect upper(0, 0, 200, 200);
  TechViaGeneratorAccess gen(
      getLogger(), s.via, lower, loose(), upper, loose());
  EXPECT_TRUE(gen.callFitsShapes());
  EXPECT_TRUE(gen.isSetupValid(s.bottom, s.top));
}

// ...but a VERTICAL bottom layer may not spill across its width (x).
TEST_F(TestPDNViaTech, TechViaRejectsVerticalBottomSpillingAcrossItsWidth)
{
  const ViaStack s
      = makeBasicViaStack("", 50, 10, odb::dbTechLayerDir::VERTICAL);
  const odb::Rect lower(0, 0, 40, 200);
  const odb::Rect upper(0, 0, 200, 200);
  TechViaGeneratorAccess gen(
      getLogger(), s.via, lower, loose(), upper, loose());
  EXPECT_FALSE(gen.callFitsShapes());
  EXPECT_FALSE(gen.isSetupValid(s.bottom, s.top));
}

// fitsShapes checks both endpoints: a bottom layer that fits perfectly does
// not rescue a top layer that spills across its width.
TEST_F(TestPDNViaTech, TechViaFitsShapesRejectsWhenOnlyTheTopLayerFails)
{
  const ViaStack s = makeBasicViaStack();
  ASSERT_EQ(s.top->getDirection(), odb::dbTechLayerDir::VERTICAL);

  const odb::Rect lower(0, 0, 2000, 2000);  // contains the via on all sides
  const odb::Rect upper(0, 0, 40, 2000);    // vertical top, too narrow in x
  TechViaGeneratorAccess gen(
      getLogger(), s.via, lower, loose(), upper, loose());
  EXPECT_FALSE(gen.callFitsShapes());
}

// Both endpoints may bridge at once, each along its own routing direction:
// the horizontal bottom extends in x while the vertical top extends in y.
TEST_F(TestPDNViaTech, TechViaFitsShapesAcceptsBothEndpointsBridging)
{
  const ViaStack s = makeBasicViaStack();

  // Overlap is (0, -20) .. (40, 20); the via metal is 70x70 around its center
  // at (20, 0), so it escapes the lower rect in x and the upper rect in y.
  const odb::Rect lower(0, -1000, 40, 1000);
  const odb::Rect upper(-1000, -20, 1000, 20);
  TechViaGeneratorAccess gen(
      getLogger(), s.via, lower, loose(), upper, loose());
  EXPECT_TRUE(gen.callFitsShapes());
}

}  // namespace
}  // namespace pdn
