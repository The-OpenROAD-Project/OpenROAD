// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>

#include "db/obj/frTrackPattern.h"
#include "dr/FlexDR.h"
#include "dr/FlexGridGraph.h"
#include "fixture.h"
#include "gtest/gtest.h"

namespace drt {

TEST_F(Fixture, SparseTrackMapsPreserveNativeEdges)
{
  router_cfg->BOTTOM_ROUTING_LAYER = 2;
  router_cfg->TOP_ROUTING_LAYER = 2;
  design->getTopBlock()->addTrackPattern(
      std::make_unique<frTrackPattern>(false, 900, 1, 100, 2));
  FlexDRViaData via_data;
  FlexDRWorker worker(&via_data, design.get(), logger.get(), router_cfg.get());
  const odb::Rect box(0, 0, 1000, 1000);
  worker.setRouteBox(box);
  worker.setExtBox(box);
  FlexGridGraph graph(
      design->getTech(), logger.get(), &worker, router_cfg.get());
  frLayerCoordTrackPatternMap x, y;
  // An absent neighboring layer must not invalidate the current layer map.
  x.reserve(1);
  y.reserve(1);
  x[2][100] = nullptr;
  x[2][200] = nullptr;
  y[2][100] = nullptr;
  graph.init(design.get(), box, box, x, y, true, false);
  FlexMazeIdx start, end;
  ASSERT_TRUE(graph.hasMazeIdx({100, 100}, 2));
  ASSERT_TRUE(graph.hasMazeIdx({200, 100}, 2));
  graph.getMazeIdx(start, {100, 100}, 2);
  graph.getMazeIdx(end, {200, 100}, 2);
  EXPECT_EQ(end.x(), start.x() + 1);
  EXPECT_TRUE(graph.hasEdge(start.x(), start.y(), start.z(), frDirEnum::E));
  EXPECT_TRUE(graph.hasGridCostE(start.x(), start.y(), start.z()));
  EXPECT_FALSE(graph.hasEdge(start.x(), start.y(), start.z(), frDirEnum::U));
}

}  // namespace drt
