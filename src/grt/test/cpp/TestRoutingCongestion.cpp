// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <string>
#include <vector>

#include "grt/GlobalRouter.h"
#include "grt/RoutingCongestion.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "tst/IntegratedFixture.h"

namespace grt {
namespace {

// Exercises the RoutingCongestion service and contrasts what its two backing
// sources can see.
//
// The design is gcd on Nangate45 restricted to two routing layers, which is
// enough to put it well past capacity: the map spans 0.0 to 1.0 across the
// core. That spread is what makes the queries testable at all -- on an
// uncongested design "rank these GCells by congestion" returns one flat value
// everywhere and any ordering satisfies it, so a broken ranking would pass.
//
// The same queries run against the same netlist twice:
//
//   * before global routing, where the only signal is the RUDY estimate
//   * after it, where the per-GCell usage/capacity map GRT wrote into ODB
//     takes over
//
// Consumers do not choose between them -- source() only reports which one
// answered -- so the tests separate the contract that has to hold either way
// (ordering, region containment, point/region agreement) from the places the
// two differ.
//
// Note that both sources account for a capacity reduction: RUDY folds one in
// through Rudy::getResourceReductions(), not only GRT. What differs is the
// demand model -- RUDY spreads net bounding boxes over the grid, GRT counts
// what it actually routed -- so the two react to the same reduction by
// different amounts rather than one seeing it and the other not.
class RoutingCongestionTest : public tst::IntegratedFixture
{
 protected:
  // gcd normally routes over metal2-metal10; two layers alone congest it,
  // without needing a design-wide capacity adjustment.
  static constexpr int kMinLayer = 2;
  static constexpr int kMaxLayer = 3;
  static constexpr float kRegionReduction = 0.9f;

  RoutingCongestionTest()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/grt/test/")
  {
  }

  void SetUp() override
  {
    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    odb::defin def_reader(db_.get(), &logger_, odb::defin::DEFAULT);
    std::vector<odb::dbLib*> search_libs;
    for (odb::dbLib* lib : db_->getLibs()) {
      search_libs.push_back(lib);
    }
    def_reader.readChip(
        search_libs, getFilePath("_main/src/grt/test/gcd.def").c_str(), chip);
    block_ = db_->getChip()->getBlock();
    sta_->postReadDef(block_);

    grt_.setMinRoutingLayer(kMinLayer);
    grt_.setMaxRoutingLayer(kMaxLayer);
    // The point is to inspect an overflowing map, not to fail on it.
    grt_.setAllowCongestion(true);

    congestion_ = grt_.getRoutingCongestion();
    ASSERT_NE(congestion_, nullptr);
  }

  odb::Rect core() const { return block_->getCoreArea(); }

  odb::Rect coreQuadrant(const int qx, const int qy) const
  {
    const odb::Rect c = core();
    const int dx = c.dx() / 2;
    const int dy = c.dy() / 2;
    return {c.xMin() + qx * dx,
            c.yMin() + qy * dy,
            c.xMin() + (qx + 1) * dx,
            c.yMin() + (qy + 1) * dy};
  }

  // Quadrant averages over gcd are nearly uniform under both sources -- the
  // congestion spread is per-GCell, not per-quadrant -- so these two are
  // interchangeable until a test starves one of them. That is deliberate: it
  // means a difference between them can only have come from the capacity
  // change, since nothing else distinguishes them.
  odb::Rect starvedRegion() const { return coreQuadrant(0, 0); }
  odb::Rect controlRegion() const { return coreQuadrant(1, 1); }

  // Reduce the routing capacity of `region` on both routing layers.
  // addRegionAdjustment takes DBU; the Tcl command converts from microns
  // before calling it.
  void starveRegion(const odb::Rect& region)
  {
    for (int layer = kMinLayer; layer <= kMaxLayer; layer++) {
      grt_.addRegionAdjustment(region.xMin(),
                               region.yMin(),
                               region.xMax(),
                               region.yMax(),
                               layer,
                               kRegionReduction);
    }
  }

  // The query the PNR-aware transforms are built on: where in `region` would
  // a cell disturb routing least. Printed so the test doubles as a way to
  // eyeball the map.
  std::vector<RoutingCongestion::GCell> reportLowestCongestion(
      const std::string& label,
      const odb::Rect& region,
      const int count)
  {
    const std::vector<RoutingCongestion::GCell> gcells
        = congestion_->gcellsByCongestion(region, region.center());
    logger_.report("{}: {} gcells, source {}", label, gcells.size(), source());
    for (int i = 0; i < count && i < static_cast<int>(gcells.size()); i++) {
      const RoutingCongestion::GCell& gcell = gcells[i];
      logger_.report("  ({:6d}, {:6d}) .. ({:6d}, {:6d})  congestion {:.3f}",
                     gcell.rect.xMin(),
                     gcell.rect.yMin(),
                     gcell.rect.xMax(),
                     gcell.rect.yMax(),
                     gcell.congestion);
    }
    return gcells;
  }

  const char* source()
  {
    switch (congestion_->source()) {
      case RoutingCongestion::Source::kGlobalRoute:
        return "global route";
      case RoutingCongestion::Source::kRudy:
        return "rudy";
      case RoutingCongestion::Source::kNone:
        return "none";
    }
    return "?";
  }

  float meanCongestion(const odb::Rect& region)
  {
    return congestion_->congestion(region, RoutingCongestion::Aggregate::kMean);
  }

  // Stack one quadrant's cells on a single point, stretching every net they
  // sit on.  Far too large a change for RUDY to report the same number
  // afterwards, so a test that still sees the old value saw a stale cache
  // rather than a small numeric coincidence.  Returns how many moved.
  // Every GCell of the core with its congestion, in a fixed order.
  std::vector<RoutingCongestion::GCell> coreMap()
  {
    return congestion_->gcellsByCongestion(core(), core().center());
  }

  // The movable instance driving the most nets: moving it drags the most
  // routing across the die, so the reroute touches a worthwhile number of
  // tiles.
  odb::dbInst* mostConnectedInst() const
  {
    odb::dbInst* best = nullptr;
    int best_nets = 0;
    for (odb::dbInst* inst : block_->getInsts()) {
      if (inst->getPlacementStatus().isFixed()) {
        continue;
      }
      int nets = 0;
      for (odb::dbITerm* iterm : inst->getITerms()) {
        odb::dbNet* net = iterm->getNet();
        if (net != nullptr && !net->isSpecial()) {
          nets++;
        }
      }
      if (nets > best_nets) {
        best_nets = nets;
        best = inst;
      }
    }
    return best;
  }

  int pileQuadrantOntoAPoint()
  {
    const odb::Point target = controlRegion().center();
    const odb::Rect source = starvedRegion();
    int moved = 0;
    for (odb::dbInst* inst : block_->getInsts()) {
      // Physical-only cells (well taps, fill) are FIRM; odb rejects moving
      // them.
      if (inst->getPlacementStatus().isFixed()) {
        continue;
      }
      if (source.overlaps(inst->getBBox()->getBox())) {
        inst->setLocation(target.x(), target.y());
        moved++;
      }
    }
    return moved;
  }

  void runGlobalRoute()
  {
    grt_.globalRoute();
    // GlobalRouter drops the cached map when routing changes; the test does
    // not depend on having caught every such path.
    congestion_->invalidate();
  }

  RoutingCongestion* congestion_ = nullptr;
};

TEST_F(RoutingCongestionTest, SourceIsRudyBeforeRoutingAndGlobalRouteAfter)
{
  // gcd.def carries no GCELLGRID, so the only signal before routing is the
  // RUDY estimate.
  EXPECT_EQ(congestion_->source(), RoutingCongestion::Source::kRudy);
  EXPECT_GT(congestion_->tileSize(), 0);

  runGlobalRoute();

  // Once GRT has filled in per-GCell usage and capacity that supersedes the
  // estimate, without the caller asking.
  EXPECT_EQ(congestion_->source(), RoutingCongestion::Source::kGlobalRoute);
  EXPECT_GT(congestion_->tileSize(), 0);
}

TEST_F(RoutingCongestionTest, RankingIsOrderedAndStaysInsideTheRegion)
{
  // Contract that has to hold whichever source answered.
  const odb::Rect region = starvedRegion();

  for (const bool routed : {false, true}) {
    if (routed) {
      runGlobalRoute();
    }
    const std::vector<RoutingCongestion::GCell> gcells = reportLowestCongestion(
        routed ? "lowest congestion in the region after global route"
               : "lowest congestion in the region before global route",
        region,
        5);
    ASSERT_FALSE(gcells.empty());

    float previous = -1.0f;
    for (const RoutingCongestion::GCell& gcell : gcells) {
      // Least congested first, and every value a usable fraction.
      EXPECT_GE(gcell.congestion, previous);
      EXPECT_GE(gcell.congestion, 0.0f);
      EXPECT_LE(gcell.congestion, 1.0f);
      previous = gcell.congestion;
      // A GCell outside the region would send the caller to place a cell
      // somewhere it did not ask about. intersects, not overlaps: the tile
      // holding region.xMax() can start exactly there, touching the region
      // without sharing interior.
      EXPECT_TRUE(gcell.rect.intersects(region));
    }
  }
}

TEST_F(RoutingCongestionTest, RankingHasSpreadOnACongestedDesign)
{
  runGlobalRoute();

  const std::vector<RoutingCongestion::GCell> gcells
      = congestion_->gcellsByCongestion(core(), core().center());
  ASSERT_FALSE(gcells.empty());

  // A ranking with no spread would satisfy the ordering check while telling a
  // caller nothing about where to put anything.
  EXPECT_LT(gcells.front().congestion, gcells.back().congestion);
  EXPECT_GT(gcells.back().congestion, 0.5f);
  logger_.report("congestion range over the core: {:.3f} .. {:.3f}",
                 gcells.front().congestion,
                 gcells.back().congestion);
}

TEST_F(RoutingCongestionTest, GlobalRouteSeesCapacityRemovedFromARegion)
{
  // A controlled before/after on the capacity alone: route once, take
  // capacity away from one region, route again, and ask about the same region
  // both times. The netlist never changes, so demand is identical across the
  // two runs and the difference can only be the capacity.
  //
  // The RUDY figure is recorded before the reduction exists, so it is this
  // region's demand density with nothing subtracted -- useful as the baseline
  // a consumer would have had to work from before routing.
  const odb::Rect region = starvedRegion();
  const float rudy = meanCongestion(region);
  ASSERT_EQ(congestion_->source(), RoutingCongestion::Source::kRudy);

  runGlobalRoute();
  const float full_capacity = meanCongestion(region);
  ASSERT_EQ(congestion_->source(), RoutingCongestion::Source::kGlobalRoute);

  starveRegion(region);
  runGlobalRoute();
  const float starved = meanCongestion(region);

  logger_.report("one region, same netlist throughout:");
  logger_.report("  rudy (demand only)            {:.3f}", rudy);
  logger_.report("  global route, full capacity   {:.3f}", full_capacity);
  logger_.report("  global route, {:.0f}% less capacity {:.3f}",
                 kRegionReduction * 100,
                 starved);

  EXPECT_GT(starved, full_capacity);
}

TEST_F(RoutingCongestionTest, BothSourcesRankAStarvedRegionWorse)
{
  // The before/after comparison. Take capacity away from one of two
  // otherwise indistinguishable quadrants, then ask each source which is
  // worse. Absolute values are not comparable across the sources -- RUDY
  // estimates demand from net bounding boxes, GRT counts routed demand -- but
  // a transform that runs pre-GRT and again post-GRT must not be sent to
  // opposite corners of the die, so both have to point at the same quadrant.
  //
  // Starve before the first query: RUDY is built lazily, and folds the
  // reduction in through Rudy::getResourceReductions() when it is.
  starveRegion(starvedRegion());

  const float rudy_starved = meanCongestion(starvedRegion());
  const float rudy_control = meanCongestion(controlRegion());
  ASSERT_EQ(congestion_->source(), RoutingCongestion::Source::kRudy);

  runGlobalRoute();
  ASSERT_EQ(congestion_->source(), RoutingCongestion::Source::kGlobalRoute);
  const float grt_starved = meanCongestion(starvedRegion());
  const float grt_control = meanCongestion(controlRegion());

  logger_.report("                  starved   full capacity   difference");
  logger_.report("rudy          {:11.3f}   {:13.3f}   {:10.3f}",
                 rudy_starved,
                 rudy_control,
                 rudy_starved - rudy_control);
  logger_.report("global route  {:11.3f}   {:13.3f}   {:10.3f}",
                 grt_starved,
                 grt_control,
                 grt_starved - grt_control);

  EXPECT_GT(rudy_starved - rudy_control, 0.2f);
  EXPECT_GT(grt_starved - grt_control, 0.2f);
}

TEST_F(RoutingCongestionTest, PointQueryAgreesWithTheRankedGCell)
{
  runGlobalRoute();

  const odb::Rect region = starvedRegion();
  const std::vector<RoutingCongestion::GCell> gcells
      = congestion_->gcellsByCongestion(region, region.center());
  ASSERT_FALSE(gcells.empty());

  // Whichever GCell the ranking names, asking for the congestion at a point
  // inside it has to give the same answer.
  for (const RoutingCongestion::GCell& gcell : gcells) {
    EXPECT_FLOAT_EQ(congestion_->congestion(gcell.rect.center()),
                    gcell.congestion);
  }
}

TEST_F(RoutingCongestionTest, PeakIsAtLeastMeanAndMatchesTheGCells)
{
  runGlobalRoute();

  const odb::Rect region = starvedRegion();
  const float mean = meanCongestion(region);
  const float peak
      = congestion_->congestion(region, RoutingCongestion::Aggregate::kPeak);
  EXPECT_GE(peak, mean);

  const std::vector<RoutingCongestion::GCell> gcells
      = congestion_->gcellsByCongestion(region, region.center());
  ASSERT_FALSE(gcells.empty());
  // gcellsByCongestion sorts ascending, so the last one is the peak.
  EXPECT_FLOAT_EQ(peak, gcells.back().congestion);
}

TEST_F(RoutingCongestionTest, IsCongestedFollowsTheThreshold)
{
  runGlobalRoute();

  const odb::Rect region = starvedRegion();
  const float peak
      = congestion_->congestion(region, RoutingCongestion::Aggregate::kPeak);
  ASSERT_GT(peak, 0.0f);

  congestion_->setCongestionThreshold(peak * 0.5f);
  EXPECT_TRUE(congestion_->isCongested(region));

  congestion_->setCongestionThreshold(1.0f);
  EXPECT_EQ(congestion_->isCongested(region), peak >= 1.0f);
}

TEST_F(RoutingCongestionTest, RudyEstimateFollowsTheNetlist)
{
  // Pre-GRT the answer comes from RUDY, which is derived from the netlist, so
  // an edit has to show up without the caller asking for a refresh.  Nothing
  // else invalidates here -- GlobalRouter only does so on routing events --
  // so without the netlist observer a consumer resizing the design would keep
  // reading the map from before any of its own changes.
  ASSERT_EQ(congestion_->source(), RoutingCongestion::Source::kRudy);
  const float before = meanCongestion(core());
  ASSERT_GT(before, 0.0f);

  const int moved = pileQuadrantOntoAPoint();
  ASSERT_GT(moved, 0);

  const float after = meanCongestion(core());
  logger_.report("rudy over the core: {:.6f} before, {:.6f} after moving {}",
                 before,
                 after,
                 moved);
  EXPECT_NE(before, after);
}

TEST_F(RoutingCongestionTest, NetlistChangeLeavesTheRoutedMapAlone)
{
  // The other half of that contract, pinned so it is not "fixed" by accident.
  // A routed map describes odb's GCell grid, which a netlist edit does not
  // touch, so invalidating here would spend a full grid rebuild to produce
  // the same numbers.  Refreshing it needs the affected nets rerouted.
  runGlobalRoute();
  ASSERT_EQ(congestion_->source(), RoutingCongestion::Source::kGlobalRoute);
  const float before = meanCongestion(core());

  ASSERT_GT(pileQuadrantOntoAPoint(), 0);

  EXPECT_FLOAT_EQ(meanCongestion(core()), before);
}

TEST_F(RoutingCongestionTest, IncrementalRerouteRefreshesTheTilesItTouched)
{
  // updateDirtyRoutes() now refreshes odb's congestion for the GCells the old
  // and new routes ran over.  IncrementalGRoute is the path that matters:
  // unlike endIncremental() it is not followed by a full rebuild, so what it
  // writes is what a consumer reads.
  runGlobalRoute();

  odb::dbInst* inst = mostConnectedInst();
  ASSERT_NE(inst, nullptr);
  const odb::Point destination = controlRegion().center();
  const float before = meanCongestion(core());

  int x0 = 0, y0 = 0;
  inst->getLocation(x0, y0);
  float after_move = 0.0f;
  {
    IncrementalGRoute incr(&grt_, block_);
    inst->setLocation(destination.x(), destination.y());
    incr.updateRoutes(/*save_guides=*/false);
    after_move = meanCongestion(core());
    // Move it back so the second reroute vacates tiles the first one filled.
    // Without this the old footprint is a subset of the new one and the
    // equivalence check cannot tell whether old routes were tracked at all.
    inst->setLocation(x0, y0);
    incr.updateRoutes(/*save_guides=*/false);
  }
  const std::vector<RoutingCongestion::GCell> incremental = coreMap();

  // A map that never moved would pass the equivalence check below against an
  // equally stale reference, so pin that the first reroute showed up.
  EXPECT_NE(after_move, before);

  // Same routing, refreshed the expensive way: startIncremental with nothing
  // dirty, then endIncremental, whose finishGlobalRouting() rewrites every
  // tile.  The incremental write has to agree with it everywhere.
  grt_.startIncremental();
  grt_.endIncremental();
  const std::vector<RoutingCongestion::GCell> full = coreMap();

  ASSERT_EQ(incremental.size(), full.size());
  for (size_t i = 0; i < incremental.size(); i++) {
    EXPECT_FLOAT_EQ(incremental[i].congestion, full[i].congestion)
        << "gcell " << i;
    EXPECT_EQ(incremental[i].rect, full[i].rect) << "gcell " << i;
  }
}

TEST_F(RoutingCongestionTest, PatchedCacheMatchesARebuiltOne)
{
  // refreshTiles() patches the cached scores for the GCells a reroute moved
  // instead of dropping the map, which would cost a rescan of every tile on
  // every layer.  The patch has to leave the cache identical to what a
  // rebuild would produce, or queries drift further from the truth with
  // every incremental round.
  runGlobalRoute();

  odb::dbInst* inst = mostConnectedInst();
  ASSERT_NE(inst, nullptr);
  const odb::Point destination = controlRegion().center();

  // Build the map first: refreshTiles() only patches a map that exists and
  // falls back to invalidate() otherwise, so without this the reroute would
  // exercise the rebuild path instead of the patch path.
  ASSERT_EQ(congestion_->source(), RoutingCongestion::Source::kGlobalRoute);
  const float before = meanCongestion(core());

  // One move, not there-and-back: the routing has to end up different from
  // where the cache started, or a patch that did nothing would still agree
  // with the rebuild.
  {
    IncrementalGRoute incr(&grt_, block_);
    inst->setLocation(destination.x(), destination.y());
    incr.updateRoutes(/*save_guides=*/false);
  }

  const std::vector<RoutingCongestion::GCell> patched = coreMap();
  EXPECT_NE(meanCongestion(core()), before);

  // Same odb grid, read through a map built from scratch.
  congestion_->invalidate();
  const std::vector<RoutingCongestion::GCell> rebuilt = coreMap();

  ASSERT_EQ(patched.size(), rebuilt.size());
  for (size_t i = 0; i < patched.size(); i++) {
    EXPECT_FLOAT_EQ(patched[i].congestion, rebuilt[i].congestion)
        << "gcell " << i;
    EXPECT_EQ(patched[i].rect, rebuilt[i].rect) << "gcell " << i;
  }
}

// The same service against the other routing engine. RoutingCongestion reads
// odb's GCell grid rather than any engine's internals, so CUGR and FastRoute
// reach it through the same path -- but only CUGR::updateDbCongestion()
// decides what CUGR writes there, and nothing else in this suite would notice
// if that stopped matching FastRoute's capacity/usage convention.
//
// Note what is *not* tested here: addRegionAdjustment(). initCUGR() applies
// only the global adjustment, never applyAdjustments(), so region adjustments
// are a FastRoute-only feature and a CUGR test built on one would be
// asserting against a reduction the engine never saw.
class RoutingCongestionCugrTest : public RoutingCongestionTest
{
 protected:
  void SetUp() override
  {
    RoutingCongestionTest::SetUp();
    // Engine selection is resolved at route time, so this still takes effect.
    grt_.setUseCUGR(true);
    ASSERT_TRUE(grt_.isUseCUGR());
  }
};

TEST_F(RoutingCongestionCugrTest, CugrProducesAUsableCongestionMap)
{
  EXPECT_EQ(congestion_->source(), RoutingCongestion::Source::kRudy);

  runGlobalRoute();

  // CUGR populated the same GCell grid FastRoute does, so the service switches
  // to it without the caller asking.
  ASSERT_EQ(congestion_->source(), RoutingCongestion::Source::kGlobalRoute);
  EXPECT_GT(congestion_->tileSize(), 0);

  const std::vector<RoutingCongestion::GCell> gcells
      = congestion_->gcellsByCongestion(core(), core().center());
  ASSERT_FALSE(gcells.empty());

  float previous = -1.0f;
  for (const RoutingCongestion::GCell& gcell : gcells) {
    EXPECT_GE(gcell.congestion, previous);
    EXPECT_GE(gcell.congestion, 0.0f);
    EXPECT_LE(gcell.congestion, 1.0f);
    previous = gcell.congestion;
  }

  // A map pinned at one value would pass every check above while telling a
  // caller nothing about where to put anything. This is the check that would
  // fail if CUGR wrote capacity for both directions, or usage without the
  // blockage term: the ratios would collapse toward a constant.
  EXPECT_LT(gcells.front().congestion, gcells.back().congestion);
  EXPECT_GT(gcells.back().congestion, 0.5f);
  logger_.report("cugr congestion range over the core: {:.3f} .. {:.3f}",
                 gcells.front().congestion,
                 gcells.back().congestion);
}

}  // namespace
}  // namespace grt
