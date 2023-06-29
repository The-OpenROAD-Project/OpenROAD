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
  grid_info_map_.clear();
  grid_info_vector_.clear();
  for (auto db_row : block_->getRows()) {
    if (db_row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    int row_height = db_row->getSite()->getHeight();
    if (grid_info_map_.find(row_height) == grid_info_map_.end()) {
      grid_info_map_.emplace(
          db_row->getSite()->getHeight(),
          GridInfo{
              getRowCount(row_height), db_row->getSiteCount(), grid_index++});
    } else {
      auto& grid_info = grid_info_map_.at(row_height);
      grid_info.site_count
          = divFloor(core_.dx(), db_row->getSite()->getWidth());
    }
  }
  grid_info_vector_.resize(grid_info_map_.size());
  for (auto& [row_height, grid_info] : grid_info_map_) {
    grid_info_vector_[grid_info.grid_index] = &grid_info;
  }
  debugPrint(logger_, DPL, "grid", 1, "grid layers map initialized");
}

void Opendp::initGrid()
{
  // the number of layers in the grid is the number of unique row heights
  // the map key is the row height, the value is a pair of row count and site
  // count

  if (grid_info_map_.empty()) {
    initGridLayersMap();
  }

  // Make pixel grid
  if (grid_.empty()) {
    grid_.resize(grid_info_map_.size());

    for (auto& [row_height, grid_info] : grid_info_map_) {
      int layer_row_count = grid_info.row_count;
      int index = grid_info.grid_index;
      grid_[index].resize(layer_row_count);
    }
  }

  for (auto& [row_height, grid_info] : grid_info_map_) {
    const int layer_row_count = grid_info.row_count;
    const int layer_row_site_count = grid_info.site_count;
    const int index = grid_info.grid_index;
    grid_[index].resize(layer_row_count);
    for (int j = 0; j < layer_row_count; j++) {
      grid_[index][j].resize(layer_row_site_count);
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

  namespace gtl = boost::polygon;
  using namespace gtl::operators;

  std::vector<gtl::polygon_90_set_data<int>> hopeless;
  hopeless.resize(grid_info_map_.size());
  for (auto [row_height, grid_info] : grid_info_map_) {
    hopeless[grid_info.grid_index] += gtl::rectangle_data<int>{
        0, 0, grid_info.site_count, grid_info.row_count};
  }
  // Fragmented row support; mark valid sites.
  for (auto db_row : block_->getRows()) {
    if (db_row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    int current_row_height = db_row->getSite()->getHeight();
    int current_row_site_count = db_row->getSiteCount();
    int current_row_count = grid_info_map_.at(current_row_height).row_count;
    int current_row_grid_index
        = grid_info_map_.at(current_row_height).grid_index;
    if (db_row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    int orig_x, orig_y;
    db_row->getOrigin(orig_x, orig_y);

    const int x_start = (orig_x - core_.xMin()) / db_row->getSite()->getWidth();
    const int x_end = x_start + current_row_site_count;
    const int y_row = (orig_y - core_.yMin()) / current_row_height;
    for (int x = x_start; x < x_end; x++) {
      Pixel* pixel = gridPixel(current_row_grid_index, x, y_row);
      if (pixel == nullptr) {
        continue;
      }
      pixel->is_valid = true;
      pixel->orient_ = db_row->getOrient();
    }

    // The safety margin is to avoid having only a very few sites
    // within the diamond search that may still lead to failures.
    const int safety = 20;
    int max_row_site_count
        = divFloor(core_.dx(), db_row->getSite()->getWidth());

    const int xl = std::max(0, x_start - max_displacement_x_ + safety);
    const int xh
        = std::min(max_row_site_count, x_end + max_displacement_x_ - safety);

    const int yl = std::max(0, y_row - max_displacement_y_ + safety);
    const int yh
        = std::min(current_row_count, y_row + max_displacement_y_ - safety);
    hopeless[current_row_grid_index]
        -= gtl::rectangle_data<int>{xl, yl, xh, yh};
  }

  std::vector<gtl::rectangle_data<int>> rects;
  for (auto& grid_layer : grid_info_map_) {
    rects.clear();
    int h_index = grid_layer.second.grid_index;
    hopeless[h_index].get_rectangles(rects);
    for (const auto& rect : rects) {
      for (int y = gtl::yl(rect); y < gtl::yh(rect); y++) {
        for (int x = gtl::xl(rect); x < gtl::xh(rect); x++) {
          Pixel& pixel = grid_[h_index][y][x];
          pixel.is_hopeless = true;
        }
      }
    }
  }
}

void Opendp::deleteGrid()
{
  grid_.clear();
}

Pixel* Opendp::gridPixel(int grid_idx, int grid_x, int grid_y) const
{
  if (grid_idx < 0 || grid_idx >= grid_info_vector_.size()) {
    return nullptr;
  }
  GridInfo* grid_info = grid_info_vector_[grid_idx];
  if (grid_x >= 0 && grid_x < grid_info->site_count && grid_y >= 0
      && grid_y < grid_info->row_count) {
    return const_cast<Pixel*>(&grid_[grid_idx][grid_y][grid_x]);
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////

void Opendp::findOverlapInRtree(bgBox& queryBox, vector<bgBox>& overlaps) const
{
  overlaps.clear();
  regions_rtree.query(boost::geometry::index::intersects(queryBox),
                      std::back_inserter(overlaps));
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
  if (isStdCell(&cell)) {
    site_width = getSiteWidth(&cell);
    row_height = getRowHeight(&cell);
  } else {
    site_width = site_width_;
    row_height = row_height_;
  }

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

      // Since there is an obstruction, we need to visit all the pixels at all
      // layers (for all row heights)
      int grid_idx = 0;
      for (const auto& [layer_row_height, grid_info] : grid_info_map_) {
        int layer_y_start
            = map_coordinates(y_start, row_height, layer_row_height);
        int layer_y_end = map_coordinates(y_end, row_height, layer_row_height);
        if (layer_y_end == layer_y_start) {
          ++layer_y_end;
        }
        for (int x = x_start; x < x_end; x++) {
          for (int y = layer_y_start; y < layer_y_end; y++) {
            Pixel* pixel = gridPixel(grid_idx, x, y);
            if (pixel) {
              visitor(pixel);
            }
          }
        }
        grid_idx++;
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
    for (auto layer_it : grid_info_map_) {
      int layer_x_start = map_coordinates(x_start, site_width, site_width);
      int layer_x_end = map_coordinates(x_end, site_width, site_width);
      int layer_y_start = map_coordinates(y_start, row_height, layer_it.first);
      int layer_y_end = map_coordinates(y_end, row_height, layer_it.first);
      if (layer_y_end == layer_y_start) {
        ++layer_y_end;
      }
      if (layer_x_end == layer_x_start) {
        ++layer_x_end;
      }

      for (int x = layer_x_start; x < layer_x_end; x++) {
        for (int y = layer_y_start; y < layer_y_end; y++) {
          Pixel* pixel = gridPixel(layer_it.second.grid_index, x, y);
          if (pixel) {
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
  GridInfo grid_info = grid_info_map_.at(row_height);
  const int index_in_grid = grid_info.grid_index;

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
        Pixel* pixel = gridPixel(index_in_grid, x, y_start);
        if (pixel) {
          visitor(pixel, odb::Direction2D::North, x, y_start);
        }
        pixel = gridPixel(index_in_grid, x, y_end - 1);
        if (pixel) {
          visitor(pixel, odb::Direction2D::South, x, y_end - 1);
        }
      }
      for (int y = y_start; y < y_end; y++) {
        Pixel* pixel = gridPixel(index_in_grid, x_start, y);
        if (pixel) {
          visitor(pixel, odb::Direction2D::West, x_start, y);
        }
        pixel = gridPixel(index_in_grid, x_end - 1, y);
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
      Pixel* pixel = gridPixel(index_in_grid, x, y_start);
      if (pixel) {
        visitor(pixel, odb::Direction2D::North, x, y_start);
      }
      pixel = gridPixel(index_in_grid, x, y_end - 1);
      if (pixel) {
        visitor(pixel, odb::Direction2D::South, x, y_end - 1);
      }
    }
    for (int y = y_start; y < y_end; y++) {
      Pixel* pixel = gridPixel(index_in_grid, x_start, y);
      if (pixel) {
        visitor(pixel, odb::Direction2D::West, x_start, y);
      }
      pixel = gridPixel(index_in_grid, x_end - 1, y);
      if (pixel) {
        visitor(pixel, odb::Direction2D::East, x_end - 1, y);
      }
    }
  }
}

void Opendp::setFixedGridCells()
{
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
    pixel->is_hopeless = true;
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
      row_height = getRowHeight(group.cells_.at(0));
      int row_count = divFloor(core_.dy(), row_height);
      auto grid_info = grid_info_map_.at(row_height);

      for (int x = 0; x < max_row_site_count; x++) {
        for (int y = 0; y < row_count; y++) {
          Pixel* pixel = gridPixel(grid_info.grid_index, x, y);
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
  for (auto& layer : grid_info_map_) {
    int row_height = layer.first;
    GridInfo& grid_info = layer.second;
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
        Pixel* pixel = gridPixel(grid_info.grid_index, x, y);
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
  for (const auto& layer : grid_info_map_) {
    const GridInfo& grid_info = layer.second;
    for (int x = 0; x < grid_info.site_count; x++) {
      for (int y = 0; y < grid_info.row_count; y++) {
        Pixel* pixel = gridPixel(grid_info.grid_index, x, y);
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
    GridInfo& grid_info = grid_info_map_[row_height];
    int grid_index = grid_info.grid_index;
    for (Rect& rect : group.regions) {
      debugPrint(logger_,
                 DPL,
                 "detailed",
                 1,
                 "Group {} region [x{} y{}] [x{} y{}]",
                 group.name,
                 rect.xMin(),
                 rect.yMin(),
                 rect.xMax(),
                 rect.yMax());
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
  if (!(isFixed(cell) || !cell->is_placed_)) {
    int row_height = getRowHeight(cell);
    int site_width = getSiteWidth(cell);
    int x_end = gridPaddedEndX(cell, site_width);
    int y_end = gridEndY(cell, row_height);
    int y_start = gridY(cell, row_height);

    for (auto [layer_row_height, grid_info] : grid_info_map_) {
      int layer_y_start
          = map_coordinates(y_start, row_height, layer_row_height);
      int layer_y_end = map_coordinates(y_end, row_height, layer_row_height);

      if (layer_y_end == layer_y_start) {
        ++layer_y_end;
      }

      for (int x = gridPaddedX(cell, site_width); x < x_end; x++) {
        for (int y = layer_y_start; y < layer_y_end; y++) {
          Pixel* pixel = gridPixel(grid_info.grid_index, x, y);
          if (nullptr == pixel) {
            continue;
          }
          pixel->cell = nullptr;
          pixel->util = 0;
        }
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
  return divFloor(original_step * original_coordinate, target_step);
}

void Opendp::paintPixel(Cell* cell, int grid_x, int grid_y)
{
  assert(!cell->is_placed_);
  int x_end = grid_x + gridPaddedWidth(cell);
  int grid_height = gridHeight(cell);
  int y_end = grid_y + grid_height;
  int site_width = getSiteWidth(cell);
  int row_height = getRowHeight(cell);
  GridInfo grid_info = grid_info_map_.at(row_height);
  const int index_in_grid = grid_info.grid_index;
  setGridPaddedLoc(cell, grid_x, grid_y, site_width, row_height);
  cell->is_placed_ = true;

  for (int x = grid_x; x < x_end; x++) {
    for (int y = grid_y; y < y_end; y++) {
      Pixel* pixel = gridPixel(index_in_grid, x, y);
      if (pixel->cell) {
        logger_->error(
            DPL, 13, "Cannot paint grid because it is already occupied.");
      } else {
        pixel->cell = cell;
        pixel->util = 1.0;
      }
    }
  }

  for (const auto& layer : grid_info_map_) {
    if (layer.first == row_height) {
      continue;
    }
    int layer_x = map_coordinates(grid_x, site_width, site_width);
    int layer_x_end = map_coordinates(x_end, site_width, site_width);
    int layer_y = map_coordinates(grid_y, row_height, layer.first);
    int layer_y_end = map_coordinates(y_end, row_height, layer.first);
    if (layer_x_end == layer_x) {
      ++layer_x_end;
    }

    if (layer_y_end == layer_y) {
      ++layer_y_end;
    }

    for (int x = layer_x; x < layer_x_end; x++) {
      for (int y = layer_y; y < layer_y_end; y++) {
        Pixel* pixel = gridPixel(layer.second.grid_index, x, y);
        if (pixel->cell) {
          // Checks that the row heights of the found cell match the row height
          // of this layer. If they don't, it means that this pixel is partially
          // filled by a single-height or shorter cell, which is allowed.
          // However, if they do match, it means that we are trying to overwrite
          // a double-height cell placement, which is an error.

          pair<int, GridInfo> grid_info_candidate = getRowInfo(pixel->cell);
          if (grid_info_candidate.first == layer.first) {
            // Occupied by a multi-height cell this should not happen.
            logger_->error(
                DPL,
                41,
                "Cannot paint grid because another layer is already occupied.");
          } else {
            // We might not want to overwrite the cell that's already here.
            continue;
          }
        }

        pixel->cell = cell;
        pixel->util = 1.0;
      }
    }
  }

  cell->orient_ = gridPixel(index_in_grid, grid_x, grid_y)->orient_;
}

}  // namespace dpl
