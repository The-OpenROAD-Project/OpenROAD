// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "gui/gui.h"

#include "gtest/gtest.h"
#include "gui/heatMap.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace gui {
namespace {

TEST(GuiHeatMapTest, HeadlessLookupSyncsSourcesToCurrentChip)
{
  utl::Logger logger;
  odb::dbDatabase* db = odb::dbDatabase::create();
  ASSERT_NE(db, nullptr);

  Gui* gui = Gui::get();
  gui->init(db, nullptr, &logger);

  HeatMapDataSource* placement = gui->getHeatMap("Placement");
  ASSERT_NE(placement, nullptr);
  EXPECT_EQ(nullptr, placement->getChip());

  odb::dbTech* tech = odb::dbTech::create(db, "tech");
  ASSERT_NE(tech, nullptr);
  odb::dbChip* chip = odb::dbChip::create(db, tech);
  ASSERT_NE(chip, nullptr);
  odb::dbBlock::create(chip, "top");

  EXPECT_EQ(chip, db->getChip());
  EXPECT_EQ(chip, gui->getHeatMap("Placement")->getChip());

  const auto& heat_maps = gui->getHeatMaps();
  ASSERT_FALSE(heat_maps.empty());
  for (auto* heat_map : heat_maps) {
    EXPECT_EQ(chip, heat_map->getChip());
  }
}

}  // namespace
}  // namespace gui
