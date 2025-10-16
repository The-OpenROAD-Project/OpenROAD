// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "Slots.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <tuple>
#include <vector>

#include "boost/container_hash/hash.hpp"
#include "odb/geom.h"
#include "ppl/IOPlacer.h"

namespace ppl {

int Section::getMaxContiguousSlots(const std::vector<Slot>& slots)
{
  int max_contiguous_slots = std::numeric_limits<int>::min();
  for (int i = begin_slot; i <= end_slot; i++) {
    // advance to the next free slot
    while (i <= end_slot && !slots[i].isAvailable()) {
      i++;
    }

    int contiguous_slots = 0;
    while (i <= end_slot && slots[i].isAvailable()) {
      contiguous_slots++;
      i++;
    }

    max_contiguous_slots = std::max(max_contiguous_slots, contiguous_slots);
  }

  return max_contiguous_slots;
}

bool Interval::operator==(const Interval& interval) const
{
  return edge_ == interval.getEdge() && begin_ == interval.getBegin()
         && end_ == interval.getEnd() && layer_ == interval.getLayer();
}

bool Interval::operator<(const Interval& interval) const
{
  if (edge_ != interval.edge_) {
    return edge_ < interval.edge_;
  }
  if (layer_ != interval.layer_) {
    return layer_ < interval.layer_;
  }
  if (begin_ != interval.begin_) {
    return begin_ < interval.begin_;
  }

  return end_ < interval.end_;
}

std::size_t IntervalHash::operator()(const Interval& interval) const
{
  return boost::hash<std::tuple<Edge, int, int, int>>()({interval.getEdge(),
                                                         interval.getBegin(),
                                                         interval.getEnd(),
                                                         interval.getLayer()});
}

std::size_t RectHash::operator()(const odb::Rect& rect) const
{
  return boost::hash<std::tuple<int, int, int, int>>()(
      {rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax()});
}

}  // namespace ppl
