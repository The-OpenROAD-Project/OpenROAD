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

namespace ord {
odb::Point
closestPtInRect(odb::Rect rect,
		odb::Point pt);
}

namespace opendp {

using std::abs;
using std::ceil;
using std::cerr;
using std::cout;
using std::endl;
using std::floor;
using std::ifstream;
using std::max;
using std::min;
using std::ofstream;
using std::round;
using std::string;
using std::to_string;
using std::vector;
using std::set;
using std::numeric_limits;

using ord::error;
using ord::warn;
using ord::closestPtInRect;

using odb::Rect;
using odb::Point;
using odb::dbBox;
using odb::dbITerm;
using odb::dbMTerm;
using odb::dbBTerm;
using odb::dbMPin;
using odb::dbBPin;
using odb::dbNet;
using odb::dbPlacementStatus;

void Opendp::displacementStats(// Return values.
			       int64_t &avg_displacement,
			       int64_t &sum_displacement,
			       int64_t &max_displacement) {
  avg_displacement = 0;
  sum_displacement = 0;
  max_displacement = 0;

  for(Cell& cell : cells_) {
    int displacement = disp(&cell);
    sum_displacement += displacement;
    if(displacement > max_displacement)
      max_displacement = displacement;
  }
  avg_displacement = sum_displacement / cells_.size();
}

int64_t Opendp::hpwl(bool initial) {
  int64_t hpwl = 0;
  for(dbNet *net : block_->getNets()) {
    Rect box;
    box.mergeInit();

    for(dbITerm *iterm : net->getITerms()) {
      dbInst* inst = iterm->getInst();
      Cell* cell = db_inst_map_[inst];
      int x, y;
      if(initial || cell == nullptr) {
	initLocation(inst, x, y);
      }
      else {
        x = cell->x_;
        y = cell->y_;
      }
      // Use inst center if no mpins.
      Rect iterm_rect(x, y, x, y);
      dbMTerm* mterm = iterm->getMTerm();
      auto mpins = mterm->getMPins();
      if(mpins.size()) {
        // Pick a pin, any pin.
        dbMPin* pin = *mpins.begin();
        auto geom = pin->getGeometry();
        if(geom.size()) {
          dbBox* pin_box = *geom.begin();
          Rect pin_rect;
          pin_box->getBox(pin_rect);
          int center_x = (pin_rect.xMin() + pin_rect.xMax()) / 2;
          int center_y = (pin_rect.yMin() + pin_rect.yMax()) / 2;
          iterm_rect = Rect(x + center_x, y + center_y,
			       x + center_x, y + center_y);
        }
      }
      box.merge(iterm_rect);
    }

    for(dbBTerm *bterm : net->getBTerms()) {
      for(dbBPin *bpin : bterm->getBPins()) {
        dbPlacementStatus status = bpin->getPlacementStatus();
        if(status.isPlaced()) {
          dbBox* pin_box = bpin->getBox();
          Rect pin_rect;
          pin_box->getBox(pin_rect);
          int center_x = (pin_rect.xMin() + pin_rect.xMax()) / 2;
          int center_y = (pin_rect.yMin() + pin_rect.yMax()) / 2;
          int core_center_x = center_x - core_.xMin();
          int core_center_y = center_y - core_.yMin();
          Rect pin_center(core_center_x, core_center_y, core_center_x,
                             core_center_y);
          box.merge(pin_center);
        }
      }
    }
    int perimeter = box.dx() + box.dy();
    hpwl += perimeter;
  }
  return hpwl;
}

Point Opendp::nearestPt(Cell* cell, Rect* rect) {
  int x, y;
  initLocation(cell, x, y);
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
  initLocation(cell, x, y);
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
  initLocation(cell, x, y);

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
  initLocation(cell, x, y);
  return x >= rect->xMin()
    && x + paddedWidth(cell) <= rect->xMax()
    && y >= rect->yMin()
    && y + cell->height_ <= rect->yMax();
}

bool Opendp::binSearch(int grid_x, Cell* cell,
		       int x, int y,
		       // Return values
		       int &avail_x,
		       int &avail_y) {
  int x_step = gridPaddedWidth(cell);
  int y_step = gridHeight(cell);

  // Check y is beyond the border.
  if((y + y_step) > (core_.yMax() / row_height_)
     // Check top power for even row multi-deck cell.
     || (y_step % 2 == 0
	 && rowTopPower(y) == topPower(cell))) {
    return false;
  }

#ifdef ODP_DEBUG
  cout << " - - - - - - - - - - - - - - - - - " << endl;
  cout << " Start Bin Search " << endl;
  cout << " cell name : " << cell->name() << endl;
  cout << " target x : " << x << endl;
  cout << " target y : " << y << endl;
#endif
  if(grid_x > x) {
    // magic number alert
    for(int i = 9; i >= 0; i--) {
      // check all grids are empty
      bool available = true;

      if(x + i + x_step > coreGridMaxX()) {
        available = false;
      }
      else {
        for(int k = y; k < y + y_step; k++) {
          for(int l = x + i; l < x + i + x_step; l++) {
	    if(grid_[k][l].cell != nullptr
	       || !grid_[k][l].is_valid
	       // check group regions
	       || (cell->inGroup()
		   && grid_[k][l].group_ != cell->group_)
	       || (!cell->inGroup() && grid_[k][l].group_ != nullptr)) {
              available = false;
              break;
            }
          }
          if(!available) break;
        }
      }
      if(available) {
	avail_x = x + i;
	avail_y = y;
        return true;
      }
    }
  }
  else {
    // magic number alert
    for(int i = 0; i < 10; i++) {
      // check all grids are empty
      bool available = true;
      if(x + i + x_step > coreGridMaxX()) {
        available = false;

      }
      else {
        for(int k = y; k < y + y_step; k++) {
          for(int l = x + i; l < x + i + x_step; l++) {
            if(grid_[k][l].cell != nullptr || !grid_[k][l].is_valid) {
              available = false;
              break;
            }
            // check group regions
            if(cell->inGroup()) {
              if(grid_[k][l].group_ != cell->group_) {
                available = false;
		break;
	      }
            }
            else {
              if(grid_[k][l].group_ != nullptr) {
		available = false;
		break;
	      }
            }
          }
        }
      }
      if(available) {
#ifdef ODP_DEBUG
        cout << " found pos x - y : " << x << " - " << y << endl;
#endif
	avail_x = x + i;
	avail_y = y;
	return true;
      }
    }
  }
  return false;
}

Pixel *Opendp::diamondSearch(Cell* cell, int x, int y) {
  int grid_x = gridX(x);
  int grid_y = gridY(y);

  // Restrict check to group region.
  Group* group = cell->group_;
  if(group) {
    Rect grid_boundary(divCeil(group->boundary.xMin(), site_width_),
			  divCeil(group->boundary.yMin(), row_height_),
			  group->boundary.xMax() / site_width_,
			  group->boundary.yMax() / row_height_);
    Point in_boundary = closestPtInRect(grid_boundary,
					   Point(grid_x, grid_y));
    grid_x = in_boundary.x();
    grid_y = in_boundary.y();
  }

  // Restrict check to core.
  Rect core(0, 0,
	       row_site_count_ - gridPaddedWidth(cell),
	       row_count_ - gridHeight(cell));
  Point in_core = closestPtInRect(core, Point(grid_x, grid_y));
  int avail_x, avail_y;
  if(binSearch(grid_x, cell, in_core.x(), in_core.y(),
	       avail_x, avail_y)) {
    return &grid_[avail_y][avail_x];
  }

  // magic number alert
  int diamond_search_width = diamond_search_height_ * 5;
  // Set search boundary max / min
  Point start = closestPtInRect(core,
                                Point(grid_x - diamond_search_width,
                                      grid_y - diamond_search_height_));
  Point end = closestPtInRect(core,
                              Point(grid_x + diamond_search_width,
                                    grid_y + diamond_search_height_));;
  int x_start = start.x();
  int y_start = start.y();
  int x_end = end.x();
  int y_end = end.y();

#ifdef ODP_DEBUG
  cout << " == Start Diamond Search ==  " << endl;
  cout << " cell_name : " << cell->name() << endl;
  cout << " x : " << x << endl;
  cout << " y : " << y << endl;
  cout << " grid_x : " << grid_x << endl;
  cout << " grid_y : " << grid_y << endl;
  cout << " x bound ( " << x_start << ") - (" << x_end << ")" << endl;
  cout << " y bound ( " << y_start << ") - (" << y_end << ")" << endl;
#endif
  
  // magic number alert
  int diamond_height = diamond_search_height_
    * ((design_util_ > 0.6 || fixed_inst_count_ > 0) ? 2 : .5);
  // magic number alert
  int diamond_width_factor = 10;
  for(int i = 1; i <  diamond_height; i++) {
    Pixel *pixel = nullptr;
    int best_dist = 0;
    // right side
    for(int j = 1; j < i * 2; j++) {
      int x_offset = -((j + 1) / 2);
      int y_offset = (i * 2 - j) / 2;
      if(j % 2 == 1)
	y_offset = -y_offset;
      if(binSearch(grid_x, cell,
		   // magic number alert
		   min(x_end, max(x_start,
				  grid_x + x_offset * diamond_width_factor)),
		   min(y_end, max(y_start, grid_y + y_offset)),
		   avail_x, avail_y)) {
        Pixel *avail = &grid_[avail_y][avail_x];
	int avail_dist = abs(x - avail->grid_x_ * site_width_) +
	  abs(y - avail->grid_y_ * row_height_);
        if (pixel == nullptr
	    || avail_dist < best_dist) {
	  pixel = avail;
	  best_dist = avail_dist;
	}
      }
    }

    // left side
    for(int j = 1; j < (i + 1) * 2; j++) {
      int x_offset = (j - 1) / 2;
      int y_offset = ((i + 1) * 2 - j) / 2;
      if(j % 2 == 1)
	y_offset = -y_offset;
      if(binSearch(grid_x, cell,
		   min(x_end, max(x_start,
				  grid_x + x_offset * diamond_width_factor)),
		   min(y_end, max(y_start, grid_y + y_offset)),
		   avail_x, avail_y)) {
        Pixel *avail = &grid_[avail_y][avail_x];
	int avail_dist = abs(x - avail->grid_x_ * site_width_) +
	  abs(y - avail->grid_y_ * row_height_);
        if (pixel == nullptr
	    || avail_dist < best_dist) {
	  pixel = avail;
	  best_dist = avail_dist;
	}
      }
    }
    if (pixel)
      return pixel;
  }
  return nullptr;
}

bool Opendp::shift_move(Cell* cell) {
  int x, y;
  initLocation(cell, x, y);
  // set region boundary
  Rect rect;
  // magic number alert
  int boundary_margin = 3;
  rect.reset(max(0, x - paddedWidth(cell) * boundary_margin),
	     max(0, y - cell->height_ * boundary_margin),
	     min(static_cast<int>(core_.dx()), x + paddedWidth(cell) * boundary_margin),
	     min(static_cast<int>(core_.dy()), y + cell->height_ * boundary_margin));
  set< Cell* > overlap_region_cells = get_cells_from_boundary(&rect);

  // erase region cells
  for(Cell* around_cell : overlap_region_cells) {
    if(cell->inGroup() == around_cell->inGroup()) {
      erase_pixel(around_cell);
    }
  }

  // place target cell
  if(!map_move(cell, x, y)) {
    warn("detailed placement failed on %s", cell->name());
    return false;
  }

  // rebuild erased cells
  for(Cell* around_cell : overlap_region_cells) {
    if(cell->inGroup() == around_cell->inGroup()) {
      int x, y;
      initLocation(around_cell, x, y);
      if(!map_move(around_cell, x, y)) {
        return false;
      }
    }
  }
  return true;
}

bool Opendp::map_move(Cell* cell) {
  int init_x, init_y;
  initPaddedLoc(cell, init_x, init_y);
  return map_move(cell, init_x, init_y);
}

bool Opendp::map_move(Cell* cell, int x, int y) {
  Pixel* pixel = diamondSearch(cell, x, y);
  if(pixel) {
    Pixel *near_pixel = diamondSearch(cell, pixel->grid_x_ * site_width_,
				      pixel->grid_y_ * row_height_);
    if(near_pixel) {
      paint_pixel(cell, near_pixel->grid_x_, near_pixel->grid_y_);
    }
    else {
      paint_pixel(cell, pixel->grid_x_, pixel->grid_y_);
    }
    return true;
  }
  else
    return false;
}

vector< Cell* > Opendp::overlap_cells(Cell* cell) {
  int step_x = gridPaddedWidth(cell);
  int step_y = gridHeight(cell);

  vector< Cell* > list;
  set< Cell* > in_list;
  for(int i = gridY(cell); i < gridY(cell) + step_y; i++) {
    for(int j = gridPaddedX(cell); j < gridPaddedX(cell) + step_x; j++) {
      Cell *pos_cell = grid_[i][j].cell;
      if(pos_cell
	 && in_list.find(pos_cell) != in_list.end()) {
	   list.push_back(pos_cell);
	   in_list.insert(pos_cell);
      }
    }
  }
  return list;
}

// rect should be position
set< Cell* > Opendp::get_cells_from_boundary(Rect* rect) {
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

int Opendp::dist_benefit(Cell* cell, int x, int y) {
  int init_x, init_y;
  initLocation(cell, init_x, init_y);
  int curr_dist = abs(cell->x_ - init_x) +
                  abs(cell->y_ - init_y);
  int new_dist = abs(init_x - x) +
                 abs(init_y - y);
  return new_dist - curr_dist;
}

bool Opendp::swap_cell(Cell* cell1, Cell* cell2) {
  if(cell1 != cell2
     && cell1->db_inst_->getMaster() == cell2->db_inst_->getMaster()
     && !isFixed(cell1)
     && !isFixed(cell2)) {
    int benefit = dist_benefit(cell1, cell2->x_, cell2->y_) +
      dist_benefit(cell2, cell1->x_, cell1->y_);

    if(benefit < 0) {
      int grid_x1 = gridPaddedX(cell2);
      int grid_y1 = gridY(cell2);
      int grid_x2 = gridPaddedX(cell1);
      int grid_y2 = gridY(cell1);

      erase_pixel(cell1);
      erase_pixel(cell2);
      paint_pixel(cell1, grid_x1, grid_y1);
      paint_pixel(cell2, grid_x2, grid_y2);
      return true;
    }
  }
  return false;
}

bool Opendp::refine_move(Cell* cell) {
  int init_x, init_y;
  initLocation(cell, init_x, init_y);
  Pixel* pixel = diamondSearch(cell, init_x, init_y);
  if(pixel) {
    double new_dist = abs(init_x - pixel->grid_x_ * site_width_)
      + abs(init_y - pixel->grid_y_ * row_height_);
    if(max_displacement_constraint_
       && (new_dist / row_height_ > max_displacement_constraint_))
      return false;

    int benefit = dist_benefit(cell, pixel->grid_x_ * site_width_,
                               pixel->grid_y_ * row_height_);
    if(benefit < 0) {
      erase_pixel(cell);
      paint_pixel(cell, pixel->grid_x_, pixel->grid_y_);
      return true;
    }
    else
      return false;
  }
  else
    return false;
}

}  // namespace opendp
