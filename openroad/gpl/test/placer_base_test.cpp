// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <string>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "src/gpl/src/placerBase.h"
#include "tst/db_fixture.h"

namespace gpl {
namespace {

struct MasterTypeCase
{
  odb::dbMasterType::Value type;
  // Whether an instance of this master stands in a row, and therefore has to
  // be part of the placer's world.  Mirrors dbMaster::isCoreAutoPlaceable().
  bool occupies_a_site;
};

// Every dbMasterType, classified.  Core cells, macros and the row based
// endcaps take up a site; the pad ring (pads, bumps, rings and the pre-LEF58
// ENDCAP corner classes, which sit outside the core) does not.
constexpr MasterTypeCase kMasterTypes[] = {
    {odb::dbMasterType::COVER, false},
    {odb::dbMasterType::COVER_BUMP, false},
    {odb::dbMasterType::RING, false},
    {odb::dbMasterType::BLOCK, true},
    {odb::dbMasterType::BLOCK_BLACKBOX, true},
    {odb::dbMasterType::BLOCK_SOFT, true},
    {odb::dbMasterType::PAD, false},
    {odb::dbMasterType::PAD_INPUT, false},
    {odb::dbMasterType::PAD_OUTPUT, false},
    {odb::dbMasterType::PAD_INOUT, false},
    {odb::dbMasterType::PAD_POWER, false},
    {odb::dbMasterType::PAD_SPACER, false},
    {odb::dbMasterType::PAD_AREAIO, false},
    {odb::dbMasterType::CORE, true},
    {odb::dbMasterType::CORE_FEEDTHRU, true},
    {odb::dbMasterType::CORE_TIEHIGH, true},
    {odb::dbMasterType::CORE_TIELOW, true},
    {odb::dbMasterType::CORE_SPACER, true},
    {odb::dbMasterType::CORE_ANTENNACELL, true},
    // gf180's TAP_TAPCELL_ROW_* cells (__filltie).
    {odb::dbMasterType::CORE_WELLTAP, true},
    {odb::dbMasterType::ENDCAP, true},
    // gf180's PHY_EDGE_ROW_* cells (__endcap).
    {odb::dbMasterType::ENDCAP_PRE, true},
    {odb::dbMasterType::ENDCAP_POST, true},
    {odb::dbMasterType::ENDCAP_TOPLEFT, false},
    {odb::dbMasterType::ENDCAP_TOPRIGHT, false},
    {odb::dbMasterType::ENDCAP_BOTTOMLEFT, false},
    {odb::dbMasterType::ENDCAP_BOTTOMRIGHT, false},
    // The LEF58 endcaps, as emitted by tap's place_endcaps.
    {odb::dbMasterType::ENDCAP_LEF58_BOTTOMEDGE, true},
    {odb::dbMasterType::ENDCAP_LEF58_TOPEDGE, true},
    {odb::dbMasterType::ENDCAP_LEF58_LEFTEDGE, true},
    {odb::dbMasterType::ENDCAP_LEF58_RIGHTEDGE, true},
    {odb::dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE, true},
    {odb::dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE, true},
    {odb::dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE, true},
    {odb::dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE, true},
    {odb::dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER, true},
    {odb::dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER, true},
    {odb::dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER, true},
    {odb::dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER, true},
};

// dbMasterType::getString() with the spaces gtest does not allow in a test
// name turned into underscores: "CORE SPACER" -> "CORE_SPACER".
std::string testName(odb::dbMasterType::Value type)
{
  std::string name = odb::dbMasterType(type).getString();
  for (char& c : name) {
    if (c == ' ') {
      c = '_';
    }
  }
  return name;
}

// A bare core: kRows rows of kRowSites sites, plus one movable core cell so
// the block is never instance-less.
class PlacerBaseTest : public tst::DbFixture
{
 protected:
  static constexpr int kDbuPerMicron = 1000;
  static constexpr int kSiteWidth = 200;
  static constexpr int kRowHeight = 2000;
  static constexpr int kRowSites = 100;
  static constexpr int kRows = 10;
  // Two sites wide, one row tall: the shape of gf180's __endcap.
  static constexpr int kEndcapWidth = 2 * kSiteWidth;

  void SetUp() override
  {
    db_->setDbuPerMicron(kDbuPerMicron);
    odb::dbTech* tech = odb::dbTech::create(db_.get(), "tech");
    odb::dbTechLayer::create(tech, "metal1", odb::dbTechLayerType::ROUTING);

    lib_ = odb::dbLib::create(db_.get(), "lib", tech);
    site_ = odb::dbSite::create(lib_, "site");
    site_->setWidth(kSiteWidth);
    site_->setHeight(kRowHeight);
    site_->setClass(odb::dbSiteClass::CORE);

    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");
    // One row of margin around the core so the die contains it.
    block_->setDieArea(odb::Rect(
        0, 0, (kRowSites + 2) * kSiteWidth, (kRows + 2) * kRowHeight));
    for (int row = 0; row < kRows; ++row) {
      odb::dbRow::create(block_,
                         ("row" + std::to_string(row)).c_str(),
                         site_,
                         kSiteWidth,
                         (row + 1) * kRowHeight,
                         odb::dbOrientType::R0,
                         odb::dbRowDir::HORIZONTAL,
                         kRowSites,
                         kSiteWidth);
    }
    block_->setCoreArea(block_->computeCoreArea());

    // A movable core cell, so the block is never instance-less.
    odb::dbInst* core = odb::dbInst::create(
        block_, makeMaster("core_cell", odb::dbMasterType::CORE), "u_core");
    core->setLocation(kSiteWidth * 10, kRowHeight * 5);
    core->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  // Debug aid for a fixture-built design, which no .def/.odb on disk holds:
  //   OR_TEST_DUMP_DB=/tmp/x.odb ./placer_base_test --gtest_filter=<one case>
  //   openroad -gui -no_init  ->  read_db /tmp/x.odb
  void TearDown() override
  {
    const char* path = std::getenv("OR_TEST_DUMP_DB");
    if (path != nullptr) {
      std::ofstream out(path, std::ios::binary);
      db_->write(out);
    }
  }

  odb::dbMaster* makeMaster(const char* name, odb::dbMasterType::Value type)
  {
    odb::dbMaster* master = odb::dbMaster::create(lib_, name);
    master->setType(odb::dbMasterType(type));
    master->setWidth(2 * kSiteWidth);
    master->setHeight(kRowHeight);
    master->setSite(site_);
    master->setFrozen();
    return master;
  }

  odb::dbLib* lib_ = nullptr;
  odb::dbSite* site_ = nullptr;
  odb::dbBlock* block_ = nullptr;
};

// PlacerBaseCommon must pick up every master that takes up a placement site,
// the same set dpl works on.  Endcaps are the interesting case: they are
// neither CORE nor BLOCK, so a placer filtering on those two classes drops
// them.  In gf180 that silently hid all the PHY_EDGE_ROW_* cells (CLASS
// ENDCAP PRE) sitting at both ends of every row, while the tap cells
// (CLASS CORE WELLTAP) were picked up as usual.
class MasterTypeVisibility : public PlacerBaseTest,
                             public testing::WithParamInterface<MasterTypeCase>
{
};

TEST_P(MasterTypeVisibility, InstanceIsSeenByGpl)
{
  const MasterTypeCase& param = GetParam();

  odb::dbInst* inst = odb::dbInst::create(
      block_, makeMaster("test_cell", param.type), "u_test");
  // Placed at the left edge of the bottom row, where tapcell puts endcaps.
  inst->setLocation(kSiteWidth, kRowHeight);
  inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);

  PlaceOptions options;
  PlacerBaseVars vars(options);
  PlacerBaseCommon pbc(db_.get(), vars, &logger_);

  const char* master_class = odb::dbMasterType(param.type).getString();
  Instance* pb_inst = pbc.dbToPb(inst);
  if (param.occupies_a_site) {
    ASSERT_NE(pb_inst, nullptr)
        << "master class " << master_class
        << " takes up a placement site but gpl did not see the instance, so "
           "its area is free for gpl to place cells on";
    EXPECT_TRUE(pb_inst->isFixed())
        << "master class " << master_class
        << " is LOCKED and must be a fixed obstacle for gpl";
  } else {
    EXPECT_EQ(pb_inst, nullptr)
        << "master class " << master_class
        << " never stands in a row and must stay out of gpl";
  }
}

INSTANTIATE_TEST_SUITE_P(
    MasterTypes,
    MasterTypeVisibility,
    testing::ValuesIn(kMasterTypes),
    [](const testing::TestParamInfo<MasterTypeCase>& info) {
      return testName(info.param.type);
    });

// A master class that odb gains but nobody classifies above would silently go
// untested, so pin the table to the enum.
TEST(MasterTypeTable, ClassifiesEveryMasterType)
{
  constexpr int kLastType = odb::dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER;
  ASSERT_EQ(std::size(kMasterTypes), kLastType + 1)
      << "dbMasterType gained or lost a value; classify it in kMasterTypes";

  std::set<int> classified;
  for (const MasterTypeCase& c : kMasterTypes) {
    EXPECT_TRUE(classified.insert(c.type).second)
        << odb::dbMasterType(c.type).getString() << " is listed twice";
  }
  for (int type = 0; type <= kLastType; ++type) {
    EXPECT_EQ(classified.count(type), 1u)
        << "master class "
        << odb::dbMasterType(static_cast<odb::dbMasterType::Value>(type))
               .getString()
        << " is not classified in kMasterTypes";
  }
}

using EndcapObstacles = PlacerBaseTest;

// The consequence of the filter above: an endcap at each row end is area the
// placer cannot fill.  Dropping them let gpl spread cells at the target
// density over sites that were already taken, which dpl then had to undo.
TEST_F(EndcapObstacles, EndcapsAreFixedObstaclesWithArea)
{
  odb::dbMaster* endcap = makeMaster("endcap", odb::dbMasterType::ENDCAP_PRE);
  const int right_x = kSiteWidth + kRowSites * kSiteWidth - kEndcapWidth;
  for (int row = 0; row < kRows; ++row) {
    const int y = (row + 1) * kRowHeight;
    for (const auto& [side, x] :
         {std::pair{"left", kSiteWidth}, std::pair{"right", right_x}}) {
      odb::dbInst* inst = odb::dbInst::create(
          block_,
          endcap,
          ("PHY_EDGE_ROW_" + std::to_string(row) + "_" + side).c_str());
      inst->setLocation(x, y);
      inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
    }
  }

  PlaceOptions options;
  PlacerBaseVars vars(options);
  auto pbc = std::make_shared<PlacerBaseCommon>(db_.get(), vars, &logger_);
  PlacerBase pb(db_.get(), pbc, &logger_, /* check_density = */ false);

  EXPECT_EQ(pb.fixedInsts().size(), 2 * kRows);
  EXPECT_EQ(pb.placeInsts().size(), 1);

  int64_t fixed_area = 0;
  for (Instance* inst : pb.fixedInsts()) {
    fixed_area += inst->getArea();
  }
  const int64_t endcap_area
      = static_cast<int64_t>(2 * kRows) * kEndcapWidth * kRowHeight;
  EXPECT_EQ(fixed_area, endcap_area)
      << "endcap area must be taken out of the area gpl is free to fill";
  EXPECT_LT(fixed_area, pb.getRegionArea());
}

}  // namespace
}  // namespace gpl
