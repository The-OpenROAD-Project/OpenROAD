// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "gtest/gtest.h"
#include "odb/db.h"
#include "src/odb/test/cpp/helper/helper.h"

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

  // Creates a frontside ROUTING layer with no track grid at all.
  dbTechLayer* makeUntrackedRoutingLayer(const char* name)
  {
    dbTechLayer* layer
        = dbTechLayer::create(tech(), name, dbTechLayerType::ROUTING);
    layer->setDirection(dbTechLayerDir::HORIZONTAL);
    return layer;
  }
};

// A BSPDN tech can have a raw max_routing_layer_ at or above the
// baseline branch's threshold while only one frontside layer is actually
// enabled below it. Dispatch must still take the small-stack path here
// and compute from that one enabled layer (M1), not reach past it for
// frontside layers that were never enabled.
//
// 4 backside layers precede M1, putting it at raw level 5.
TEST_F(TestGCellTileSize, UsesSingleEnabledFrontsideLayerRatherThanErroring)
{
  makeBacksideLayer("BPR");
  makeBacksideLayer("BM1");
  makeBacksideLayer("BM2");
  makeBacksideLayer("BRDL");
  dbTechLayer* m1 = makeRoutingLayer("M1", 56);
  ASSERT_EQ(m1->getRoutingLevel(), 5);  // 4 backside + M1
  block()->setMaxRoutingLayer(5);

  EXPECT_EQ(block()->getGCellTileSize(), 56 * 15);
}

// A technology with zero frontside layers enabled at all -- every
// ROUTING layer up to max_routing_layer_ is backside -- leaves
// getAverageTrackSpacing(0) unable to find anything, so this must raise
// ODB-1219 rather than dereferencing a null tech_layer.
TEST_F(TestGCellTileSize, ErrorsWhenNoFrontsideLayerIsEnabledAtAll)
{
  makeBacksideLayer("BPR");
  makeBacksideLayer("BM1");
  block()->setMaxRoutingLayer(2);  // both backside; 0 frontside enabled

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1219";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1219");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1219)";
  }
}

// The split's other error path: tech_layer is found (so it's guaranteed
// non-null when findTrackGrid()/getName() are called), but that layer
// has no track grid built yet. Must raise the pre-existing ODB-0358,
// distinct from ODB-1219 above.
TEST_F(TestGCellTileSize, ErrorsOnFoundLayerWithNoTrackGrid)
{
  makeUntrackedRoutingLayer("M1");
  block()->setMaxRoutingLayer(1);

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-0358";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-0358");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-0358)";
  }
}

// max_routing_layer_ counts backside layers too (e.g. BSPDN
// BPR/BM*/BRDL), but getGCellTileSize()'s branch dispatch and
// getAverageTrackSpacing()'s layer_idx are frontside-only. On a BSPDN
// tech with backside layers ordered ahead of the frontside stack, a raw
// level at or above the baseline branch's >=4 threshold can still leave
// only 2 frontside layers actually enabled; dispatching on the enabled
// count, not the raw level, keeps this from silently pulling M3/M4
// pitches from layers above max_routing_layer_ into the tile size.
//
// 3 backside layers put M1/M2 at raw levels 4/5; max_routing_layer_ is
// set to 5 (M2), enabling only those 2 frontside layers. M3/M4 exist in
// the tech above that but are not enabled.
TEST_F(TestGCellTileSize, DispatchCountsOnlyEnabledFrontsideLayers)
{
  makeBacksideLayer("BPR");
  makeBacksideLayer("BM1");
  makeBacksideLayer("BM2");
  dbTechLayer* m1 = makeRoutingLayer("M1", 50);
  dbTechLayer* m2 = makeRoutingLayer("M2", 56);
  makeRoutingLayer("M3", 90);
  makeRoutingLayer("M4", 100);
  ASSERT_EQ(m1->getRoutingLevel(), 4);  // 3 backside + M1
  ASSERT_EQ(m2->getRoutingLevel(), 5);  // 3 backside + M1 + M2
  block()->setMaxRoutingLayer(5);  // enables M1, M2 only -- 2 frontside layers

  // Only 2 frontside layers are enabled, so this must take the
  // small-stack path and use M2's own pitch (56 * 15 = 840), not fall
  // through to the baseline branch and pull the never-enabled M3/M4
  // (90, 100) into a median instead.
  EXPECT_EQ(block()->getGCellTileSize(), 840);
}

// getGCellTileSize() is a public dbBlock method, callable directly (e.g.
// from Tcl/Python) without ever going through GlobalRouter, which is the
// only caller that guarantees max_routing_layer_ is set before reaching
// here. On a block where it was never set (-1, the default), no raw
// level in the loop above can ever equal it, so this must still raise
// ODB-1219 rather than counting every frontside layer in the tech and
// silently succeeding from the baseline branch.
TEST_F(TestGCellTileSize, ErrorsWhenMaxRoutingLayerWasNeverSet)
{
  makeRoutingLayer("M1", 60);
  makeRoutingLayer("M2", 80);
  makeRoutingLayer("M3", 90);
  makeRoutingLayer("M4", 100);
  // No setMaxRoutingLayer() call: it stays at its default, -1.

  try {
    block()->getGCellTileSize();
    FAIL() << "Expected ODB-1219";
  } catch (const std::exception& e) {
    EXPECT_STREQ(e.what(), "ODB-1219");
  } catch (...) {
    FAIL() << "Unexpected exception (other than ODB-1219)";
  }
}

}  // namespace
}  // namespace odb
