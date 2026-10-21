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
  const SpatialIndex index({});
  const SpatialIndex::Nearest n = index.nearest({1, 2, 3});
  EXPECT_EQ(n.box, SpatialIndex::kGround);
  EXPECT_DOUBLE_EQ(n.dist, 3);
  // Below the ground plane counts as touching it.
  EXPECT_DOUBLE_EQ(index.nearest({1, 2, -1e-12}).dist, 0);
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
  const SpatialIndex index(boxes);
  for (int q = 0; q < 20000; q++) {
    const double scale = q % 4 == 0 ? 400 : 24;
    const Vec3 p{rng.uniform() * scale - scale / 4,
                 rng.uniform() * scale - scale / 4,
                 rng.uniform() * scale};
    double best = p[2];
    for (const Box& b : boxes) {
      best = std::min(best, linfDistance(p, b));
    }
    const SpatialIndex::Nearest n = index.nearest(p);
    ASSERT_DOUBLE_EQ(n.dist, best);
    if (n.box != SpatialIndex::kGround) {
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
  const SpatialIndex index(boxes);
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

}  // namespace
}  // namespace fsv
