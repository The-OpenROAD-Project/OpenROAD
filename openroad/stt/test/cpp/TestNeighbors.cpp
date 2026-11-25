// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstddef>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "neighbors.h"
#include "odb/geom.h"

namespace pdr {
namespace {

using odb::Point;
using std::vector;

// Reference definition: coincident points link to the lowest indexed
// point at their location and other pairs are neighbors when their
// closed bounding box holds no other location.
vector<Neighbors> bruteForceNeighbors(const vector<Point>& pts)
{
  const int count = pts.size();
  vector<int> rep(count);
  for (int i = 0; i < count; ++i) {
    rep[i] = std::find(pts.begin(), pts.end(), pts[i]) - pts.begin();
  }

  vector<Neighbors> neighbors(count);
  for (int i = 0; i < count; ++i) {
    if (rep[i] != i) {
      neighbors[i].push_back(rep[i]);
      neighbors[rep[i]].push_back(i);
    }
  }
  for (int i = 0; i < count; ++i) {
    for (int j = 0; j < count; ++j) {
      if (i == j || rep[i] != i || rep[j] != j) {
        continue;
      }
      const odb::Rect bbox(pts[i], pts[j]);
      bool empty = true;
      for (int k = 0; k < count && empty; ++k) {
        if (k != i && k != j && rep[k] == k && bbox.intersects(pts[k])) {
          empty = false;
        }
      }
      if (empty) {
        neighbors[i].push_back(j);
      }
    }
  }
  for (Neighbors& pt_neighbors : neighbors) {
    std::sort(pt_neighbors.begin(), pt_neighbors.end());
  }
  return neighbors;
}

size_t countEntries(const vector<Neighbors>& neighbors)
{
  size_t count = 0;
  for (const Neighbors& pt_neighbors : neighbors) {
    count += pt_neighbors.size();
  }
  return count;
}

TEST(Neighbors, MatchesBruteForceWithTies)
{
  std::mt19937 rng(1);
  for (int trial = 0; trial < 20000; ++trial) {
    const int count = 1 + rng() % 14;
    const int x_range = 1 + rng() % 6;
    const int y_range = 1 + rng() % 6;
    vector<Point> pts;
    for (int i = 0; i < count; ++i) {
      pts.emplace_back(rng() % x_range, rng() % y_range);
    }
    ASSERT_EQ(getNearestNeighbors(pts), bruteForceNeighbors(pts))
        << "trial " << trial;
  }
}

TEST(Neighbors, MatchesBruteForceGeneralPosition)
{
  std::mt19937 rng(2);
  for (int trial = 0; trial < 500; ++trial) {
    const int count = 2 + rng() % 60;
    vector<int> xs(count);
    vector<int> ys(count);
    for (int i = 0; i < count; ++i) {
      xs[i] = i;
      ys[i] = i;
    }
    std::shuffle(xs.begin(), xs.end(), rng);
    std::shuffle(ys.begin(), ys.end(), rng);
    vector<Point> pts;
    for (int i = 0; i < count; ++i) {
      pts.emplace_back(xs[i] * 7, ys[i] * 3);
    }
    ASSERT_EQ(getNearestNeighbors(pts), bruteForceNeighbors(pts))
        << "trial " << trial;
  }
}

TEST(Neighbors, CollinearOnlyLinksAdjacent)
{
  constexpr int kCount = 1000;
  vector<Point> vertical;
  vector<Point> horizontal;
  for (int i = 0; i < kCount; ++i) {
    vertical.emplace_back(5, (i * 37) % kCount);
    horizontal.emplace_back((i * 37) % kCount, 5);
  }
  EXPECT_EQ(countEntries(getNearestNeighbors(vertical)), 2 * (kCount - 1));
  EXPECT_EQ(countEntries(getNearestNeighbors(horizontal)), 2 * (kCount - 1));
}

TEST(Neighbors, GridOnlyLinksOrthogonal)
{
  constexpr int kSide = 50;
  vector<Point> pts;
  for (int x = 0; x < kSide; ++x) {
    for (int y = 0; y < kSide; ++y) {
      pts.emplace_back(x * 10, y * 20);
    }
  }
  EXPECT_EQ(countEntries(getNearestNeighbors(pts)),
            2 * 2 * kSide * (kSide - 1));
}

TEST(Neighbors, ColumnBesideStaircase)
{
  // A column of points with a staircase descending to its right.  Each
  // staircase point only sees the column point in its row.
  constexpr int kCount = 500;
  vector<Point> pts;
  for (int i = 0; i < kCount; ++i) {
    pts.emplace_back(0, i);
  }
  for (int i = 0; i < kCount; ++i) {
    pts.emplace_back(1 + i, kCount - 1 - i);
  }
  const vector<Neighbors> neighbors = getNearestNeighbors(pts);
  EXPECT_EQ(countEntries(neighbors), 2 * (kCount - 1) * 3 + 2);
  for (int i = 0; i < kCount; ++i) {
    for (const int neighbor : neighbors[kCount + i]) {
      EXPECT_TRUE(neighbor >= kCount || neighbor == kCount - 1 - i);
    }
  }
}

TEST(Neighbors, CoincidentPointsLinkToFirst)
{
  const vector<Point> pts{{3, 3}, {0, 0}, {3, 3}, {3, 3}, {6, 6}};
  const vector<Neighbors> expected{{1, 2, 3, 4}, {0}, {0}, {0}, {0}};
  EXPECT_EQ(getNearestNeighbors(pts), expected);
}

TEST(Neighbors, SinglePoint)
{
  EXPECT_EQ(getNearestNeighbors({{4, 2}}), vector<Neighbors>{{}});
}

}  // namespace
}  // namespace pdr
