// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <map>
#include <utility>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "tst/db_fixture.h"

namespace odb {
namespace {

class TechLayerSpacingTableFixture : public tst::DbFixture
{
 protected:
  dbTechLayer* createRoutingLayer(const char* name)
  {
    dbTech* tech = dbTech::create(db_.get(), "tech");
    return dbTechLayer::create(tech, name, dbTechLayerType::ROUTING);
  }
};

TEST_F(TechLayerSpacingTableFixture, SpacingTablePrlRuleHandlesLeadingBuckets)
{
  dbTechLayer* layer = createRoutingLayer("M1");
  ASSERT_NE(layer, nullptr);

  dbTechLayerSpacingTablePrlRule* rule
      = dbTechLayerSpacingTablePrlRule::create(layer);
  ASSERT_NE(rule, nullptr);

  const std::map<uint32_t, std::pair<int, int>> within_tbl = {{0, {50, 90}}};
  rule->setTable({100, 200, 400},
                 {0, 300, 600},
                 {{11, 12, 13}, {21, 22, 23}, {31, 32, 33}},
                 within_tbl);

  // Below and at the first width/length thresholds (lower_bound == begin()).
  EXPECT_EQ(rule->getSpacing(50, -10), 11);
  EXPECT_EQ(rule->getSpacing(100, 0), 11);
  EXPECT_EQ(rule->getSpacing(100, 300), 11);
  EXPECT_EQ(rule->getSpacing(200, 0), 11);

  EXPECT_TRUE(rule->hasExceptWithin(50));
  EXPECT_TRUE(rule->hasExceptWithin(100));
  EXPECT_EQ(rule->getExceptWithin(50), (std::pair<int, int>{50, 90}));
  EXPECT_EQ(rule->getExceptWithin(100), (std::pair<int, int>{50, 90}));
  EXPECT_FALSE(rule->hasExceptWithin(201));

  // Interior and upper buckets.
  EXPECT_EQ(rule->getSpacing(201, 0), 21);
  EXPECT_EQ(rule->getSpacing(201, 301), 22);
  EXPECT_EQ(rule->getSpacing(400, 600), 22);
  EXPECT_EQ(rule->getSpacing(401, 601), 33);
}

TEST_F(TechLayerSpacingTableFixture, V55SpacingTableHandlesLeadingBuckets)
{
  dbTechLayer* layer = createRoutingLayer("M2");
  ASSERT_NE(layer, nullptr);

  layer->initV55WidthIndex(3);
  layer->addV55WidthEntry(100);
  layer->addV55WidthEntry(200);
  layer->addV55WidthEntry(400);

  layer->initV55LengthIndex(3);
  layer->addV55LengthEntry(10);
  layer->addV55LengthEntry(300);
  layer->addV55LengthEntry(600);

  layer->initV55SpacingTable(3, 3);
  const uint32_t vals[3][3] = {{11, 12, 13}, {21, 22, 23}, {31, 32, 33}};
  for (uint32_t r = 0; r < 3; ++r) {
    for (uint32_t c = 0; c < 3; ++c) {
      layer->addV55SpacingTableEntry(r, c, vals[r][c]);
    }
  }

  // Negative/zero PRL and positive values below/at first thresholds
  // (lower_bound == begin()).
  EXPECT_EQ(layer->findV55Spacing(50, -10), 11);
  EXPECT_EQ(layer->findV55Spacing(50, 5), 11);
  EXPECT_EQ(layer->findV55Spacing(100, 10), 11);
  EXPECT_EQ(layer->findV55Spacing(100, 300), 11);
  EXPECT_EQ(layer->findV55Spacing(200, 10), 11);

  // Interior and upper buckets.
  EXPECT_EQ(layer->findV55Spacing(201, 10), 21);
  EXPECT_EQ(layer->findV55Spacing(201, 301), 22);
  EXPECT_EQ(layer->findV55Spacing(400, 600), 22);
  EXPECT_EQ(layer->findV55Spacing(401, 601), 33);
}

}  // namespace
}  // namespace odb
