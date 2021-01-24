/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu
// Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.
//
// Copyright (c) 2019, OpenROAD
// Copyright (c) 2018, SangGi Do and Mingyu Woo
// All rights reserved.
//
// BSD 3-Clause License
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

#include "opendp/Opendp.h"

#include <cmath>
#include <limits>

#include "utility/Logger.h"

namespace dpl {

using std::max;
using std::min;

using utl::DPL;

void
Opendp::initGrid()
{
  // Make pixel grid
  if (grid_ == nullptr)
    grid_ = makeGrid();
  else
    initGridPixels(grid_);
  // Paint fixed cells.
  assignFixedCells();
  // group mapping & x_axis dummycell insertion
  group_pixel_assign2();
  // y axis dummycell insertion
  group_pixel_assign();
}

Grid *
Opendp::makeGrid()
{
  Grid *grid = new Pixel *[row_count_];
  for (int i = 0; i < row_count_; i++)
    grid[i] = new Pixel[row_site_count_];
  initGridPixels(grid);
  return grid;
}

void
Opendp::initGridPixels(Grid *grid)
{
  for (int i = 0; i < row_count_; i++) {
    for (int j = 0; j < row_site_count_; j++) {
      Pixel &pixel = grid[i][j];
      pixel.cell = nullptr;
      pixel.group_ = nullptr;
      pixel.util = 0.0;
      pixel.is_valid = false;
    }
  }

  // Fragmented row support; mark valid sites.
  for (auto db_row : block_->getRows()) {
    int orig_x, orig_y;
    db_row->getOrigin(orig_x, orig_y);

    int x_start = (orig_x - core_.xMin()) / site_width_;
    int y_start = (orig_y - core_.yMin()) / row_height_;

    int x_end = x_start + db_row->getSiteCount();
    int y_end = y_start + 1;

    for (int i = x_start; i < x_end; i++) {
      for (int j = y_start; j < y_end; j++) {
        grid[j][i].is_valid = true;
      }
    }
  }
}

void
Opendp::deleteGrid(Grid *grid)
{
  if (grid) {
    for (int i = 0; i < row_count_; i++) {
      delete [] grid[i];
    }
    delete [] grid;
  }
}

Pixel *
Opendp::gridPixel(int x,
                  int y) const
{
  if (x >= 0 && x < row_site_count_
      && y >= 0 && y < row_count_)
    return &grid_[y][x];
  else
    return nullptr;
}

////////////////////////////////////////////////////////////////

void
Opendp::assignFixedCells()
{
  for (Cell &cell : cells_) {
    if (isFixed(&cell)) {
      int y_start = gridY(&cell);
      int y_end = gridEndY(&cell);
      int x_start = gridPaddedX(&cell);
      int x_end = gridPaddedEndX(&cell);
      for (int x = x_start; x < x_end; x++) {
        for (int y = y_start; y < y_end; y++) {
          Pixel *pixel = gridPixel(x, y);
          if (pixel) {
            pixel->cell = &cell;
            pixel->util = 1.0;
          }
        }
      }
    }
  }
}

void
Opendp::group_cell_region_assign()
{
  for (Group &group : groups_) {
    int64_t site_count = 0;
    for (int j = 0; j < row_count_; j++) {
      for (int k = 0; k < row_site_count_; k++) {
        Pixel &pixel = grid_[j][k];
        if (pixel.is_valid && pixel.group_ == &group) {
          site_count++;
        }
      }
    }
    int64_t site_area = site_count * site_width_ * row_height_;

    int64_t cell_area = 0;
    for (Cell *cell : group.cells_) {
      cell_area += cell->area();

      for (Rect &rect : group.regions) {
        if (isInside(cell, &rect)) {
          cell->region_ = &rect;
        }
      }
      if (cell->region_ == nullptr) {
        cell->region_ = &group.regions[0];
      }
    }
    group.util = static_cast<double>(cell_area) / site_area;
  }
}

void
Opendp::group_pixel_assign2()
{
  for (int i = 0; i < row_count_; i++) {
    for (int j = 0; j < row_site_count_; j++) {
      Rect sub;
      sub.init(j * site_width_,
               i * row_height_,
               (j + 1) * site_width_,
               (i + 1) * row_height_);
      for (Group &group : groups_) {
        for (Rect &rect : group.regions) {
          if (!isInside(sub, rect) && check_overlap(sub, rect)) {
            Pixel &pixel = grid_[i][j];
            pixel.util = 0.0;
            pixel.cell = &dummy_cell_;
            pixel.is_valid = false;
          }
        }
      }
    }
  }
}

/* static */
bool
Opendp::isInside(const Rect &cell, const Rect &box)
{
  return cell.xMin() >= box.xMin() && cell.xMax() <= box.xMax() && cell.yMin() >= box.yMin() && cell.yMax() <= box.yMax();
}

bool
Opendp::check_overlap(const Rect &cell, const Rect &box)
{
  return box.xMin() < cell.xMax() && box.xMax() > cell.xMin() && box.yMin() < cell.yMax() && box.yMax() > cell.yMin();
}

void
Opendp::group_pixel_assign()
{
  for (int i = 0; i < row_count_; i++) {
    for (int j = 0; j < row_site_count_; j++) {
      grid_[i][j].util = 0.0;
    }
  }

  for (Group &group : groups_) {
    for (Rect &rect : group.regions) {
      int row_start = divCeil(rect.yMin(), row_height_);
      int row_end = divFloor(rect.yMax(), row_height_);

      for (int k = row_start; k < row_end; k++) {
        int col_start = divCeil(rect.xMin(), site_width_);
        int col_end = divFloor(rect.xMax(), site_width_);

        for (int l = col_start; l < col_end; l++) {
          grid_[k][l].util += 1.0;
        }
        if (rect.xMin() % site_width_ != 0) {
          grid_[k][col_start].util -= (rect.xMin() % site_width_) / static_cast<double>(site_width_);
        }
        if (rect.xMax() % site_width_ != 0) {
          grid_[k][col_end - 1].util -= ((site_width_ - rect.xMax()) % site_width_) / static_cast<double>(site_width_);
        }
      }
    }
    for (Rect &rect : group.regions) {
      int row_start = divCeil(rect.yMin(), row_height_);
      int row_end = divFloor(rect.yMax(), row_height_);

      for (int k = row_start; k < row_end; k++) {
        int col_start = divCeil(rect.xMin(), site_width_);
        int col_end = divFloor(rect.xMax(), site_width_);

        // Assign group to each pixel.
        for (int l = col_start; l < col_end; l++) {
          Pixel &pixel = grid_[k][l];
          if (pixel.util == 1.0) {
            pixel.group_ = &group;
            pixel.is_valid = true;
            pixel.util = 1.0;
          }
          else if (pixel.util > 0.0 && pixel.util < 1.0) {
            pixel.cell = &dummy_cell_;
            pixel.util = 0.0;
            pixel.is_valid = false;
          }
        }
      }
    }
  }
}

void
Opendp::erase_pixel(Cell *cell)
{
  if (!(isFixed(cell) || !cell->is_placed_)) {
    int x_end = gridEndX(cell);
    int y_end = gridEndY(cell);
    for (int i = gridY(cell); i < y_end; i++) {
      for (int j = gridPaddedX(cell); j < x_end; j++) {
        Pixel &pixel = grid_[i][j];
        pixel.cell = nullptr;
        pixel.util = 0;
      }
    }
    cell->is_placed_ = false;
    cell->hold_ = false;
  }
}

void
Opendp::paint_pixel(Cell *cell, int grid_x, int grid_y)
{
  assert(!cell->is_placed_);
  int x_step = gridPaddedWidth(cell);
  int y_step = gridHeight(cell);

  setGridPaddedLoc(cell, grid_x, grid_y);
  cell->is_placed_ = true;

#ifdef ODP_DEBUG
  cout << "paint cell : " << cell->name() << endl;
  cout << "x_ - y_ : " << cell->x_ << " - " << cell->y_ << endl;
  cout << "x_step - y_step : " << x_step << " - " << y_step << endl;
  cout << "grid_x - grid_y : " << grid_x << " - " << grid_y << endl;
#endif

  for (int i = grid_y; i < grid_y + y_step; i++) {
    for (int j = grid_x; j < grid_x + x_step; j++) {
      Pixel &pixel = grid_[i][j];
      if (pixel.cell) {
        logger_->error(DPL, 13, "Cannot paint grid because it is already occupied.");
      }
      else {
        pixel.cell = cell;
        pixel.util = 1.0;
      }
    }
  }
  if (have_multi_height_cells_) {
    if (y_step % 2 == 1) {
      if (rowTopPower(grid_y) != topPower(cell)) {
        cell->orient_ = dbOrientType::MX;
      }
      else {
        cell->orient_ = dbOrientType::R0;
      }
    }
  }
  else {
    cell->orient_ = rowOrient(grid_y);
  }
}

}  // namespace opendp
