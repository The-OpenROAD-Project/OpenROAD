// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <string>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "src/gpl/src/nesterovBase.h"
#include "src/gpl/src/placerBase.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace gpl {
namespace {

// Builds a bare core (kRows rows of kRowSites sites) populated with `count`
// movable 2-site-wide cells, then returns what
// NesterovBase::estimateTargetDensity(overflow) reports for it. Each call
// gets its own independent, freshly built database, so a test can call this
// several times (e.g. to compare overflow values, or a clustered layout
// against a spread-out one) without one call's placer state leaking into
// the next.
class EstimateTargetDensityTest : public ::testing::Test
{
 protected:
  static constexpr int kDbuPerMicron = 1000;
  static constexpr int kSiteWidth = 200;
  static constexpr int kRowHeight = 2000;
  static constexpr int kRowSites = 100;
  static constexpr int kRows = 10;
  static constexpr int kCellWidth = 2 * kSiteWidth;

  enum class Layout
  {
    // One cell per row, spread evenly across the full row width.
    kSpread,
    // All cells stacked on top of one another at a single site, creating
    // congestion that cannot be relieved no matter the target density.
    kClustered,
  };

  float estimateDensity(Layout layout, int count, float overflow)
  {
    auto db = utl::UniquePtrWithDeleter<odb::dbDatabase>(
        odb::dbDatabase::create(), odb::dbDatabase::destroy);
    db->setLogger(&logger_);
    db->setDbuPerMicron(kDbuPerMicron);

    odb::dbTech* tech = odb::dbTech::create(db.get(), "tech");
    odb::dbTechLayer::create(tech, "metal1", odb::dbTechLayerType::ROUTING);
    odb::dbLib* lib = odb::dbLib::create(db.get(), "lib", tech);
    odb::dbSite* site = odb::dbSite::create(lib, "site");
    site->setWidth(kSiteWidth);
    site->setHeight(kRowHeight);
    site->setClass(odb::dbSiteClass::CORE);

    odb::dbChip* chip = odb::dbChip::create(db.get(), tech);
    odb::dbBlock* block = odb::dbBlock::create(chip, "top");
    block->setDieArea(odb::Rect(
        0, 0, (kRowSites + 2) * kSiteWidth, (kRows + 2) * kRowHeight));
    for (int row = 0; row < kRows; ++row) {
      odb::dbRow::create(block,
                         ("row" + std::to_string(row)).c_str(),
                         site,
                         kSiteWidth,
                         (row + 1) * kRowHeight,
                         odb::dbOrientType::R0,
                         odb::dbRowDir::HORIZONTAL,
                         kRowSites,
                         kSiteWidth);
    }
    block->setCoreArea(block->computeCoreArea());

    odb::dbMaster* master = odb::dbMaster::create(lib, "core_cell");
    master->setType(odb::dbMasterType(odb::dbMasterType::CORE));
    master->setWidth(kCellWidth);
    master->setHeight(kRowHeight);
    master->setSite(site);
    master->setFrozen();

    for (int i = 0; i < count; ++i) {
      int x = 0;
      int y = 0;
      if (layout == Layout::kSpread) {
        // A grid over every row and evenly spaced columns, so the bin grid
        // (which is much finer than kRows) sees roughly the same density
        // everywhere rather than a handful of occupied rows.
        const int cols = count / kRows;
        const int row = i % kRows;
        const int col = i / kRows;
        x = kSiteWidth * (1 + col * (kRowSites / cols));
        y = kRowHeight * (row + 1);
      } else {
        // Every cell stacked on top of the same site: the bin(s) covering
        // that site hold `count` cells' worth of area, far more than they
        // can ever hold even at full (1.0) density, so relieving the
        // congestion elsewhere in the core cannot make the overflow at that
        // site disappear.
        x = kSiteWidth;
        y = kRowHeight;
      }
      odb::dbInst* inst = odb::dbInst::create(
          block, master, ("u_" + std::to_string(i)).c_str());
      inst->setLocation(x, y);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }

    PlaceOptions options;
    // NesterovBase otherwise randomly perturbs each cell's initial location
    // by up to half a site height, which would scatter the deliberately
    // co-located cells in the kClustered layout across several bins.
    options.initialPlacePerturbationDist = 0.0f;
    const PlacerBaseVars pb_vars(options);
    auto pbc = std::make_shared<PlacerBaseCommon>(db.get(), pb_vars, &logger_);
    auto pb = std::make_shared<PlacerBase>(
        db.get(), pbc, &logger_, /* check_density = */ false);

    const NesterovBaseVars nb_vars(options);
    auto nbc = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc, &logger_, /* num_threads = */ 1, Clusters{});
    NesterovBase nb(nb_vars, pb, nbc, &logger_);

    return nb.estimateTargetDensity(overflow);
  }

  utl::Logger logger_;
};

// The estimate is a density found by binary search between the design's
// uniform target density and full (1.0) density, so it must always land in
// that range regardless of how tight the requested overflow is. A very loose
// overflow request (1.0) converges to the lower bound of that search, so it
// stands in here for the uniform target density itself.
TEST_F(EstimateTargetDensityTest, ResultIsWithinUniformToFullRange)
{
  const float uniform_density = estimateDensity(Layout::kSpread, 100, 1.0f);
  const float loose = estimateDensity(Layout::kSpread, 100, 0.5f);
  const float tight = estimateDensity(Layout::kSpread, 100, 0.01f);

  for (float density : {loose, tight}) {
    EXPECT_GE(density, uniform_density - 1e-3f);
    EXPECT_LE(density, 1.0f);
  }
}

// A design spread out evenly has essentially the same density everywhere, so
// asking for a tight overflow should not need a target density much higher
// than the design's own uniform density: there is no local congestion to
// relieve.
TEST_F(EstimateTargetDensityTest, UniformDesignNeedsLittleAboveUniformDensity)
{
  const float uniform_density = estimateDensity(Layout::kSpread, 100, 1.0f);
  const float tight_density = estimateDensity(Layout::kSpread, 100, 0.02f);

  EXPECT_NEAR(tight_density, uniform_density, 0.1f);
}

// Stacking cells on top of one another creates local congestion far above
// the design-wide average, and no achievable density can fully relieve it,
// so it needs a distinctly higher density than the same cell count spread
// out evenly.
TEST_F(EstimateTargetDensityTest, ClusteredDesignNeedsHigherDensityThanUniform)
{
  const float clustered_density
      = estimateDensity(Layout::kClustered, 40, 0.02f);
  const float spread_density = estimateDensity(Layout::kSpread, 100, 0.02f);

  EXPECT_GT(clustered_density, spread_density);
}

// Tightening the requested overflow can only ask the placer to reserve more
// headroom, never less, so the returned density must be monotonically
// non-decreasing as the overflow argument shrinks.
TEST_F(EstimateTargetDensityTest, DensityIsMonotonicInOverflow)
{
  const float loose = estimateDensity(Layout::kClustered, 40, 0.5f);
  const float medium = estimateDensity(Layout::kClustered, 40, 0.1f);
  const float tight = estimateDensity(Layout::kClustered, 40, 0.01f);

  EXPECT_LE(loose, medium);
  EXPECT_LE(medium, tight);
}

}  // namespace
}  // namespace gpl
