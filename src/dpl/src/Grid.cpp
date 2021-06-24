/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu
// Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "dpl/Opendp.h"

#include <cmath>
#include <limits>

#include "opendb/dbTransform.h"
#include "utl/Logger.h"

namespace dpl {

using std::max;
using std::min;

using utl::DPL;

using odb::dbBox;
using odb::dbTransform;

void
Opendp::initGrid()
{
  // Make pixel grid
  if (grid_ == nullptr) {
    grid_ = new Pixel *[row_count_];
    for (int y = 0; y < row_count_; y++)
      grid_[y] = new Pixel[row_site_count_];
  }

  // Init pixels.
  for (int y = 0; y < row_count_; y++) {
    for (int x = 0; x < row_site_count_; x++) {
      Pixel &pixel = grid_[y][x];
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
    int x_end = x_start + db_row->getSiteCount();
    int y = (orig_y - core_.yMin()) / row_height_;

    for (int x = x_start; x < x_end; x++) {
      grid_[y][x].is_valid = true;
    }
  }
}

void
Opendp::deleteGrid()
{
  if (grid_) {
    for (int i = 0; i < row_count_; i++) {
      delete [] grid_[i];
    }
    delete [] grid_;
  }
  grid_ = nullptr;
}

Pixel *
Opendp::gridPixel(int grid_x,
                  int grid_y) const
{
  if (grid_x >= 0 && grid_x < row_site_count_
      && grid_y >= 0 && grid_y < row_count_)
    return &grid_[grid_y][grid_x];
  else
    return nullptr;
}

////////////////////////////////////////////////////////////////

void
Opendp::visitCellPixels(Cell &cell,
                        bool padded,
                        const std::function <void (Pixel *pixel)>& visitor) const
{
  dbInst *inst = cell.db_inst_;
  dbMaster *master = inst->getMaster();
  auto obstructions = master->getObstructions();
  bool have_obstructions = false;
  for (dbBox *obs : obstructions) {
    if (obs->getTechLayer()->getType() == odb::dbTechLayerType::Value::OVERLAP) {
      have_obstructions = true;

      Rect rect;
      obs->getBox(rect);
      dbTransform transform;
      inst->getTransform(transform);
      transform.apply(rect);
      int x_start = gridX(rect.xMin() - core_.xMin());
      int x_end = gridEndX(rect.xMax() - core_.xMin());
      int y_start = gridY(rect.yMin() - core_.yMin());
      int y_end = gridEndY(rect.yMax() - core_.yMin());
      for (int x = x_start; x < x_end; x++) {
        for (int y = y_start; y < y_end; y++) {
          Pixel *pixel = gridPixel(x, y);
          if (pixel)
            visitor(pixel);
        }
      }
    }
  }
  if (!have_obstructions) {
    int x_start = padded ? gridPaddedX(&cell) : gridX(&cell);
    int x_end = padded ? gridPaddedEndX(&cell) : gridEndX(&cell);
    int y_start = gridY(&cell);
    int y_end = gridEndY(&cell);
    for (int x = x_start; x < x_end; x++) {
      for (int y = y_start; y < y_end; y++) {
        Pixel *pixel = gridPixel(x, y);
        if (pixel)
          visitor(pixel);
      }
    }
  }
}

void
Opendp::setFixedGridCells()
{
  for (Cell &cell : cells_) {
    if (isFixed(&cell))
      visitCellPixels(cell, true,
                      [&] (Pixel *pixel) { setGridCell(cell, pixel); } );
  }
}

void
Opendp::setGridCell(Cell &cell,
                    Pixel *pixel)
{
  pixel->cell = &cell;
  pixel->util = 1.0;
}

void
Opendp::groupAssignCellRegions()
{
  for (Group &group : groups_) {
    int64_t site_count = 0;
    for (int x = 0; x < row_site_count_; x++) {
      for (int y = 0; y < row_count_; y++) {
        Pixel *pixel = gridPixel(x, y);
        if (pixel->is_valid && pixel->group_ == &group) {
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
Opendp::groupInitPixels2()
{
  for (int x = 0; x < row_site_count_; x++) {
    for (int y = 0; y < row_count_; y++) {
      Rect sub;
      sub.init(x * site_width_,
               y * row_height_,
               (x + 1) * site_width_,
               (y + 1) * row_height_);
      Pixel *pixel = gridPixel(x, y);
      for (Group &group : groups_) {
        for (Rect &rect : group.regions) {
          if (!isInside(sub, rect) && checkOverlap(sub, rect)) {
            pixel->util = 0.0;
            pixel->cell = &dummy_cell_;
            pixel->is_valid = false;
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
  return cell.xMin() >= box.xMin()
    && cell.xMax() <= box.xMax()
    && cell.yMin() >= box.yMin()
    && cell.yMax() <= box.yMax();
}

bool
Opendp::checkOverlap(const Rect &cell, const Rect &box)
{
  return box.xMin() < cell.xMax() && box.xMax() > cell.xMin() && box.yMin() < cell.yMax() && box.yMax() > cell.yMin();
}

void
Opendp::groupInitPixels()
{
  for (int x = 0; x < row_site_count_; x++) {
    for (int y = 0; y < row_count_; y++) {
      Pixel *pixel = gridPixel(x, y);
      pixel->util = 0.0;
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
          Pixel *pixel = gridPixel(l, k);
          pixel->util += 1.0;
        }
        if (rect.xMin() % site_width_ != 0) {
          Pixel *pixel = gridPixel(col_start, k);
          pixel->util -= (rect.xMin() % site_width_) / static_cast<double>(site_width_);
        }
        if (rect.xMax() % site_width_ != 0) {
          Pixel *pixel = gridPixel(col_end - 1, k);
          pixel->util -= ((site_width_ - rect.xMax()) % site_width_)
            / static_cast<double>(site_width_);
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
          Pixel *pixel = gridPixel(l, k);
          if (pixel->util == 1.0) {
            pixel->group_ = &group;
            pixel->is_valid = true;
            pixel->util = 1.0;
          }
          else if (pixel->util > 0.0 && pixel->util < 1.0) {
            pixel->cell = &dummy_cell_;
            pixel->util = 0.0;
            pixel->is_valid = false;
          }
        }
      }
    }
  }
}

void
Opendp::erasePixel(Cell *cell)
{
  if (!(isFixed(cell) || !cell->is_placed_)) {
    int x_end = gridPaddedEndX(cell);
    int y_end = gridEndY(cell);
    for (int x = gridPaddedX(cell); x < x_end; x++) {
      for (int y = gridY(cell); y < y_end; y++) {
        Pixel *pixel = gridPixel(x, y);
        pixel->cell = nullptr;
        pixel->util = 0;
      }
    }
    cell->is_placed_ = false;
    cell->hold_ = false;
  }
}

void
Opendp::paintPixel(Cell *cell, int grid_x, int grid_y)
{
  assert(!cell->is_placed_);
  int x_end = grid_x + gridPaddedWidth(cell);
  int grid_height = gridHeight(cell);
  int y_end = grid_y + grid_height;

  setGridPaddedLoc(cell, grid_x, grid_y);
  cell->is_placed_ = true;

  debugPrint(logger_, DPL, "place", 1, " paint {} ({}-{}, {}-{})",
             cell->name(), grid_x, x_end - 1, grid_y, y_end - 1);

  for (int x = grid_x; x < x_end; x++) {
    for (int y = grid_y; y < y_end; y++) {
      Pixel *pixel = gridPixel(x, y);
      if (pixel->cell) {
        logger_->error(DPL, 13, "Cannot paint grid because it is already occupied.");
      }
      else {
        pixel->cell = cell;
        pixel->util = 1.0;
      }
    }
  }
  // This is most likely broken. -cherry
  if (have_multi_row_cells_) {
    if (grid_height % 2 == 1) {
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
