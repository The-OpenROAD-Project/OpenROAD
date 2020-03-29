/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.
//
// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
// Copyright (c) 2018, SangGi Do and Mingyu Woo
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
///////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <limits>
#include <set>
#include "openroad/Error.hh"
#include "opendp/Opendp.h"

namespace opendp {

using std::abs;
using std::cerr;
using std::cout;
using std::endl;

using odb::Rect;
using odb::Point;

Point Opendp::nearestPt(Cell* cell, Rect* rect) {
  int x, y;
  initialLocation(cell, x, y);
  int size_x = gridNearestWidth(cell);
  int size_y = gridNearestHeight(cell);
  int temp_x = x;
  int temp_y = y;

  if(check_overlap(cell, rect)) {
    int dist_x = 0;
    int dist_y = 0;
    if(abs(x - rect->xMin() + paddedWidth(cell)) > abs(rect->xMax() - x)) {
      dist_x = abs(rect->xMax() - x);
      temp_x = rect->xMax();
    }
    else {
      dist_x = abs(x - rect->xMin());
      temp_x = rect->xMin() - paddedWidth(cell);
    }
    if(abs(y - rect->yMin() + cell->height_) > abs(rect->yMax() - y)) {
      dist_y = abs(rect->yMax() - y);
      temp_y = rect->yMax();
    }
    else {
      dist_y = abs(y - rect->yMin());
      temp_y = rect->yMin() - cell->height_;
    }
    assert(dist_x >= 0);
    assert(dist_y >= 0);
    if(dist_x < dist_y)
      return Point(temp_x, y);
    else
      return Point(x, temp_y);
  }

  if(x < rect->xMin())
    temp_x = rect->xMin();
  else if(x + paddedWidth(cell) > rect->xMax())
    temp_x = rect->xMax() - paddedWidth(cell);

  if(y < rect->yMin())
    temp_y = rect->yMin();
  else if(y + cell->height_ > rect->yMax())
    temp_y = rect->yMax() - cell->height_;

  return Point(temp_x, temp_y);
}

int Opendp::dist_for_rect(Cell* cell, Rect* rect) {
  int x, y;
  initialLocation(cell, x, y);
  int dist_x = 0;
  int dist_y = 0;

  if(x < rect->xMin())
    dist_x = rect->xMin() - x;
  else if(x + paddedWidth(cell) > rect->xMax())
    dist_x = x + paddedWidth(cell) - rect->xMax();

  if(y < rect->yMin())
    dist_y = rect->yMin() - y;
  else if(y + cell->height_ > rect->yMax())
    dist_y = y + cell->height_ - rect->yMax();

  assert(dist_y >= 0);
  assert(dist_x >= 0);

  return dist_y + dist_x;
}

bool Opendp::check_overlap(Rect rect, Rect box) {
  return box.xMin() < rect.xMax()
    && box.xMax() > rect.xMin()
    && box.yMin() < rect.yMax()
    && box.yMax() > rect.yMin();
}

bool Opendp::check_overlap(Cell* cell, Rect* rect) {
  int x, y;
  initialPaddedLocation(cell, x, y);

  return x + paddedWidth(cell) > rect->xMin()
    && x < rect->xMax()
    && y + cell->height_ > rect->yMin()
    && y < rect->yMax();
}

bool Opendp::check_inside(Rect rect, Rect box) {
  return rect.xMin() >= box.xMin()
    && rect.xMax() <= box.xMax()
    && rect.yMin() >= box.yMin()
    && rect.yMax() <= box.yMax();
}

bool Opendp::check_inside(Cell* cell, Rect* rect) {
  int x, y;
  initialPaddedLocation(cell, x, y);
  return x >= rect->xMin()
    && x + paddedWidth(cell) <= rect->xMax()
    && y >= rect->yMin()
    && y + cell->height_ <= rect->yMax();
}

set< Cell* > Opendp::gridCellsInBoundary(Rect* rect) {
  int x_start = divFloor(rect->xMin(), site_width_);
  int y_start = divFloor(rect->yMin(), row_height_);
  int x_end = divFloor(rect->xMax(), site_width_);
  int y_end = divFloor(rect->yMax(), row_height_);

  set< Cell* > cells;
  for(int i = y_start; i < y_end; i++) {
    for(int j = x_start; j < x_end; j++) {
      Cell *cell = grid_[i][j].cell;
      if(cell && !isFixed(cell)) {
	cells.insert(cell);
      }
    }
  }
  return cells;
}

void
Opendp::rectDist(Cell *cell,
		 Rect *rect,
		 // Return values.
		 int x,
		 int y)
{
  x = 0;
  y = 0;
  int init_x, init_y;
  initialLocation(cell, init_x, init_y);
  if(init_x > (rect->xMin() + rect->xMax()) / 2)
    x = rect->xMax();
  else
    x = rect->xMin();
  if(init_y > (rect->yMin() + rect->yMax()) / 2)
    y = rect->yMax();
  else
    y = rect->yMin();
}

int
Opendp::rectDist(Cell *cell,
		 Rect *rect)
{
  int x, y;
  rectDist(cell, rect,
	   x, y);
  int init_x, init_y;
  initialLocation(cell, init_x, init_y);
  return abs(init_x - x) + abs(init_y - y);
}

}  // namespace opendp
