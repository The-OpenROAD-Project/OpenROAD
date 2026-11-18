// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "PdnTest.h"
#include "domain.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "pdn/PdnGen.hh"
#include "polygon.h"
#include "tst/physical_builder.h"

namespace pdn {
namespace {

// Fixture for VoltageDomain tests. Creates power/ground nets and a
// floorplan area so getDomainArea/getRows have something to work with.
class TestDomain : public PdnTest
{
 protected:
  void SetUp() override
  {
    PdnTest::SetUp();
    power_ = odb::dbNet::create(block(), "VDD");
    power_->setSigType(odb::dbSigType::POWER);
    power_->setSpecial();
    ground_ = odb::dbNet::create(block(), "VSS");
    ground_->setSigType(odb::dbSigType::GROUND);
    ground_->setSpecial();
    block()->setDieArea(odb::Rect(0, 0, 10000, 10000));
    block()->setCoreArea(odb::Rect(100, 100, 9900, 9900));
    pdngen_ = std::make_unique<PdnGen>(db_.get(), getLogger());
  }

  // A row of `num_sites` 100 x 1000 core sites starting at (x, y).
  odb::dbRow* addCoreRow(int x, int y, int num_sites)
  {
    odb::dbSite* site = db_->findLib("lib")->findSite("row_site");
    if (site == nullptr) {
      site = odb::dbSite::create(db_->findLib("lib"), "row_site");
      site->setWidth(100);
      site->setHeight(1000);
      site->setClass(odb::dbSiteClass::CORE);
    }
    const std::string name
        = "row_" + std::to_string(x) + "_" + std::to_string(y);
    return odb::dbRow::create(block(),
                              name.c_str(),
                              site,
                              x,
                              y,
                              odb::dbOrientType::R0,
                              odb::dbRowDir::HORIZONTAL,
                              num_sites,
                              site->getWidth());
  }

  odb::dbInst* placeInst(odb::dbMaster* master,
                         const char* name,
                         int x,
                         int y,
                         odb::dbPlacementStatus status
                         = odb::dbPlacementStatus::FIRM)
  {
    odb::dbInst* inst = odb::dbInst::create(block(), master, name);
    inst->setLocation(x, y);
    inst->setPlacementStatus(status);
    return inst;
  }

  std::unique_ptr<PdnGen> pdngen_;
  odb::dbNet* power_ = nullptr;
  odb::dbNet* ground_ = nullptr;
};

// -------- Core (named) constructor --------

// The core 6-arg constructor stores all of its arguments and sets the
// name to "Core".
TEST_F(TestDomain, CoreConstructorStoresValuesAndDefaultName)
{
  VoltageDomain d(pdngen_.get(),
                  block(),
                  power_,
                  ground_,
                  /*secondary=*/{},
                  getLogger());
  EXPECT_EQ(d.getName(), "Core");
  EXPECT_EQ(d.getBlock(), block());
  EXPECT_EQ(d.getLogger(), getLogger());
  EXPECT_EQ(d.getPDNGen(), pdngen_.get());
  EXPECT_EQ(d.getPower(), power_);  // no switched -> falls through
  EXPECT_EQ(d.getAlwaysOnPower(), power_);
  EXPECT_EQ(d.getGround(), ground_);
  EXPECT_EQ(d.getSwitchedPower(), nullptr);
  EXPECT_FALSE(d.hasSwitchedPower());
  EXPECT_FALSE(d.hasRegion());
  EXPECT_EQ(d.getRegion(), nullptr);
}

// -------- Named region constructor --------

// The 7-arg constructor stores the supplied name and region.
TEST_F(TestDomain, NamedConstructorStoresRegion)
{
  odb::dbRegion* region = odb::dbRegion::create(block(), "macroDom");
  odb::dbBox::create(region, /*x1=*/0, /*y1=*/0, /*x2=*/100, /*y2=*/100);

  VoltageDomain d(pdngen_.get(),
                  "macroDom",
                  block(),
                  power_,
                  ground_,
                  /*secondary=*/{},
                  region,
                  getLogger());
  EXPECT_EQ(d.getName(), "macroDom");
  EXPECT_TRUE(d.hasRegion());
  EXPECT_EQ(d.getRegion(), region);
}

// A region with zero boundary rectangles makes the constructor log an
// error (which throws).
TEST_F(TestDomain, NamedConstructorThrowsOnEmptyRegion)
{
  odb::dbRegion* region = odb::dbRegion::create(block(), "emptyDom");
  // No dbBox::create() call -> region has 0 rects.
  EXPECT_THROW(VoltageDomain(pdngen_.get(),
                             "emptyDom",
                             block(),
                             power_,
                             ground_,
                             /*secondary=*/{},
                             region,
                             getLogger()),
               std::runtime_error);
}

// A region with multiple boundary rectangles is rejected with an error.
TEST_F(TestDomain, NamedConstructorThrowsOnMultiRectRegion)
{
  odb::dbRegion* region = odb::dbRegion::create(block(), "multiDom");
  odb::dbBox::create(region, 0, 0, 100, 100);
  odb::dbBox::create(region, 200, 200, 300, 300);
  EXPECT_THROW(VoltageDomain(pdngen_.get(),
                             "multiDom",
                             block(),
                             power_,
                             ground_,
                             /*secondary=*/{},
                             region,
                             getLogger()),
               std::runtime_error);
}

// -------- determinePowerGroundNets --------

// When power is supplied but ground is null, the constructor finds the
// ground net by sig type and logs an info message.
TEST_F(TestDomain, CoreConstructorFindsGroundFromTypeWhenNull)
{
  VoltageDomain d(
      pdngen_.get(), block(), power_, /*ground=*/nullptr, {}, getLogger());
  EXPECT_EQ(d.getGround(), ground_);
}

// And the symmetric case: ground supplied, power null -> looked up.
TEST_F(TestDomain, CoreConstructorFindsPowerFromTypeWhenNull)
{
  VoltageDomain d(
      pdngen_.get(), block(), /*power=*/nullptr, ground_, {}, getLogger());
  EXPECT_EQ(d.getAlwaysOnPower(), power_);
}

// When the lookup finds zero matching nets, the constructor errors.
// We force this by retyping the fixture's POWER net to SIGNAL so the
// block contains no power-typed nets at all.
TEST_F(TestDomain, CoreConstructorThrowsWhenNoPowerNet)
{
  power_->setSigType(odb::dbSigType::SIGNAL);  // no POWER nets remain
  EXPECT_THROW(
      VoltageDomain(
          pdngen_.get(), block(), /*power=*/nullptr, ground_, {}, getLogger()),
      std::runtime_error);
}

// When the lookup finds multiple matching nets, the constructor errors.
// Add a second POWER-typed net so findDomainNet returns more than one.
TEST_F(TestDomain, CoreConstructorThrowsWhenMultiplePowerNets)
{
  odb::dbNet* p2 = odb::dbNet::create(block(), "VDD2");
  p2->setSigType(odb::dbSigType::POWER);
  EXPECT_THROW(
      VoltageDomain(
          pdngen_.get(), block(), /*power=*/nullptr, ground_, {}, getLogger()),
      std::runtime_error);
}

// -------- getNets ordering --------

// start_with_power=true puts power first, then switched, then ground.
TEST_F(TestDomain, GetNetsStartWithPowerOrder)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  const auto nets = d.getNets(/*start_with_power=*/true);
  ASSERT_EQ(nets.size(), 2u);
  EXPECT_EQ(nets[0], power_);
  EXPECT_EQ(nets[1], ground_);
}

// start_with_power=false swaps the order.
TEST_F(TestDomain, GetNetsStartWithGroundOrder)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  const auto nets = d.getNets(/*start_with_power=*/false);
  ASSERT_EQ(nets.size(), 2u);
  EXPECT_EQ(nets[0], ground_);
  EXPECT_EQ(nets[1], power_);
}

// Secondary nets are appended at the end regardless of order.
TEST_F(TestDomain, GetNetsAppendsSecondaryNets)
{
  odb::dbNet* sec = odb::dbNet::create(block(), "SEC");
  sec->setSigType(odb::dbSigType::POWER);
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {sec}, getLogger());
  const auto nets_p = d.getNets(true);
  ASSERT_EQ(nets_p.size(), 3u);
  EXPECT_EQ(nets_p.back(), sec);
  const auto nets_g = d.getNets(false);
  ASSERT_EQ(nets_g.size(), 3u);
  EXPECT_EQ(nets_g.back(), sec);
}

// When a switched power net is set, it appears between power and ground
// (start_with_power=true).
TEST_F(TestDomain, GetNetsIncludesSwitchedPowerStartWithPower)
{
  odb::dbNet* sw = odb::dbNet::create(block(), "VDDS");
  sw->setSigType(odb::dbSigType::POWER);
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  d.setSwitchedPower(sw);
  EXPECT_TRUE(d.hasSwitchedPower());
  EXPECT_EQ(d.getSwitchedPower(), sw);

  const auto nets = d.getNets(/*start_with_power=*/true);
  ASSERT_EQ(nets.size(), 3u);
  EXPECT_EQ(nets[0], power_);
  EXPECT_EQ(nets[1], sw);
  EXPECT_EQ(nets[2], ground_);
}

// And the symmetric start_with_power=false ordering.
TEST_F(TestDomain, GetNetsIncludesSwitchedPowerStartWithGround)
{
  odb::dbNet* sw = odb::dbNet::create(block(), "VDDS2");
  sw->setSigType(odb::dbSigType::POWER);
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  d.setSwitchedPower(sw);

  const auto nets = d.getNets(/*start_with_power=*/false);
  ASSERT_EQ(nets.size(), 3u);
  EXPECT_EQ(nets[0], ground_);
  EXPECT_EQ(nets[1], power_);
  EXPECT_EQ(nets[2], sw);
}

// getPower returns switched_power_ when set, else power_.
TEST_F(TestDomain, GetPowerReturnsSwitchedWhenSet)
{
  odb::dbNet* sw = odb::dbNet::create(block(), "VDDS3");
  sw->setSigType(odb::dbSigType::POWER);
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_EQ(d.getPower(), power_);
  d.setSwitchedPower(sw);
  EXPECT_EQ(d.getPower(), sw);
}

// -------- Grid management --------

// addGrid stores the grid; getGrids returns the stored grids in order.
TEST_F(TestDomain, AddGridAppendsAndGetGridsReturnsThem)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_TRUE(d.getGrids().empty());

  auto grid = std::make_unique<TestGrid>(
      &d, "g1", /*starts_with_power=*/true, std::vector<odb::dbTechLayer*>{});
  Grid* raw = grid.get();
  d.addGrid(std::move(grid));
  ASSERT_EQ(d.getGrids().size(), 1u);
  EXPECT_EQ(d.getGrids().front().get(), raw);
}

// clearGrids removes all grids.
TEST_F(TestDomain, ClearGridsEmptiesTheVector)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  d.addGrid(std::make_unique<TestGrid>(
      &d, "g", true, std::vector<odb::dbTechLayer*>{}));
  ASSERT_EQ(d.getGrids().size(), 1u);
  d.clearGrids();
  EXPECT_TRUE(d.getGrids().empty());
}

// removeGrid removes the specific grid by pointer.
TEST_F(TestDomain, RemoveGridErasesMatchingGrid)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  auto g1 = std::make_unique<TestGrid>(
      &d, "g1", true, std::vector<odb::dbTechLayer*>{});
  Grid* g1_raw = g1.get();
  auto g2 = std::make_unique<TestGrid>(
      &d, "g2", true, std::vector<odb::dbTechLayer*>{});
  Grid* g2_raw = g2.get();
  d.addGrid(std::move(g1));
  d.addGrid(std::move(g2));
  ASSERT_EQ(d.getGrids().size(), 2u);
  d.removeGrid(g1_raw);
  ASSERT_EQ(d.getGrids().size(), 1u);
  EXPECT_EQ(d.getGrids().front().get(), g2_raw);
}

// -------- getDomainArea --------

// Without a region, getDomainArea returns the block's core area.
TEST_F(TestDomain, GetDomainAreaWithoutRegionReturnsCoreArea)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_EQ(d.getDomainArea(), block()->getCoreArea());
}

// With a region, getDomainArea returns the region's first boundary rect.
TEST_F(TestDomain, GetDomainAreaWithRegionReturnsRegionRect)
{
  odb::dbRegion* region = odb::dbRegion::create(block(), "areaDom");
  odb::dbBox::create(region,
                     /*x1=*/200,
                     /*y1=*/300,
                     /*x2=*/800,
                     /*y2=*/700);
  VoltageDomain d(pdngen_.get(),
                  "areaDom",
                  block(),
                  power_,
                  ground_,
                  {},
                  region,
                  getLogger());
  EXPECT_EQ(d.getDomainArea(), odb::Rect(200, 300, 800, 700));
}

// -------- getDomainRegion --------

// A region domain is the region's rectangle.
TEST_F(TestDomain, GetDomainRegionWithRegionIsTheRegionRect)
{
  odb::dbRegion* region = odb::dbRegion::create(block(), "areaDom");
  odb::dbBox::create(region, 200, 300, 800, 700);
  addCoreRow(0, 0, 40);
  VoltageDomain d(pdngen_.get(),
                  "areaDom",
                  block(),
                  power_,
                  ground_,
                  {},
                  region,
                  getLogger());
  EXPECT_THAT(d.getDomainRegion().getRects(),
              ::testing::ElementsAre(odb::Rect(200, 300, 800, 700)));
}

// Pad rows are not core, so with only those there is nothing but the core
// bounding box to go on.
TEST_F(TestDomain, GetDomainRegionWithoutCoreRowsIsTheCoreArea)
{
  odb::dbSite* pad_site = odb::dbSite::create(db_->findLib("lib"), "pad");
  pad_site->setWidth(500);
  pad_site->setHeight(500);
  pad_site->setClass(odb::dbSiteClass::PAD);
  odb::dbRow::create(block(),
                     "pad_row",
                     pad_site,
                     0,
                     0,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     1,
                     500);

  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_THAT(d.getDomainRegion().getRects(),
              ::testing::ElementsAre(block()->getCoreArea()));
}

// Rows that leave a notch in a corner of the core describe an L-shaped core,
// and the region follows them rather than the core bounding box.
TEST_F(TestDomain, GetDomainRegionFollowsTheRows)
{
  const odb::Rect core(0, 0, 4000, 4000);
  block()->setCoreArea(core);
  addCoreRow(0, 0, 40);
  addCoreRow(0, 1000, 40);
  addCoreRow(0, 2000, 40);
  addCoreRow(0, 3000, 20);

  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  const Region region = d.getDomainRegion();
  EXPECT_EQ(region.getPolygons().size(), 1u);
  EXPECT_EQ(region.getEnclosingRect(), core);
  EXPECT_TRUE(region.contains(Region(odb::Rect(0, 0, 4000, 3000))));
  EXPECT_FALSE(region.contains(Region(odb::Rect(2000, 3000, 4000, 4000))));
}

// A fixed macro standing where the rows stop is core too. A macro that is
// only placed, or a fixed instance that is not a macro, is not.
TEST_F(TestDomain, GetDomainRegionCountsFixedMacrosAsCore)
{
  const odb::Rect core(0, 0, 4000, 4000);
  block()->setCoreArea(core);
  addCoreRow(0, 0, 40);
  addCoreRow(0, 1000, 40);
  addCoreRow(0, 2000, 40);
  addCoreRow(0, 3000, 20);
  odb::dbMaster* macro = tst::makeMaster(
      db_->findLib("lib"), "macro", 2000, 1000, {}, odb::dbMasterType::BLOCK);
  odb::dbMaster* cell
      = tst::makeMaster(db_->findLib("lib"), "cell", 2000, 1000, {});
  odb::dbInst* inst
      = placeInst(macro, "macro", 2000, 3000, odb::dbPlacementStatus::PLACED);
  placeInst(cell, "cell", 2000, 3000);

  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  const Region notch(odb::Rect(2000, 3000, 4000, 4000));
  EXPECT_FALSE(d.getDomainRegion().contains(notch));

  inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  EXPECT_THAT(d.getDomainRegion().getRects(), ::testing::ElementsAre(core));
}

// Rows that stop short of a side of the core, or that fall apart into
// separate pieces, do not describe it, so the core bounding box is used.
TEST_F(TestDomain, GetDomainRegionFallsBackWhenRowsDoNotDescribeTheCore)
{
  const odb::Rect core(0, 0, 4000, 4000);
  block()->setCoreArea(core);
  addCoreRow(0, 0, 40);
  addCoreRow(0, 1000, 40);

  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_THAT(d.getDomainRegion().getRects(), ::testing::ElementsAre(core));

  addCoreRow(0, 3000, 40);
  EXPECT_THAT(d.getDomainRegion().getRects(), ::testing::ElementsAre(core));
}

// -------- pad ring inner area --------

// The pads on each side of the core bound the pad ring from the inside.
// Corner pads, area IO pads, unplaced pads and non-pad instances say nothing
// about it.
TEST_F(TestDomain, PadRingInnerAreaIsBoundedByThePadsOnEachSide)
{
  block()->setCoreArea(odb::Rect(2000, 2000, 8000, 8000));
  odb::dbLib* lib = db_->findLib("lib");
  odb::dbMaster* pad
      = tst::makeMaster(lib, "pad", 1000, 1000, {}, odb::dbMasterType::PAD);
  odb::dbMaster* area_io = tst::makeMaster(
      lib, "area_io", 1000, 1000, {}, odb::dbMasterType::PAD_AREAIO);
  odb::dbMaster* cell = tst::makeMaster(lib, "cell", 1000, 1000, {});

  placeInst(pad, "north", 4000, 9000);
  placeInst(pad, "south", 4000, 0);
  placeInst(pad, "west", 500, 4000);
  placeInst(pad, "east", 8500, 4000);
  placeInst(pad, "corner", 0, 0);
  placeInst(pad, "unplaced", 4000, 8100, odb::dbPlacementStatus::UNPLACED);
  placeInst(area_io, "area_io", 4000, 8100);
  placeInst(cell, "cell", 4000, 8100);

  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_EQ(d.getPadRingInnerArea(), odb::Rect(1500, 1000, 8500, 9000));
}

// With no pad on any side of the core there is no pad ring.
TEST_F(TestDomain, PadRingInnerAreaIsUnsetWithoutSidePads)
{
  block()->setCoreArea(odb::Rect(2000, 2000, 8000, 8000));
  odb::dbMaster* pad = tst::makeMaster(
      db_->findLib("lib"), "pad", 1000, 1000, {}, odb::dbMasterType::PAD);
  placeInst(pad, "corner", 0, 0);

  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_FALSE(d.getPadRingInnerArea().has_value());
}

// -------- getRows --------

// With no rows on the block, getRows returns empty.
TEST_F(TestDomain, GetRowsEmptyBlock)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_TRUE(d.getRows().empty());
}

// getRows skips PAD rows and includes core rows.
TEST_F(TestDomain, GetRowsSkipsPadRows)
{
  odb::dbSite* core_site = odb::dbSite::create(db_->findLib("lib"), "core");
  core_site->setWidth(200);
  core_site->setHeight(2800);
  core_site->setClass(odb::dbSiteClass::CORE);
  odb::dbRow::create(block(),
                     "core_row",
                     core_site,
                     100,
                     100,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     /*num_sites=*/10,
                     /*spacing=*/200);

  odb::dbSite* pad_site = odb::dbSite::create(db_->findLib("lib"), "pad");
  pad_site->setWidth(500);
  pad_site->setHeight(500);
  pad_site->setClass(odb::dbSiteClass::PAD);
  odb::dbRow::create(block(),
                     "pad_row",
                     pad_site,
                     0,
                     0,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     1,
                     500);

  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  const auto rows = d.getRows();
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows.front()->getName(), "core_row");
}

// getRows with a region returns rows that overlap the region.
TEST_F(TestDomain, GetRowsWithRegionFiltersByOverlap)
{
  odb::dbSite* core_site = odb::dbSite::create(db_->findLib("lib"), "rcore");
  core_site->setWidth(200);
  core_site->setHeight(100);
  core_site->setClass(odb::dbSiteClass::CORE);
  // Row inside the region.
  odb::dbRow::create(block(),
                     "in",
                     core_site,
                     100,
                     100,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     5,
                     200);
  // Row outside the region.
  odb::dbRow::create(block(),
                     "out",
                     core_site,
                     5000,
                     5000,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     5,
                     200);

  odb::dbRegion* region = odb::dbRegion::create(block(), "regdom");
  odb::dbBox::create(region, 0, 0, 2000, 2000);
  VoltageDomain d(pdngen_.get(),
                  "regdom",
                  block(),
                  power_,
                  ground_,
                  {},
                  region,
                  getLogger());
  const auto rows = d.getRows();
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows.front()->getName(), "in");
}

// getRegionRows skips PAD-class rows the same way the core-domain path
// does. With one PAD row overlapping the region and one CORE row, only
// the CORE row should be returned.
TEST_F(TestDomain, GetRegionRowsSkipsPadRows)
{
  odb::dbSite* core_site = odb::dbSite::create(db_->findLib("lib"), "regcore");
  core_site->setWidth(200);
  core_site->setHeight(100);
  core_site->setClass(odb::dbSiteClass::CORE);
  odb::dbSite* pad_site = odb::dbSite::create(db_->findLib("lib"), "regpad");
  pad_site->setWidth(500);
  pad_site->setHeight(500);
  pad_site->setClass(odb::dbSiteClass::PAD);

  odb::dbRow::create(block(),
                     "rcore",
                     core_site,
                     100,
                     100,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     2,
                     200);
  odb::dbRow::create(block(),
                     "rpad",
                     pad_site,
                     200,
                     200,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     1,
                     500);
  odb::dbRegion* region = odb::dbRegion::create(block(), "padfilter");
  odb::dbBox::create(region, 0, 0, 5000, 5000);
  VoltageDomain d(pdngen_.get(),
                  "padfilter",
                  block(),
                  power_,
                  ground_,
                  {},
                  region,
                  getLogger());
  const auto rows = d.getRows();
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows.front()->getName(), "rcore");
}

// When the core domain is registered with the PdnGen, getDomainRows
// iterates pdngen->getDomains() and skips the domain that == this.
TEST_F(TestDomain, GetRowsCoreDomainSkipsItselfInPdnGen)
{
  // Register a core domain with the pdngen. getDomains() will then
  // include this very instance.
  pdngen_->setCoreDomain(power_, /*switched=*/nullptr, ground_, {});
  VoltageDomain* core = pdngen_->getDomains().front();
  ASSERT_NE(core, nullptr);

  // Add one core row so getRows has something to consider.
  odb::dbSite* site = odb::dbSite::create(db_->findLib("lib"), "selfsite");
  site->setWidth(200);
  site->setHeight(100);
  site->setClass(odb::dbSiteClass::CORE);
  odb::dbRow::create(block(),
                     "self_row",
                     site,
                     100,
                     100,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     2,
                     200);

  const auto rows = core->getRows();
  EXPECT_EQ(rows.size(), 1u);
}

// When the block contains a region with zero boundary rectangles, the
// core domain's getRows -> getDomainRows -> getRegionBoundary call
// exercises the empty-boundary fall-through (returning {}). The region
// is just registered in the block; we don't have to wrap it in a domain.
TEST_F(TestDomain, GetRowsHandlesBlockRegionWithoutBoundaries)
{
  odb::dbRegion::create(block(), "empty_region");  // no boundary
  odb::dbSite* site = odb::dbSite::create(db_->findLib("lib"), "drsite");
  site->setWidth(200);
  site->setHeight(100);
  site->setClass(odb::dbSiteClass::CORE);
  odb::dbRow::create(block(),
                     "drow",
                     site,
                     100,
                     100,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     2,
                     200);
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_NO_THROW(d.getRows());
}

// A core-domain (no region) excludes rows already claimed by another
// region domain registered with the PdnGen.
TEST_F(TestDomain, GetRowsCoreDomainExcludesClaimedRows)
{
  odb::dbSite* core_site = odb::dbSite::create(db_->findLib("lib"), "csite");
  core_site->setWidth(200);
  core_site->setHeight(100);
  core_site->setClass(odb::dbSiteClass::CORE);
  odb::dbRow::create(block(),
                     "claimed",
                     core_site,
                     100,
                     100,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     2,
                     200);
  odb::dbRow::create(block(),
                     "free",
                     core_site,
                     5000,
                     5000,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     2,
                     200);

  // Register a region domain with the PdnGen that owns the "claimed" row.
  odb::dbRegion* region = odb::dbRegion::create(block(), "claimsRegion");
  odb::dbBox::create(region, 0, 0, 1000, 1000);
  pdngen_->makeRegionVoltageDomain(
      "claimsRegion", power_, /*sw=*/nullptr, ground_, {}, region);

  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  const auto rows = d.getRows();
  // Only "free" row remains for the core domain.
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows.front()->getName(), "free");
}

// -------- getRegionRectCount / getRegionBoundary (via getDomainArea) --------

// getDomainArea on a domain whose region had a single rect returns
// exactly that rect (covered above), and a region created with a
// removed/empty boundary just returns an inverted/default Rect.
// (Behavior is reachable through getDomainArea even though
// getRegionBoundary is private.)
// -- additional coverage via the empty-region constructor error path
// already exercised by NamedConstructorThrowsOnEmptyRegion above.

// -------- report / checkSetup (no-throw smoke tests) --------

// report on a bare domain logs without error.
TEST_F(TestDomain, ReportEmitsLines)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_NO_THROW(d.report());
}

// report with switched power + secondary + region exercises every
// branch of the reporting code.
TEST_F(TestDomain, ReportEmitsAllOptionalFields)
{
  odb::dbNet* sw = odb::dbNet::create(block(), "VDDSW");
  sw->setSigType(odb::dbSigType::POWER);
  odb::dbNet* sec = odb::dbNet::create(block(), "SEC2");
  sec->setSigType(odb::dbSigType::POWER);
  odb::dbRegion* region = odb::dbRegion::create(block(), "fullDom");
  odb::dbBox::create(region, 0, 0, 100, 100);
  VoltageDomain d(pdngen_.get(),
                  "fullDom",
                  block(),
                  power_,
                  ground_,
                  {sec},
                  region,
                  getLogger());
  d.setSwitchedPower(sw);
  EXPECT_NO_THROW(d.report());
  // Also include at least one grid so report() walks the grids loop.
  d.addGrid(std::make_unique<TestGrid>(
      &d, "g", true, std::vector<odb::dbTechLayer*>{}));
  EXPECT_NO_THROW(d.report());
}

// checkSetup on a bare domain (no grids) is a no-op.
TEST_F(TestDomain, CheckSetupOnEmptyDomainIsNoOp)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  EXPECT_NO_THROW(d.checkSetup());
}

// checkSetup forwards to each grid's checkSetup.
TEST_F(TestDomain, CheckSetupDelegatesToGrids)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  d.addGrid(std::make_unique<TestGrid>(
      &d, "g", true, std::vector<odb::dbTechLayer*>{}));
  EXPECT_NO_THROW(d.checkSetup());
}

// -------- resetGrids --------

// resetGrids forwards to each grid's resetShapes (no-op for a fresh
// TestGrid).
TEST_F(TestDomain, ResetGridsForwardsToEachGrid)
{
  VoltageDomain d(pdngen_.get(), block(), power_, ground_, {}, getLogger());
  d.addGrid(std::make_unique<TestGrid>(
      &d, "g", true, std::vector<odb::dbTechLayer*>{}));
  EXPECT_NO_THROW(d.resetGrids());
}

// -------- PdnGen::writeToDb BTerm resolution --------
//
// PdnGen resolves one BTerm per power/ground net up front and hands it down
// to Shape::writeToDb; Shape no longer looks one up itself. These cover that
// resolution -- reuse, reconnect and the name-collision error.

// With no BTerm on the net, PdnGen creates one named after the net. It is
// INOUT, special, and carries the net's signal type. Nothing was written to
// it here (no grids), so the cleanup pass removes it again.
TEST_F(TestDomain, WriteToDbCreatesAndThenDropsAnUnusedBTerm)
{
  pdngen_->setCoreDomain(power_, /*switched=*/nullptr, ground_, {});
  ASSERT_EQ(power_->getBTermCount(), 0u);

  pdngen_->writeToDb(/*add_pins=*/true);

  // Created, then destroyed because no BPin was ever attached to it.
  EXPECT_EQ(power_->getBTermCount(), 0u);
}

// An existing BTerm on the net is reused rather than replaced, and is
// normalized to the net's signal type + special.
TEST_F(TestDomain, WriteToDbReusesAnExistingBTermOnTheNet)
{
  odb::dbBTerm* existing = odb::dbBTerm::create(power_, "VDD");
  pdngen_->setCoreDomain(power_, /*switched=*/nullptr, ground_, {});

  pdngen_->writeToDb(/*add_pins=*/true);

  ASSERT_EQ(power_->getBTermCount(), 1u);
  EXPECT_EQ(power_->get1stBTerm(), existing);
  EXPECT_EQ(existing->getSigType(), odb::dbSigType::POWER);
  EXPECT_TRUE(existing->isSpecial());
}

// A BTerm with the net's name exists in the block but is disconnected: it is
// re-attached to the net rather than duplicated. Being found (not created)
// it also survives the unused-BTerm cleanup.
TEST_F(TestDomain, WriteToDbReconnectsADanglingBTerm)
{
  odb::dbNet* tmp = odb::dbNet::create(block(), "TMP");
  odb::dbBTerm* dangling = odb::dbBTerm::create(tmp, "VDD");
  dangling->disconnect();
  ASSERT_EQ(dangling->getNet(), nullptr);
  ASSERT_EQ(power_->getBTermCount(), 0u);

  pdngen_->setCoreDomain(power_, /*switched=*/nullptr, ground_, {});
  pdngen_->writeToDb(/*add_pins=*/true);

  EXPECT_EQ(dangling->getNet(), power_);
}

// A BTerm with the net's name already belongs to a *different* net -- that
// is unrecoverable, so PDN-214 is raised.
TEST_F(TestDomain, WriteToDbErrorsOnBTermNameCollision)
{
  odb::dbNet* foreign = odb::dbNet::create(block(), "FOO");
  odb::dbBTerm::create(foreign, "VDD");
  pdngen_->setCoreDomain(power_, /*switched=*/nullptr, ground_, {});

  EXPECT_THROW(pdngen_->writeToDb(/*add_pins=*/true), std::runtime_error);
}

// Without add_pins the whole BTerm resolution is skipped.
TEST_F(TestDomain, WriteToDbWithoutPinsCreatesNoBTerms)
{
  pdngen_->setCoreDomain(power_, /*switched=*/nullptr, ground_, {});

  pdngen_->writeToDb(/*add_pins=*/false);

  EXPECT_EQ(power_->getBTermCount(), 0u);
  EXPECT_EQ(ground_->getBTermCount(), 0u);
}

}  // namespace
}  // namespace pdn
