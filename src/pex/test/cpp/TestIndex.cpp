// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "odb/geom.h"
#include "odb/isotropy.h"
#include "pex/Index.h"

namespace pex {
namespace {

// Three layers like a routed block's: horizontal, vertical, horizontal,
// pitch 100. Wires on tracks, short and long; wires off their track; via
// pads; and obstructions spanning many tracks.
std::vector<IndexBox> randomBlock(std::mt19937& rng)
{
  std::uniform_int_distribution<int> track(0, 60);
  std::uniform_int_distribution<int> pos(0, 6000);
  std::uniform_int_distribution<int> len(10, 400);
  std::uniform_int_distribution<int> longer(400, 6000);
  std::uniform_int_distribution<int> kind(0, 9);
  std::uniform_int_distribution<int> offset(-60, 60);
  std::vector<IndexBox> boxes;
  for (int i = 0; i < 3000; i++) {
    const int layer = i % 3;
    const odb::Orientation2D along
        = layer == 1 ? odb::vertical : odb::horizontal;
    const int k = kind(rng);
    int center = track(rng) * 100;
    int half = 20;
    int length = len(rng);
    if (k == 0) {
      center += offset(rng);  // off its track
    } else if (k == 1) {
      length = longer(rng);  // a long route or a rail
    } else if (k == 2) {
      half = 25;  // a via pad
      length = 50;
    } else if (k == 3) {
      half = 300 + offset(rng);  // an obstruction across many lanes
    }
    const int start = pos(rng);
    odb::Rect rect;
    rect.set(along, odb::low, start);
    rect.set(along, odb::high, start + length);
    rect.set(along.turn_90(), odb::low, center - half);
    rect.set(along.turn_90(), odb::high, center + half);
    boxes.push_back({.layer = layer, .rect = rect});
  }
  return boxes;
}

TEST(Index, QueriesEqualABruteForceScan)
{
  std::mt19937 rng(7);
  const std::vector<IndexBox> boxes = randomBlock(rng);
  const Index one(boxes, 3, 1);
  const Index four(boxes, 3, 4);
  std::uniform_int_distribution<int> pos(-200, 6200);
  std::uniform_int_distribution<int> size(0, 800);
  for (int q = 0; q < 2000; q++) {
    const int x = pos(rng);
    const int y = pos(rng);
    const odb::Rect r(x, y, x + size(rng), y + size(rng));
    std::vector<int> expected;
    for (int i = 0; std::cmp_less(i, boxes.size()); i++) {
      if (boxes[i].rect.intersects(r)) {
        expected.push_back(i);
      }
    }
    ASSERT_EQ(one.query(r), expected) << "query " << q;
    ASSERT_EQ(four.query(r), expected) << "query " << q;
    // One layer's query.
    std::vector<int> layer;
    one.query(1, r, layer);
    std::ranges::sort(layer);
    std::vector<int> expected_layer;
    for (const int i : expected) {
      if (boxes[i].layer == 1) {
        expected_layer.push_back(i);
      }
    }
    ASSERT_EQ(layer, expected_layer) << "query " << q;
  }
}

// An index with no boxes, or a layer with none, finds nothing.
TEST(Index, Empty)
{
  const odb::Rect all(-1000, -1000, 1000, 1000);
  EXPECT_EQ(Index({}, 0).query(all), (std::vector<int>{}));
  const Index index({{.layer = 1, .rect = odb::Rect(0, 0, 10, 10)}}, 3);
  std::vector<int> out;
  index.query(0, all, out);
  index.query(2, all, out);
  EXPECT_EQ(out, (std::vector<int>{}));
  EXPECT_EQ(index.query(all), (std::vector<int>{0}));
}

// Queries that only touch a box's edge or corner find it.
TEST(Index, Edges)
{
  const std::vector<IndexBox> boxes{
      {.layer = 0, .rect = odb::Rect(0, 80, 1000, 120)},
      {.layer = 0, .rect = odb::Rect(0, 130, 1000, 170)},
      {.layer = 1, .rect = odb::Rect(2000, 0, 2010, 10)},
  };
  const Index index(boxes, 2);
  EXPECT_EQ(index.query(odb::Rect(1000, 120, 1100, 125)),
            (std::vector<int>{0}));  // touching the corner
  EXPECT_EQ(index.query(odb::Rect(500, 151, 501, 152)), (std::vector<int>{1}));
  EXPECT_EQ(index.query(odb::Rect(500, 125, 501, 129)), (std::vector<int>{}));
  EXPECT_EQ(index.query(odb::Rect(1990, -5, 2000, 0)), (std::vector<int>{2}));
  EXPECT_EQ(index.query(odb::Rect(0, 0, 3000, 3000)),
            (std::vector<int>{0, 1, 2}));
  EXPECT_THROW(Index({}, -1), std::invalid_argument);
  EXPECT_THROW(Index({{.layer = 1, .rect = odb::Rect(0, 0, 1, 1)}}, 1),
               std::invalid_argument);
  EXPECT_THROW(Index({{.layer = 0, .rect = odb::Rect(0, 0, 0, 1)}}, 1),
               std::invalid_argument);
}

}  // namespace
}  // namespace pex
