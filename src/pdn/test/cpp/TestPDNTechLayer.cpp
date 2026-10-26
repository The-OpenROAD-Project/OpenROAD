// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <optional>
#include <vector>

#include "PdnTest.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "techlayer.h"
#include "tst/physical_builder.h"

namespace pdn {
namespace {

using TestTechLayer = PdnTest;

// Helper: create a routing layer with a given direction and width.
static odb::dbTechLayer* makeMetal(odb::dbTech* tech,
                                   const char* name,
                                   odb::dbTechLayerDir dir,
                                   int width = 70)
{
  return tst::addRoutingLayer(tech, name, {.direction = dir, .width = width});
}

// -------- Construction + simple getters --------

// Constructor stores the layer pointer and exposes it via getLayer().
TEST_F(TestTechLayer, ConstructorExposesUnderlyingLayer)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL, /*width=*/100);
  TechLayer tl(layer);
  EXPECT_EQ(tl.getLayer(), layer);
  EXPECT_EQ(tl.getName(), "m1");
}

// getMinWidth / getMaxWidth round-trip from the underlying dbTechLayer.
TEST_F(TestTechLayer, MinMaxWidthForwardToDbLayer)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  layer->setMinWidth(40);
  layer->setMaxWidth(200);
  TechLayer tl(layer);
  EXPECT_EQ(tl.getMinWidth(), 40);
  EXPECT_EQ(tl.getMaxWidth(), 200);
}

// dbuToMicron divides by the tech's LEF units.
TEST_F(TestTechLayer, DbuToMicronUsesTechUnits)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  // Fixture sets LefUnits=1000, so 2500 dbu = 2.5 microns.
  EXPECT_DOUBLE_EQ(tl.dbuToMicron(2500), 2.5);
}

// getLefUnits forwards to the tech.
TEST_F(TestTechLayer, GetLefUnitsForwardsToTech)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  EXPECT_EQ(tl.getLefUnits(), tech()->getDbUnitsPerMicron());
}

// -------- snapToManufacturingGrid --------

// Static snapToManufacturingGrid: no grid set on the tech -> no-op.
TEST_F(TestTechLayer, StaticSnapNoGridIsNoOp)
{
  tech()->setManufacturingGrid(0);  // clear the fixture's grid
  ASSERT_FALSE(tech()->hasManufacturingGrid());
  EXPECT_EQ(TechLayer::snapToManufacturingGrid(tech(), 17), 17);
}

// Static snapToManufacturingGrid: rounds down by default.
TEST_F(TestTechLayer, StaticSnapRoundsDownByDefault)
{
  tech()->setManufacturingGrid(10);
  EXPECT_EQ(TechLayer::snapToManufacturingGrid(tech(), 13), 10);
  EXPECT_EQ(TechLayer::snapToManufacturingGrid(tech(), 20), 20);  // on-grid
}

// Static snapToManufacturingGrid with round_up=true rounds up.
TEST_F(TestTechLayer, StaticSnapRoundsUpWhenRequested)
{
  tech()->setManufacturingGrid(10);
  EXPECT_EQ(TechLayer::snapToManufacturingGrid(tech(),
                                               13,
                                               /*round_up=*/true),
            20);
  EXPECT_EQ(TechLayer::snapToManufacturingGrid(tech(),
                                               20,
                                               /*round_up=*/true),
            20);  // already on-grid
}

// grid_multiplier scales the effective grid: multiplier=3, grid=10 -> snap to
// multiples of 30.
TEST_F(TestTechLayer, StaticSnapHonorsGridMultiplier)
{
  tech()->setManufacturingGrid(10);
  EXPECT_EQ(TechLayer::snapToManufacturingGrid(tech(), 35, false, 3), 30);
  EXPECT_EQ(TechLayer::snapToManufacturingGrid(tech(), 60, false, 3), 60);
}

// Instance snapToManufacturingGrid delegates to the static version using
// the layer's tech.
TEST_F(TestTechLayer, InstanceSnapDelegatesToStatic)
{
  tech()->setManufacturingGrid(10);
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  EXPECT_EQ(tl.snapToManufacturingGrid(27), 20);
}

// -------- checkIfManufacturingGrid --------

// Static check: no grid means everything passes.
TEST_F(TestTechLayer, StaticCheckNoGridIsAlwaysTrue)
{
  tech()->setManufacturingGrid(0);
  EXPECT_TRUE(TechLayer::checkIfManufacturingGrid(tech(), 17));
}

// Static check: only multiples pass.
TEST_F(TestTechLayer, StaticCheckOnlyMultiplesPass)
{
  tech()->setManufacturingGrid(5);
  EXPECT_TRUE(TechLayer::checkIfManufacturingGrid(tech(), 0));
  EXPECT_TRUE(TechLayer::checkIfManufacturingGrid(tech(), 10));
  EXPECT_FALSE(TechLayer::checkIfManufacturingGrid(tech(), 11));
}

// getMinIncrementStep returns the manufacturing grid when one is set.
TEST_F(TestTechLayer, GetMinIncrementStepReturnsGrid)
{
  tech()->setManufacturingGrid(7);
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  EXPECT_EQ(tl.getMinIncrementStep(), 7);
}

// getMinIncrementStep returns 1 when no manufacturing grid is set.
TEST_F(TestTechLayer, GetMinIncrementStepReturnsOneWithoutGrid)
{
  tech()->setManufacturingGrid(0);
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  EXPECT_EQ(tl.getMinIncrementStep(), 1);
}

// -------- snapToGrid --------

// snapToGrid on an empty grid returns the position unchanged.
TEST_F(TestTechLayer, SnapToGridEmptyGridReturnsPositionUnchanged)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  // No populateGrid call -> grid_ is empty.
  EXPECT_FALSE(tl.hasGrid());
  EXPECT_EQ(tl.snapToGrid(123), 123);
}

// After populateGrid with a horizontal layer + Y track grid, snapToGrid
// picks the closest grid line.
TEST_F(TestTechLayer, SnapToGridSelectsClosestGridLine)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), layer);
  // y tracks at 0, 100, 200, 300, 400.
  tracks->addGridPatternY(/*origin=*/0, /*line_count=*/5, /*step=*/100);
  TechLayer tl(layer);
  tl.populateGrid(block());
  ASSERT_TRUE(tl.hasGrid());
  EXPECT_EQ(tl.snapToGrid(110), 100);  // 110 -> 100
  EXPECT_EQ(tl.snapToGrid(160), 200);  // 160 -> 200 (closer)
  EXPECT_EQ(tl.snapToGrid(200), 200);  // exact
}

// snapToGrid honors a greater_than threshold by skipping grid lines below
// it.
TEST_F(TestTechLayer, SnapToGridGreaterThanSkipsLowerLines)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), layer);
  tracks->addGridPatternY(0, 5, 100);
  TechLayer tl(layer);
  tl.populateGrid(block());
  // greater_than=150 excludes 0, 100 -> closest is 200.
  EXPECT_EQ(tl.snapToGrid(110, /*greater_than=*/150), 200);
}

// Explicit VERTICAL direction reads the X grid even if the layer's direction
// is horizontal.
TEST_F(TestTechLayer, PopulateGridVerticalReadsXGrid)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), layer);
  tracks->addGridPatternX(0, 4, 50);  // x: 0, 50, 100, 150
  TechLayer tl(layer);
  tl.populateGrid(block(), odb::dbTechLayerDir::VERTICAL);
  EXPECT_EQ(tl.snapToGrid(55), 50);
  EXPECT_EQ(tl.getGrid(), (std::vector<int>{0, 50, 100, 150}));
}

// populateGrid on a layer with no track grid is a no-op (grid stays empty).
TEST_F(TestTechLayer, PopulateGridWithNoTracksIsNoOp)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  tl.populateGrid(block());
  EXPECT_FALSE(tl.hasGrid());
}

// populateGrid with dir == NONE on a layer whose own direction is also
// NONE falls through to the final `else` branch (defaults to Y grid).
TEST_F(TestTechLayer, PopulateGridNoneDirectionFallsThroughToYGrid)
{
  odb::dbTechLayer* layer
      = odb::dbTechLayer::create(tech(), "any", odb::dbTechLayerType::ROUTING);
  layer->setDirection(odb::dbTechLayerDir::NONE);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), layer);
  tracks->addGridPatternY(0, 4, 80);
  TechLayer tl(layer);
  tl.populateGrid(block(), odb::dbTechLayerDir::NONE);
  ASSERT_TRUE(tl.hasGrid());
  // The fallback selected the Y grid; the first entry should be 0.
  EXPECT_EQ(tl.snapToGrid(5), 0);
}

// snapToGrid with every grid line below greater_than returns the original
// position (the `delta_pos has no value` branch).
TEST_F(TestTechLayer, SnapToGridReturnsPositionWhenAllGridBelowGreaterThan)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), layer);
  tracks->addGridPatternY(0, 2, 50);  // grid = {0, 50}
  TechLayer tl(layer);
  tl.populateGrid(block());
  // greater_than larger than every grid line -> nothing qualifies.
  EXPECT_EQ(tl.snapToGrid(40, /*greater_than=*/100), 40);
}

// -------- snapToGridInterval --------

// With no track grid, snapToGridInterval returns the distance unchanged.
TEST_F(TestTechLayer, SnapToGridIntervalNoTracksReturnsDist)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  EXPECT_EQ(tl.snapToGridInterval(block(), 73), 73);
}

// With a horizontal layer and Y tracks at step=100, asking for 250 rounds
// down to a multiple of step -> 2 * 100 = 200.
TEST_F(TestTechLayer, SnapToGridIntervalRoundsToStep)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), layer);
  tracks->addGridPatternY(0, 5, 100);
  TechLayer tl(layer);
  EXPECT_EQ(tl.snapToGridInterval(block(), 250), 200);
}

// Vertical layer reads X track step.
TEST_F(TestTechLayer, SnapToGridIntervalVerticalReadsXStep)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::VERTICAL);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), layer);
  tracks->addGridPatternX(0, 5, 60);
  TechLayer tl(layer);
  EXPECT_EQ(tl.snapToGridInterval(block(), 200), 180);  // 3 * 60
}

// snapToGridInterval skips track grids that belong to OTHER layers. With
// only an "other layer" track grid present, the result falls back to the
// input distance (num/step still 0).
TEST_F(TestTechLayer, SnapToGridIntervalSkipsOtherLayerTracks)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechLayer* other
      = makeMetal(tech(), "m2", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), other);
  tracks->addGridPatternY(0, 5, 100);
  TechLayer tl(layer);
  // The only track grid in the block belongs to a different layer -> the
  // loop skips it and `num == 0` makes snapToGridInterval return `dist`.
  EXPECT_EQ(tl.snapToGridInterval(block(), 73), 73);
}

// snapToGridInterval skips its own track grid when the relevant direction
// has no pattern: a HORIZONTAL layer with only X patterns falls back to dist.
TEST_F(TestTechLayer, SnapToGridIntervalHorizontalWithoutYPatternIsNoOp)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), layer);
  tracks->addGridPatternX(0, 5, 100);  // x pattern only, no y
  TechLayer tl(layer);
  EXPECT_EQ(tl.snapToGridInterval(block(), 73), 73);
}

// And symmetric: VERTICAL layer with only Y patterns falls back to dist.
TEST_F(TestTechLayer, SnapToGridIntervalVerticalWithoutXPatternIsNoOp)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::VERTICAL);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), layer);
  tracks->addGridPatternY(0, 5, 100);  // y pattern only, no x
  TechLayer tl(layer);
  EXPECT_EQ(tl.snapToGridInterval(block(), 73), 73);
}

// A single-track pattern has no step to snap to, so the distance is returned
// unchanged even though the pattern has a track.
TEST_F(TestTechLayer, SnapToGridIntervalSingleTrackPatternIsNoOp)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTrackGrid* tracks = odb::dbTrackGrid::create(block(), layer);
  tracks->addGridPatternY(0, 1, 0);
  TechLayer tl(layer);
  EXPECT_EQ(tl.snapToGridInterval(block(), 73), 73);
}

// The instance checkIfManufacturingGrid returns true when value is on grid
// (no logger error needed).
TEST_F(TestTechLayer, InstanceCheckIfManufacturingGridReturnsTrueOnGrid)
{
  tech()->setManufacturingGrid(5);
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  EXPECT_TRUE(tl.checkIfManufacturingGrid(20, getLogger(), "edge"));
}

// The instance checkIfManufacturingGrid calls logger->error when off-grid,
// which throws inside utl::Logger.
TEST_F(TestTechLayer, InstanceCheckIfManufacturingGridThrowsWhenOffGrid)
{
  tech()->setManufacturingGrid(10);
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  EXPECT_THROW(tl.checkIfManufacturingGrid(7, getLogger(), "edge"),
               std::runtime_error);
}

// -------- adjustToMinArea --------

// A layer with no area rule and no LEF area returns the rect unchanged.
TEST_F(TestTechLayer, AdjustToMinAreaWithoutRuleIsNoOp)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  const odb::Rect r(0, 0, 100, 100);
  EXPECT_EQ(tl.adjustToMinArea(r), r);
}

// LEF AREA: a horizontal layer with insufficient area gets widened in x.
// LefUnits = 1000, layer area is in micron^2; min_area_dbu = area * units^2.
// area = 0.04 -> 0.04 * 1000^2 = 40000 dbu^2. For a 100x100 rect (10000),
// required_width = ceil(40000/100) = 400, added = 300, half = 150 on each
// side. New rect: (-150, 0, 250, 100). dx = 400.
TEST_F(TestTechLayer, AdjustToMinAreaLefAreaHorizontalWidensX)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  layer->setArea(40000);
  TechLayer tl(layer);

  const odb::Rect r(0, 0, 100, 100);
  const odb::Rect adj = tl.adjustToMinArea(r);
  EXPECT_EQ(adj.dx(), 400);
  EXPECT_EQ(adj.dy(), 100);  // y unchanged
  EXPECT_EQ(adj.area(), 40000);
  EXPECT_EQ(adj.xMin(), -150);
  EXPECT_EQ(adj.xMax(), 250);
}

// LEF AREA on a vertical layer expands in y instead of x; symmetric.
TEST_F(TestTechLayer, AdjustToMinAreaLefAreaVerticalWidensY)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::VERTICAL);
  layer->setArea(40000);
  TechLayer tl(layer);

  const odb::Rect r(0, 0, 100, 100);
  const odb::Rect adj = tl.adjustToMinArea(r);
  EXPECT_EQ(adj.dx(), 100);  // x unchanged
  EXPECT_EQ(adj.dy(), 400);
  EXPECT_EQ(adj.yMin(), -150);
  EXPECT_EQ(adj.yMax(), 250);
}

// The explicit direction argument overrides the layer's own direction.
TEST_F(TestTechLayer, AdjustToMinAreaExplicitDirectionOverridesLayer)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::VERTICAL);
  layer->setArea(40000);
  TechLayer tl(layer);

  // Pass HORIZONTAL explicitly -> x grows (not y) even though layer is
  // VERTICAL. Result identical to the HORIZONTAL test above.
  const odb::Rect r(0, 0, 100, 100);
  const odb::Rect adj = tl.adjustToMinArea(r, odb::dbTechLayerDir::HORIZONTAL);
  EXPECT_EQ(adj.dx(), 400);
  EXPECT_EQ(adj.dy(), 100);
}

// A LEF58 AREA rule (dbTechLayerAreaRule) drives the rule-based code
// path. Its area is in dbu^2 like the legacy LEF AREA, so 100000 here is
// 0.1 um^2. Expansion of a 100x100 rect (10000): required_width =
// ceil(100000/100) = 1000, added = 900, half = 450 each side.
TEST_F(TestTechLayer, AdjustToMinAreaUsesLef58Rule)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechLayerAreaRule* rule = odb::dbTechLayerAreaRule::create(layer);
  rule->setArea(100000);
  TechLayer tl(layer);

  const odb::Rect r(0, 0, 100, 100);
  const odb::Rect adj = tl.adjustToMinArea(r);
  EXPECT_EQ(adj.dx(), 1000);
  EXPECT_EQ(adj.dy(), 100);
  EXPECT_EQ(adj.xMin(), -450);
  EXPECT_EQ(adj.xMax(), 550);
}

// LEF58 AREA rules add to the layer's own AREA rather than replace it, so a
// smaller LEF58 rule does not lower the minimum: 40000 still governs.
TEST_F(TestTechLayer, AdjustToMinAreaKeepsTheLayerAreaAlongsideLef58Rules)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  layer->setArea(40000);
  odb::dbTechLayerAreaRule* rule = odb::dbTechLayerAreaRule::create(layer);
  rule->setArea(20000);
  TechLayer tl(layer);

  const odb::Rect adj = tl.adjustToMinArea(odb::Rect(0, 0, 100, 100));
  EXPECT_EQ(adj, odb::Rect(-150, 0, 250, 100));
}

// A LEF58 area rule with zero area is skipped (no expansion happens just
// because the rule exists -- it must have a real area requirement).
TEST_F(TestTechLayer, AdjustToMinAreaIgnoresZeroAreaRule)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechLayerAreaRule* rule = odb::dbTechLayerAreaRule::create(layer);
  rule->setArea(0);
  TechLayer tl(layer);

  const odb::Rect r(0, 0, 100, 100);
  EXPECT_EQ(tl.adjustToMinArea(r), r);
}

// A rect that already meets the minimum area is returned unchanged.
TEST_F(TestTechLayer, AdjustToMinAreaLeavesALargeEnoughRectAlone)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  layer->setArea(40000);
  TechLayer tl(layer);

  const odb::Rect r(0, 0, 400, 100);
  EXPECT_EQ(tl.adjustToMinArea(r), r);
}

// An inverted rect (the mergeInit() sentinel) is returned unchanged rather
// than overflowing in the area arithmetic.
TEST_F(TestTechLayer, AdjustToMinAreaLeavesAnInvertedRectAlone)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  layer->setArea(40000);
  TechLayer tl(layer);

  odb::Rect r;
  r.mergeInit();
  EXPECT_EQ(tl.adjustToMinArea(r), r);
}

// -------- getMinCutRules --------

// No min-cut rules on the layer -> empty result.
TEST_F(TestTechLayer, GetMinCutRulesEmptyByDefault)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  TechLayer tl(layer);
  EXPECT_TRUE(tl.getMinCutRules().empty());
}

// A LEF58 MINIMUMCUT rule with a per-cut-class entry is reflected in
// getMinCutRules with the cut_class pointer populated from the cut layer
// below the routing layer.
TEST_F(TestTechLayer, GetMinCutRulesReadsLefV58RuleWithCutClass)
{
  // Build a tiny routing-cut-routing stack so getLowerLayer() of the top
  // metal returns the cut layer (where we'll register the cut class).
  odb::dbTechLayer* m1
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechLayer* cut
      = odb::dbTechLayer::create(tech(), "v1", odb::dbTechLayerType::CUT);
  cut->setWidth(50);
  odb::dbTechLayer* m2 = makeMetal(tech(), "m2", odb::dbTechLayerDir::VERTICAL);
  (void) m1;

  odb::dbTechLayerCutClassRule* cls
      = odb::dbTechLayerCutClassRule::create(cut, "VC");
  cls->setWidth(50);
  cls->setLength(50);
  cls->setLengthValid(true);

  // Attach the LEF58 MINIMUMCUT rule to m2 (so getLowerLayer = cut).
  odb::dbTechLayerMinCutRule* rule = odb::dbTechLayerMinCutRule::create(m2);
  rule->setWidth(100);
  rule->setCutsPerCutClass("VC", 4);

  TechLayer tl(m2);
  auto rules = tl.getMinCutRules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0].cuts, 4);
  EXPECT_EQ(rules[0].width, 100);
  EXPECT_EQ(rules[0].cut_class, cls);
}

// When the cut-class name isn't found on the routing layer's lower layer,
// getMinCutRules falls through to its upper layer. Build m0-v0-m1-v1-m2
// and put the class only on v1; attach the rule to m1 so getLowerLayer
// (v0) misses and getUpperLayer (v1) finds the class.
TEST_F(TestTechLayer, GetMinCutRulesFallsBackToUpperLayerForCutClass)
{
  odb::dbTechLayer* m0
      = makeMetal(tech(), "m0", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechLayer* v0
      = odb::dbTechLayer::create(tech(), "v0", odb::dbTechLayerType::CUT);
  v0->setWidth(50);
  odb::dbTechLayer* m1 = makeMetal(tech(), "m1", odb::dbTechLayerDir::VERTICAL);
  odb::dbTechLayer* v1
      = odb::dbTechLayer::create(tech(), "v1", odb::dbTechLayerType::CUT);
  v1->setWidth(50);
  odb::dbTechLayer* m2
      = makeMetal(tech(), "m2", odb::dbTechLayerDir::HORIZONTAL);
  (void) m0;
  (void) m2;

  odb::dbTechLayerCutClassRule* cls
      = odb::dbTechLayerCutClassRule::create(v1, "VC");
  cls->setWidth(50);

  odb::dbTechLayerMinCutRule* rule = odb::dbTechLayerMinCutRule::create(m1);
  rule->setWidth(100);
  rule->setCutsPerCutClass("VC", 6);

  TechLayer tl(m1);
  auto rules = tl.getMinCutRules();
  ASSERT_EQ(rules.size(), 1u);
  // The cut_class was found on the upper layer (v1), not the lower (v0).
  EXPECT_EQ(rules[0].cut_class, cls);
  EXPECT_EQ(rules[0].cuts, 6);
}

// A LEF v5.4 MINIMUMCUT rule is reflected in getMinCutRules with the
// correct cut count and width. FROMBELOW covers vias that come from below
// the layer, so the layer is above the via.
TEST_F(TestTechLayer, GetMinCutRulesReadsLefV54Rule)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechMinCutRule* rule = odb::dbTechMinCutRule::create(layer);
  rule->setMinimumCuts(/*numcuts=*/3,
                       /*width=*/50,
                       /*above_only=*/false,
                       /*below_only=*/true);  // FROMBELOW
  TechLayer tl(layer);
  auto rules = tl.getMinCutRules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0].cuts, 3);
  EXPECT_EQ(rules[0].width, 50);
  EXPECT_TRUE(rules[0].above);
  EXPECT_FALSE(rules[0].below);
  EXPECT_EQ(rules[0].cut_class, nullptr);  // v5.4 rule has no cut class
}

// FROMABOVE covers vias that come from above the layer, so the layer is
// below the via. The LEF v5.4 and LEF58 rules must agree.
TEST_F(TestTechLayer, GetMinCutRulesReadsFromAboveTheSameForBothLefVersions)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechMinCutRule::create(layer)->setMinimumCuts(
      /*numcuts=*/2, /*width=*/50, /*above_only=*/true, /*below_only=*/false);
  odb::dbTechLayerMinCutRule* rule = odb::dbTechLayerMinCutRule::create(layer);
  rule->setWidth(50);
  rule->setNumCuts(2);
  rule->setFromAbove(true);

  TechLayer tl(layer);
  auto rules = tl.getMinCutRules();
  ASSERT_EQ(rules.size(), 2u);
  for (const auto& min_cut : rules) {
    EXPECT_FALSE(min_cut.above);
    EXPECT_TRUE(min_cut.below);
  }
}

// A LEF58 MINIMUMCUT rule without CUTCLASS gives its cut count for every cut.
TEST_F(TestTechLayer, GetMinCutRulesReadsLef58RuleWithoutCutClass)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechLayerMinCutRule* rule = odb::dbTechLayerMinCutRule::create(layer);
  rule->setWidth(100);
  rule->setNumCuts(4);

  TechLayer tl(layer);
  auto rules = tl.getMinCutRules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0].cut_class, nullptr);
  EXPECT_EQ(rules[0].cuts, 4);
  EXPECT_EQ(rules[0].width, 100);
  EXPECT_FALSE(rules[0].above);
  EXPECT_FALSE(rules[0].below);
}

// A class name defined on the cut layers both below and above the routing
// layer gives a rule for each, so vias on either cut layer find theirs.
TEST_F(TestTechLayer, GetMinCutRulesBindsTheCutClassOnBothCutLayers)
{
  makeMetal(tech(), "m0", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechLayer* v0
      = odb::dbTechLayer::create(tech(), "v0", odb::dbTechLayerType::CUT);
  odb::dbTechLayer* m1 = makeMetal(tech(), "m1", odb::dbTechLayerDir::VERTICAL);
  odb::dbTechLayer* v1
      = odb::dbTechLayer::create(tech(), "v1", odb::dbTechLayerType::CUT);
  makeMetal(tech(), "m2", odb::dbTechLayerDir::HORIZONTAL);
  auto* below = odb::dbTechLayerCutClassRule::create(v0, "VC");
  auto* above = odb::dbTechLayerCutClassRule::create(v1, "VC");

  odb::dbTechLayerMinCutRule* rule = odb::dbTechLayerMinCutRule::create(m1);
  rule->setWidth(100);
  rule->setCutsPerCutClass("VC", 2);

  TechLayer tl(m1);
  auto rules = tl.getMinCutRules();
  ASSERT_EQ(rules.size(), 2u);
  EXPECT_EQ(rules[0].cut_class, below);
  EXPECT_EQ(rules[1].cut_class, above);
}

// FROMABOVE covers only vias on the cut layer above, and FROMBELOW only those
// on the cut layer below, so each binds the class on that cut layer alone.
TEST_F(TestTechLayer, GetMinCutRulesBindsADirectedCutClassOnItsSideOnly)
{
  makeMetal(tech(), "m0", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechLayer* v0
      = odb::dbTechLayer::create(tech(), "v0", odb::dbTechLayerType::CUT);
  odb::dbTechLayer* m1 = makeMetal(tech(), "m1", odb::dbTechLayerDir::VERTICAL);
  odb::dbTechLayer* v1
      = odb::dbTechLayer::create(tech(), "v1", odb::dbTechLayerType::CUT);
  makeMetal(tech(), "m2", odb::dbTechLayerDir::HORIZONTAL);
  auto* below = odb::dbTechLayerCutClassRule::create(v0, "VC");
  auto* above = odb::dbTechLayerCutClassRule::create(v1, "VC");

  odb::dbTechLayerMinCutRule* from_above
      = odb::dbTechLayerMinCutRule::create(m1);
  from_above->setWidth(100);
  from_above->setCutsPerCutClass("VC", 2);
  from_above->setFromAbove(true);
  odb::dbTechLayerMinCutRule* from_below
      = odb::dbTechLayerMinCutRule::create(m1);
  from_below->setWidth(100);
  from_below->setCutsPerCutClass("VC", 3);
  from_below->setFromBelow(true);

  TechLayer tl(m1);
  auto rules = tl.getMinCutRules();
  ASSERT_EQ(rules.size(), 2u);
  EXPECT_EQ(rules[0].cut_class, above);
  EXPECT_EQ(rules[0].cuts, 2);
  EXPECT_EQ(rules[1].cut_class, below);
  EXPECT_EQ(rules[1].cuts, 3);
}

// The top routing layer has no cut layer above it; a class missing from the
// one below gives no rule rather than a lookup on a missing layer.
TEST_F(TestTechLayer, GetMinCutRulesSkipsACutLayerThatIsNotThere)
{
  makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  odb::dbTechLayer::create(tech(), "v1", odb::dbTechLayerType::CUT);
  odb::dbTechLayer* m2 = makeMetal(tech(), "m2", odb::dbTechLayerDir::VERTICAL);
  ASSERT_EQ(m2->getUpperLayer(), nullptr);

  odb::dbTechLayerMinCutRule* rule = odb::dbTechLayerMinCutRule::create(m2);
  rule->setWidth(100);
  rule->setCutsPerCutClass("VC", 2);

  TechLayer tl(m2);
  EXPECT_TRUE(tl.getMinCutRules().empty());
}

// -------- getSpacing --------

// A PARALLELRUNLENGTH table row applies to widths greater than its WIDTH and
// a column to runs longer than its PARALLELRUNLENGTH, so a width or a run
// exactly at a threshold takes the row or column before it.
TEST_F(TestTechLayer, GetSpacingTableAppliesAboveItsThresholds)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  layer->initV55LengthIndex(2);
  layer->addV55LengthEntry(0);
  layer->addV55LengthEntry(1000);
  layer->initV55WidthIndex(2);
  layer->addV55WidthEntry(0);
  layer->addV55WidthEntry(500);
  layer->initV55SpacingTable(2, 2);
  layer->addV55SpacingTableEntry(0, 0, 100);
  layer->addV55SpacingTableEntry(0, 1, 100);
  layer->addV55SpacingTableEntry(1, 0, 100);
  layer->addV55SpacingTableEntry(1, 1, 300);
  TechLayer tl(layer);

  EXPECT_EQ(tl.getSpacing(500, 2000), 100);
  EXPECT_EQ(tl.getSpacing(510, 2000), 300);
  EXPECT_EQ(tl.getSpacing(510, 1000), 100);
  EXPECT_EQ(tl.getSpacing(510, 1010), 300);
}

// getSpacing returns max(dbSpacing, twoWidthsSpacing). With only the
// default spacing set, the two-widths lookup returns 0, so the result
// is exactly the default spacing.
TEST_F(TestTechLayer, GetSpacingReadsLayerSpacing)
{
  odb::dbTechLayer* layer
      = makeMetal(tech(), "m1", odb::dbTechLayerDir::HORIZONTAL);
  layer->setSpacing(15);
  TechLayer tl(layer);
  EXPECT_EQ(tl.getSpacing(/*width=*/50), 15);
}

}  // namespace
}  // namespace pdn
