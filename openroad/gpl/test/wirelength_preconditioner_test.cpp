// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <limits>
#include <memory>
#include <string>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "openroad/gpl/src/nesterovBase.h"
#include "openroad/gpl/src/placerBase.h"
#include "openroad/gpl/src/point.h"
#include "tst/db_fixture.h"

namespace gpl {
namespace {

constexpr float kTolerance = 1e-5f;

class WirelengthPreconditionerTest : public tst::DbFixture
{
 protected:
  static constexpr int kDbuPerMicron = 1000;
  static constexpr int kSiteWidth = 200;
  static constexpr int kRowHeight = 2000;
  static constexpr int kRowSites = 50;
  static constexpr int kRows = 6;

  void SetUp() override
  {
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
    master->setWidth(2 * kSiteWidth);
    master->setHeight(kRowHeight);
    master->setSite(site);
    for (const char* pin_name : {"p0", "p1", "p2"}) {
      odb::dbMTerm::create(
          master, pin_name, odb::dbIoType::INOUT, odb::dbSigType::SIGNAL);
    }
    master->setFrozen();

    // 3 cells connected by 3 nets:
    //   n0: u0.p0 -- u1.p0
    //   n1: u1.p1 -- u2.p0
    //   n2: u0.p1 -- u1.p2 -- u2.p1
    // Pin counts: u0 = 2, u1 = 3, u2 = 2.
    u0_ = odb::dbInst::create(block, master, "u0");
    u1_ = odb::dbInst::create(block, master, "u1");
    u2_ = odb::dbInst::create(block, master, "u2");
    u0_->setLocation(2 * kSiteWidth, 2 * kRowHeight);
    u1_->setLocation(10 * kSiteWidth, 2 * kRowHeight);
    u2_->setLocation(20 * kSiteWidth, 2 * kRowHeight);
    for (odb::dbInst* inst : {u0_, u1_, u2_}) {
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }

    n0_ = odb::dbNet::create(block, "n0");
    n1_ = odb::dbNet::create(block, "n1");
    n2_ = odb::dbNet::create(block, "n2");
    u0_->findITerm("p0")->connect(n0_);
    u1_->findITerm("p0")->connect(n0_);
    u1_->findITerm("p1")->connect(n1_);
    u2_->findITerm("p0")->connect(n1_);
    u0_->findITerm("p1")->connect(n2_);
    u1_->findITerm("p2")->connect(n2_);
    u2_->findITerm("p1")->connect(n2_);

    PlaceOptions options;
    const PlacerBaseVars pb_vars(options);
    pbc_ = std::make_shared<PlacerBaseCommon>(db_.get(), pb_vars, &logger_);
    const NesterovBaseVars nb_vars(options);
    nbc_ = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc_, &logger_, /*num_threads=*/1, Clusters{});
  }

  odb::dbInst* u0_ = nullptr;
  odb::dbInst* u1_ = nullptr;
  odb::dbInst* u2_ = nullptr;
  odb::dbNet* n0_ = nullptr;
  odb::dbNet* n1_ = nullptr;
  odb::dbNet* n2_ = nullptr;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
};

TEST_F(WirelengthPreconditionerTest, DefaultWeightsEqualPinCount)
{
  const GCell* g0 = nbc_->dbToNb(u0_);
  const GCell* g1 = nbc_->dbToNb(u1_);
  const GCell* g2 = nbc_->dbToNb(u2_);
  ASSERT_NE(g0, nullptr);
  ASSERT_NE(g1, nullptr);
  ASSERT_NE(g2, nullptr);

  EXPECT_EQ(nbc_->getWireLengthPreconditioner(g0).x, 2.0f);
  EXPECT_EQ(nbc_->getWireLengthPreconditioner(g0).y, 2.0f);
  EXPECT_EQ(nbc_->getWireLengthPreconditioner(g1).x, 3.0f);
  EXPECT_EQ(nbc_->getWireLengthPreconditioner(g1).y, 3.0f);
  EXPECT_EQ(nbc_->getWireLengthPreconditioner(g2).x, 2.0f);
  EXPECT_EQ(nbc_->getWireLengthPreconditioner(g2).y, 2.0f);

  const GCell filler(/*cx=*/1000, /*cy=*/1000, /*dx=*/400, /*dy=*/2000);
  ASSERT_TRUE(filler.isFiller());
  EXPECT_EQ(nbc_->getWireLengthPreconditioner(&filler).x, 0.0f);
  EXPECT_EQ(nbc_->getWireLengthPreconditioner(&filler).y, 0.0f);
}

TEST_F(WirelengthPreconditionerTest, WeightedNetsSumTotalWeightAndFillerIsZero)
{
  GNet* gn0 = nbc_->dbToNb(n0_);
  GNet* gn1 = nbc_->dbToNb(n1_);
  GNet* gn2 = nbc_->dbToNb(n2_);
  ASSERT_NE(gn0, nullptr);
  ASSERT_NE(gn1, nullptr);
  ASSERT_NE(gn2, nullptr);

  // Total weights: (timing + power) * custom
  // gn0: (3.0 + 0.5) * 1.0 = 3.5
  // gn1: (1.0 + 0.0) * 2.0 = 2.0
  // gn2: (4.0 + 1.0) * 1.5 = 7.5
  gn0->setTimingWeight(3.0f);
  gn0->setPowerWeight(0.5f);
  gn1->setCustomWeight(2.0f);
  gn2->setTimingWeight(4.0f);
  gn2->setPowerWeight(1.0f);
  gn2->setCustomWeight(1.5f);

  nbc_->refreshWireLengthPreconditioners();

  // u0: n0 (3.5) + n2 (7.5) = 11.0
  // u1: n0 (3.5) + n1 (2.0) + n2 (7.5) = 13.0
  // u2: n1 (2.0) + n2 (7.5) = 9.5
  const FloatPoint p0 = nbc_->getWireLengthPreconditioner(nbc_->dbToNb(u0_));
  const FloatPoint p1 = nbc_->getWireLengthPreconditioner(nbc_->dbToNb(u1_));
  const FloatPoint p2 = nbc_->getWireLengthPreconditioner(nbc_->dbToNb(u2_));
  EXPECT_NEAR(p0.x, 11.0f, kTolerance);
  EXPECT_NEAR(p0.y, 11.0f, kTolerance);
  EXPECT_NEAR(p1.x, 13.0f, kTolerance);
  EXPECT_NEAR(p1.y, 13.0f, kTolerance);
  EXPECT_NEAR(p2.x, 9.5f, kTolerance);
  EXPECT_NEAR(p2.y, 9.5f, kTolerance);

  // Fillers created in NesterovBase must retain zero WL preconditioner.
  PlaceOptions options;
  options.density = 0.5f;
  auto pb = std::make_shared<PlacerBase>(
      db_.get(), pbc_, &logger_, /*check_density=*/false);
  NesterovBase nb(NesterovBaseVars(options), pb, nbc_, &logger_);
  int filler_count = 0;
  for (const GCellHandle& handle : nb.getGCells()) {
    if (handle->isFiller()) {
      ++filler_count;
      const FloatPoint fp = nbc_->getWireLengthPreconditioner(handle);
      EXPECT_EQ(fp.x, 0.0f);
      EXPECT_EQ(fp.y, 0.0f);
    }
  }
  EXPECT_GT(filler_count, 0);
}

TEST_F(WirelengthPreconditionerTest, RefreshAfterWeightUpdates)
{
  GNet* gn0 = nbc_->dbToNb(n0_);
  GNet* gn1 = nbc_->dbToNb(n1_);
  GNet* gn2 = nbc_->dbToNb(n2_);

  gn0->setTimingWeight(5.0f);
  nbc_->refreshDeviceNetWeights();
  EXPECT_NEAR(
      nbc_->getWireLengthPreconditioner(nbc_->dbToNb(u0_)).x, 6.0f, kTolerance);
  EXPECT_NEAR(
      nbc_->getWireLengthPreconditioner(nbc_->dbToNb(u1_)).x, 7.0f, kTolerance);
  EXPECT_NEAR(
      nbc_->getWireLengthPreconditioner(nbc_->dbToNb(u2_)).x, 2.0f, kTolerance);

  // Update weights again and refresh through rebuildDeviceState().
  gn0->setTimingWeight(1.0f);
  gn1->setTimingWeight(2.5f);
  gn2->setCustomWeight(3.0f);
  nbc_->rebuildDeviceState();
  // u0: n0 (1.0) + n2 (3.0) = 4.0
  // u1: n0 (1.0) + n1 (2.5) + n2 (3.0) = 6.5
  // u2: n1 (2.5) + n2 (3.0) = 5.5
  EXPECT_NEAR(
      nbc_->getWireLengthPreconditioner(nbc_->dbToNb(u0_)).x, 4.0f, kTolerance);
  EXPECT_NEAR(
      nbc_->getWireLengthPreconditioner(nbc_->dbToNb(u1_)).x, 6.5f, kTolerance);
  EXPECT_NEAR(
      nbc_->getWireLengthPreconditioner(nbc_->dbToNb(u2_)).x, 5.5f, kTolerance);
}

TEST_F(WirelengthPreconditionerTest, GuardsInvalidWeightsAndNullNet)
{
  GNet* gn0 = nbc_->dbToNb(n0_);
  GNet* gn1 = nbc_->dbToNb(n1_);
  GNet* gn2 = nbc_->dbToNb(n2_);

  gn0->setTimingWeight(-2.0f);
  gn1->setTimingWeight(std::numeric_limits<float>::quiet_NaN());
  gn2->setTimingWeight(std::numeric_limits<float>::infinity());
  nbc_->refreshWireLengthPreconditioners();

  for (odb::dbInst* inst : {u0_, u1_, u2_}) {
    const FloatPoint p = nbc_->getWireLengthPreconditioner(nbc_->dbToNb(inst));
    EXPECT_TRUE(std::isfinite(p.x));
    EXPECT_EQ(p.x, 0.0f);
    EXPECT_EQ(p.y, 0.0f);
  }

  // Restore gn2 to a valid weight and detach one pin of u0 from its GNet.
  gn2->setTimingWeight(4.0f);
  GPin* u0_p0 = nbc_->dbToNb(u0_->findITerm("p0"));
  ASSERT_NE(u0_p0, nullptr);
  u0_p0->setGNet(nullptr);
  nbc_->refreshWireLengthPreconditioners();

  // u0: null-net pin (1.0) + n2 (4.0) = 5.0
  const FloatPoint p0 = nbc_->getWireLengthPreconditioner(nbc_->dbToNb(u0_));
  EXPECT_NEAR(p0.x, 5.0f, kTolerance);
  EXPECT_NEAR(p0.y, 5.0f, kTolerance);
}

}  // namespace
}  // namespace gpl
