// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "Slots.h"

#include <algorithm>
#include <boost/functional/hash.hpp>
#include <limits>
#include <vector>

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

std::size_t IntervalHash::operator()(const Interval& interval) const
{
  return boost::hash<std::tuple<Edge, int, int, int>>()({interval.getEdge(),
                                                         interval.getBegin(),
                                                         interval.getEnd(),
                                                         interval.getLayer()});
}

std::size_t RectHash::operator()(const Rect& rect) const
{
  return boost::hash<std::tuple<int, int, int, int>>()(
      {rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax()});
}

}  // namespace ppl
