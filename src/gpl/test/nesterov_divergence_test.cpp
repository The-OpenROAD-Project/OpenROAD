// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <stdexcept>
#include <string>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "src/gpl/src/graphicsNone.h"
#include "src/gpl/src/nesterovBase.h"
#include "src/gpl/src/nesterovPlace.h"
#include "src/gpl/src/placerBase.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace gpl {
namespace {

// Builds the same PlacerBase/NesterovBase/NesterovPlace object graph
// Replace::initNesterovPlace wires up, minus odb/sta/rsz, with every movable
// cell stacked on a single site and an unreachable overflow target. That
// local congestion can never be relieved, so densityPenalty_ (which the main
// loop multiplies by phiCoef every iteration, nesterovBase.cpp:4368) keeps
// escalating until it pushes the wirelength/density gradients to Inf/NaN -
// the same genuine numerical divergence (GPL-305/306) a pathological
// incremental-placement input can hit in phase 1. This mirrors
// Replace::doIncrementalPlace's own locked_options.nesterovPlaceMaxIter cap
// (replace.cpp:196), just uncapped here so the test does not depend on
// exactly how many iterations the escalation takes to overflow a float.
class NesterovDivergenceRecoveryTest : public ::testing::Test
{
 protected:
  static constexpr int kDbuPerMicron = 1000;
  static constexpr int kSiteWidth = 200;
  static constexpr int kRowHeight = 2000;
  static constexpr int kRowSites = 100;
  static constexpr int kRows = 10;
  static constexpr int kCellWidth = 2 * kSiteWidth;
  static constexpr int kCellCount = 80;

  void SetUp() override
  {
    db_ = utl::UniquePtrWithDeleter<odb::dbDatabase>(odb::dbDatabase::create(),
                                                     odb::dbDatabase::destroy);
    db_->setLogger(&logger_);
    db_->setDbuPerMicron(kDbuPerMicron);

    odb::dbTech* tech = odb::dbTech::create(db_.get(), "tech");
    odb::dbTechLayer::create(tech, "metal1", odb::dbTechLayerType::ROUTING);
    odb::dbLib* lib = odb::dbLib::create(db_.get(), "lib", tech);
    odb::dbSite* site = odb::dbSite::create(lib, "site");
    site->setWidth(kSiteWidth);
    site->setHeight(kRowHeight);
    site->setClass(odb::dbSiteClass::CORE);

    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech);
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

    // Every cell stacked on the same site: the bin(s) covering it hold
    // kCellCount cells' worth of area, far more than any achievable density
    // can relieve, so the overflow target below can never be satisfied.
    for (int i = 0; i < kCellCount; ++i) {
      odb::dbInst* inst = odb::dbInst::create(
          block, master, ("u_" + std::to_string(i)).c_str());
      inst->setLocation(kSiteWidth, kRowHeight);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }

    PlaceOptions options;
    options.density = 0.99f;
    options.overflow = 1e-6f;  // unreachable given the clustered layout above
    options.nesterovPlaceMaxIter = 20000;

    const PlacerBaseVars pb_vars(options);
    pbc_ = std::make_shared<PlacerBaseCommon>(db_.get(), pb_vars, &logger_);
    pbVec_.push_back(std::make_shared<PlacerBase>(
        db_.get(), pbc_, &logger_, /* check_density = */ false));

    const NesterovBaseVars nb_vars(options);
    nbc_ = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc_, &logger_, /* num_threads = */ 1, Clusters{});
    nbVec_.push_back(
        std::make_shared<NesterovBase>(nb_vars, pbVec_[0], nbc_, &logger_));

    const NesterovPlaceVars np_vars(options, nbc_->getHpwl());
    np_ = std::make_unique<NesterovPlace>(np_vars,
                                          pbc_,
                                          nbc_,
                                          pbVec_,
                                          nbVec_,
                                          /* rb = */ nullptr,
                                          /* tb = */ nullptr,
                                          /* cb = */ nullptr,
                                          std::make_unique<GraphicsNone>(),
                                          &logger_);
  }

  utl::Logger logger_;
  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;
  std::vector<std::shared_ptr<NesterovBase>> nbVec_;
  std::unique_ptr<NesterovPlace> np_;
};

// Reproduces the incremental-placement recovery bug with a real numerical
// divergence instead of a hand-set flag: when phase 1 diverges,
// Replace::doIncrementalPlace catches the exception and calls
// NesterovPlace::clearDivergence() so phase 2 gets a clean start.
// clearDivergence() only resets NesterovPlace's own
// num_region_diverged_/divergeMsg_/divergeCode_ (nesterovPlace.cpp:591-596);
// it never touches NesterovBase::isDiverged_ on the region that actually
// diverged. The getter nb->isDiverged() just returns that stale flag
// (nesterovBase.h), which doBackTracking() reads unconditionally on every
// call (nesterovPlace.cpp:1012/1026), so phase 2's very first iteration
// would see "diverged" again before it has run any gradient computation of
// its own, and NesterovPlace::doNesterovPlace() would re-throw immediately -
// exactly the "continuing to phase 2 anyway" recovery not actually
// recovering.
TEST_F(NesterovDivergenceRecoveryTest, ClearDivergenceLeavesPerRegionFlagSet)
{
  EXPECT_THROW(np_->doNesterovPlace(), std::runtime_error);

  NesterovBase& nb = *nbVec_[0];
  ASSERT_TRUE(nb.isDiverged())
      << "test setup must drive a real numerical divergence";

  np_->clearDivergence();

  EXPECT_FALSE(nb.isDiverged())
      << "clearDivergence() is supposed to give phase 2 a clean start, but "
         "NesterovBase::isDiverged_ is still set on the region that "
         "diverged in phase 1, so phase 2 will see \"divergence\" again "
         "before it runs a single iteration of its own.";
}

}  // namespace
}  // namespace gpl
