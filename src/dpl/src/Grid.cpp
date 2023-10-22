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

int Opendp::calculateHybridSitesRowCount(dbSite* parent_hybrid_site) const
{
  auto row_pattern = parent_hybrid_site->getRowPattern();
  int rows_count = getRowCount(parent_hybrid_site->getHeight());
  int remaining_core_height
      = core_.dy() - (rows_count * parent_hybrid_site->getHeight());

  rows_count *= (int) row_pattern.size();

  for (auto [site, site_orientation] : row_pattern) {
    if (remaining_core_height >= site->getHeight()) {
      remaining_core_height -= site->getHeight();
      rows_count++;
    }
    if (remaining_core_height <= 0) {
      break;
    }
  }
  logger_->info(DPL,
                7251,
                "parent hybrid site {} has {} rows",
                parent_hybrid_site->getName(),
                rows_count);
  return rows_count;
}

void Opendp::initGridLayersMap()
{
  int grid_index = 0;
  grid_info_map_.clear();
  grid_info_vector_.clear();
  int min_site_height = std::numeric_limits<int>::max();
  for (auto db_row : block_->getRows()) {
    auto site = db_row->getSite();

    if (site_idx_to_grid_idx.find(site->getId())
        != site_idx_to_grid_idx.end()) {
      continue;
    } else {
      if (site->isHybrid() && site->hasRowPattern()) {
        debugPrint(logger_,
                   DPL,
                   "hybrid",
                   1,
                   "Mapping {} to grid_index: {}",
                   site->getName(),
                   grid_index);
        site_idx_to_grid_idx[site->getId()] = grid_index++;
        auto rp = site->getRowPattern();
        bool updated = false;
        for (auto& [child_site, child_site_orientation] : rp) {
          if (site_idx_to_grid_idx.find(child_site->getId())
              == site_idx_to_grid_idx.end()) {
            // FIXME(mina1460): this might need more work in the future if we
            // want to allow the same hybrid cell to be part of multiple grids
            // (For example, A in AB and AC). This is good enough for now.
            debugPrint(logger_,
                       DPL,
                       "hybrid",
                       1,
                       "Mapping child site {} to grid_index: {}",
                       site->getName(),
                       grid_index);
            site_idx_to_grid_idx[child_site->getId()] = grid_index;
            updated = true;
          }
        }
        if (updated) {
          ++grid_index;
        }
      } else if (!site->isHybrid()) {
        debugPrint(logger_,
                   DPL,
                   "hybrid",
                   1,
                   "Mapping non-hybrid {} to grid_index: {}",
                   site->getName(),
                   grid_index);
        site_idx_to_grid_idx[site->getId()] = grid_index++;
      }
    }
    if (!site->isHybrid()) {
      if (site->getHeight() < min_site_height) {
        min_site_height = site->getHeight();
        smallest_non_hybrid_grid_key
            = Grid_map_key{site_idx_to_grid_idx[site->getId()]};
      }
    }
  }
  if (min_site_height == std::numeric_limits<int>::max()) {
    logger_->error(
        DPL, 128, "Cannot find a non-hybrid grid to use for placement.");
  }

  for (auto db_row : block_->getRows()) {
    if (db_row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    dbSite* working_site = db_row->getSite();
    int row_height = working_site->getHeight();
    Grid_map_key gmk = getGridMapKey(working_site);
    if (grid_info_map_.find(gmk) == grid_info_map_.end()) {
      if (working_site->isHybridParent() || !working_site->isHybrid()) {
        GridInfo newGridInfo = {
            getRowCount(row_height),
            db_row->getSiteCount(),
            site_idx_to_grid_idx.at(working_site->getId()),
            std::vector<std::pair<dbSite*, dbOrientType>>{
                {working_site, db_row->getOrient()}},
        };
        grid_info_map_.emplace(gmk, newGridInfo);
      } else {
        GridInfo newGridInfo = {
            calculateHybridSitesRowCount(working_site->getParent()),
            db_row->getSiteCount(),
            site_idx_to_grid_idx.at(working_site->getId()),
            working_site->getParent()
                ->getRowPattern(),  // FIXME(mina1460): this is wrong! in the
                                    // case of HybridAB, and HybridBA. each of A
                                    // and B maps to both, but the
                                    // hybrid_sites_mapper only record one of
                                    // them, resulting in the wrong RowPattern
                                    // here.
        };
        grid_info_map_.emplace(gmk, newGridInfo);
      }
    }
  }
  grid_info_vector_.resize(grid_info_map_.size());
  for (auto& [gmk, grid_info] : grid_info_map_) {
    assert(gmk.grid_index == grid_info.getGridIndex());
    grid_info_vector_[grid_info.getGridIndex()] = &grid_info;
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
    for (auto& [gmk, grid_info] : grid_info_map_) {
      grid_[grid_info.getGridIndex()].resize(grid_info.getRowCount());
    }
  }

  for (auto& [gmk, grid_info] : grid_info_map_) {
    const int layer_row_count = grid_info.getRowCount();
    const int layer_row_site_count = grid_info.getSiteCount();
    const int index = grid_info.getGridIndex();
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
        pixel.site = nullptr;
        auto grid_sites = grid_info.getSites();
        if (!grid_sites.empty()) {
          pixel.site = grid_sites[j % grid_sites.size()].first;
        }
      }
    }
  }

  namespace gtl = boost::polygon;
  using namespace gtl::operators;

  std::vector<gtl::polygon_90_set_data<int>> hopeless;
  hopeless.resize(grid_info_map_.size());
  for (auto [row_height, grid_info] : grid_info_map_) {
    hopeless[grid_info.getGridIndex()] += gtl::rectangle_data<int>{
        0, 0, grid_info.getSiteCount(), grid_info.getRowCount()};
  }
  // Fragmented row support; mark valid sites.
  for (auto db_row : block_->getRows()) {
    const auto db_row_site = db_row->getSite();
    if (db_row_site->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    int current_row_height = db_row_site->getHeight();
    int current_row_site_count = db_row->getSiteCount();
    auto gmk = getGridMapKey(db_row_site);
    auto entry = grid_info_map_.at(gmk);
    int current_row_count = entry.getRowCount();
    int current_row_grid_index = entry.getGridIndex();
    int orig_x, orig_y;
    db_row->getOrigin(orig_x, orig_y);

    const int x_start = (orig_x - core_.xMin()) / db_row_site->getWidth();
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
    int max_row_site_count = divFloor(core_.dx(), db_row_site->getWidth());

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
    int h_index = grid_layer.second.getGridIndex();
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
  if (grid_x >= 0 && grid_x < grid_info->getSiteCount() && grid_y >= 0
      && grid_y < grid_info->getRowCount()) {
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
  auto obstructions = inst->getMaster()->getObstructions();
  bool have_obstructions = false;

  for (dbBox* obs : obstructions) {
    if (obs->getTechLayer()->getType()
        == odb::dbTechLayerType::Value::OVERLAP) {
      have_obstructions = true;

      Rect rect = obs->getBox();
      dbTransform transform;
      inst->getTransform(transform);
      transform.apply(rect);
      int x_start = gridX(rect.xMin() - core_.xMin(), site_width_);
      int x_end = gridEndX(rect.xMax() - core_.xMin(), site_width_);
      int y_start = gridY(rect.yMin() - core_.yMin(), row_height_);
      int y_end = gridEndY(rect.yMax() - core_.yMin(), row_height_);

      // Since there is an obstruction, we need to visit all the pixels at all
      // layers (for all row heights)
      int grid_idx = 0;
      for (const auto& [target_grid_map_key, target_grid_info] :
           grid_info_map_) {
        int layer_y_start = map_ycoordinates(
            y_start, smallest_non_hybrid_grid_key, target_grid_map_key);
        int layer_y_end = map_ycoordinates(
            y_end, smallest_non_hybrid_grid_key, target_grid_map_key);
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
        = padded ? gridPaddedX(&cell, site_width_) : gridX(&cell, site_width_);
    int x_end = padded ? gridPaddedEndX(&cell, site_width_)
                       : gridEndX(&cell, site_width_);
    int y_start = gridY(&cell);
    int y_end = gridEndY(&cell);
    auto src_gmk = getGridMapKey(&cell);
    for (const auto& layer_it : grid_info_map_) {
      int layer_x_start = x_start;
      int layer_x_end = x_end;
      int layer_y_start = map_ycoordinates(y_start, src_gmk, layer_it.first);
      int layer_y_end = map_ycoordinates(y_end, src_gmk, layer_it.first);
      if (layer_y_end == layer_y_start) {
        ++layer_y_end;
      }
      if (layer_x_end == layer_x_start) {
        ++layer_x_end;
      }

      for (int x = layer_x_start; x < layer_x_end; x++) {
        for (int y = layer_y_start; y < layer_y_end; y++) {
          Pixel* pixel = gridPixel(layer_it.second.getGridIndex(), x, y);
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
  Grid_map_key gmk = getGridMapKey(&cell);
  GridInfo grid_info = grid_info_map_.at(gmk);
  const int index_in_grid = grid_info.getGridIndex();
  auto grid_sites = grid_info.getSites();
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
      int y_start = gridY(rect.yMin() - core_.yMin(), grid_sites).first;
      int y_end = gridEndY(rect.yMax() - core_.yMin(), grid_sites).first;
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
    int y_start = gridY(&cell);
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} isHybrid {}",
               cell.name(),
               cell.isHybrid());
    int y_end = gridEndY(&cell);
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} in rows. Y start {} y end {}",
               cell.name(),
               y_start,
               y_end);

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
      auto group_cell = group.cells_.at(0);
      site_width = getSiteWidth(group_cell);
      int max_row_site_count = divFloor(core_.dx(), site_width);
      row_height = getRowHeight(group_cell);
      int row_count = divFloor(core_.dy(), row_height);
      auto gmk = getGridMapKey(group_cell);
      auto grid_info = grid_info_map_.at(gmk);

      for (int x = 0; x < max_row_site_count; x++) {
        for (int y = 0; y < row_count; y++) {
          Pixel* pixel = gridPixel(grid_info.getGridIndex(), x, y);
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
    GridInfo& grid_info = layer.second;
    int row_count = layer.second.getRowCount();
    int row_site_count = layer.second.getSiteCount();
    auto grid_sites = layer.second.getSites();
    for (int x = 0; x < row_site_count; x++) {
      for (int y = 0; y < row_count; y++) {
        int row_height = grid_sites[y % grid_sites.size()].first->getHeight();
        Rect sub;
        // TODO: Site width here is wrong if multiple site widths are
        // supported!
        sub.init(x * site_width_,
                 y * row_height,
                 (x + 1) * site_width_,
                 (y + 1) * row_height);
        Pixel* pixel = gridPixel(grid_info.getGridIndex(), x, y);
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
    for (int x = 0; x < grid_info.getSiteCount(); x++) {
      for (int y = 0; y < grid_info.getRowCount(); y++) {
        Pixel* pixel = gridPixel(grid_info.getGridIndex(), x, y);
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
    Grid_map_key gmk = getGridMapKey(group.cells_[0]);
    GridInfo& grid_info = grid_info_map_.at(gmk);
    int grid_index = grid_info.getGridIndex();
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
    auto gmk = getGridMapKey(cell);
    int site_width = getSiteWidth(cell);
    int x_end = gridPaddedEndX(cell, site_width);
    int y_end = gridEndY(cell);
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} isHybrid {}",
               cell->name(),
               cell->isHybrid());
    int y_start = gridY(cell);
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} in rows. Y start {} y end {}",
               cell->name(),
               y_start,
               y_end);

    for (auto [target_grid_map_key, target_grid_info] : grid_info_map_) {
      int layer_y_start = map_ycoordinates(y_start, gmk, target_grid_map_key);
      int layer_y_end = map_ycoordinates(y_end, gmk, target_grid_map_key);

      if (layer_y_end == layer_y_start) {
        ++layer_y_end;
      }

      for (int x = gridPaddedX(cell, site_width); x < x_end; x++) {
        for (int y = layer_y_start; y < layer_y_end; y++) {
          Pixel* pixel = gridPixel(target_grid_info.getGridIndex(), x, y);
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

int Opendp::map_ycoordinates(int source_grid_coordinate,
                             const Grid_map_key& source_grid_key,
                             const Grid_map_key& target_grid_key) const
{
  if (source_grid_key == target_grid_key) {
    return source_grid_coordinate;
  }
  auto src_grid_info = grid_info_map_.at(source_grid_key);
  auto target_grid_info = grid_info_map_.at(target_grid_key);
  if (!src_grid_info.isHybrid()) {
    if (!target_grid_info.isHybrid()) {
      int original_step = src_grid_info.getSites()[0].first->getHeight();
      int target_step = target_grid_info.getSites()[0].first->getHeight();
      return divCeil(original_step * source_grid_coordinate, target_step);
    } else {
      // count until we find it.
      return gridY(source_grid_coordinate * source_grid_key.grid_index,
                   target_grid_info.getSites())
          .first;
    }
  } else {
    // src is hybrid
    int src_total_sites_height = src_grid_info.getSitesTotalHeight();
    auto src_sites = src_grid_info.getSites();
    const int src_sites_size = src_sites.size();
    int src_height
        = (source_grid_coordinate / src_sites_size) * src_total_sites_height;
    for (int s = 0; s < source_grid_coordinate % src_sites_size; s++) {
      src_height += src_sites[s].first->getHeight();
    }
    if (target_grid_info.isHybrid()) {
      // both are hybrids.
      return gridY(src_height, target_grid_info.getSites()).first;
    } else {
      int target_step = target_grid_info.getSites()[0].first->getHeight();
      return divCeil(src_height, target_step);
    }
  }
}

void Opendp::paintPixel(Cell* cell, int grid_x, int grid_y)
{
  assert(!cell->is_placed_);
  int x_end = grid_x + gridPaddedWidth(cell);
  int grid_height = gridHeight(cell);
  int y_end = grid_y + grid_height;
  int site_width = getSiteWidth(cell);
  Grid_map_key gmk = getGridMapKey(cell);
  GridInfo grid_info = grid_info_map_.at(gmk);
  const int index_in_grid = gmk.grid_index;
  setGridPaddedLoc(cell, grid_x, grid_y, site_width);
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
    if (layer.first == gmk) {
      continue;
    }
    int layer_x = grid_x;
    int layer_x_end = x_end;
    int layer_y = map_ycoordinates(grid_y, gmk, layer.first);
    int layer_y_end = map_ycoordinates(y_end, gmk, layer.first);
    debugPrint(logger_,
               DPL,
               "detailed",
               176,
               "y_end {} original grid {} target grid {} layer_y_end {}",
               y_end,
               gmk.grid_index,
               layer.first.grid_index,
               layer_y_end);

    debugPrint(logger_,
               DPL,
               "detailed",
               1,
               "Mapping coordinates start from grid idx {} to grid idx {}. "
               "From [x{} y{}] "
               "it became [x{} y{}].",
               gmk.grid_index,
               layer.first.grid_index,
               grid_x,
               grid_y,
               layer_x,
               layer_y);
    debugPrint(logger_,
               DPL,
               "detailed",
               1,
               "Mapping coordinates end from grid idx {} to grid idx {}. From "
               "[x{} y{}] "
               "it became [x{} y{}].",
               gmk.grid_index,
               layer.first.grid_index,
               x_end,
               y_end,
               layer_x_end,
               layer_y_end);

    if (layer_x_end == layer_x) {
      ++layer_x_end;
    }

    if (layer_y_end == layer_y) {
      ++layer_y_end;
      debugPrint(
          logger_, DPL, "detailed", 1, "added 1 go layer_end {}", layer_y_end);
    }

    if (layer_y_end > layer.second.getRowCount()) {
      // If there's an uneven number of single row height cells, say 21.
      // The above layer mapping coordinates on double height rows will
      // round up, because they don't know if there's 11 or 10 rows of
      // double height cells. This just caps it to the right amount.
      layer_y_end = layer.second.getRowCount();
    }

    for (int x = layer_x; x < layer_x_end; x++) {
      for (int y = layer_y; y < layer_y_end; y++) {
        Pixel* pixel = gridPixel(layer.second.getGridIndex(), x, y);
        if (pixel && pixel->cell) {
          // Checks that the row heights of the found cell match the row
          // height of this layer. If they don't, it means that this pixel is
          // partially filled by a single-height or shorter cell, which is
          // allowed. However, if they do match, it means that we are trying
          // to overwrite a double-height cell placement, which is an error.
          auto candidate_grid_key = getGridMapKey(pixel->cell);
          if (candidate_grid_key == layer.first) {
            // Occupied by a multi-height cell this should not happen.
            logger_->error(DPL,
                           41,
                           "Cannot paint grid with cell {} because another "
                           "layer [{}] is already occupied by cell {}.",
                           cell->name(),
                           layer.first.grid_index,
                           pixel->cell->name());
          } else {
            // We might not want to overwrite the cell that's already here.
            continue;
          }
        }
        if (pixel) {
          pixel->cell = cell;
          pixel->util = 1.0;
        }
      }
    }
  }

  cell->orient_ = gridPixel(index_in_grid, grid_x, grid_y)->orient_;
}

}  // namespace dpl
