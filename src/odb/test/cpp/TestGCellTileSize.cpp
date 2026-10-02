// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <exception>
#include <vector>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

class TestGCellTileSize : public SimpleDbFixture
{
 protected:
  void SetUp() override { createSimpleDB(); }

  dbTech* tech() { return db_->getTech(); }
  dbBlock* block() { return db_->getChip()->getBlock(); }

  // Creates a frontside ROUTING layer with a track grid of the given pitch.
  // Layers are numbered in creation order (dbTechLayer::getRoutingLevel()),
  // matching how set_routing_layers/dbBlock::setMaxRoutingLayer resolve a
  // layer name.
  dbTechLayer* makeRoutingLayer(const char* name, int pitch)
  {
    dbTechLayer* layer
        = dbTechLayer::create(tech(), name, dbTechLayerType::ROUTING);
    layer->setDirection(dbTechLayerDir::HORIZONTAL);
    dbTrackGrid* track = dbTrackGrid::create(block(), layer);
    track->addGridPatternY(0, 1000, pitch);
    return layer;
  }

  // Creates a backside ROUTING layer with no track grid -- getGCellTileSize
  // never needs a backside layer's track grid, only its presence in the
  // raw getRoutingLevel() numbering.
  dbTechLayer* makeBacksideLayer(const char* name)
  {
    dbTechLayer* layer
        = dbTechLayer::create(tech(), name, dbTechLayerType::ROUTING);
    layer->setBackside(true);
    return layer;
  }

  // Creates a frontside ROUTING layer with a track grid made of multiple
  // (origin, count, step) grid patterns, to exercise the multi-pattern
  // getGridY() path instead of the single-pattern one makeRoutingLayer()
  // uses.
  dbTechLayer* makeMultiPatternRoutingLayer(
      const char* name,
      const std::vector<std::array<int, 3>>& patterns)
  {
    dbTechLayer* layer
        = dbTechLayer::create(tech(), name, dbTechLayerType::ROUTING);
    layer->setDirection(dbTechLayerDir::HORIZONTAL);
    dbTrackGrid* track = dbTrackGrid::create(block(), layer);
    for (const auto& [origin, count, step] : patterns) {
      track->addGridPatternY(origin, count, step);
    }
    return layer;
  }

  // Creates a frontside ROUTING layer with NO track grid, to exercise the
  // floor loop's missing-grid error path (no currently-known caller
  // reaches this with one missing; see dbBlock.cpp's comment).
  dbTechLayer* makeUntrackedRoutingLayer(const char* name)
  {
    dbTechLayer* layer
        = dbTechLayer::create(tech(), name, dbTechLayerType::ROUTING);
    layer->setDirection(dbTechLayerDir::HORIZONTAL);
    return layer;
  }

  // Creates a frontside ROUTING layer with a track grid but a never-set
  // (NONE) direction, to exercise the floor loop's direction dispatch
  // error path.
  dbTechLayer* makeNoDirectionRoutingLayer(const char* name, int pitch)
  {
    dbTechLayer* layer
        = dbTechLayer::create(tech(), name, dbTechLayerType::ROUTING);
    // Deliberately no layer->setDirection() call.
    dbTrackGrid* track = dbTrackGrid::create(block(), layer);
    track->addGridPatternY(0, 1000, pitch);
    return layer;
  }

  // Creates a frontside ROUTING layer with a track grid containing only a
  // single track (no adjacent-gap to floor against).
  dbTechLayer* makeSingleTrackRoutingLayer(const char* name)
  {
    dbTechLayer* layer
        = dbTechLayer::create(tech(), name, dbTechLayerType::ROUTING);
    layer->setDirection(dbTechLayerDir::HORIZONTAL);
    dbTrackGrid* track = dbTrackGrid::create(block(), layer);
    track->addGridPatternY(0, 1, 100);
    return layer;
  }
};

// gt2n's real (public, BSD-3-Clause: github.com/azadnaeemi/GT2N) tech LEF
// pitches, src/techlib/gt2_fspdn_tech.lef, converted to DBU at gt2n's own
// DATABASE MICRONS 2000: M2 0.024um=48, M3 0.028um=56, M4 0.042um=84,
// M12/M13 0.72um=1440.
constexpr int kGt2nM2Pitch = 48;
constexpr int kGt2nM3Pitch = 56;
constexpr int kGt2nM4Pitch = 84;
constexpr int kGt2nTopLayerPitch = 1440;  // M12/M13

// Baseline behavior is unchanged: with a typical bottom-to-top pitch
// progression and no backside layers, the tile size is still the median of
// M2/M3/M4 track spacing times 15 pitches, and the new per-layer floor loop
// (M4..top) doesn't perturb it because M4 is included in the baseline set
// and no higher layer is wider.
TEST_F(TestGCellTileSize, BaselineMedianOfM2M3M4Unchanged)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  makeRoutingLayer("M4", kGt2nM4Pitch);
  block()->setMaxRoutingLayer(4);

  // median(48, 56, 84) = 56
  EXPECT_EQ(block()->getGCellTileSize(), kGt2nM3Pitch * 15);
}

// gt2n's actual pitches (M5-M11 omitted for test minimality -- only the
// M2-M4 baseline and the top enabled layer's own pitch matter to
// getGCellTileSize()). Sizing from M2-M4 alone can be smaller than a
// single top-layer track pitch, giving some GCells zero top-layer
// tracks and firing DRT-0406 ("No tracks found") the moment global
// routing assigns a wire to one of those tiles; flooring the tile size
// at the pitch of every enabled layer at/above M4, not just the top
// one, closes that.
//
// One pitch is enough here, not a padded margin: DRT's real track
// assignment (FlexTAWorker::getTrackIdx, src/drt/src/ta/FlexTA_assign.cpp)
// treats a GCell's track range as half-open, so any span of at least one
// pitch is guaranteed a track regardless of the grid's phase. See the
// comment in dbBlock.cpp for the full argument.
TEST_F(TestGCellTileSize, FloorsTileSizeForWideTopLayerPitch)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  makeRoutingLayer("M4", kGt2nM4Pitch);
  makeRoutingLayer("M12", kGt2nTopLayerPitch);
  block()->setMaxRoutingLayer(5);

  // Baseline: median(48, 56, 84) * 15 = 840, smaller than one M12/M13
  // pitch (1440) -- exactly the real failure mode. The floor requires one
  // top-layer pitch (1440), which dominates and matches gt2n's real M12
  // pitch exactly.
  EXPECT_EQ(block()->getGCellTileSize(), kGt2nTopLayerPitch);
}

// The floor must check every enabled layer from M4 up to the top, not just
// the top layer, because pitch does not necessarily increase monotonically
// with layer index (unlike gt2n's real stack, which is monotonic -- this
// is a deliberately constructed edge case the real motivating technology
// doesn't exercise, so it stays synthetic rather than using real pitches).
// Here the widest layer (M5) sits *below* the top layer (M6), which is
// comparatively narrow -- checking only the top layer would miss M5
// entirely and leave the same DRT-0406 failure mode on M5.
TEST_F(TestGCellTileSize, FloorConsidersEveryLayerNotJustTheTop)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 80);
  makeRoutingLayer("M3", 90);
  makeRoutingLayer("M4", 100);
  makeRoutingLayer("M5", 3000);  // widest layer, not the top
  makeRoutingLayer("M6", 200);   // top layer, narrower than M5
  block()->setMaxRoutingLayer(6);

  // Baseline: median(80, 90, 100) * 15 = 1350.
  // Top-layer-only floor would give max(1350, 200) = 1350 -- still
  // smaller than M5's own pitch (3000), reproducing DRT-0406 on M5.
  // Checking every layer M4..M6 gives max(1350, 100, 3000, 200) = 3000,
  // which covers M5.
  EXPECT_EQ(block()->getGCellTileSize(), 3000);
}

// GlobalRouter::getMinMaxLayer() grids up to
// max(getMaxRoutingLayer(), getMaxLayerForClock()), since
// max_layer_for_clock_ (set_routing_layers -clock min-max) can allow clock
// nets onto a layer above max_routing_layer_. getGCellTileSize() must
// floor against that same effective max, or a coarse clock-only layer
// reintroduces DRT-0406 through a path this function never saw.
TEST_F(TestGCellTileSize, FloorConsidersMaxLayerForClockAboveMaxRoutingLayer)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 80);
  makeRoutingLayer("M3", 90);
  makeRoutingLayer("M4", 100);
  makeRoutingLayer("M5", 200);
  makeRoutingLayer("M6", 3000);  // clock-only layer
  block()->setMaxRoutingLayer(5);
  block()->setMaxLayerForClock(6);

  // Baseline: median(80, 90, 100) * 15 = 1350. Ignoring max_layer_for_clock,
  // the floor loop would stop at M5 (200): max(1350, 100, 200) = 1350,
  // smaller than M6's own pitch (3000) -- reproducing DRT-0406 on the
  // clock-only layer. Accounting for it extends the floor through M6.
  EXPECT_EQ(block()->getGCellTileSize(), 3000);
}

// Same max_layer_for_clock_ extension, but crossing the exactly-4 boundary
// rather than landing well past it (the case above already has 5 enabled
// layers without clock; this one has only 3): with max_routing_layer_ at 3,
// enabled_frontside_layers is 3, taking the small-stack branch, which never
// looks at M4 at all. Extending through max_layer_for_clock_ to 4 must not
// just append M4 to that branch's tail -- it must cross into the >=4
// branch entirely, making M4 one of the M2/M3/M4 baseline positions.
TEST_F(TestGCellTileSize, MaxLayerForClockCrossesIntoBaselineBranch)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 56);
  makeRoutingLayer("M3", 84);
  makeRoutingLayer("M4", 3000);  // clock-only layer
  block()->setMaxRoutingLayer(3);
  block()->setMaxLayerForClock(4);

  // Correctly crossing into the baseline branch: median(56, 84, 3000) = 84,
  // baseline 84 * 15 = 1260; the floor loop then covers M4 alone (position
  // 4 to 4), whose own pitch (3000) dominates. If the crossing were missed
  // instead (small-stack branch on M2/M3 alone, M4 never considered), the
  // result would be 84 * 15 = 1260 with no floor candidate to beat it --
  // a visibly different, smaller answer.
  EXPECT_EQ(block()->getGCellTileSize(), 3000);
}

// Regression test for a second bug found while generalizing the fix, in the
// already-merged backside-skip code this patch builds on (not introduced by
// this patch): max_routing_layer_ is a *raw* routing level that counts
// backside layers, while the M2/M3/M4 lookup counts frontside layers only.
// On a BSPDN tech with backside layers ordered ahead of the frontside stack
// (gt2n's LEF ordering), a raw max_routing_layer_ can be >= 4 even though
// only one frontside layer is actually enabled. The old code would then
// take the M2/M3/M4 branch, fail to find a 2nd/3rd frontside layer
// (nullptr), and crash dereferencing tech_layer->getName() on that null
// pointer. This fix counts *enabled frontside* layers explicitly so the
// single-layer branch is taken correctly instead.
TEST_F(TestGCellTileSize, BacksideLayersDoNotSkewEnabledLayerCount)
{
  makeBacksideLayer("BPR");
  makeBacksideLayer("BM1");
  makeBacksideLayer("BM2");
  makeBacksideLayer("BRDL");
  dbTechLayer* m1 = makeRoutingLayer("M1", kGt2nM3Pitch);
  ASSERT_EQ(m1->getRoutingLevel(), 5);  // 4 backside + M1

  // Raw routing level 5 -- would fail the old "< 4" check even though only
  // one frontside layer (M1) is enabled.
  block()->setMaxRoutingLayer(5);

  EXPECT_EQ(block()->getGCellTileSize(), kGt2nM3Pitch * 15);
}

// An ordinary stack (nangate45's real tech LEF pitches,
// test/Nangate45/Nangate45_tech.lef, DATABASE MICRONS 2000: metal2
// 0.19um=380, metal3 0.14um=280, metal4 0.28um=560, metal9/metal10
// 1.6um=3200) must come out unfloored: a 1x floor only overrides the
// baseline once a layer's pitch exceeds pitches_in_tile (15x), and
// nangate45's real M2-4-to-M9/M10 ratio (3200/median(380,280,560) ~=
// 8.4x) doesn't reach that. A 2x floor would (8.4x exceeds
// pitches_in_tile/2 = 7.5x), forcing the tile size up on this
// already-working stack for no correctness reason.
TEST_F(TestGCellTileSize, DoesNotFloorOrdinaryNangate45Stack)
{
  makeRoutingLayer("M1", 280);
  makeRoutingLayer("M2", 380);
  makeRoutingLayer("M3", 280);
  makeRoutingLayer("M4", 560);
  makeRoutingLayer("M9", 3200);
  block()->setMaxRoutingLayer(5);

  // Baseline: median(380, 280, 560) * 15 = 380 * 15 = 5700, comfortably
  // >= one top-layer pitch (3200), so the floor must be a no-op.
  EXPECT_EQ(block()->getGCellTileSize(), 380 * 15);
}

// The floor must use the true worst-case gap between adjacent tracks, not
// dbTrackGrid::getAverageTrackSpacing()'s averaged pitch. For a layer with
// more than one grid pattern, _dbTrackGrid::getAverageTrackPattern()
// computes ceil(span / count) -- a mean over every track on the layer, not
// a guaranteed minimum periodicity. Here M5 has two widely separated
// two-track patterns: {0, 50} and {10050, 10100}. The true worst gap
// (10000, between 50 and 10050) is far larger than the averaged pitch
// (ceil(10100 / 4) = 2525) an average-based floor would use instead --
// flooring against that average would leave the tile small enough to
// land entirely inside the real 10000-wide gap, reproducing DRT-0406
// despite the floor appearing to have "done something" (2525 > the
// 1350 baseline).
TEST_F(TestGCellTileSize, FloorUsesWorstGapNotAveragedPitch)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 80);
  makeRoutingLayer("M3", 90);
  makeRoutingLayer("M4", 100);
  makeMultiPatternRoutingLayer("M5", {{0, 2, 50}, {10050, 2, 50}});
  block()->setMaxRoutingLayer(5);

  // Baseline: median(80, 90, 100) * 15 = 1350.
  // True M5 track coordinates: {0, 50, 10050, 10100}; worst gap = 10000.
  EXPECT_EQ(block()->getGCellTileSize(), 10000);
}

// getGCellTileSize() must fail loudly, not silently, when called before
// setMaxRoutingLayer() -- max_routing_layer_ defaults to -1, and silently
// treating that as "every frontside layer is enabled" would size the tile
// for a routing-layer range the caller never actually requested.
TEST_F(TestGCellTileSize, ErrorsWhenMaxRoutingLayerNotSet)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 80);
  makeRoutingLayer("M3", 90);
  makeRoutingLayer("M4", 100);
  // Deliberately no block()->setMaxRoutingLayer() call.

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1226";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1226");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1226)";
  }
}

// 0 is likewise never a real dbTechLayer::getRoutingLevel() value (routing
// levels start at 1), so it must be rejected the same way -1 is, not
// treated as "every frontside layer enabled" (the enabled_frontside_layers
// counting loop's break condition never fires for 0, since its raw_level
// counter starts incrementing from 1).
TEST_F(TestGCellTileSize, ErrorsWhenMaxRoutingLayerIsZero)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 80);
  makeRoutingLayer("M3", 90);
  makeRoutingLayer("M4", 100);
  block()->setMaxRoutingLayer(0);

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1226";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1226");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1226)";
  }
}

// The main floor loop's requireTrackGrid() call hard-errors with the
// pre-existing ODB-0358 when a found layer has no track grid yet,
// matching the small-stack branch's identical behavior (see
// ErrorsOnFoundLayerWithNoTrackGridInSmallStack) -- consistent even
// though no currently-known caller reaches getGCellTileSize() with a
// missing grid in range (every real path, including gpl::RouteBase's
// routability-driven congestion check, calls
// grt::GlobalRouter::initRoutingLayers() first, which hard-errors on
// any missing in-range grid).
TEST_F(TestGCellTileSize, ErrorsOnFoundLayerWithNoTrackGridInFloorLoop)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  makeRoutingLayer("M4", kGt2nM4Pitch);
  makeUntrackedRoutingLayer("M5");
  block()->setMaxRoutingLayer(5);

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-0358";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-0358");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-0358)";
  }
}

// A routing layer whose direction was never set (dbTechLayerDir::NONE)
// must error explicitly in the floor loop's direction dispatch, matching
// dbTrackGrid::getAverageTrackSpacing()'s own dispatch for the same case
// (ODB-416, "Layer {} has invalid direction."), rather than silently
// falling into the getGridX() branch and reading the wrong axis.
TEST_F(TestGCellTileSize, ErrorsOnLayerWithNoDirectionSet)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 80);
  makeRoutingLayer("M3", 90);
  makeRoutingLayer("M4", 100);
  makeNoDirectionRoutingLayer("M5", 3000);
  block()->setMaxRoutingLayer(5);

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1228";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1228");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1228)";
  }
}

// An odd max_track_gap must round *up* to the nearest even value, not
// down: rounding down would shave a floor that's a hard lower bound (not
// a heuristic estimate) below the true required minimum, undermining the
// exact half-open-span guarantee the floor relies on.
TEST_F(TestGCellTileSize, OddMaxTrackGapRoundsUpNotDown)
{
  makeRoutingLayer("M1", 6);
  makeRoutingLayer("M2", 6);
  makeRoutingLayer("M3", 6);
  makeRoutingLayer("M4", 6);
  makeRoutingLayer("M5", 101);  // odd pitch
  block()->setMaxRoutingLayer(5);

  // Baseline: median(6, 6, 6) * 15 = 90. M5's true gap is 101 (odd); if
  // rounded down (matching getAverageTrackSpacing()'s baseline rounding)
  // the floor would be 101, still above the 90 baseline. Rounded up
  // instead, it's 102.
  EXPECT_EQ(block()->getGCellTileSize(), 102);
}

// The small-stack (<4 frontside layers) branch has nothing to fall back
// on if its sole top layer has fewer than two tracks: a tile_size of 0
// here would divide-by-zero downstream (grt::GlobalRouter::initGrid()'s
// dx / tile_size); this must error instead.
TEST_F(TestGCellTileSize, ErrorsOnInsufficientTracksInSmallStack)
{
  makeSingleTrackRoutingLayer("M1");
  makeSingleTrackRoutingLayer("M2");
  block()->setMaxRoutingLayer(2);

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1221";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1221");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1221)";
  }
}

// The main floor loop hard-errors on a found layer with fewer than two
// tracks too, matching the small-stack branch (see above) rather than
// silently skipping it: a distinct id (each must map to a single call
// site, see :dup_id_test), same condition.
TEST_F(TestGCellTileSize, ErrorsOnInsufficientTracksInFloorLoop)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 80);
  makeRoutingLayer("M3", 90);
  makeSingleTrackRoutingLayer("M4");
  block()->setMaxRoutingLayer(4);

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1227";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1227");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1227)";
  }
}

// max_routing_layer_ can resolve to a raw level whose every ROUTING layer
// up to it is backside (a BSPDN stack with no frontside layer enabled at
// all). This must error with a clear message, not fall through to a
// generic "layer <null> not found".
TEST_F(TestGCellTileSize, ErrorsWhenNoFrontsideLayerEnabled)
{
  dbTechLayer* b1 = makeBacksideLayer("BPR");
  makeBacksideLayer("BM1");
  ASSERT_EQ(b1->getRoutingLevel(), 1);
  block()->setMaxRoutingLayer(2);  // both enabled layers are backside

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1222";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1222");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1222)";
  }
}

// The small-stack (<4 frontside layers) branch switched from
// getAverageTrackSpacing()'s averaged pitch to getMaxTrackGap()'s true
// worst-case gap, the same fix FloorUsesWorstGapNotAveragedPitch pins for
// the main floor loop -- but every other small-stack test uses a
// single-pattern grid where the two coincide, so that switch is only
// exercised here, with a genuine multi-pattern grid.
TEST_F(TestGCellTileSize, SmallStackUsesWorstGapNotAveragedPitch)
{
  makeMultiPatternRoutingLayer("M1", {{0, 2, 50}, {10050, 2, 50}});
  block()->setMinRoutingLayer(1);  // min_routing_layer_ defaults to 2
  block()->setMaxRoutingLayer(1);

  // True M1 track coordinates: {0, 50, 10050, 10100}; worst gap = 10000.
  // The averaged pitch (ceil(10100/4) = 2525, rounded down to 2524)
  // would give 2524 * 15 = 37860 instead.
  EXPECT_EQ(block()->getGCellTileSize(), 10000 * 15);
}

// The small-stack branch's boundary-shrink term
// (max_track_gap + getBoundaryShrink(layer)) must actually be able to win
// over its own 15x-pitch side of the max(), the same way the main floor
// loop's comment notes "nothing guarantees [15x] covers it in general" --
// an unusually large via relative to its own layer's fine pitch. M1 here
// has a fine 40 DBU pitch (15x = 600) but an artificially huge default
// via enclosure to M2 (boundary shrink = 1,000,000), so the boundary term
// must dominate.
TEST_F(TestGCellTileSize, SmallStackBoundaryShrinkCanDominate15xPitch)
{
  dbTechLayer* m1 = makeRoutingLayer("M1", 40);
  m1->setRectOnly(true);

  dbTechLayer* cut = dbTechLayer::create(tech(), "V1CUT", dbTechLayerType::CUT);
  dbTechLayer* m2 = dbTechLayer::create(tech(), "M2", dbTechLayerType::ROUTING);
  m2->setDirection(dbTechLayerDir::VERTICAL);

  dbTechVia* via = dbTechVia::create(tech(), "V1_0");
  dbBox::create(via, cut, -10, -10, 10, 10);
  dbBox::create(via, m1, -50, -50, 50, 50);
  dbBox::create(via, m2, -1000000, -1000000, 1000000, 1000000);

  block()->setMinRoutingLayer(1);  // min_routing_layer_ defaults to 2
  block()->setMaxRoutingLayer(1);  // only M1 enabled -- small-stack branch

  // 15x pitch: 40 * 15 = 600. Boundary shrink: merged enclosure height is
  // 2,000,000 DBU (M1 is HORIZONTAL), so the single-end shrink is
  // 1,000,000. Required: 40 + 1,000,000 = 1,000,040, which dominates 600.
  EXPECT_EQ(block()->getGCellTileSize(), 40 + 1000000);
}

// The small-stack (<4 frontside layers) branch has no M2-M4 baseline to
// fall back on, so it must check every enabled layer directly, the same
// reason FloorConsidersEveryLayerNotJustTheTop checks every layer in the
// >=4-layer floor loop instead of only the top -- pitch isn't guaranteed
// to increase monotonically with layer index here either. Here the widest
// layer (M1) sits *below* the top layer (M3); checking only the top layer
// would miss M1 entirely.
TEST_F(TestGCellTileSize, SmallStackConsidersEveryLayerNotJustTheTop)
{
  makeRoutingLayer("M1", 200);  // widest layer, not the top
  makeRoutingLayer("M2", 60);
  makeRoutingLayer("M3", 90);      // top layer, narrower than M1
  block()->setMinRoutingLayer(1);  // min_routing_layer_ defaults to 2;
                                   // must include M1 to prove this
  block()->setMaxRoutingLayer(3);  // 3 frontside layers -- small-stack branch

  // Top-layer-only would give 90 * 15 = 1350, missing M1's own wider pitch
  // (200 * 15 = 3000) entirely.
  EXPECT_EQ(block()->getGCellTileSize(), 200 * 15);
}

// Regression test for src/grt/test/congestion4.tcl (nangate45,
// `set_routing_layers -signal metal3-metal3`): min_routing_layer_ (not
// just max_routing_layer_) can restrict which layers are actually
// enabled, and defaults to 2 even when a caller only ever calls
// setMaxRoutingLayer(). The small-stack loop must start from
// min_routing_layer_'s own position,
// not always position 1 -- otherwise a layer below the real minimum (M2
// here, wider than the one actually enabled) incorrectly inflates the
// tile size. min=max=M3 mirrors congestion4.tcl's metal3-metal3 exactly.
TEST_F(TestGCellTileSize, SmallStackRespectsMinRoutingLayer)
{
  makeRoutingLayer("M1", 50);
  makeRoutingLayer("M2", 300);  // widest, but below min_routing_layer_
  makeRoutingLayer("M3", 90);   // the only layer actually enabled
  block()->setMinRoutingLayer(3);
  block()->setMaxRoutingLayer(3);

  // Only M3 is in range: 90 * 15 = 1350. Including M2 (out of range) would
  // incorrectly give 300 * 15 = 4500.
  EXPECT_EQ(block()->getGCellTileSize(), 90 * 15);
}

// Mirrors FloorConsidersMaxLayerForClockAboveMaxRoutingLayer, but for the
// low side: GlobalRouter::getMinMaxLayer() grids down to
// min(min_routing_layer_, min_layer_for_clock_) whenever
// min_layer_for_clock_ (set_routing_layers -clock min-max) is set, since a
// clock net can be allowed onto a layer below min_routing_layer_. M1 here is
// excluded by min_routing_layer_ alone, but min_layer_for_clock_ pulls it back
// into range.
TEST_F(TestGCellTileSize,
       SmallStackConsidersMinLayerForClockBelowMinRoutingLayer)
{
  makeRoutingLayer("M1", 200);  // widest layer, excluded by min_routing_layer_
                                // alone
  makeRoutingLayer("M2", 60);
  makeRoutingLayer("M3", 90);
  block()->setMinRoutingLayer(2);   // excludes M1 ...
  block()->setMinLayerForClock(1);  // ... but clock nets can still reach it
  block()->setMaxRoutingLayer(3);   // 3 frontside layers -- small-stack branch

  // Without accounting for min_layer_for_clock_, the loop would start at
  // M2 and give 90 * 15 = 1350, missing M1's own wider pitch entirely.
  // Accounting for it starts the loop at M1: 200 * 15 = 3000.
  EXPECT_EQ(block()->getGCellTileSize(), 200 * 15);
}

// getBoundaryShrink()'s routing-range check gates ENABLE_VIA_GEN rotation
// on *both* ends of the connecting via, not just the top: min=max=M2
// (M2 both the only enabled layer and the tech's own physical top) means
// the via connecting down to M1 has its bottom end (M1) below
// min_routing_layer_ even though its top end (M2) is in range. DRT's own
// gate (io_parser_helper.cpp) requires the cut layer's position to be at
// or above BOTTOM_ROUTING_LAYER too, which fails here since the cut
// layer sits below M2 -- so DRT would not rotate this via, and rotating
// it anyway could produce a smaller (unsafe) margin than the un-rotated
// enclosure actually requires.
TEST_F(TestGCellTileSize, BoundaryShrinkRotationRespectsMinRoutingLayer)
{
  dbTechLayer* m1 = makeRoutingLayer("M1", 60);
  // The cut layer must be created *between* M1 and M2, matching real LEF
  // stacking order: dbBox::create(dbTechVia*, ...) infers a via's
  // top/bottom metal layers from the layer with the highest/lowest
  // creation-order number among all of the via's box layers (cut
  // included), so a cut layer created after both metals would itself be
  // mistaken for the top layer.
  dbTechLayer* cut = dbTechLayer::create(tech(), "V1CUT", dbTechLayerType::CUT);
  dbTechLayer* m2 = makeRoutingLayer("M2", 10);
  m2->setRectOnly(true);
  // No layer created after M2: M2 is both min/max routing layer and the
  // tech's own physical top.

  dbTechVia* via = dbTechVia::create(tech(), "V1_2");
  dbBox::create(via, cut, -10, -10, 10, 10);
  // M1 is HORIZONTAL, but its own box here is taller than wide (20 x
  // 2000) -- misaligned by ENABLE_VIA_GEN's own rule, but the via's
  // bottom end (M1) is below min_routing_layer_, so it must not be
  // rotated here.
  dbBox::create(via, m1, -10, -1000, 10, 1000);
  dbBox::create(via, m2, -10, -10, 10, 10);  // square: unaffected either way

  block()->setMinRoutingLayer(2);
  block()->setMaxRoutingLayer(2);

  // Un-rotated merge: M1 y-range [-1000,1000], M2 y-range [-10,10] ->
  // combined y-range [-1000,1000], shrink 1000. Required for M2:
  // max(10 * 15, 10 + 1000) = 1010. Rotating M1's box instead would swap
  // its extents to x:[-1000,1000], y:[-10,10], giving a combined
  // y-range of only [-10,10], shrink 10, required max(150, 20) = 150 --
  // a visibly smaller, incorrect result.
  EXPECT_EQ(block()->getGCellTileSize(), 1010);
}

// set_routing_layers -signal (define_layer_range, src/odb/src/swig/tcl/
// odb.tcl) never validates min <= max the way its -clock counterpart
// does, so min_routing_layer_ set above max_routing_layer_ is reachable
// in practice, leaving no layer in range at all. An empty small-stack
// loop would otherwise silently return a tile size of 0.
TEST_F(TestGCellTileSize, ErrorsWhenMinRoutingLayerExceedsMax)
{
  makeRoutingLayer("M1", 50);
  makeRoutingLayer("M2", 60);
  makeRoutingLayer("M3", 90);
  block()->setMinRoutingLayer(3);
  block()->setMaxRoutingLayer(2);  // min > max: no layer is actually in range

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1224";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1224");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1224)";
  }
}

// Same min > max scenario as ErrorsWhenMinRoutingLayerExceedsMax, but in
// the >=4-layer branch now that its floor loop also reads
// min_routing_layer_ -- a distinct message id from ODB-1224 since each
// one must map to a single call site (see :dup_id_test).
TEST_F(TestGCellTileSize, ErrorsWhenMinRoutingLayerExceedsMaxInMainBranch)
{
  makeRoutingLayer("M1", 50);
  makeRoutingLayer("M2", 60);
  makeRoutingLayer("M3", 70);
  makeRoutingLayer("M4", 80);
  block()->setMinRoutingLayer(5);
  block()->setMaxRoutingLayer(4);  // min > max: no layer is actually in range

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1225";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1225");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1225)";
  }
}

// The small-stack branch's own upper boundary
// (enabled_frontside_layers == upper_layer_for_gcell_size - 1, i.e.
// exactly 3) -- every other small-stack test uses 1 or 2 layers only.
TEST_F(TestGCellTileSize, SmallStackHandlesExactlyThreeFrontsideLayers)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 80);
  makeRoutingLayer("M3", 90);
  block()->setMinRoutingLayer(1);  // min_routing_layer_ defaults to 2;
                                   // set explicitly so all 3 are in range
  block()->setMaxRoutingLayer(3);

  // Small-stack branch uses M3's own gap (90), not a median of 3 layers
  // (that path needs 4 enabled layers). 90 * 15 = 1350.
  EXPECT_EQ(block()->getGCellTileSize(), 90 * 15);
}

// The floor loop's per-layer requirement must include the die-boundary
// via-overhang margin (getBoundaryShrink), not just the interior-case
// track gap, for a layer DRT treats as unidirectional. Uses gt2n's real
// M12 pitch and its real M12-M13 via geometry (VIA V12_0: M12 RECT
// -0.200/-0.180 to 0.200/0.180, M13 RECT -0.180/-0.800 to 0.180/0.800,
// converted to DBU at gt2n's DATABASE MICRONS 2000) to confirm this
// against the exact numbers that motivated the fix, not invented ones.
// Only one candidate via exists here, so this alone doesn't prove
// multiple candidates get merged correctly when there's more than one
// -- see BoundaryShrinkMergesMultipleViaCandidates for that.
TEST_F(TestGCellTileSize, FloorIncludesBoundaryShrinkForUnidirectionalLayer)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  makeRoutingLayer("M4", kGt2nM4Pitch);
  dbTechLayer* m12 = makeRoutingLayer("M12", kGt2nTopLayerPitch);
  m12->setRectOnly(true);  // gt2n's real M12 carries LEF58_RECTONLY

  dbTechLayer* cut
      = dbTechLayer::create(tech(), "V12CUT", dbTechLayerType::CUT);
  dbTechLayer* m13
      = dbTechLayer::create(tech(), "M13", dbTechLayerType::ROUTING);
  m13->setDirection(dbTechLayerDir::VERTICAL);

  dbTechVia* via = dbTechVia::create(tech(), "V12_0");
  dbBox::create(via, cut, -20, -20, 20, 20);        // single cut, in DBU
  dbBox::create(via, m12, -400, -360, 400, 360);    // M12 RECT, in DBU
  dbBox::create(via, m13, -360, -1600, 360, 1600);  // M13 RECT, in DBU

  block()->setMaxRoutingLayer(5);  // M1-M4, M12 enabled; M13 is not

  // Baseline: median(48, 56, 84) * 15 = 840. M12's own interior
  // requirement is its pitch, 1440. The default M12-M13 via's merged
  // enclosure is 800 DBU wide, 3200 DBU tall; M12 is HORIZONTAL, so the
  // single-end boundary shrink is half the height: 1600. Required for
  // M12: 1440 + 1600 = 3040, which dominates the 840 baseline.
  EXPECT_EQ(block()->getGCellTileSize(), 1440 + 1600);
}

// The M2-M4 baseline has no boundary-shrink margin of its own, so the
// floor loop must cover M2 and M3 too, not just M4 and up -- otherwise a
// unidirectional M2 or M3 would be exempt from the same check every
// other enabled layer gets.
TEST_F(TestGCellTileSize, FloorIncludesBoundaryShrinkForUnidirectionalM2)
{
  makeRoutingLayer("M1", 50);
  dbTechLayer* m2 = makeRoutingLayer("M2", 60);
  m2->setRectOnly(true);

  dbTechLayer* cut = dbTechLayer::create(tech(), "V2CUT", dbTechLayerType::CUT);
  makeRoutingLayer("M3", 70);
  makeRoutingLayer("M4", 80);

  dbTechVia* via = dbTechVia::create(tech(), "V2_0");
  dbBox::create(via, cut, -50, -50, 50, 50);
  dbBox::create(via, tech()->findLayer("M2"), -5000, -2000, 5000, 2000);
  dbBox::create(via, tech()->findLayer("M3"), -5000, -2000, 5000, 2000);

  block()->setMaxRoutingLayer(4);

  // Baseline: median(60, 70, 80) * 15 = 1050. M2's own interior
  // requirement is its pitch, 60. The M2-M3 via's merged enclosure is
  // 10000 DBU wide, 4000 DBU tall; M2 is HORIZONTAL, so the single-end
  // boundary shrink is half the height: 2000. Required for M2:
  // 60 + 2000 = 2060, which dominates the baseline and M3/M4's own
  // (shrink-free) requirements.
  EXPECT_EQ(block()->getGCellTileSize(), 60 + 2000);
}

// The main (>=4-layer) floor loop must also start from
// min_routing_layer_'s own position, not a hardcoded M2, the same reason
// computeSmallStackTileSize() reads it (see
// SmallStackRespectsMinRoutingLayer): `set_routing_layers -signal
// met1-...` (M1 enabled) is a common real config -- src/grt/test has
// dozens of tests using it -- and without this, M1's own pitch would
// never be checked by either this loop or the M2-M4 baseline, silently
// reproducing this fix's own failure mode on M1.
TEST_F(TestGCellTileSize, MainLoopRespectsMinRoutingLayer)
{
  makeRoutingLayer("M1", 3000);  // widest layer, below the M2-M4 baseline
  makeRoutingLayer("M2", 60);
  makeRoutingLayer("M3", 70);
  makeRoutingLayer("M4", 80);
  block()->setMinRoutingLayer(1);  // min_routing_layer_ defaults to 2;
                                   // must include M1 to prove this
  block()->setMaxRoutingLayer(4);

  // Baseline: median(60, 70, 80) * 15 = 1050. Without floor coverage
  // starting at M1, the answer would stay 1050, missing M1's own
  // requirement (3000 * 1 = 3000) entirely.
  EXPECT_EQ(block()->getGCellTileSize(), 3000);
}

// Ties directly back to the real motivating scenario: gt2n's currently
// deployed MAX_ROUTING_LAYER=M11 setting, using gt2n's real M11 pitch and
// M11-M12 via geometry (VIA V11_0, converted to DBU at gt2n's DATABASE
// MICRONS 2000). Pitch alone (224) never exceeds the M2-M4 baseline
// (840), and neither does M11's own boundary-shrink margin
// (224 + 400 = 624), so the tile size is unchanged from the pre-fix
// baseline even with M11 (not just gt2n's default M11 top layer) enabled.
TEST_F(TestGCellTileSize, DoesNotInflateGt2nM11Setting)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  makeRoutingLayer("M4", kGt2nM4Pitch);

  // Built manually rather than via makeRoutingLayer() (which always
  // populates a Y-axis grid): M11 is VERTICAL, so getMaxTrackGap() reads
  // the X-axis grid instead -- an unpopulated one would silently read 0
  // tracks and skip this layer's floor contribution entirely.
  dbTechLayer* m11
      = dbTechLayer::create(tech(), "M11", dbTechLayerType::ROUTING);
  m11->setDirection(dbTechLayerDir::VERTICAL);
  m11->setRectOnly(true);  // gt2n's real M11 carries LEF58_RECTONLY
  dbTrackGrid* m11_track = dbTrackGrid::create(block(), m11);
  m11_track->addGridPatternX(0, 1000, 224);

  dbTechLayer* cut
      = dbTechLayer::create(tech(), "V11CUT", dbTechLayerType::CUT);
  dbTechLayer* m12
      = dbTechLayer::create(tech(), "M12", dbTechLayerType::ROUTING);
  m12->setDirection(dbTechLayerDir::HORIZONTAL);

  dbTechVia* via = dbTechVia::create(tech(), "V11_0");
  dbBox::create(via, cut, -56, -360, 56, 360);    // gt2n's real V11 cut box
  dbBox::create(via, m11, -56, -384, 56, 384);    // gt2n's real M11 via box
  dbBox::create(via, m12, -400, -360, 400, 360);  // gt2n's real M12 via box

  block()->setMaxRoutingLayer(5);  // M1-M4, M11 enabled; M12 is not

  // Baseline: median(48, 56, 84) * 15 = 840. M11 alone needs
  // 224 + 400 = 624 -- under the baseline, so it never dominates.
  EXPECT_EQ(block()->getGCellTileSize(), kGt2nM3Pitch * 15);
}

// getBoundaryShrink()'s above/below physical-adjacency scan (and
// tech_top_layer) must skip backside layers even when one is interleaved
// *between* two frontside layers, not just when backside layers all
// precede the frontside stack (gt2n's real layout, and every other test in
// this file). DRT's own frTech never contains a backside layer at all
// (io::Parser::setLayers(), src/drt/src/io/io.cpp filters them out before
// computing its own top-layer/adjacency notions), so treating one as "the
// layer above" here would pick the wrong neighbor. Same setup as
// FloorIncludesBoundaryShrinkForUnidirectionalLayer (M12 RECTONLY,
// default M12-M13 via), except a backside layer is created between M12 and
// M13 -- with no via connecting M12 to that backside layer. Before this
// fix, the above-scan would land on the backside layer first, find no
// connecting via, and silently return a 0 boundary shrink instead of using
// the real M12-M13 via -- reproducing the same die-boundary zero-track
// failure this whole fix exists to close, just for a different (but
// plausible) backside-layer ordering than gt2n's.
TEST_F(TestGCellTileSize, BoundaryShrinkSkipsInterleavedBacksideLayer)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  makeRoutingLayer("M4", kGt2nM4Pitch);
  dbTechLayer* m12 = makeRoutingLayer("M12", kGt2nTopLayerPitch);
  m12->setRectOnly(true);

  makeBacksideLayer("BM_INTERLEAVED");  // no via connects to this

  dbTechLayer* cut
      = dbTechLayer::create(tech(), "V12CUT", dbTechLayerType::CUT);
  dbTechLayer* m13
      = dbTechLayer::create(tech(), "M13", dbTechLayerType::ROUTING);
  m13->setDirection(dbTechLayerDir::VERTICAL);

  dbTechVia* via = dbTechVia::create(tech(), "V12_0");
  dbBox::create(via, cut, -20, -20, 20, 20);  // single cut
  dbBox::create(via, m12, -400, -360, 400, 360);
  dbBox::create(via, m13, -360, -1600, 360, 1600);

  block()->setMaxRoutingLayer(5);  // M1-M4, M12 enabled; M13 is not

  // Identical math to FloorIncludesBoundaryShrinkForUnidirectionalLayer:
  // 1440 + 1600 = 3040. A buggy above-scan that doesn't skip the
  // interleaved backside layer would instead get 1440 + 0 = 1440.
  EXPECT_EQ(block()->getGCellTileSize(), 1440 + 1600);
}

// Two distinct cut layers, each backing a via connecting the same
// M12/M13 pair (not seen on any real tech checked, but not ruled out by
// dbTechVia's own bottom_/top_ tracking either -- that only guarantees a
// via's cut layer sits between bottom and top, not that it's unique).
// Which one DRT would actually use as its default isn't determinable
// from bottom/top alone, but this code doesn't need to know: both are
// simply merged into the margin like any other candidate, since the
// margin only needs to be a safe upper bound, not an exact match.
TEST_F(TestGCellTileSize, BoundaryShrinkMergesAmbiguousCutLayersWithoutErroring)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  makeRoutingLayer("M4", kGt2nM4Pitch);
  dbTechLayer* m12 = makeRoutingLayer("M12", kGt2nTopLayerPitch);
  m12->setRectOnly(true);

  dbTechLayer* cut1
      = dbTechLayer::create(tech(), "V12CUT_A", dbTechLayerType::CUT);
  dbTechLayer* cut2
      = dbTechLayer::create(tech(), "V12CUT_B", dbTechLayerType::CUT);
  dbTechLayer* m13
      = dbTechLayer::create(tech(), "M13", dbTechLayerType::ROUTING);
  m13->setDirection(dbTechLayerDir::VERTICAL);

  dbTechVia* via_a = dbTechVia::create(tech(), "V12_A");
  dbBox::create(via_a, cut1, -20, -20, 20, 20);
  dbBox::create(via_a, m12, -400, -360, 400, 360);
  dbBox::create(via_a, m13, -360, -1600, 360, 1600);

  dbTechVia* via_b = dbTechVia::create(tech(), "V12_B");
  dbBox::create(via_b, cut2, -20, -20, 20, 20);
  dbBox::create(via_b, m12, -400, -360, 400, 360);
  dbBox::create(via_b, m13, -360, -1600, 360, 1600);

  block()->setMaxRoutingLayer(5);  // M1-M4, M12 enabled; M13 is not

  // Both vias have identical enclosures, so merging them changes nothing
  // versus either alone: same result as
  // FloorIncludesBoundaryShrinkForUnidirectionalLayer (1440 + 1600 =
  // 3040). The point here is that this doesn't throw.
  EXPECT_EQ(block()->getGCellTileSize(), 1440 + 1600);
}

// Mirrors DRT's ENABLE_VIA_GEN: if the default via's own enclosure on a
// layer isn't oriented along that layer's routing direction, DRT
// regenerates the via with that box rotated (swapped X/Y) before using
// it. M11 here is HORIZONTAL but its own box is declared taller than
// wide -- misaligned, so it must be rotated before use. Both M11 and M12
// are enabled (unlike the other boundary-shrink tests, which leave the
// via's top layer disabled) since ENABLE_VIA_GEN's rotation only applies
// within [BOTTOM_ROUTING_LAYER, TOP_ROUTING_LAYER]: a via reaching above
// max_routing_layer_ keeps its original, un-rotated LEF enclosure. M12
// has no track grid, so it doesn't itself contribute to the floor.
TEST_F(TestGCellTileSize, BoundaryShrinkRotatesMisalignedEnclosure)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  makeRoutingLayer("M4", kGt2nM4Pitch);
  dbTechLayer* m11 = makeRoutingLayer("M11", kGt2nTopLayerPitch);
  m11->setRectOnly(true);

  dbTechLayer* cut
      = dbTechLayer::create(tech(), "V11CUT", dbTechLayerType::CUT);
  dbTechLayer* m12
      = dbTechLayer::create(tech(), "M12", dbTechLayerType::ROUTING);
  m12->setDirection(dbTechLayerDir::VERTICAL);
  // M12's own floor contribution isn't the point of this test (M11's
  // boundary shrink is); a small pitch keeps it well under the expected
  // result below.
  dbTrackGrid::create(block(), m12)->addGridPatternX(0, 1000, 10);

  dbTechVia* via = dbTechVia::create(tech(), "V11_0");
  dbBox::create(via, cut, -20, -20, 20, 20);
  // M11 is HORIZONTAL, but its own box here is taller than wide (720 x
  // 3200) -- misaligned, so it gets rotated to 3200 x 720 before use.
  dbBox::create(via, m11, -360, -1600, 360, 1600);
  dbBox::create(via, m12, -50, -500, 50, 500);

  block()->setMaxRoutingLayer(6);  // M1-M4, M11, M12 all enabled

  // Rotated M11 box (-1600,-360,1600,360) merged with M12's
  // (-50,-500,50,500): y range [-500,500] (1000), so shrink is 500.
  // Required: 1440 + 500 = 1940. The un-rotated merge would instead give
  // y range [-1600,1600] (3200), shrink 1600, required 1440+1600 = 3040.
  EXPECT_EQ(block()->getGCellTileSize(), 1440 + 500);
}

// When multiple vias connect the same M12/M13 pair, each is merged into
// the margin independently rather than one being selected -- the point
// is a safe upper bound, not DRT's exact default-via pick. via_a's own
// M12 box is much wider than via_b's, and via_b's own M13 box is much
// taller than via_a's, so neither via alone reaches the combined result:
// merging must pick up via_a's M12 extent and via_b's M13 extent
// together. via_a is also multi-cut, confirming multi-cut vias are
// merged in too, not just single-cut ones.
TEST_F(TestGCellTileSize, BoundaryShrinkMergesMultipleViaCandidates)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  makeRoutingLayer("M4", kGt2nM4Pitch);
  dbTechLayer* m12 = makeRoutingLayer("M12", kGt2nTopLayerPitch);
  m12->setRectOnly(true);

  dbTechLayer* cut
      = dbTechLayer::create(tech(), "V12CUT", dbTechLayerType::CUT);
  dbTechLayer* m13
      = dbTechLayer::create(tech(), "M13", dbTechLayerType::ROUTING);
  m13->setDirection(dbTechLayerDir::VERTICAL);

  // Multi-cut (2 cuts). Wide M12 box (y:[-2000,0]), small M13 box.
  dbTechVia* via_a = dbTechVia::create(tech(), "V12_A");
  dbBox::create(via_a, cut, -20, -20, 20, 20);
  dbBox::create(via_a, cut, 40, 40, 80, 80);
  dbBox::create(via_a, m12, -2000, -2000, 2000, 0);
  dbBox::create(via_a, m13, -50, -50, 50, 50);

  // Single-cut. Small M12 box, tall M13 box (y:[0,2000]).
  dbTechVia* via_b = dbTechVia::create(tech(), "V12_B");
  dbBox::create(via_b, cut, -20, -20, 20, 20);
  dbBox::create(via_b, m12, -50, -50, 50, 50);
  dbBox::create(via_b, m13, -50, 0, 50, 2000);

  block()->setMaxRoutingLayer(5);

  // Merged M12 y-range: [-2000,0] union [-50,50] = [-2000,50]. Merged
  // M13 y-range: [-50,50] union [0,2000] = [-50,2000]. Combined y-range:
  // [-2000,2000], shrink 2000, required 1440 + 2000 = 3440. Either via
  // alone would give only 1440 + 1025 = 2465 (y-range [-2000,50] or
  // [-50,2000], both 2050 wide) -- clearly distinguishable from the
  // merged result.
  EXPECT_EQ(block()->getGCellTileSize(), 1440 + 2000);
}

// getBoundaryShrink() must also apply when `layer` is the tech's own
// physical top (no layer above it at all) -- it should look at the via
// connecting it to the layer *below* instead, mirroring DRT's own
// lNum +/- 1 fallback. Every other boundary-shrink test uses a non-top
// enabled layer (M12, with M13 created above it); this test makes the
// enabled top layer also the tech's physical top.
TEST_F(TestGCellTileSize, BoundaryShrinkLooksBelowForTechsPhysicalTopLayer)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  dbTechLayer* cut = dbTechLayer::create(tech(), "V3CUT", dbTechLayerType::CUT);
  dbTechLayer* m4 = makeRoutingLayer("M4", kGt2nM4Pitch);
  m4->setRectOnly(true);
  // No layer created after M4: M4 is both the enabled top layer and the
  // tech's absolute physical top layer.

  // M4's own enclosure is wider than tall (dx > dy), aligned with its
  // HORIZONTAL direction -- an unaligned one would be rotated (see
  // getBoundaryShrink()'s rotateIfMisaligned) and isn't what this test
  // means to exercise.
  dbTechVia* via = dbTechVia::create(tech(), "V3_4");
  dbBox::create(via, cut, -20, -20, 20, 20);  // single cut
  dbBox::create(via, tech()->findLayer("M3"), -400, -360, 400, 360);
  dbBox::create(via, m4, -2000, -1600, 2000, 1600);

  block()->setMaxRoutingLayer(4);

  // Baseline: median(48, 56, 84) * 15 = 840. M4's own gap is 84; its
  // boundary shrink (M4 is HORIZONTAL) is half the merged enclosure
  // height: 1600. Required: 84 + 1600 = 1684, which dominates.
  EXPECT_EQ(block()->getGCellTileSize(), 84 + 1600);
}

// If no via at all connects a unidirectional layer to its neighbor,
// getBoundaryShrink() must return 0 (no extra margin) rather than
// erroring or crashing -- the interior-case floor still applies on its
// own.
TEST_F(TestGCellTileSize, BoundaryShrinkIsZeroWithNoConnectingVia)
{
  makeRoutingLayer("M1", kGt2nM3Pitch);
  makeRoutingLayer("M2", kGt2nM2Pitch);
  makeRoutingLayer("M3", kGt2nM3Pitch);
  makeRoutingLayer("M4", kGt2nM4Pitch);
  dbTechLayer* m12 = makeRoutingLayer("M12", kGt2nTopLayerPitch);
  m12->setRectOnly(true);
  // No via created at all, and no M13 above it (M12 is the tech's own
  // physical top too), so there is nothing to floor against.

  block()->setMaxRoutingLayer(5);

  EXPECT_EQ(block()->getGCellTileSize(), kGt2nTopLayerPitch);
}

// max_routing_layer_ set beyond the tech's actual ROUTING layer count is
// invalid input, the same class of problem as -1 or 0 (both already
// guarded), just from the other direction: the enabled_frontside_layers
// counting loop's break condition never fires, and without this check it
// would silently treat every layer in the tech as enabled instead of
// erroring.
TEST_F(TestGCellTileSize, ErrorsWhenMaxRoutingLayerExceedsLayerCount)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 80);
  makeRoutingLayer("M3", 90);
  makeRoutingLayer("M4", 100);
  block()->setMaxRoutingLayer(5);  // only 4 routing layers exist

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1223";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1223");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1223)";
  }
}

// The small-stack branch's requireTrackGrid() call hard-errors with the
// pre-existing ODB-0358 when a found layer has no track grid yet, same
// as the main floor loop (see ErrorsOnFoundLayerWithNoTrackGridInFloorLoop
// above).
TEST_F(TestGCellTileSize, ErrorsOnFoundLayerWithNoTrackGridInSmallStack)
{
  makeUntrackedRoutingLayer("M1");
  block()->setMinRoutingLayer(1);  // min_routing_layer_ defaults to 2
  block()->setMaxRoutingLayer(1);

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-0358";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-0358");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-0358)";
  }
}

}  // namespace
}  // namespace odb
