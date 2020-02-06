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
#include <set>
#include "opendp/Opendp.h"

namespace opendp {

using odb::adsRect;
using odb::dbBox;
using odb::dbITerm;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbPlacementStatus;

using std::abs;
using std::ceil;
using std::cerr;
using std::cout;
using std::endl;
using std::floor;
using std::ifstream;
using std::make_pair;
using std::max;
using std::min;
using std::ofstream;
using std::pair;
using std::round;
using std::string;
using std::to_string;
using std::vector;
using std::set;

void Opendp::power_mapping() {
  power macro_top_power = undefined;
  bool found_multi = false;
  for(Macro& macro : macros_) {
    if(macro.isMulti) {
      found_multi = true;
    }
    if(!macro.isMulti && macro.top_power != power::undefined) {
      macro_top_power = macro.top_power;
      break;
    }
  }

  if(found_multi && initial_power_ == undefined) {
    cout << "ERROR: Cannot find VDD/VSS SPECIALNETS definition" << endl;
    cout << "       Please remove multi-height cells or " << endl;
    cout << "       define SPECIALNETS for VDD/VSS nets." << endl;
    exit(1);
  }

  power opposite_power = undefined;
  if(macro_top_power == power::undefined) {
    cout << "ERROR: Cannot find cells that have VDD/VSS pins." << endl;
    exit(1);
  }
  else if(macro_top_power == VDD) {
    opposite_power = VSS;
  }
  else if(macro_top_power == VSS) {
    opposite_power = VDD;
  }
  //  cout << "VDD " << VDD << " VSS " << VSS << endl;
  //  cout << "initial_power: " << initial_power << endl;
  //  cout << "Power " << macro_top_power << " " << opposite_power << endl;

  for(int i = 0; i < rows_.size(); i++) {
    Row* row = &rows_[i];
    if(initial_power_ == undefined) {
      if(i % 2 == 0) {
        row->top_power = macro_top_power;
      }
      else {
        row->top_power = opposite_power;
      }
    }
    else if(initial_power_ == VDD) {
      if(i % 2 == 0)
        row->top_power = VDD;
      else
        row->top_power = VSS;
    }
    else if(initial_power_ == VSS) {
      if(i % 2 == 0)
        row->top_power = VSS;
      else
        row->top_power = VDD;
    }
    //    cout << "row" << i << " has top_power " << row->top_power << endl;
  }
}

void Opendp::displacementStats(// Return values.
			       int &avg_displacement,
			       int &sum_displacement,
			       int &max_displacement) {
  avg_displacement = 0;
  sum_displacement = 0;
  max_displacement = 0;

  for(Cell& cell : cells_) {
    int displacement = cell.disp();
    sum_displacement += displacement;
    if(displacement > max_displacement)
      max_displacement = displacement;
  }
  avg_displacement = sum_displacement / cells_.size();
}

double Opendp::hpwl(bool initial) {
  int64_t hpwl = 0;
  for(auto net : block_->getNets()) {
    adsRect box;
    box.mergeInit();

    for(auto iterm : net->getITerms()) {
      dbInst* inst = iterm->getInst();
      Cell* cell = db_inst_map_[inst];
      int x, y;
      if(initial) {
        x = cell->init_x_coord;
        y = cell->init_y_coord;
      }
      else {
        x = cell->x_coord;
        y = cell->y_coord;
      }
      // Use inst center if no mpins.
      adsRect iterm_rect(x, y, x, y);
      dbMTerm* mterm = iterm->getMTerm();
      auto mpins = mterm->getMPins();
      if(mpins.size()) {
        // Pick a pin, any pin.
        dbMPin* pin = *mpins.begin();
        auto geom = pin->getGeometry();
        if(geom.size()) {
          dbBox* pin_box = *geom.begin();
          adsRect pin_rect;
          pin_box->getBox(pin_rect);
          int center_x = (pin_rect.xMin() + pin_rect.xMax()) / 2;
          int center_y = (pin_rect.yMin() + pin_rect.yMax()) / 2;
          iterm_rect =
              adsRect(x + center_x, y + center_y, x + center_x, y + center_y);
        }
      }
      box.merge(iterm_rect);
    }

    for(auto bterm : net->getBTerms()) {
      for(auto bpin : bterm->getBPins()) {
        dbPlacementStatus status = bpin->getPlacementStatus();
        if(status.isPlaced()) {
          dbBox* pin_box = bpin->getBox();
          adsRect pin_rect;
          pin_box->getBox(pin_rect);
          int center_x = (pin_rect.xMin() + pin_rect.xMax()) / 2;
          int center_y = (pin_rect.yMin() + pin_rect.yMax()) / 2;
          int core_center_x = center_x - core_.xMin();
          int core_center_y = center_y - core_.yMin();
          adsRect pin_center(core_center_x, core_center_y, core_center_x,
                             core_center_y);
          box.merge(pin_center);
        }
      }
    }
    int perimeter = box.dx() + box.dy();
    hpwl += perimeter;
  }
  return dbuToMicrons(hpwl);
}

void Opendp::group_analyze() {
  for(Group& group : groups_) {
    double region_area = 0;
    double avail_region_area = 0;

    for(adsRect& rect : group.regions) {
      region_area += (rect.xMax() - rect.xMin()) * (rect.yMax() - rect.yMin());
      avail_region_area += (rect.xMax() - rect.xMin() - rect.xMax() % 200 +
			    rect.xMin() % 200 - 200) *
	(rect.yMax() - rect.yMin() - rect.yMax() % 2000 +
	 rect.yMin() % 2000 - 2000);
    }

    double cell_area = 0;
    for(Cell* cell : group.siblings)
      cell_area += cell->width * cell->height;

    cout << " GROUP : " << group.name << endl;
    cout << " region count : " << group.regions.size() << endl;
    cout << " cell count : " << group.siblings.size() << endl;
    cout << " region area : " << region_area << endl;
    cout << " avail region area : " << avail_region_area << endl;
    cout << " cell area : " << cell_area << endl;
    cout << " utilization : " << cell_area / region_area << endl;
    cout << " avail util : " << cell_area / avail_region_area << endl;
    cout << " - - - - - - - - - - - - - - - - - - - - " << endl;
  }
}

pair< int, int > Opendp::nearest_coord_to_rect_boundary(Cell* cell,
                                                        adsRect* rect) {
  int x = cell->init_x_coord;
  int y = cell->init_y_coord;
  int size_x = gridNearestWidth(cell);
  int size_y = gridNearestHeight(cell);
  int temp_x = x;
  int temp_y = y;

  if(check_overlap(cell, rect)) {
    int dist_x = 0;
    int dist_y = 0;
    if(abs(x - rect->xMin() + cell->width) > abs(rect->xMax() - x)) {
      dist_x = abs(rect->xMax() - x);
      temp_x = rect->xMax();
    }
    else {
      dist_x = abs(x - rect->xMin());
      temp_x = rect->xMin() - cell->width;
    }
    if(abs(y - rect->yMin() + cell->height) > abs(rect->yMax() - y)) {
      dist_y = abs(rect->yMax() - y);
      temp_y = rect->yMax();
    }
    else {
      dist_y = abs(y - rect->yMin());
      temp_y = rect->yMin() - cell->height;
    }
    assert(dist_x > -1);
    assert(dist_y > -1);
    if(dist_x < dist_y)
      return make_pair(temp_x, y);
    else
      return make_pair(x, temp_y);
  }

  if(x < rect->xMin())
    temp_x = rect->xMin();
  else if(x + cell->width > rect->xMax())
    temp_x = rect->xMax() - cell->width;

  if(y < rect->yMin())
    temp_y = rect->yMin();
  else if(y + cell->height > rect->yMax())
    temp_y = rect->yMax() - cell->height;

#ifdef ODP_DEBUG
  cout << " - - - - - - - - - - - - - - - " << endl;
  cout << " input x_coord : " << x << endl;
  cout << " input y_coord : " << y << endl;
  cout << " found x_coord : " << temp_x << endl;
  cout << " found y_coord : " << temp_y << endl;
#endif

  return make_pair(temp_x, temp_y);
}

int Opendp::dist_for_rect(Cell* cell, adsRect* rect) {
  int x = cell->init_x_coord;
  int y = cell->init_y_coord;
  int temp_x = 0;
  int temp_y = 0;

  if(x < rect->xMin())
    temp_x = rect->xMin() - x;
  else if(x + cell->width > rect->xMax())
    temp_x = x + cell->width - rect->xMax();

  if(y < rect->yMin())
    temp_y = rect->yMin() - y;
  else if(y + cell->height > rect->yMax())
    temp_y = y + cell->height - rect->yMax();

  assert(temp_y > -1);
  assert(temp_x > -1);

  return temp_y + temp_x;
}

bool Opendp::check_overlap(adsRect cell, adsRect box) {
  if(box.xMin() >= cell.xMax() || box.xMax() <= cell.xMin()) return false;
  if(box.yMin() >= cell.yMax() || box.yMax() <= cell.yMin()) return false;
  return true;
}

bool Opendp::check_overlap(Cell* cell, adsRect* rect) {
  int x = cell->init_x_coord;
  int y = cell->init_y_coord;

  if(rect->xMax() <= x || rect->xMin() >= x + cell->width) return false;
  if(rect->yMax() <= y || rect->yMin() >= y + cell->height) return false;

  return true;
}

bool Opendp::check_inside(adsRect cell, adsRect box) {
  if(box.xMin() > cell.xMin() || box.xMax() < cell.xMax()) return false;
  if(box.yMin() > cell.yMin() || box.yMax() < cell.yMax()) return false;
  return true;
}

bool Opendp::check_inside(Cell* cell, adsRect* rect) {
  int x = cell->init_x_coord;
  int y = cell->init_y_coord;

  if(rect->xMax() < x + cell->width || rect->xMin() > x) return false;
  if(rect->yMax() < y + cell->height || rect->yMin() > y) return false;

  return true;
}

pair< bool, pair< int, int > > Opendp::bin_search(int x_pos, Cell* cell,
                                                  int x, int y) {
  pair< int, int > pos;
  Macro* macro = cell->cell_macro;

  // EDGETYPE 1 - 1 : 400, 1 - 2 : 400, 2 - 2 : 0
  int edge_left = 0;
  int edge_right = 0;
  if(macro->edgetypeLeft == 1) edge_left = 2;
  if(macro->edgetypeRight == 1) edge_right = 2;

  int x_step = gridWidth(cell) + edge_left + edge_right;
  int y_step = gridHeight(cell);

  // IF y is out of border
  if(y + y_step > (core_.yMax() / row_height_)) return make_pair(false, pos);

  // If even number multi-deck cell -> check top power
  if(y_step % 2 == 0) {
    //    cout << cell->name << " power: " << rows[y].top_power << " " <<
    //    macro->top_power << endl;
    if(rows_[y].top_power == macro->top_power) return make_pair(false, pos);
  }

#ifdef ODP_DEBUG
  cout << " - - - - - - - - - - - - - - - - - " << endl;
  cout << " Start Bin Search " << endl;
  cout << " cell name : " << cell->name() << endl;
  cout << " target x : " << x << endl;
  cout << " target y : " << y << endl;
#endif
  if(x_pos > x) {
    for(int i = 9; i > -1; i--) {
      // check all grids are empty
      bool available = true;

      if(x + i + x_step > coreGridMaxX()) {
        available = false;
      }
      else {
        for(int k = y; k < y + y_step; k++) {
          for(int l = x + i; l < x + i + x_step; l++) {
	    // cout << "BinSearch: chk " << k << " " << l << endl;
	    if(grid_[k][l].linked_cell != NULL || !grid_[k][l].is_valid) {
	      // cout << "BinSearch: chk " << k << " " << l << " NonEmpty" << endl;
              available = false;
              break;
            }
            // check group regions
            if(cell->inGroup()) {
              if(grid_[k][l].pixel_group != cell->cell_group)
                available = false;
            }
            else {
              if(grid_[k][l].pixel_group != nullptr) available = false;
            }
          }
          if(!available) break;
        }
      }
      if(available) {
        if(edge_left == 0)
          pos = make_pair(y, x + i);
        else
          pos = make_pair(y, x + i + edge_left);

	// cout << "Bin Search Success: " << pos.first << " " << pos.second
	//    << " edgeLeft: " << edge_left << endl;
        return make_pair(available, pos);
      }
    }
  }
  else {
    for(int i = 0; i < 10; i++) {
      // check all grids are empty
      bool available = true;
      if(x + i + x_step > coreGridMaxX()) {
        available = false;
      }
      else {
        for(int k = y; k < y + y_step; k++) {
          for(int l = x + i; l < x + i + x_step; l++) {
            if(grid_[k][l].linked_cell != NULL || !grid_[k][l].is_valid) {
              available = false;
              break;
            }
            // check group regions
            if(cell->inGroup()) {
              if(grid_[k][l].pixel_group != cell->cell_group)
                available = false;
            }
            else {
              if(grid_[k][l].pixel_group != nullptr) available = false;
            }
          }
          if(!available) break;
        }
      }
      if(available) {
#ifdef ODP_DEBUG
        cout << " found pos x - y : " << x << " - " << y << " Finish Search "
             << endl;
        cout << " - - - - - - - - - - - - - - - - - - - - - - - - " << endl;
#endif
        if(edge_left == 0)
          pos = make_pair(y, x + i);
        else
          pos = make_pair(y, x + i + edge_left);

        return make_pair(available, pos);
      }
    }
  }
  return make_pair(false, pos);
}

pair< bool, Pixel* > Opendp::diamond_search(Cell* cell, int x_coord,
                                            int y_coord) {
  Pixel* pixel = NULL;
  pair< bool, pair< int, int > > found;
  int x_pos = gridX(x_coord);
  int y_pos = gridY(y_coord);

  int x_start = 0;
  int x_end = 0;
  int y_start = 0;
  int y_end = 0;

  // set search boundary max / min
  if(cell->inGroup()) {
    Group* group = cell->cell_group;
    x_start = max(x_pos - diamond_search_height_ * 5,
                  group->boundary.xMin() / site_width_);
    x_end = min(x_pos + diamond_search_height_ * 5,
		group->boundary.xMax() / site_width_ - gridNearestWidth(cell));
    y_start = max(y_pos - diamond_search_height_,
                  divCeil(group->boundary.yMin(), row_height_));
    y_end = min(y_pos + diamond_search_height_,
                divCeil(group->boundary.yMax(), row_height_) - gridNearestHeight(cell));
  }
  else {
    x_start = max(x_pos - diamond_search_height_ * 5, 0);
    x_end = min(x_pos + diamond_search_height_ * 5,
                gridWidth() - gridNearestWidth(cell));
    y_start = max(y_pos - diamond_search_height_, 0);
    y_end = min(y_pos + diamond_search_height_, 
		gridHeight() - gridNearestHeight(cell));
  }
#ifdef ODP_DEBUG
  cout << " == Start Diamond Search ==  " << endl;
  cout << " cell_name : " << cell->name() << endl;
  cout << " cell width : " << cell->width << endl;
  cout << " cell height : " << cell->height << endl;
  cout << " cell x step : " << gridNearestWidth(cell)
       << endl;
  cout << " cell y step : " << gridNearestHeight(cell)
       << endl;
  cout << " input x : " << x_coord << endl;
  cout << " inpuy y : " << y_coord << endl;
  cout << " x_pos : " << x_pos << endl;
  cout << " y_pos : " << y_pos << endl;
  cout << " x bound ( " << x_start << ") - (" << x_end << ")" << endl;
  cout << " y bound ( " << y_start << ") - (" << y_end << ")" << endl;
#endif
  found = bin_search(x_pos, cell, min(x_end, max(x_start, x_pos)),
                     max(y_start, min(y_end, y_pos)));
  if(found.first) {
    //    cout << "DiamondSearch: found: " << found.second.first << " - " <<
    //    found.second.second << endl;
    pixel = &grid_[found.second.first][found.second.second];
    return make_pair(found.first, pixel);
  }

  int div = 4;
  if(design_util_ > 0.6 || fixed_inst_count_ > 0) div = 1;

  for(int i = 1; i < diamond_search_height_ * 2 / div; i++) {
    vector< Pixel* > avail_list;
    avail_list.reserve(i * 4);
    Pixel* pixel = NULL;

    int x_offset = 0;
    int y_offset = 0;

    // right side
    for(int j = 1; j < i * 2; j++) {
      x_offset = -((j + 1) / 2);
      if(j % 2 == 1)
        y_offset = (i * 2 - j) / 2;
      else
        y_offset = -(i * 2 - j) / 2;
      found = bin_search(x_pos, cell,
                         min(x_end, max(x_start, (x_pos + x_offset * 10))),
                         min(y_end, max(y_start, (y_pos + y_offset))));
      if(found.first) {
        pixel = &grid_[found.second.first][found.second.second];
        avail_list.push_back(pixel);
      }
    }

    // left side
    for(int j = 1; j < (i + 1) * 2; j++) {
      x_offset = (j - 1) / 2;
      if(j % 2 == 1)
        y_offset = ((i + 1) * 2 - j) / 2;
      else
        y_offset = -((i + 1) * 2 - j) / 2;
      found = bin_search(x_pos, cell,
                         min(x_end, max(x_start, (x_pos + x_offset * 10))),
                         min(y_end, max(y_start, (y_pos + y_offset))));
      if(found.first) {
        pixel = &grid_[found.second.first][found.second.second];
        avail_list.push_back(pixel);
      }
    }

    // check from vector
    unsigned dist = UINT_MAX;
    int best = INT_MAX;
    for(int j = 0; j < avail_list.size(); j++) {
      int temp_dist = abs(x_coord - avail_list[j]->x_pos * site_width_) +
                      abs(y_coord - avail_list[j]->y_pos * row_height_);
      if(temp_dist < dist) {
        dist = temp_dist;
        best = j;
      }
    }
    if(best != INT_MAX) {
      return make_pair(true, avail_list[best]);
    }
  }
  return make_pair(false, pixel);
}

bool Opendp::shift_move(Cell* cell) {
  int x = cell->init_x_coord;
  int y = cell->init_y_coord;
  //	cout << " shift_move start " << endl;
  // set region boundary
  adsRect rect;
  rect.reset(max(core_.xMin(), x - cell->width * 3),
	     max(core_.yMin(), y - cell->height * 3),
	     min(core_.xMax(), x + cell->width * 3),
	     min(core_.yMax(), y + cell->height * 3));
  vector< Cell* > overlap_region_cells = get_cells_from_boundary(&rect);

  // erase region cells
  for(int i = 0; i < overlap_region_cells.size(); i++) {
    Cell* around_cell = overlap_region_cells[i];
    if(cell->inGroup() == around_cell->inGroup()) {
      erase_pixel(around_cell);
    }
  }

  // place target cell
  if(!map_move(cell, x, y)) {
    cout << "Error: cannot insert center cell" << cell->name()
	 << " target (" << x << " " << y << ")" << endl;
    return false;
  }

  // rebuild erased cells
  for(Cell* around_cell : overlap_region_cells) {
    if(cell->inGroup() == around_cell->inGroup()) {
      if(!map_move(around_cell, around_cell->init_x_coord,
                   around_cell->init_y_coord)) {
#ifdef ODP_DEBUG
        cout << "Error: shift move failure for cell "
	     << around_cell->name() << "("
	     << around_cell->init_x_coord << ", "
	     << around_cell->init_y_coord << ")" << endl;
#endif
        return false;
      }
    }
  }
  return true;
}

bool Opendp::map_move(Cell* cell) {
  return map_move(cell, cell->init_x_coord, cell->init_y_coord);
}

bool Opendp::map_move(Cell* cell, int x, int y) {
  pair< bool, Pixel* > pixel = diamond_search(cell, x, y);
  if(pixel.first) {
    pair< bool, Pixel* > nearPixel =
        diamond_search(cell, pixel.second->x_pos * site_width_,
                       pixel.second->y_pos * row_height_);
    if(nearPixel.first) {
      paint_pixel(cell, nearPixel.second->x_pos, nearPixel.second->y_pos);
    }
    else
      paint_pixel(cell, pixel.second->x_pos, pixel.second->y_pos);
    return true;
  }
  else {
#ifdef ODP_DEBUG
    cout << "Error: map move failure " << cell->name()
	 << " (" << cell->init_x_coord << ", "
	 << cell->init_y_coord << ")" << endl;
#endif
    return false;
  }
}

vector< Cell* > Opendp::overlap_cells(Cell* cell) {
  int step_x = gridWidth(cell);
  int step_y = gridHeight(cell);

  vector< Cell* > list;
  set< Cell* > in_list;
  for(int i = cell->y_pos; i < cell->y_pos + step_y; i++) {
    for(int j = cell->x_pos; j < cell->y_pos + step_x; j++) {
      Cell *pos_cell = grid_[i][j].linked_cell;
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
vector< Cell* > Opendp::get_cells_from_boundary(adsRect* rect) {
  // inside
  assert(rect->xMin() >= core_.xMin());
  assert(rect->yMin() >= core_.yMin());
  assert(rect->xMax() <= core_.xMax());
  assert(rect->yMax() <= core_.yMax());

  int x_start = divRound(rect->xMin(), site_width_);
  int y_start = divRound(rect->yMin(), row_height_);
  int x_end = divRound(rect->xMax(), site_width_);
  int y_end = divRound(rect->yMax(), row_height_);

  vector< Cell* > list;
  set< Cell* > in_list;
  for(int i = y_start; i < y_end; i++) {
    for(int j = x_start; j < x_end; j++) {
      Cell *pos_cell = grid_[i][j].linked_cell;
      if(pos_cell
	 && !isFixed(pos_cell)
	 && in_list.find(pos_cell) != in_list.end()) {
	list.push_back(pos_cell);
	in_list.insert(pos_cell);
      }
    }
  }
  return list;
}

int Opendp::dist_benefit(Cell* cell, int x_coord, int y_coord) {
  int curr_dist = abs(cell->x_coord - cell->init_x_coord) +
                  abs(cell->y_coord - cell->init_y_coord);
  int new_dist = abs(cell->init_x_coord - x_coord) +
                 abs(cell->init_y_coord - y_coord);
  return new_dist - curr_dist;
}

bool Opendp::swap_cell(Cell* cellA, Cell* cellB) {
  if(cellA == cellB)
    return false;
  else if(cellA->cell_macro != cellB->cell_macro)
    return false;
  else if(isFixed(cellA) || isFixed(cellB))
    return false;

  int benefit = dist_benefit(cellA, cellB->x_coord, cellB->y_coord) +
                dist_benefit(cellB, cellA->x_coord, cellA->y_coord);

  if(benefit < 0) {
    int A_x_pos = cellB->x_pos;
    int A_y_pos = cellB->y_pos;
    int B_x_pos = cellA->x_pos;
    int B_y_pos = cellA->y_pos;

    erase_pixel(cellA);
    erase_pixel(cellB);
    paint_pixel(cellA, A_x_pos, A_y_pos);
    paint_pixel(cellB, B_x_pos, B_y_pos);
    // cout << "swap benefit : " << benefit << endl;
    // save_score();
    return true;
  }
  return false;
}

bool Opendp::refine_move(Cell* cell) {
  int x_coord = cell->init_x_coord;
  int y_coord = cell->init_y_coord;
  pair< bool, Pixel* > pixel = diamond_search(cell, x_coord, y_coord);
  if(pixel.first) {
    double new_dist =
        abs(cell->init_x_coord - pixel.second->x_pos * site_width_) +
        abs(cell->init_y_coord - pixel.second->y_pos * row_height_);
    if(new_dist / row_height_ > max_displacement_constraint_) return false;

    int benefit = dist_benefit(cell, pixel.second->x_pos * site_width_,
                               pixel.second->y_pos * row_height_);
    if(benefit < 0) {
      // cout << " refine benefit : " << benefit << " : " << 2001 -
      erase_pixel(cell);
      paint_pixel(cell, pixel.second->x_pos, pixel.second->y_pos);
      // save_score();
      return true;
    }
    else
      return false;
  }
  else
    return false;
}

pair< bool, Cell* > Opendp::nearest_cell(int x_coord, int y_coord) {
  bool found = false;
  Cell* nearest_cell = NULL;
  double nearest_dist = 99999999999;
  for(int i = 0; i < cells_.size(); i++) {
    Cell* cell = &cells_[i];
    if(cell->is_placed) {
      double dist =
        abs(cell->x_coord - x_coord) + abs(cell->y_coord - y_coord);
      if(dist < row_height_ * 2) {
	if(nearest_dist > dist) {
	  nearest_dist = dist;
	  nearest_cell = cell;
	  found = true;
	}
      }
    }
  }
  return make_pair(found, nearest_cell);
}

}  // namespace opendp
