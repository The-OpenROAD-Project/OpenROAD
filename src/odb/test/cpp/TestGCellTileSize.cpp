// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

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
  // never needs a backside layer's track grid, only its presence in the raw
  // getRoutingLevel() numbering.
  dbTechLayer* makeBacksideLayer(const char* name)
  {
    dbTechLayer* layer
        = dbTechLayer::create(tech(), name, dbTechLayerType::ROUTING);
    layer->setBackside(true);
    return layer;
  }
};

// Reproduces a null-pointer dereference in getGCellTileSize()'s
// getAverageTrackSpacing() lambda: it counts frontside (non-backside)
// ROUTING layers to find the Nth one, but the baseline call site
// unconditionally asks for the 2nd/3rd/4th frontside layer regardless of
// how many frontside layers actually exist once max_routing_layer_ is at
// least 4 in the tech's raw (backside-inclusive) numbering. On a
// technology whose leading routing layers are backside (e.g. a BSPDN
// stack), that lookup can fail to find a layer at all, leaving the local
// tech_layer pointer null. The unguarded error-message call then
// dereferences it (tech_layer->getName()) even though the surrounding
// null check already proved it might be null -- crashing on the
// error-reporting path itself rather than raising a proper error.
//
// Here, 4 backside layers precede a single frontside layer (M1), so
// getRoutingLevel() puts M1 at raw level 5 -- past the early-return
// threshold (< 4), so getGCellTileSize() takes the baseline path and asks
// for the 2nd frontside layer, which doesn't exist.
TEST_F(TestGCellTileSize, ErrorsRatherThanCrashesWithTooFewFrontsideLayers)
{
  makeBacksideLayer("BPR");
  makeBacksideLayer("BM1");
  makeBacksideLayer("BM2");
  makeBacksideLayer("BRDL");
  dbTechLayer* m1 = makeRoutingLayer("M1", 56);
  ASSERT_EQ(m1->getRoutingLevel(), 5);  // 4 backside + M1
  block()->setMaxRoutingLayer(5);

  EXPECT_THROW(block()->getGCellTileSize(), std::runtime_error);
}

}  // namespace
}  // namespace odb
