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

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "openroad/Error.hh"
#include "opendp/Opendp.h"

namespace opendp {

using std::abs;
using std::cerr;
using std::cout;
using std::endl;
using std::sort;
using std::string;
using std::vector;
using std::numeric_limits;

using ord::warn;

static bool cellAreaLess(Cell* cell1, Cell* cell2);

void Opendp::detailedPlacement() {
  if(!groups_.empty()) {
    prePlaceGroups();
    prePlace();

    // naive method placement ( Multi -> single )
    placeGroups();
    for(Group &group : groups_) {
      for(int pass = 0; pass < 3; pass++) {
        int refine_count = groupRefine(&group);
        int anneal_count = anneal(&group);
        // magic number alert
	if(refine_count < 10 || anneal_count < 100) break;
      }
    }
  }
  place();
}

void Opendp::prePlace() {
  for(Cell& cell : cells_) {
    bool in_group = false;
    Rect* target;
    if(!cell.inGroup() && !cell.is_placed_) {
      for(int j = 0; j < groups_.size(); j++) {
	Group* group = &groups_[j];
	for(Rect& rect : group->regions) {
	  if(check_overlap(&cell, &rect)) {
	    in_group = true;
	    target = &rect;
	  }
	}
      }
      if(in_group) {
	Point nearest = nearestPt(&cell, target);
	if(map_move(&cell, nearest.x(), nearest.y()))
	  cell.hold_ = true;
      }
    }
  }
}

void Opendp::prePlaceGroups() {
  for(Group &group : groups_) {
    for(Cell* cell : group.cells_) {
      if(!(isFixed(cell) || cell->is_placed_)) {
	int dist = numeric_limits<int>::max();
	bool in_group = false;
	Rect* target;
	for(Rect& rect : group.regions) {
	  if(check_inside(cell, &rect))
	    in_group = true;
	  int temp_dist = dist_for_rect(cell, &rect);
	  if(temp_dist < dist) {
	    dist = temp_dist;
	    target = &rect;
	  }
	}
	if(!in_group) {
	  Point nearest = nearestPt(cell, target);
	  if(map_move(cell, nearest.x(), nearest.y())) cell->hold_ = true;
	}
      }
    }
  }
}

void Opendp::place() {
  vector< Cell* > sorted_cells;
  sorted_cells.reserve(cells_.size());

  for(Cell& cell : cells_) {
    if(!(isFixed(&cell) || cell.inGroup() || cell.is_placed_))
      sorted_cells.push_back(&cell);
  }
  sort(sorted_cells.begin(), sorted_cells.end(), cellAreaLess);

  for(Cell* cell : sorted_cells) {
    if(isMultiRow(cell)) {
      if(!map_move(cell))
	shift_move(cell);
    }
  }
  for(Cell* cell : sorted_cells) {
    if(!isMultiRow(cell)) {
      if(!map_move(cell)) {
	shift_move(cell);
      }
    }
  }
}

static bool cellAreaLess(Cell* cell1, Cell* cell2) {
  int area1 = cell1->area();
  int area2 = cell2->area();
  if(area1 > area2)
    return true;
  else if(area1 < area2)
    return false;
  else
    return cell1->db_inst_->getId() < cell2->db_inst_->getId();
}

void Opendp::placeGroups() {
  for(Group &group : groups_) {
    bool single_pass = true;
    bool multi_pass = true;
    vector< Cell* > cell_list;
    cell_list.reserve(cells_.size());
    for(Cell* cell : group.cells_) {
      if(!isFixed(cell) && !cell->is_placed_) {
	cell_list.push_back(cell);
      }
    }
    sort(cell_list.begin(), cell_list.end(), cellAreaLess);
    // place multi-deck cells on each group region
    for(int j = 0; j < cell_list.size(); j++) {
      Cell* cell = cell_list[j];
      if(!isFixed(cell) && !cell->is_placed_) {
	assert(cell->inGroup());
	if(isMultiRow(cell)) {
	  multi_pass = map_move(cell);
	  if(!multi_pass) {
	    break;
	  }
	}
      }
    }
    // cout << "Group util : " << group->util << endl;
    if(multi_pass) {
      //				cout << " Group : " << group->name <<
      //" multi-deck placement done - ";
      // place single-deck cells on each group region
      for(int j = 0; j < cell_list.size(); j++) {
        Cell* cell = cell_list[j];
        if(!isFixed(cell) && !cell->is_placed_) {
	  assert(cell->inGroup());
	  if(!isMultiRow(cell)) {
	    single_pass = map_move(cell);
	    if(!single_pass) {
	      break;
	    }
	  }
	}
      }
    }

    if(!single_pass || !multi_pass) {
      // Erase group cells
      for(Cell* cell : group.cells_) {
        erase_pixel(cell);
      }

      // determine brick placement by utilization
      // magic number alert
      if(group.util > 0.95) {
        brickPlace1(&group);
      }
      else {
        brickPlace2(&group);
      }
    }
  }
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
  initLocation(cell, init_x, init_y);
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
  initLocation(cell, init_x, init_y);
  return abs(init_x - x) + abs(init_y - y);
}

// place toward group edges
void Opendp::brickPlace1(Group* group) {
  Rect *boundary = &group->boundary;
  vector< Cell* > sort_by_dist(group->cells_);

  sort(sort_by_dist.begin(), sort_by_dist.end(),
       [&](Cell* cell1, Cell* cell2) {
         return rectDist(cell1, boundary) < rectDist(cell2, boundary);
       });
			       
  for(Cell* cell : sort_by_dist) {
    int x, y;
    rectDist(cell, boundary);

    bool valid = map_move(cell, x, y);
    if(!valid) {
      warn("cannot place single cell (brick place 1) %s", cell->name());
    }
  }
}

// place toward region edges
void Opendp::brickPlace2(Group* group) {
  vector< Cell* > sort_by_dist(group->cells_);

  sort(sort_by_dist.begin(), sort_by_dist.end(),
       [&](Cell* cell1, Cell* cell2) {
         return rectDist(cell1, cell1->region_) < rectDist(cell2, cell2->region_);
       });

  for(Cell* cell : sort_by_dist) {
    if(!cell->hold_) {
      int x, y;
      rectDist(cell, cell->region_,
	       x, y);
      bool valid = map_move(cell, x, y);
      if(!valid) {
	warn("cannot place instance (brick place 2) %s" , cell->name());
      }
    }
  }
}

int Opendp::groupRefine(Group* group) {
  vector< Cell* > sort_by_disp(group->cells_);

  sort(sort_by_disp.begin(), sort_by_disp.end(),
       [&](Cell* cell1, Cell* cell2) {
         return (disp(cell1) > disp(cell1));
       });

  int count = 0;
  for(int i = 0; i < sort_by_disp.size() * group_refine_percent_; i++) {
    Cell* cell = sort_by_disp[i];
    if(!cell->hold_) {
      if(refine_move(cell)) count++;
    }
  }
  return count;
}

int Opendp::anneal(Group* group) {
  srand(rand_seed_);
  int count = 0;

  // magic number alert
  for(int i = 0; i < 1000 * group->cells_.size(); i++) {
    Cell* cell1 = group->cells_[rand() % group->cells_.size()];
    Cell* cell2 = group->cells_[rand() % group->cells_.size()];

    if(!cell1->hold_ && !cell2->hold_) {
      if(swap_cell(cell1, cell2)) count++;
    }
  }
  return count;
}

int Opendp::anneal() {
  srand(rand_seed_);
  int count = 0;
  // magic number alert
  for(int i = 0; i < 100 * cells_.size(); i++) {
    Cell* cell1 = &cells_[rand() % cells_.size()];
    Cell* cell2 = &cells_[rand() % cells_.size()];
    if(!cell1->hold_ && !cell2->hold_) {
      if(swap_cell(cell1, cell2)) count++;
    }
  }
  return count;
}

int Opendp::refine() {
  vector< Cell* > sorted;
  sorted.reserve(cells_.size());

  for(Cell &cell : cells_) {
    if(!(isFixed(&cell) || cell.hold_ || cell.inGroup()))
      sorted.push_back(&cell);
  }
  sort(sorted.begin(), sorted.end(),
       [&](Cell* cell1, Cell* cell2) {
         return disp(cell1) > disp(cell2);
       });

  int count = 0;
  for(int i = 0; i < sorted.size() * refine_percent_; i++) {
    Cell* cell = sorted[i];
    if(!cell->hold_) {
      if(refine_move(cell)) count++;
    }
  }
  return count;
}

} // namespace opendp
