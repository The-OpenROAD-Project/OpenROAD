/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.

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
#include "opendp/Opendp.h"

namespace opendp {

using std::cerr;
using std::cout;
using std::endl;
using std::max;
using std::min;
using std::abs;
using std::pair;
using std::round;

// Fixed cell handle on parser ( no need to use this function during placement)
// //
void Opendp::fixed_cell_assign() {
  for(Cell &cell : cells_) {
    if(isFixed(&cell)) {
      Macro* macro = cell.cell_macro;

      int y_start = gridY(&cell);
      int y_end = y_start + gridHeight(&cell);
      int x_start = gridX(&cell);
      int x_end = x_start + gridWidth(&cell);

      int y_start_rf = 0;
      int y_end_rf = gridHeight();
      int x_start_rf = 0;
      int x_end_rf = gridWidth();

      y_start = max(y_start, y_start_rf);
      y_end = min(y_end, y_end_rf);
      x_start = max(x_start, x_start_rf);
      x_end = min(x_end, x_end_rf);

#ifdef ODP_DEBUG
      cout << "FixedCellAssign: cell_name : "
           << cell.name() << endl;
      cout << "FixedCellAssign: y_start : " << y_start << endl;
      cout << "FixedCellAssign: y_end   : " << y_end << endl;
      cout << "FixedCellAssign: x_start : " << x_start << endl;
      cout << "FixedCellAssign: x_end   : " << x_end << endl;
#endif
      for(int j = y_start; j < y_end; j++) {
        for(int k = x_start; k < x_end; k++) {
          grid_[j][k].linked_cell = &cell;
          grid_[j][k].util = 1.0;
        }
      }
    }
  }
}

void Opendp::print_pixels() {
  cout << " print grid " << endl;

  for(int i = 0; i < rows_.size(); i++) {
    for(int j = 0; j < row_site_count_; j++) {
      // cout << grid[i][j].util << " ";
      if(grid_[i][j].util > 0.00001) {
        cout << grid_[i][j].util << " ";
        if(grid_[i][j].pixel_group == nullptr) {
          cout << " no_group ";
        }
        else {
          cout << grid_[i][j].pixel_group->name << " ";
        }
      }
    }
    cout << endl;
  }

  cout << " end print grid" << endl;
}

void Opendp::group_cell_region_assign() {
  for(Group& group : groups_) {
    int64_t area = 0;
    for(int j = 0; j < rows_.size(); j++) {
      for(int k = 0; k < row_site_count_; k++) {
        if(grid_[j][k].pixel_group != nullptr) {
          if(grid_[j][k].is_valid) {
            if(grid_[j][k].pixel_group == &group)
              area += site_width_ * row_height_;
          }
        }
      }
    }

    int64_t cell_area = 0;
    for(Cell* cell : group.siblings) {
      cell_area += cell->width * cell->height;
      int dist = INT_MAX;
      adsRect* region_backup = nullptr;
      for(adsRect &rect : group.regions) {
        if(check_inside(cell, &rect)) cell->region = &rect;
        int temp_dist = dist_for_rect(cell, &rect);
        if(temp_dist < dist) {
          dist = temp_dist;
          region_backup = &rect;
        }
      }
      if(cell->region == nullptr) {
        cell->region = region_backup;
      }
    }
    group.util = static_cast<double>(cell_area) / area;
  }
}

void Opendp::non_group_cell_region_assign() {
  unsigned non_group_cell_count = 0;
  unsigned cell_num_check = 0;
  unsigned fixed_cell_count = 0;

  for(Cell &cell : cells_) {
    if(isFixed(&cell)) {
      fixed_cell_count++;
    }
    else if(!cell.inGroup())
      non_group_cell_count++;
  }

  int group_num = non_group_cell_count / 5000;

  if(group_num == 0) group_num = 1;

  int x_step = core_.dx() / group_num;
  sub_regions_.reserve(group_num);

#ifdef ODP_DEBUG
  cout << "fixed_cell_count : " << fixed_cell_count << endl;
  cout << "non_group_cell_count : " << non_group_cell_count << endl;
  cout << "group_num : " << group_num << endl;
  cout << "x_step : " << x_step << endl;
#endif

  for(int j = 0; j < group_num; j++) {
    sub_region theSub;

    theSub.boundary.init(j * x_step,
			 0,
			 min((j + 1) * x_step, static_cast<int>(core_.dx())),
			 core_.dy());

    for(Cell &cell : cells_) {
      if(!isFixed(&cell) && !cell.inGroup()) {
#ifdef ODP_DEBUG
        cell.print();
        cout << "j: " << j << endl;
        cout << "Xmin: " << theSub.boundary.xMin() << endl;
        cout << " sibilings size : " << theSub.siblings.size() << endl;
#endif
	if(cell.init_x_coord >= j * x_step &&
	   cell.init_x_coord < (j + 1) * x_step) {
	  theSub.siblings.push_back(&cell);
	  cell_num_check++;
	}
	else if(j == 0 && cell.init_x_coord < 0.0) {
	  theSub.siblings.push_back(&cell);
	  cell_num_check++;
	}
	else if(j == group_num - 1 && cell.init_x_coord >= core_.dx()) {
#ifdef ODP_DEBUG
	  cell.print();
	  cout << "j: " << j << endl;
	  cout << "xMin: " << theSub.boundary.xMin() << endl;
	  cout << " sibilings size : " << theSub.siblings.size() << endl;
#endif
	  theSub.siblings.push_back(&cell);
	  cell_num_check++;
	}
      }
    }
    sub_regions_.push_back(theSub);
  }
#ifdef ODP_DEBUG
  cout << "non_group_cell_count : " << non_group_cell_count << endl;
  cout << "cell_num_check : " << cell_num_check << endl;
  cout << "fixed_cell_count : " << fixed_cell_count << endl;
  cout << "sub_region_num : " << sub_regions.size() << endl;
  cout << "- - - - - - - - - - - - - - - - -" << endl;
#endif
  assert(non_group_cell_count == cell_num_check);
}

void Opendp::group_pixel_assign2() {
  for(int64_t i = 0; i < rows_.size(); i++) {
    Row* row = &rows_[i];
    for(int64_t j = 0; j < row_site_count_; j++) {
      adsRect grid2;
      grid2.init(j * site_width_, i * row_height_,
		(j + 1) * site_width_, (i + 1) * row_height_);
      for(Group& group : groups_) {
        for(adsRect &rect : group.regions) {
	  if(!check_inside(grid2, rect) &&
             check_overlap(grid2, rect)) {
            grid_[i][j].util = 0.0;
            grid_[i][j].linked_cell = &dummy_cell_;
            grid_[i][j].is_valid = false;
            // cout << "invalid grid[" << i << "][" << j << "] marked" << endl;
          }
        }
      }
    }
  }
}

void Opendp::group_pixel_assign() {
  for(int i = 0; i < rows_.size(); i++) {
    Row* row = &rows_[i];
    for(int j = 0; j < row_site_count_; j++) {
      grid_[i][j].util = 0.0;
    }
  }

  for(Group& group : groups_) {
    for(adsRect &rect : group.regions) {
      int row_start = divCeil(rect.yMin(), row_height_);
      int row_end = divFloor(rect.yMax(), row_height_);

      for(int k = row_start; k < row_end; k++) {
	Row* row = &rows_[k];
        int col_start = divCeil(rect.xMin(), site_width_);
        int col_end = divFloor(rect.xMax(), site_width_);

        for(int l = col_start; l < col_end; l++) {
          grid_[k][l].util += 1.0;
        }
        if(static_cast<int>(rect.xMin()) % site_width_ != 0) {
          grid_[k][col_start].util -=
	    (rect.xMin() % site_width_) / (double)site_width_;
        }
        if(static_cast<int>(rect.xMax()) % site_width_ != 0) {
          grid_[k][col_end - 1].util -=
	    ((200 - rect.xMax()) % site_width_) / (double)site_width_;
        }
      }
    }
    for(adsRect& rect : group.regions) {      
      int row_start = divCeil(rect.yMin(), row_height_);
      int row_end = divFloor(rect.yMax(), row_height_);

      for(int k = row_start; k < row_end; k++) {
        Row* row = &rows_[k];
        int col_start = divCeil(rect.xMin(), site_width_);
        int col_end = divFloor(rect.xMax(), site_width_);

        // assign groupid to each pixel ( grid )
        for(int l = col_start; l < col_end; l++) {
          if(abs(grid_[k][l].util - 1.0) < 1e-6) {
            grid_[k][l].pixel_group = &group;

            // Can have FIXED cells, so no need to initialize as NULL
            // grid[k][l].linked_cell = NULL;
            grid_[k][l].is_valid = true;
            grid_[k][l].util = 1.0;
            // cout << "GroupPixelAssign: " << k << " " << l << " cleanned." << endl;
          }
          else if(grid_[k][l].util > 0 && grid_[k][l].util < 1) {
#ifdef ODP_DEBUG
            cout << "grid[" << k << "][" << l << "]" << endl;
            cout << "util : " << grid_[k][l].util << endl;
#endif
            grid_[k][l].linked_cell = &dummy_cell_;
            grid_[k][l].util = 0.0;
            grid_[k][l].is_valid = false;
          }
        }
      }
    }
  }
}

void Opendp::erase_pixel(Cell* cell) {
  if(!(isFixed(cell) || !cell->is_placed)) {
    Macro* macro = cell->cell_macro;
    int x_step = gridWidth(cell);
    int y_step = gridHeight(cell);

    cell->is_placed = false;
    cell->hold = false;

    assert(cell->x_pos == gridNearestX(cell));
    assert(cell->y_pos == gridNearestY(cell));
    for(int i = cell->y_pos; i < cell->y_pos + y_step; i++) {
      for(int j = cell->x_pos; j < cell->x_pos + x_step; j++) {
	grid_[i][j].linked_cell = NULL;
	grid_[i][j].util = 0;
      }
    }
    cell->x_coord = 0;
    cell->y_coord = 0;
    cell->x_pos = 0;
    cell->y_pos = 0;
  }
}

void Opendp::paint_pixel(Cell* cell, int x_pos, int y_pos) {
  assert(!cell->is_placed);
  Macro* macro = cell->cell_macro;
  int x_step = gridWidth(cell);
  int y_step = gridHeight(cell);

  cell->x_pos = x_pos;
  cell->y_pos = y_pos;
  cell->x_coord = x_pos * site_width_;
  cell->y_coord = y_pos * row_height_;
  cell->is_placed = true;
#ifdef ODP_DEBUG
  cout << "paint cell : " << cell->name() << endl;
  cout << "init_x_coord - init_y_coord : " << cell->init_x_coord << " - "
       << cell->init_y_coord << endl;
  cout << "x_coord - y_coord : " << cell->x_coord << " - "
       << cell->y_coord << endl;
  cout << "x_step - y_step : " << x_step << " - " << y_step << endl;
  cout << "x_pos - y_pos : " << x_pos << " - " << y_pos << endl;
#endif
  for(int i = y_pos; i < y_pos + y_step; i++) {
    for(int j = x_pos; j < x_pos + x_step; j++) {
      if(grid_[i][j].linked_cell != NULL) {
        cerr << " Cannot paint grid [" << i << "][" << j << "]" << endl;
        cerr << " cell name : "
             << grid_[i][j].linked_cell->name()
             << " already occupied grid" << endl;
        exit(2);
      }
      else {
        grid_[i][j].linked_cell = cell;
        grid_[i][j].util = 1.0;
      }
    }
  }

  if(max_cell_height_ > 1) {
    if(y_step % 2 == 1) {
      if(rows_[y_pos].top_power != macro->top_power)
        cell->db_inst->setOrient(dbOrientType::MX);
      else
        cell->db_inst->setOrient(dbOrientType::R0);
    }
  }
  else {
    cell->db_inst->setOrient(rows_[y_pos].orient);
  }
}

}  // namespace opendp
