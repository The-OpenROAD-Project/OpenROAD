// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <vector>

#include "Philox.h"
#include "SpatialIndex.h"
#include "fsv/Geometry.h"
#include "gtest/gtest.h"

namespace fsv {
namespace {

TEST(SpatialIndex, GroundOnly)
{
  const SpatialIndex<3> index({});
  const SpatialIndex<3>::Nearest n = index.nearest({1, 2, 3});
  EXPECT_EQ(n.box, SpatialIndex<3>::kGround);
  EXPECT_DOUBLE_EQ(n.dist, 3);
  // Below the ground plane counts as touching it.
  EXPECT_DOUBLE_EQ(index.nearest({1, 2, -1e-12}).dist, 0);
}

// An index over no boxes finds no overlaps.
TEST(SpatialIndex, EmptyHasNoOverlaps)
{
  int found = 0;
  SpatialIndex<3>({}).forEachOverlapping({{0, 0, 0}, {1, 1, 1}},
                                         [&](int) { found++; });
  SpatialIndex<2>({}).forEachOverlapping({{0, 0}, {1, 1}},
                                         [&](int) { found++; });
  EXPECT_EQ(found, 0);
}

// Branch-and-bound result matches a brute-force scan, inside the layout and
// far outside it.
TEST(SpatialIndex, MatchesBruteForce)
{
  Philox rng(3, 0);
  std::vector<Box> boxes;
  for (int b = 0; b < 500; b++) {
    Box box;
    for (int k = 0; k < 3; k++) {
      box.lo[k] = (k == 2 ? 0.1 : 0.0) + rng.uniform() * 20;
      box.hi[k] = box.lo[k] + 0.05 + rng.uniform() * 2;
    }
    boxes.push_back(box);
  }
  const SpatialIndex<3> index(boxes);
  for (int q = 0; q < 20000; q++) {
    const double scale = q % 4 == 0 ? 400 : 24;
    const Vec3 p{rng.uniform() * scale - scale / 4,
                 rng.uniform() * scale - scale / 4,
                 rng.uniform() * scale};
    double best = p[2];
    for (const Box& b : boxes) {
      best = std::min(best, linfDistance(p, b));
    }
    const SpatialIndex<3>::Nearest n = index.nearest(p);
    ASSERT_DOUBLE_EQ(n.dist, best);
    if (n.box != SpatialIndex<3>::kGround) {
      ASSERT_DOUBLE_EQ(linfDistance(p, boxes[n.box]), best);
    }
  }
}

// Routed layers with different wire counts: long wires along x on one layer
// and along y on the other, as the layer-first split is built for.
TEST(SpatialIndex, LayeredWiresMatchBruteForce)
{
  std::vector<Box> boxes;
  for (int i = 0; i < 70; i++) {
    const double y = 0.2 * i;
    boxes.push_back({{0, y, 0.5}, {14, y + 0.1, 0.6}});
  }
  for (int i = 0; i < 10; i++) {
    const double x = 1.4 * i;
    boxes.push_back({{x, 0, 0.8}, {x + 0.1, 14, 0.9}});
  }
  const SpatialIndex<3> index(boxes);
  Philox rng(5, 0);
  for (int q = 0; q < 20000; q++) {
    const Vec3 p{
        rng.uniform() * 16 - 1, rng.uniform() * 16 - 1, rng.uniform() * 1.5};
    double best = p[2];
    for (const Box& b : boxes) {
      best = std::min(best, linfDistance(p, b));
    }
    ASSERT_DOUBLE_EQ(index.nearest(p).dist, best);
  }
}

// The 2D index (x-z): the ground is the last coordinate.
TEST(SpatialIndex, RectsMatchBruteForce)
{
  Philox rng(9, 0);
  std::vector<Rect> rects;
  for (int r = 0; r < 300; r++) {
    Rect rect;
    for (int k = 0; k < 2; k++) {
      rect.lo[k] = (k == 1 ? 0.1 : 0.0) + rng.uniform() * 20;
      rect.hi[k] = rect.lo[k] + 0.05 + rng.uniform() * 2;
    }
    rects.push_back(rect);
  }
  const SpatialIndex<2> index(rects);
  EXPECT_DOUBLE_EQ(SpatialIndex<2>({}).nearest({5, 3}).dist, 3);
  for (int q = 0; q < 20000; q++) {
    const double scale = q % 4 == 0 ? 400 : 24;
    const Vec2 p{rng.uniform() * scale - scale / 4, rng.uniform() * scale};
    double best = p[1];
    for (const Rect& r : rects) {
      best = std::min(best, linfDistance(p, r));
    }
    const SpatialIndex<2>::Nearest n = index.nearest(p);
    ASSERT_DOUBLE_EQ(n.dist, best);
    if (n.box != SpatialIndex<2>::kGround) {
      ASSERT_DOUBLE_EQ(linfDistance(p, rects[n.box]), best);
    }
  }
}

// The overlap query returns exactly the boxes a scan finds, touching ones
// included.
TEST(SpatialIndex, OverlapsMatchBruteForce)
{
  Philox rng(13, 0);
  std::vector<Box> boxes;
  for (int b = 0; b < 400; b++) {
    Box box;
    for (int k = 0; k < 3; k++) {
      // On a 0.05 grid, so that queries often touch boxes exactly.
      box.lo[k] = 0.05 * static_cast<int>(rng.uniform() * 400);
      box.hi[k] = box.lo[k] + 0.05 * (1 + static_cast<int>(rng.uniform() * 40));
    }
    boxes.push_back(box);
  }
  const SpatialIndex<3> index(boxes);
  for (int q = 0; q < 2000; q++) {
    Box query;
    for (int k = 0; k < 3; k++) {
      query.lo[k] = 0.05 * static_cast<int>(rng.uniform() * 440) - 1;
      query.hi[k] = query.lo[k] + 0.05 * static_cast<int>(rng.uniform() * 20);
    }
    std::vector<int> found;
    index.forEachOverlapping(query, [&](const int i) { found.push_back(i); });
    std::ranges::sort(found);
    std::vector<int> expected;
    for (int i = 0; i < static_cast<int>(boxes.size()); i++) {
      if (linfGap(query, boxes[i]) == 0) {
        expected.push_back(i);
      }
    }
    ASSERT_EQ(found, expected) << "query " << q;
  }
}

// A visitor that returns false stops the query.
TEST(SpatialIndex, OverlapQueryStops)
{
  std::vector<Box> boxes;
  for (int b = 0; b < 100; b++) {
    boxes.push_back({{0.1 * b, 0, 0.5}, {0.1 * b + 0.05, 1, 1}});
  }
  const SpatialIndex<3> index(boxes);
  int visits = 0;
  index.forEachOverlapping({{-1, -1, 0}, {20, 2, 2}}, [&](int) {
    visits++;
    return visits < 7;
  });
  EXPECT_EQ(visits, 7);
}

}  // namespace
}  // namespace fsv
