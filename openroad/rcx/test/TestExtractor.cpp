// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "gtest/gtest.h"
#include "odb/db.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

TEST(TestExtractor, InitSearchWithSingleDirectionTrackGrid)
{
  utl::Logger logger;
  odb::dbDatabase* db = odb::dbDatabase::create();
  db->setLogger(&logger);

  odb::dbTech* tech = odb::dbTech::create(db, "tech");

  odb::dbTechLayer* met1
      = odb::dbTechLayer::create(tech, "met1", odb::dbTechLayerType::ROUTING);
  met1->setWidth(100);
  met1->setPitch(200);

  odb::dbTechLayer* met2
      = odb::dbTechLayer::create(tech, "met2", odb::dbTechLayerType::ROUTING);
  met2->setWidth(100);
  met2->setPitch(200);

  odb::dbChip* chip = odb::dbChip::create(db, tech, "chip");
  odb::dbBlock* block = odb::dbBlock::create(chip, "block");
  block->setDieArea(odb::Rect(1000, 2000, 11000, 12000));

  // met1 has tracks only in Y; met2 has tracks only in X.
  odb::dbTrackGrid* met1_grid = odb::dbTrackGrid::create(block, met1);
  met1_grid->addGridPatternY(2500, 10, 200);

  odb::dbTrackGrid* met2_grid = odb::dbTrackGrid::create(block, met2);
  met2_grid->addGridPatternX(1700, 10, 200);

  extMain extractor;
  extractor.init(db, &logger);
  extractor.setBlockFromChip(chip);

  constexpr int kTableSize = 8;
  int x_origins[kTableSize] = {};
  int y_origins[kTableSize] = {};
  uint32_t pitch_table[kTableSize] = {};
  uint32_t width_table[kTableSize] = {};
  uint32_t dir_table[kTableSize] = {};
  odb::Rect ext_rect(0, 0, 0, 0);

  extractor.initSearchForNets(x_origins,
                              y_origins,
                              pitch_table,
                              width_table,
                              dir_table,
                              ext_rect,
                              false);

  // A direction without tracks falls back to the die origin; a direction
  // with tracks anchors at the first track minus half the layer width.
  EXPECT_EQ(x_origins[1], 1000);
  EXPECT_EQ(y_origins[1], 2500 - 100 / 2);

  EXPECT_EQ(x_origins[2], 1700 - 100 / 2);
  EXPECT_EQ(y_origins[2], 2000);
}

}  // namespace rcx
