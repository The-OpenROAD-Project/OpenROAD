// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#define MAX_SLOTS_RECOMMENDED 600
#define MAX_SECTIONS_RECOMMENDED 600

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <set>
#include <vector>

#include "Netlist.h"
#include "ppl/IOPlacer.h"

namespace ppl {

class Interval
{
 public:
  Interval(Edge edge, int begin, int end, int layer = -1)
      : edge_(edge), begin_(begin), end_(end), layer_(layer)
  {
  }
  Edge getEdge() const { return edge_; }
  int getBegin() const { return begin_; }
  int getEnd() const { return end_; }
  int getLayer() const { return layer_; }
  bool operator==(const Interval& interval) const;

 private:
  Edge edge_;
  int begin_;
  int end_;
  int layer_;
};

struct IntervalHash
{
  std::size_t operator()(const Interval& interval) const;
};

struct RectHash
{
  std::size_t operator()(const Rect& rect) const;
};

// Slot: an on-track position in the die boundary where a pin
// can be placed
struct Slot
{
  bool blocked;
  bool used;
  odb::Point pos;
  int layer;
  Edge edge;

  bool isAvailable() const { return (!blocked && !used); }
};

// Section: a region in the die boundary that contains a set
// of slots. By default, each section has 200 slots
struct Section
{
  odb::Point pos;
  std::vector<int> pin_indices;
  std::vector<PinGroupByIndex> pin_groups;
  int cost = 0;
  int begin_slot = 0;
  int end_slot = 0;
  int used_slots = 0;
  int num_slots = 0;
  Edge edge;

  int getMaxContiguousSlots(const std::vector<Slot>& slots);
};

struct Constraint
{
  Constraint(const PinSet& pins, Direction dir, Interval interv)
      : pin_list(pins), direction(dir), interval(interv)
  {
    box = odb::Rect(-1, -1, -1, -1);
    pins_per_slots = 0;
    first_slot = -1;
    last_slot = -1;
  }
  Constraint(const PinSet& pins, Direction dir, odb::Rect b)
      : pin_list(pins), direction(dir), interval(Edge::invalid, -1, -1), box(b)
  {
    pins_per_slots = 0;
    first_slot = -1;
    last_slot = -1;
  }

  PinSet pin_list;
  Direction direction;
  Interval interval;
  odb::Rect box;
  std::vector<Section> sections;
  std::vector<int> pin_indices;
  std::set<int> pin_groups;
  float pins_per_slots;
  int first_slot = 0;
  int last_slot = 0;
  int mirrored_pins_count = 0;
};

template <typename T>
std::vector<size_t> sortIndexes(const std::vector<T>& v,
                                const std::vector<T>& tie_break)
{
  // initialize original index locations
  std::vector<size_t> idx(v.size());
  std::iota(idx.begin(), idx.end(), 0);

  // sort indexes based on comparing values in v
  std::stable_sort(
      idx.begin(), idx.end(), [&v, &tie_break](size_t i1, size_t i2) {
        return std::tie(v[i1], tie_break[i1]) < std::tie(v[i2], tie_break[i2]);
      });
  return idx;
}

}  // namespace ppl
