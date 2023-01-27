/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_SLOTS_RECOMMENDED 600
#define MAX_SECTIONS_RECOMMENDED 600

#include <algorithm>
#include <numeric>
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

 private:
  Edge edge_;
  int begin_;
  int end_;
  int layer_;
};

struct TopLayerGrid
{
  int layer = -1;
  int x_step = -1;
  int y_step = -1;
  Rect region;
  int pin_width = -1;
  int pin_height = -1;
  int keepout = -1;

  int llx() { return region.xMin(); }
  int lly() { return region.yMin(); }
  int urx() { return region.xMax(); }
  int ury() { return region.yMax(); }
};

// Section: a region in the die boundary that contains a set
// of slots. By default, each section has 200 slots
struct Section
{
  odb::Point pos;
  std::vector<int> pin_indices;
  std::vector<std::vector<int>> pin_groups;
  int cost;
  int begin_slot;
  int end_slot;
  int used_slots;
  int num_slots;
  Edge edge;
};

struct Constraint
{
  Constraint(PinList pins, Direction dir, Interval interv)
      : pin_list(pins), direction(dir), interval(interv)
  {
    box = odb::Rect(-1, -1, -1, -1);
    pins_per_slots = 0;
  }
  Constraint(PinList pins, Direction dir, odb::Rect b)
      : pin_list(pins), direction(dir), interval(Edge::invalid, -1, -1), box(b)
  {
    pins_per_slots = 0;
  }

  PinList pin_list;
  Direction direction;
  Interval interval;
  odb::Rect box;
  std::vector<Section> sections;
  float pins_per_slots;
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
};

template <typename T>
std::vector<size_t> sortIndexes(const std::vector<T>& v)
{
  // initialize original index locations
  std::vector<size_t> idx(v.size());
  std::iota(idx.begin(), idx.end(), 0);
  // sort indexes based on comparing values in v
  std::stable_sort(idx.begin(), idx.end(), [&v](size_t i1, size_t i2) {
    return v[i1] < v[i2];
  });
  return idx;
}

}  // namespace ppl
