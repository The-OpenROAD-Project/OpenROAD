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

#include <boost/polygon/polygon.hpp>
#include <cmath>
#include <limits>

#include "dpl/Opendp.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

namespace dpl {

using std::max;
using std::min;

using utl::DPL;

using odb::dbBox;
using odb::dbTransform;

void Opendp::initGridLayersMap()
{
  int grid_index = 0;
  for (auto db_row : block_->getRows()) {
    // TODO: this is potentially a bug due to cut rows.
    // if a cut was inserted before the original row, the site count will be
    // smaller
    // it is also a bug in the row count
    int row_height = db_row->getSite()->getHeight();
    if (grid_layers_.find(row_height) == grid_layers_.end()) {
      grid_layers_.emplace(
          db_row->getSite()->getHeight(),
          LayerInfo{
              getRowCount(row_height), db_row->getSiteCount(), grid_index++});
    } else {
      auto& layer_info = grid_layers_.at(row_height);
      layer_info.site_count
          = max(layer_info.site_count, db_row->getSiteCount());
    }
  }
  grid_layers_vector.resize(grid_layers_.size());
  for (auto& [row_height, layer_info] : grid_layers_) {
    grid_layers_vector[layer_info.grid_index] = &layer_info;
    debugPrint(logger_,
               DPL,
               "grid",
               1,
               "grid layer {} {} site count: {} and row_site_count_ {}",
               row_height,
               layer_info.row_count,
               layer_info.site_count,
               row_site_count_);
  }
  debugPrint(logger_, DPL, "grid", 1, "grid layers map initialized");
}

void Opendp::initGrid()
{
  // the number of layers in the grid is the number of unique row heights
  // the map key is the row height, the value is a pair of row count and site
  // count

  if (grid_layers_.empty()) {
    initGridLayersMap();
  }

  grid_depth_ = grid_layers_.size();

  // Make pixel grid
  if (grid_ == nullptr) {
    grid_ = new Pixel**[grid_depth_];

    for (auto& [row_height, layer_info] : grid_layers_) {
      int layer_row_count = layer_info.row_count;
      int layer_row_site_count = layer_info.site_count;
      int index = layer_info.grid_index;
      grid_[index] = new Pixel*[layer_row_count];
      for (int j = 0; j < layer_row_count; j++) {
        grid_[index][j] = new Pixel[layer_row_site_count];
        for (int k = 0; k < layer_row_site_count; k++) {
          Pixel& pixel = grid_[index][j][k];
          pixel.cell = nullptr;
          pixel.group_ = nullptr;
          pixel.util = 0.0;
          pixel.is_valid = false;
          pixel.is_hopeless = false;
        }
      }
    }
    debugPrint(logger_, DPL, "grid", 1, "grid initialized");
  }

  namespace gtl = boost::polygon;
  using namespace gtl::operators;

  std::vector<gtl::polygon_90_set_data<int>> hopeless;
  hopeless.resize(grid_depth_);
  debugPrint(logger_, DPL, "grid", 1, "hopeless grid initialized");
  for (auto [row_height, layer_info] : grid_layers_) {
    hopeless[layer_info.grid_index] += gtl::rectangle_data<int>{
        0, 0, layer_info.site_count, layer_info.row_count};
  }
  // Fragmented row support; mark valid sites.
  debugPrint(
      logger_, DPL, "grid", 1, "Number of rows: {}", block_->getRows().size());
  for (auto db_row : block_->getRows()) {
    int current_row_height = db_row->getSite()->getHeight();
    int current_row_site_count = db_row->getSiteCount();
    int current_row_count = grid_layers_.at(current_row_height).row_count;
    int current_row_grid_index = grid_layers_.at(current_row_height).grid_index;
    auto layer_info = grid_layers_.at(current_row_height);
    debugPrint(
        logger_,
        DPL,
        "grid",
        1,
        "row {} has height {} and {} sites. grid index is {} and row count {}",
        db_row->getName(),
        current_row_height,
        current_row_site_count,
        current_row_grid_index,
        current_row_count);
    if (db_row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    int orig_x, orig_y;
    db_row->getOrigin(orig_x, orig_y);

    const int x_start = (orig_x - core_.xMin()) / db_row->getSite()->getWidth();
    const int x_end = x_start + current_row_site_count;
    const int y_row = (orig_y - core_.yMin()) / current_row_height;
    debugPrint(logger_,
               DPL,
               "grid",
               1,
               "row {} has x_start {} and x_end {} and y_row {}",
               db_row->getName(),
               x_start,
               x_end,
               y_row);
    for (int x = x_start; x < x_end; x++) {
      Pixel* pixel;
      pixel = gridPixel(current_row_grid_index, x, y_row);
      pixel->is_valid = true;
      pixel->orient_ = db_row->getOrient();
    }

    // The safety margin is to avoid having only a very few sites
    // within the diamond search that may still lead to failures.
    const int safety = 20;
    // TODO: fix this, it should be
    // divFloor(core_.dx() / layer_site_width)
    int max_row_site_count = row_site_count_;

    const int xl = std::max(0, x_start - max_displacement_x_ + safety);
    const int xh
        = std::min(max_row_site_count, x_end + max_displacement_x_ - safety);
    debugPrint(logger_,
               DPL,
               "grid",
               1,
               "current row {} current_row_site_count {} and x_end {} "
               "max_displacement_x_ {} safety {}",
               db_row->getName(),
               max_row_site_count,
               x_end,
               max_displacement_x_,
               safety);

    const int yl = std::max(0, y_row - max_displacement_y_ + safety);
    const int yh
        = std::min(current_row_count, y_row + max_displacement_y_ - safety);
    hopeless[current_row_grid_index]
        -= gtl::rectangle_data<int>{xl, yl, xh, yh};
    debugPrint(logger_,
               DPL,
               "grid",
               1,
               "Removing rectangle ({}, {}, {}, {}) from hopeless.",
               xl,
               yl,
               xh,
               yh);
  }

  std::vector<gtl::rectangle_data<int>> rects;
  for (auto& grid_layer : grid_layers_) {
    debugPrint(logger_,
               DPL,
               "grid",
               1,
               "Marking hopeless pixels for layer {}",
               grid_layer.first);
    rects.clear();
    int h_index = grid_layer.second.grid_index;
    hopeless[h_index].get_rectangles(rects);
    for (const auto& rect : rects) {
      for (int y = gtl::yl(rect); y < gtl::yh(rect); y++) {
        for (int x = gtl::xl(rect); x < gtl::xh(rect); x++) {
          debugPrint(logger_,
                     DPL,
                     "grid",
                     1,
                     "Marking grid pixel ({}, {}, {}) as hopeless.",
                     h_index,
                     x,
                     y);
          grid_[h_index][y][x].is_hopeless = true;
        }
      }
    }
  }
}

void Opendp::deleteGrid()
{
  if (grid_) {
    int i = 0;
    for (auto [row_height, layer_info] : grid_layers_) {
      int N = layer_info.row_count;
      for (int j = 0; j < N; j++) {
        delete[] grid_[i][j];
      }
      delete[] grid_[i];
      i++;
    }
    delete[] grid_;
  }
  grid_ = nullptr;
}

Pixel* Opendp::gridPixel(int layer_idx, int grid_x, int grid_y) const
{
  LayerInfo* layer_info = grid_layers_vector[layer_idx];
  if (grid_x >= 0 && grid_x < layer_info->site_count && grid_y >= 0
      && grid_y < layer_info->row_count && layer_idx >= 0
      && layer_idx < grid_depth_) {
    return &grid_[layer_idx][grid_y][grid_x];
  }

  return nullptr;
}

////////////////////////////////////////////////////////////////

void Opendp::visitCellPixels(
    Cell& cell,
    bool padded,
    const std::function<void(Pixel* pixel)>& visitor) const
{
  dbInst* inst = cell.db_inst_;
  dbMaster* master = inst->getMaster();
  auto obstructions = master->getObstructions();
  bool have_obstructions = false;

  int site_width, row_height;
  debugPrint(logger_, DPL, "hopeless", 1, "Visiting cell {}.", cell.name());
  if (isStdCell(&cell)) {
    site_width = getSiteWidth(&cell);
    row_height = getRowHeight(&cell);
    debugPrint(logger_,
               DPL,
               "hopeless",
               1,
               "Cell {} is a std cell with site width {} and row height {}.",
               cell.name(),
               site_width,
               row_height);
  } else {
    site_width = site_width_;
    row_height = row_height_;
    debugPrint(logger_,
               DPL,
               "hopeless",
               1,
               "Cell {} is a macro with site width {} and row height {}.",
               cell.name(),
               site_width,
               row_height);
  }

  for (dbBox* obs : obstructions) {
    if (obs->getTechLayer()->getType()
        == odb::dbTechLayerType::Value::OVERLAP) {
      have_obstructions = true;

      Rect rect = obs->getBox();
      dbTransform transform;
      inst->getTransform(transform);
      transform.apply(rect);
      // TODO: - use map_coordiantes here and move this to inside the loop
      int x_start = gridX(rect.xMin() - core_.xMin(), site_width);
      int x_end = gridEndX(rect.xMax() - core_.xMin(), site_width);
      int y_start = gridY(rect.yMin() - core_.yMin(), row_height);
      int y_end = gridEndY(rect.yMax() - core_.yMin(), row_height);

      // Since there is an obstruction, we need to visit all the pixels at all
      // layers (for all row heights)
      for (int layer_idx = 0; layer_idx < grid_layers_.size(); layer_idx++) {
        for (int x = x_start; x < x_end; x++) {
          for (int y = y_start; y < y_end; y++) {
            Pixel* pixel = gridPixel(layer_idx, x, y);
            if (pixel) {
              debugPrint(logger_,
                         DPL,
                         "hopeless",
                         1,
                         "Visiting pixel ({}, {}, {}) due to obstruction.",
                         layer_idx,
                         x,
                         y);
              visitor(pixel);
            }
          }
        }
      }
    }
  }
  if (!have_obstructions) {
    int x_start
        = padded ? gridPaddedX(&cell, site_width) : gridX(&cell, site_width);
    int x_end = padded ? gridPaddedEndX(&cell, site_width)
                       : gridEndX(&cell, site_width);
    int y_start = gridY(&cell, row_height);
    int y_end = gridEndY(&cell, row_height);
    for (auto layer_it : grid_layers_) {
      int layer_x_start = map_coordinates(x_start, site_width, site_width);
      int layer_x_end = map_coordinates(x_end, site_width, site_width);

      for (int x = layer_x_start; x < layer_x_end; x++) {
        int layer_y_start
            = map_coordinates(y_start, row_height, layer_it.first);
        int layer_y_end = map_coordinates(y_end, row_height, layer_it.first);
        for (int y = layer_y_start; y < layer_y_end; y++) {
          Pixel* pixel = gridPixel(layer_it.second.grid_index, x, y);
          if (pixel) {
            debugPrint(logger_,
                       DPL,
                       "hopeless",
                       1,
                       "Visiting pixel ({}, {}, {}) due to cell {}",
                       layer_it.second.grid_index,
                       x,
                       y,
                       cell.name());
            visitor(pixel);
          }
        }
      }
    }
  }
}

void Opendp::visitCellBoundaryPixels(
    Cell& cell,
    bool padded,
    const std::function<
        void(Pixel* pixel, odb::Direction2D edge, int x, int y)>& visitor) const
{
  dbInst* inst = cell.db_inst_;
  int site_width = getSiteWidth(&cell);
  int row_height = getRowHeight(&cell);
  LayerInfo layer_info = grid_layers_.at(row_height);
  const int layer_index_in_grid = layer_info.grid_index;

  dbMaster* master = inst->getMaster();
  auto obstructions = master->getObstructions();
  bool have_obstructions = false;
  for (dbBox* obs : obstructions) {
    if (obs->getTechLayer()->getType()
        == odb::dbTechLayerType::Value::OVERLAP) {
      have_obstructions = true;

      Rect rect = obs->getBox();
      dbTransform transform;
      inst->getTransform(transform);
      transform.apply(rect);

      int x_start = gridX(rect.xMin() - core_.xMin(), site_width);
      int x_end = gridEndX(rect.xMax() - core_.xMin(), site_width);
      int y_start = gridY(rect.yMin() - core_.yMin(), row_height);
      int y_end = gridEndY(rect.yMax() - core_.yMin(), row_height);

      for (int x = x_start; x < x_end; x++) {
        Pixel* pixel = gridPixel(layer_index_in_grid, x, y_start);
        if (pixel) {
          visitor(pixel, odb::Direction2D::North, x, y_start);
        }
        pixel = gridPixel(layer_index_in_grid, x, y_end - 1);
        if (pixel) {
          visitor(pixel, odb::Direction2D::South, x, y_end - 1);
        }
      }
      for (int y = y_start; y < y_end; y++) {
        Pixel* pixel = gridPixel(layer_index_in_grid, x_start, y);
        if (pixel) {
          visitor(pixel, odb::Direction2D::West, x_start, y);
        }
        pixel = gridPixel(layer_index_in_grid, x_end - 1, y);
        if (pixel) {
          visitor(pixel, odb::Direction2D::East, x_end - 1, y);
        }
      }
    }
  }
  if (!have_obstructions) {
    int x_start = padded ? gridPaddedX(&cell) : gridX(&cell);
    int x_end = padded ? gridPaddedEndX(&cell) : gridEndX(&cell);
    int y_start = gridY(&cell, row_height);
    int y_end = gridEndY(&cell, row_height);

    for (int x = x_start; x < x_end; x++) {
      Pixel* pixel = gridPixel(layer_index_in_grid, x, y_start);
      if (pixel) {
        visitor(pixel, odb::Direction2D::North, x, y_start);
      }
      pixel = gridPixel(layer_index_in_grid, x, y_end - 1);
      if (pixel) {
        visitor(pixel, odb::Direction2D::South, x, y_end - 1);
      }
    }
    for (int y = y_start; y < y_end; y++) {
      Pixel* pixel = gridPixel(layer_index_in_grid, x_start, y);
      if (pixel) {
        visitor(pixel, odb::Direction2D::West, x_start, y);
      }
      pixel = gridPixel(layer_index_in_grid, x_end - 1, y);
      if (pixel) {
        visitor(pixel, odb::Direction2D::East, x_end - 1, y);
      }
    }
  }
}

void Opendp::setFixedGridCells()
{
  debugPrint(logger_, DPL, "place", 1, "Setting fixed grid cells.");
  for (Cell& cell : cells_) {
    if (isFixed(&cell)) {
      visitCellPixels(
          cell, true, [&](Pixel* pixel) { setGridCell(cell, pixel); });
    }
  }
}

void Opendp::setGridCell(Cell& cell, Pixel* pixel)
{
  pixel->cell = &cell;
  pixel->util = 1.0;
  if (isBlock(&cell)) {
    // Try the is_hopeless strategy to get off of a block
    debugPrint(
        logger_, DPL, "hopeless", 1, "Setting cell {} hopeless.", cell.name());
    pixel->is_hopeless = true;
  } else {
    debugPrint(logger_,
               DPL,
               "hopeless",
               1,
               "Failed isBlock() in Setting cell {} as hopeless.",
               cell.name());
  }
}

void Opendp::groupAssignCellRegions()
{
  for (Group& group : groups_) {
    int64_t site_count = 0;
    int site_width = site_width_;
    int row_height = row_height_;
    if (!group.cells_.empty()) {
      site_width = getSiteWidth(group.cells_.at(0));
      int max_row_site_count = divFloor(core_.dx(), site_width);
      int row_count = divFloor(core_.dy(), row_height);
      row_height = getRowHeight(group.cells_.at(0));

      int cell_heights_in_group = group.cells_.at(0)->height_;
      auto layer_info = grid_layers_.at(cell_heights_in_group);

      for (int x = 0; x < max_row_site_count; x++) {
        for (int y = 0; y < row_count; y++) {
          Pixel* pixel = gridPixel(layer_info.grid_index, x, y);
          if (pixel->is_valid && pixel->group_ == &group) {
            site_count++;
          }
        }
      }
    }
    int64_t site_area = site_count * site_width * row_height;

    int64_t cell_area = 0;
    for (Cell* cell : group.cells_) {
      cell_area += cell->area();

      for (Rect& rect : group.regions) {
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

void Opendp::groupInitPixels2()
{
  for (auto& layer : grid_layers_) {
    int row_height = layer.first;
    LayerInfo& layer_info = layer.second;
    int row_count = divFloor(core_.dy(), row_height);
    int row_site_count = divFloor(core_.dx(), site_width_);
    for (int x = 0; x < row_site_count; x++) {
      for (int y = 0; y < row_count; y++) {
        Rect sub;
        // TODO: Site width here is wrong if multiple site widths are supported!
        sub.init(x * site_width_,
                 y * row_height,
                 (x + 1) * site_width_,
                 (y + 1) * row_height);
        Pixel* pixel = gridPixel(layer_info.grid_index, x, y);
        for (Group& group : groups_) {
          for (Rect& rect : group.regions) {
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
}

/* static */
bool Opendp::isInside(const Rect& cell, const Rect& box)
{
  return cell.xMin() >= box.xMin() && cell.xMax() <= box.xMax()
         && cell.yMin() >= box.yMin() && cell.yMax() <= box.yMax();
}

bool Opendp::checkOverlap(const Rect& cell, const Rect& box)
{
  return box.xMin() < cell.xMax() && box.xMax() > cell.xMin()
         && box.yMin() < cell.yMax() && box.yMax() > cell.yMin();
}

void Opendp::groupInitPixels()
{
  for (auto layer : grid_layers_) {
    LayerInfo& layer_info = layer.second;
    for (int x = 0; x < layer_info.site_count; x++) {
      for (int y = 0; y < layer_info.row_count; y++) {
        Pixel* pixel = gridPixel(layer_info.grid_index, x, y);
        pixel->util = 0.0;
      }
    }
  }
  for (Group& group : groups_) {
    if (group.cells_.empty()) {
      logger_->warn(DPL, 42, "No cells found in group {}. ", group.name);
      continue;
    }
    int row_height = group.cells_[0]->height_;
    int site_width = getSiteWidth(group.cells_[0]);
    LayerInfo& layer_info = grid_layers_[row_height];
    int grid_index = layer_info.grid_index;
    for (Rect& rect : group.regions) {
      int row_start = divCeil(rect.yMin(), row_height);
      int row_end = divFloor(rect.yMax(), row_height);

      for (int k = row_start; k < row_end; k++) {
        int col_start = divCeil(rect.xMin(), site_width);
        int col_end = divFloor(rect.xMax(), site_width);

        for (int l = col_start; l < col_end; l++) {
          Pixel* pixel = gridPixel(grid_index, l, k);
          pixel->util += 1.0;
        }
        if (rect.xMin() % site_width != 0) {
          Pixel* pixel = gridPixel(grid_index, col_start, k);
          pixel->util
              -= (rect.xMin() % site_width) / static_cast<double>(site_width);
        }
        if (rect.xMax() % site_width != 0) {
          Pixel* pixel = gridPixel(grid_index, col_end - 1, k);
          pixel->util -= ((site_width - rect.xMax()) % site_width)
                         / static_cast<double>(site_width);
        }
      }
    }
    for (Rect& rect : group.regions) {
      int row_start = divCeil(rect.yMin(), row_height);
      int row_end = divFloor(rect.yMax(), row_height);

      for (int k = row_start; k < row_end; k++) {
        int col_start = divCeil(rect.xMin(), site_width);
        int col_end = divFloor(rect.xMax(), site_width);

        // Assign group to each pixel.
        for (int l = col_start; l < col_end; l++) {
          Pixel* pixel = gridPixel(grid_index, l, k);
          if (pixel->util == 1.0) {
            pixel->group_ = &group;
            pixel->is_valid = true;
            pixel->util = 1.0;
          } else if (pixel->util > 0.0 && pixel->util < 1.0) {
            pixel->cell = &dummy_cell_;
            pixel->util = 0.0;
            pixel->is_valid = false;
          }
        }
      }
    }
  }
}

void Opendp::erasePixel(Cell* cell)
{
  debugPrint(logger_, DPL, "place", 1, "Erasing cell {}.", cell->name());
  if (!(isFixed(cell) || !cell->is_placed_)) {
    int row_height = getRowHeight(cell);
    int site_width = getSiteWidth(cell);
    LayerInfo layer_info = grid_layers_.at(row_height);
    int x_end = gridPaddedEndX(cell, site_width);
    int y_end = gridEndY(cell, row_height);

    // TODO: We still need to set the pixel free in all layers.
    for (int x = gridPaddedX(cell, site_width); x < x_end; x++) {
      for (int y = gridY(cell, row_height); y < y_end; y++) {
        Pixel* pixel = gridPixel(layer_info.grid_index, x, y);
        pixel->cell = nullptr;
        pixel->util = 0;
      }
    }
    cell->is_placed_ = false;
    cell->hold_ = false;
  }
}

int Opendp::map_coordinates(int original_coordinate,
                            int original_step,
                            int target_step) const
{
  return original_step * original_coordinate / target_step;
}

void Opendp::paintPixel(Cell* cell, int grid_x, int grid_y)
{
  assert(!cell->is_placed_);
  int x_end = grid_x + gridPaddedWidth(cell);
  int grid_height = gridHeight(cell);
  int y_end = grid_y + grid_height;
  int site_width = getSiteWidth(cell);
  int row_height = getRowHeight(cell);
  LayerInfo layer_info = grid_layers_.at(row_height);
  const int layer_index_in_grid = layer_info.grid_index;
  setGridPaddedLoc(cell, grid_x, grid_y, site_width, row_height);
  cell->is_placed_ = true;

  debugPrint(logger_,
             DPL,
             "place",
             1,
             " paint {} ({}-{}, {}-{})",
             cell->name(),
             grid_x,
             x_end - 1,
             grid_y,
             y_end - 1);

  for (int x = grid_x; x < x_end; x++) {
    for (int y = grid_y; y < y_end; y++) {
      debugPrint(logger_,
                 DPL,
                 "place",
                 1,
                 "  painting {} ({}-{})",
                 cell->name(),
                 x,
                 y);
      Pixel* pixel = gridPixel(layer_index_in_grid, x, y);
      if (pixel->cell) {
        logger_->error(
            DPL, 13, "Cannot paint grid because it is already occupied.");
      } else {
        pixel->cell = cell;
        pixel->util = 1.0;
      }
    }
  }

  for (auto layer : grid_layers_) {
    if (layer.first == row_height) {
      continue;
    }
    int layer_x = map_coordinates(grid_x, site_width, site_width);
    int layer_x_end = map_coordinates(x_end, site_width, site_width);
    int layer_y = map_coordinates(grid_y, row_height, layer.first);
    int layer_y_end = map_coordinates(y_end, row_height, layer.first);
    for (int x = layer_x; x < layer_x_end; x++) {
      for (int y = layer_y; y < layer_y_end; y++) {
        Pixel* pixel = gridPixel(layer.second.grid_index, x, y);
        debugPrint(logger_,
                   DPL,
                   "place",
                   1,
                   "  painting in layer {} ({}-{}, {}-{})",
                   cell->name(),
                   grid_x,
                   x_end - 1,
                   grid_y,
                   y_end - 1);
        if (pixel->cell) {
          logger_->error(
              DPL,
              41,
              "Cannot paint grid because another layer is already occupied.");
        } else {
          pixel->cell = cell;
          pixel->util = 1.0;
        }
      }
    }
  }

  cell->orient_ = gridPixel(layer_index_in_grid, grid_x, grid_y)->orient_;
}

}  // namespace dpl
