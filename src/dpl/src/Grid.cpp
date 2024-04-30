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

#include "Grid.h"

#include <boost/polygon/polygon.hpp>
#include <cmath>
#include <limits>

#include "Objects.h"
#include "Padding.h"
#include "dpl/Opendp.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

namespace dpl {

using std::max;
using std::min;

using utl::DPL;

using odb::dbBox;
using odb::dbRow;

PixelPt::PixelPt(Pixel* pixel1, int grid_x, int grid_y)
    : pixel(pixel1), pt(grid_x, grid_y)
{
}

////////////////////////////////////////////////////////////////

GridInfo::GridInfo(const int row_count,
                   const int site_count,
                   const int grid_index,
                   const dbSite::RowPattern& sites)
    : row_count_(row_count),
      site_count_(site_count),
      grid_index_(grid_index),
      sites_(sites)
{
}

int GridInfo::getSitesTotalHeight() const
{
  return std::accumulate(sites_.begin(),
                         sites_.end(),
                         0,
                         [](int sum, const dbSite::OrientedSite& entry) {
                           return sum + entry.site->getHeight();
                         });
}

bool GridInfo::isHybrid() const
{
  return sites_.size() > 1 || sites_[0].site->hasRowPattern();
}

////////////////////////////////////////////////////////////////

void Grid::visitDbRows(dbBlock* block,
                       const std::function<void(dbRow*)>& func) const
{
  for (auto row : block->getRows()) {
    // dpl doesn't deal with pads
    if (row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    func(row);
  }
}

int Grid::calculateHybridSitesRowCount(dbSite* parent_hybrid_site) const
{
  auto row_pattern = parent_hybrid_site->getRowPattern();
  int rows_count = getRowCount(parent_hybrid_site->getHeight());
  int remaining_core_height
      = core_.dy() - (rows_count * parent_hybrid_site->getHeight());

  rows_count *= (int) row_pattern.size();

  for (const auto& [site, site_orientation] : row_pattern) {
    if (remaining_core_height >= site->getHeight()) {
      remaining_core_height -= site->getHeight();
      rows_count++;
    }
    if (remaining_core_height <= 0) {
      break;
    }
  }
  return rows_count;
}

void Grid::initGridLayersMap(dbDatabase* db, dbBlock* block)
{
  hybrid_parent_.clear();
  for (auto lib : db->getLibs()) {
    for (auto site : lib->getSites()) {
      if (site->hasRowPattern()) {
        for (const auto& [child_site, orient] : site->getRowPattern()) {
          if (getHybridParent().count(child_site) == 0) {
            addHybridParent(child_site, site);
          }
        }
      }
    }
  }

  int grid_index = 0;
  grid_info_map_.clear();
  grid_info_vector_.clear();
  int min_site_height = std::numeric_limits<int>::max();
  int min_row_y_coordinate = std::numeric_limits<int>::max();
  visitDbRows(block, [&](odb::dbRow* db_row) {
    auto site = db_row->getSite();
    odb::Point row_base = db_row->getOrigin();
    min_row_y_coordinate = min(min_row_y_coordinate, row_base.y());

    if (getSiteToGrid().find(site) != getSiteToGrid().end()) {
      return;
    }

    if (site->isHybrid() && site->hasRowPattern()) {
      debugPrint(logger_,
                 DPL,
                 "hybrid",
                 1,
                 "Mapping {} to grid_index: {}",
                 site->getName(),
                 grid_index);
      addSiteToGrid(site, GridMapKey{grid_index++});
      auto rp = site->getRowPattern();
      bool updated = false;
      for (auto& [child_site, child_site_orientation] : rp) {
        if (getSiteToGrid().find(child_site) == getSiteToGrid().end()) {
          // FIXME(mina1460): this might need more work in the future if we
          // want to allow the same hybrid cell to be part of multiple grids
          // (For example, A in AB and AC). This is good enough for now.
          debugPrint(logger_,
                     DPL,
                     "hybrid",
                     1,
                     "Mapping child site {} to grid_index: {}",
                     child_site->getName(),
                     grid_index);
          addSiteToGrid(child_site, GridMapKey{grid_index});
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
      addSiteToGrid(site, GridMapKey{grid_index++});
    }

    if (!site->isHybrid()) {
      if (site->getHeight() < min_site_height) {
        min_site_height = site->getHeight();
        setSmallestNonHybridGridKey(getSiteToGrid().at(site));
      }
    }
  });

  visitDbRows(block, [&](odb::dbRow* db_row) {
    const odb::Point row_base = db_row->getOrigin();
    dbSite* working_site = db_row->getSite();
    if (row_base.y() == min_row_y_coordinate) {
      if (working_site->hasRowPattern()) {
        for (const auto& [child_site, orient] : working_site->getRowPattern()) {
          if (getHybridParent().at(child_site) != working_site) {
            debugPrint(logger_,
                       DPL,
                       "hybrid",
                       1,
                       "Overriding the parent of site {} from {} to {}",
                       child_site->getName(),
                       getHybridParent().at(child_site)->getName(),
                       working_site->getName());
            addHybridParent(child_site, working_site);
          }
        }
      }
    }
  });
  if (!hasHybridRows() && min_site_height == std::numeric_limits<int>::max()) {
    logger_->error(
        DPL, 128, "Cannot find a non-hybrid grid to use for placement.");
  }

  visitDbRows(block, [&](odb::dbRow* db_row) {
    dbSite* working_site = db_row->getSite();
    int row_height = working_site->getHeight();
    GridMapKey gmk = site_to_grid_key_.at(working_site);
    if (getInfoMap().find(gmk) == getInfoMap().end()) {
      if (working_site->hasRowPattern() || !working_site->isHybrid()) {
        GridInfo newGridInfo = {
            getRowCount(row_height),
            divFloor(core_.dx(), db_row->getSite()->getWidth()),
            getSiteToGrid().at(working_site).grid_index,
            dbSite::RowPattern{{working_site, db_row->getOrient()}},
        };
        if (working_site->isHybrid()) {
          int row_offset = db_row->getBBox().ll().getY() - min_row_y_coordinate;
          newGridInfo.setOffset(row_offset);
          debugPrint(logger_,
                     DPL,
                     "hybrid",
                     1,
                     "Offset for site: {} is {}",
                     working_site->getName(),
                     row_offset);
        }
        addInfoMap(gmk, newGridInfo);
      } else {
        dbSite* parent_site = getHybridParent().at(working_site);
        GridInfo newGridInfo = {
            calculateHybridSitesRowCount(parent_site),
            divFloor(core_.dx(), db_row->getSite()->getWidth()),
            getSiteToGrid().at(working_site).grid_index,
            parent_site->getRowPattern(),  // FIXME(mina1460): this is wrong! in
                                           // the case of HybridAB, and
                                           // HybridBA. each of A and B maps to
                                           // both, but the hybrid_sites_mapper
                                           // only record one of them, resulting
                                           // in the wrong RowPattern here.
        };
        if (working_site->isHybrid()) {
          int row_offset = db_row->getBBox().ll().getY() - min_row_y_coordinate;
          newGridInfo.setOffset(row_offset);
          debugPrint(logger_,
                     DPL,
                     "hybrid",
                     1,
                     "Offset for grid index: {} is {}",
                     newGridInfo.getGridIndex(),
                     newGridInfo.getOffset());
        }
        addInfoMap(gmk, newGridInfo);
      }
    } else {
      int row_offset = db_row->getBBox().ll().getY() - min_row_y_coordinate;
      auto grid_info = getInfoMap().at(gmk);
      if (grid_info.isHybrid()) {
        grid_info.setOffset(min(grid_info.getOffset(), row_offset));
        addInfoMap(gmk, grid_info);
      }
    }
  });
  grid_info_vector_.resize(getInfoMap().size());
  int min_height(std::numeric_limits<int>::max());
  for (auto& [gmk, grid_info] : getInfoMap()) {
    assert(gmk.grid_index == grid_info.getGridIndex());
    grid_info_vector_[grid_info.getGridIndex()] = &grid_info;
    if (grid_info.getSitesTotalHeight() < min_height) {
      min_height = grid_info.getSitesTotalHeight();
      if (hasHybridRows()) {
        setSmallestNonHybridGridKey(gmk);
      }
    }
  }
  debugPrint(logger_, DPL, "grid", 1, "grid layers map initialized");
}

void Grid::initGrid(dbDatabase* db,
                    dbBlock* block,
                    std::shared_ptr<Padding> padding,
                    int max_displacement_x,
                    int max_displacement_y)
{
  padding_ = std::move(padding);

  // the number of layers in the grid is the number of unique row heights
  // the map key is the row height, the value is a pair of row count and site
  // count

  if (infoMapEmpty()) {
    initGridLayersMap(db, block);
  }

  // Make pixel grid
  if (pixels_.empty()) {
    resize(getInfoMap().size());
    for (auto& [gmk, grid_info] : getInfoMap()) {
      resize(grid_info.getGridIndex(), grid_info.getRowCount());
    }
  }

  for (auto& [gmk, grid_info] : getInfoMap()) {
    const int layer_row_count = grid_info.getRowCount();
    const int layer_row_site_count = grid_info.getSiteCount();
    const int index = grid_info.getGridIndex();
    resize(index, layer_row_count);
    for (int j = 0; j < layer_row_count; j++) {
      resize(index, j, layer_row_site_count);
      const auto& grid_sites = grid_info.getSites();
      dbSite* row_site = nullptr;
      if (!grid_sites.empty()) {
        row_site = grid_sites[j % grid_sites.size()].site;
      }

      for (int k = 0; k < layer_row_site_count; k++) {
        Pixel& pixel = pixels_[index][j][k];
        pixel.cell = nullptr;
        pixel.group = nullptr;
        pixel.util = 0.0;
        pixel.is_valid = false;
        pixel.is_hopeless = false;
        pixel.site = nullptr;
        pixel.site = row_site;
      }
    }
  }

  namespace gtl = boost::polygon;
  using gtl::operators::operator+=;
  using gtl::operators::operator-=;

  std::vector<gtl::polygon_90_set_data<int>> hopeless;
  hopeless.resize(getInfoMap().size());
  for (const auto& [row_height, grid_info] : getInfoMap()) {
    hopeless[grid_info.getGridIndex()] += gtl::rectangle_data<int>{
        0, 0, grid_info.getSiteCount(), grid_info.getRowCount()};
  }
  // Fragmented row support; mark valid sites.
  visitDbRows(block, [&](odb::dbRow* db_row) {
    const auto db_row_site = db_row->getSite();
    int current_row_site_count = db_row->getSiteCount();
    auto gmk = getGridMapKey(db_row_site);
    auto entry = getInfoMap().at(gmk);
    int current_row_count = entry.getRowCount();
    int current_row_grid_index = entry.getGridIndex();
    const odb::Point orig = db_row->getOrigin();

    const Rect core = getCore();
    const int x_start = (orig.x() - core.xMin()) / db_row_site->getWidth();
    const int x_end = x_start + current_row_site_count;
    const int y_row = gridY(orig.y() - core.yMin(), entry).first;
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
    int max_row_site_count = divFloor(core.dx(), db_row_site->getWidth());

    const int xl = max(0, x_start - max_displacement_x + safety);
    const int xh = min(max_row_site_count, x_end + max_displacement_x - safety);

    const int yl = max(0, y_row - max_displacement_y + safety);
    const int yh = min(current_row_count, y_row + max_displacement_y - safety);
    hopeless[current_row_grid_index]
        -= gtl::rectangle_data<int>{xl, yl, xh, yh};
  });

  std::vector<gtl::rectangle_data<int>> rects;
  for (auto& grid_layer : getInfoMap()) {
    rects.clear();
    int h_index = grid_layer.second.getGridIndex();
    hopeless[h_index].get_rectangles(rects);
    for (const auto& rect : rects) {
      for (int y = gtl::yl(rect); y < gtl::yh(rect); y++) {
        for (int x = gtl::xl(rect); x < gtl::xh(rect); x++) {
          Pixel& pixel = pixels_[h_index][y][x];
          pixel.is_hopeless = true;
        }
      }
    }
  }
}

Pixel* Grid::gridPixel(int grid_idx, int grid_x, int grid_y) const
{
  if (grid_idx < 0 || grid_idx >= grid_info_vector_.size()) {
    return nullptr;
  }
  const GridInfo* grid_info = grid_info_vector_[grid_idx];
  if (grid_x >= 0 && grid_x < grid_info->getSiteCount() && grid_y >= 0
      && grid_y < grid_info->getRowCount()) {
    return const_cast<Pixel*>(&pixels_[grid_idx][grid_y][grid_x]);
  }
  return nullptr;
}

void Grid::visitCellPixels(
    Cell& cell,
    bool padded,
    const std::function<void(Pixel* pixel)>& visitor) const
{
  dbInst* inst = cell.db_inst_;
  auto obstructions = inst->getMaster()->getObstructions();
  bool have_obstructions = false;
  const Rect core = getCore();

  for (dbBox* obs : obstructions) {
    if (obs->getTechLayer()->getType()
        == odb::dbTechLayerType::Value::OVERLAP) {
      have_obstructions = true;

      Rect rect = obs->getBox();
      inst->getTransform().apply(rect);
      int x_start = gridX(rect.xMin() - core.xMin());
      int x_end = gridEndX(rect.xMax() - core.xMin());
      int y_start = gridY(rect.yMin() - core.yMin(), &cell);
      int y_end = gridEndY(rect.yMax() - core.yMin(), &cell);

      // Since there is an obstruction, we need to visit all the pixels at all
      // layers (for all row heights)
      int grid_idx = 0;
      for (const auto& [target_GridMapKey, target_grid_info] : getInfoMap()) {
        const auto smallest_non_hybrid_grid_key = getSmallestNonHybridGridKey();
        int layer_y_start = map_ycoordinates(
            y_start, smallest_non_hybrid_grid_key, target_GridMapKey, true);
        int layer_y_end = map_ycoordinates(
            y_end, smallest_non_hybrid_grid_key, target_GridMapKey, false);
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
    int x_start = padded ? gridPaddedX(&cell) : gridX(&cell);
    int x_end = padded ? gridPaddedEndX(&cell) : gridEndX(&cell);
    int y_start = gridY(&cell);
    int y_end = gridEndY(&cell);
    auto src_gmk = getGridMapKey(&cell);
    for (const auto& layer_it : getInfoMap()) {
      int layer_x_start = x_start;
      int layer_x_end = x_end;
      int layer_y_start
          = map_ycoordinates(y_start, src_gmk, layer_it.first, true);
      int layer_y_end = map_ycoordinates(y_end, src_gmk, layer_it.first, false);
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

void Grid::visitCellBoundaryPixels(
    Cell& cell,
    bool padded,
    const std::function<
        void(Pixel* pixel, odb::Direction2D edge, int x, int y)>& visitor) const
{
  dbInst* inst = cell.db_inst_;
  const GridMapKey& gmk = getGridMapKey(&cell);
  GridInfo grid_info = getInfoMap().at(gmk);
  const int index_in_grid = grid_info.getGridIndex();

  auto visit = [&visitor, index_in_grid, this](const int x_start,
                                               const int x_end,
                                               const int y_start,
                                               const int y_end) {
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
  };

  dbMaster* master = inst->getMaster();
  auto obstructions = master->getObstructions();
  bool have_obstructions = false;
  const Rect core = getCore();
  for (dbBox* obs : obstructions) {
    if (obs->getTechLayer()->getType()
        == odb::dbTechLayerType::Value::OVERLAP) {
      have_obstructions = true;

      Rect rect = obs->getBox();
      inst->getTransform().apply(rect);

      int x_start = gridX(rect.xMin() - core.xMin());
      int x_end = gridEndX(rect.xMax() - core.xMin());
      int y_start = gridY(rect.yMin() - core.yMin(), grid_info).first;
      int y_end = gridEndY(rect.yMax() - core.yMin(), grid_info).first;
      visit(x_start, x_end, y_start, y_end);
    }
  }
  if (!have_obstructions) {
    const int x_start = padded ? gridPaddedX(&cell) : gridX(&cell);
    const int x_end = padded ? gridPaddedEndX(&cell) : gridEndX(&cell);
    const int y_start = gridY(&cell);
    const int y_end = gridEndY(&cell);
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} isHybrid {} in rows. Y start {} y end {}",
               cell.name(),
               cell.isHybrid(),
               y_start,
               y_end);

    visit(x_start, x_end, y_start, y_end);
  }
}

void Grid::erasePixel(Cell* cell)
{
  if (!(cell->isFixed() || !cell->is_placed_)) {
    auto gmk = getGridMapKey(cell);
    int x_end = gridPaddedEndX(cell);
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

    for (const auto& [target_GridMapKey, target_grid_info] : getInfoMap()) {
      int layer_y_start
          = map_ycoordinates(y_start, gmk, target_GridMapKey, true);
      int layer_y_end = map_ycoordinates(y_end, gmk, target_GridMapKey, false);

      if (layer_y_end == layer_y_start) {
        ++layer_y_end;
      }

      for (int x = gridPaddedX(cell); x < x_end; x++) {
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

int Grid::map_ycoordinates(int source_grid_coordinate,
                           const GridMapKey& source_grid_key,
                           const GridMapKey& target_grid_key,
                           const bool start) const
{
  if (source_grid_key == target_grid_key) {
    return source_grid_coordinate;
  }
  auto src_grid_info = getInfoMap().at(source_grid_key);
  auto target_grid_info = getInfoMap().at(target_grid_key);
  if (!src_grid_info.isHybrid()) {
    if (!target_grid_info.isHybrid()) {
      int original_step = src_grid_info.getSites()[0].site->getHeight();
      int target_step = target_grid_info.getSites()[0].site->getHeight();
      if (start) {
        return divFloor(original_step * source_grid_coordinate, target_step);
      }
      return divCeil(original_step * source_grid_coordinate, target_step);
    }
    // count until we find it.
    return gridY(source_grid_coordinate * source_grid_key.grid_index,
                 target_grid_info)
        .first;
  }
  // src is hybrid
  int src_total_sites_height = src_grid_info.getSitesTotalHeight();
  const auto& src_sites = src_grid_info.getSites();
  const int src_sites_size = src_sites.size();
  int src_height
      = (source_grid_coordinate / src_sites_size) * src_total_sites_height;
  for (int s = 0; s < source_grid_coordinate % src_sites_size; s++) {
    src_height += src_sites[s].site->getHeight();
  }
  if (target_grid_info.isHybrid()) {
    // both are hybrids.
    return gridY(src_height, target_grid_info).first;
  }
  int target_step = target_grid_info.getSites()[0].site->getHeight();
  if (start) {
    return divFloor(src_height, target_step);
  }
  return divCeil(src_height, target_step);
}

void Grid::paintPixel(Cell* cell, int grid_x, int grid_y)
{
  assert(!cell->is_placed_);
  int x_end = grid_x + gridPaddedWidth(cell);
  int grid_height = gridHeight(cell);
  int y_end = grid_y + grid_height;
  GridMapKey gmk = getGridMapKey(cell);
  GridInfo grid_info = getInfoMap().at(gmk);
  const int index_in_grid = gmk.grid_index;
  setGridPaddedLoc(cell, grid_x, grid_y);
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

  for (const auto& layer : getInfoMap()) {
    if (layer.first == gmk) {
      continue;
    }
    int layer_x = grid_x;
    int layer_x_end = x_end;
    int layer_y = map_ycoordinates(grid_y, gmk, layer.first, true);
    int layer_y_end = map_ycoordinates(y_end, gmk, layer.first, false);
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

void Grid::addSiteToGrid(dbSite* site, const GridMapKey& key)
{
  site_to_grid_key_[site] = key;
}
const map<const dbSite*, GridMapKey>& Grid::getSiteToGrid() const
{
  return site_to_grid_key_;
}

void Grid::setSmallestNonHybridGridKey(const GridMapKey& key)
{
  smallest_non_hybrid_grid_key_ = key;
}

void Grid::addHybridParent(dbSite* child, dbSite* parent)
{
  hybrid_parent_[child] = parent;
}

void Grid::addInfoMap(const GridMapKey& key, const GridInfo& info)
{
  grid_info_map_.emplace(key, info);
}

const std::unordered_map<dbSite*, dbSite*>& Grid::getHybridParent() const
{
  return hybrid_parent_;
}

GridMapKey Grid::getSmallestNonHybridGridKey() const
{
  return smallest_non_hybrid_grid_key_;
}

int Grid::gridPaddedWidth(const Cell* cell) const
{
  return divCeil(padding_->paddedWidth(cell), getSiteWidth());
}

int Grid::coordinateToHeight(int y_coordinate, GridMapKey gmk) const
{
  // gets a coordinate and its grid, and returns the height of the coordinate.
  // This is useful for hybrid sites
  auto grid_info = infoMap(gmk);
  if (grid_info.isHybrid()) {
    auto& grid_sites = grid_info.getSites();
    const int total_height = grid_info.getSitesTotalHeight();
    int patterns_below = divFloor(y_coordinate, grid_sites.size());
    int remaining_rows_height = grid_info.getSitesTotalHeight();
    int height = patterns_below * total_height + remaining_rows_height;
    return height;
  }
  return y_coordinate * grid_info.getSitesTotalHeight();
}

int Grid::gridHeight(const Cell* cell) const
{
  int row_height = getRowHeight(cell);
  return max(1, divCeil(cell->height_, row_height));
}

int Grid::gridEndX(int x) const
{
  return divCeil(x, getSiteWidth());
}

int Grid::gridX(int x) const
{
  return x / getSiteWidth();
}

int Grid::gridX(const Cell* cell) const
{
  return gridX(cell->x_);
}

int Grid::gridPaddedX(const Cell* cell) const
{
  return gridX(cell->x_ - padding_->padLeft(cell) * getSiteWidth());
}

int Opendp::getRowCount(const Cell* cell) const
{
  return grid_->getRowCount(grid_->getRowHeight(cell));
}

int Grid::getRowCount(int row_height) const
{
  return divFloor(core_.dy(), row_height);
}

int Grid::getRowHeight(const Cell* cell) const
{
  int row_height = getRowHeight();
  if (cell->isStdCell() || cell->isHybrid()) {
    row_height = cell->height_;
  }
  return row_height;
}

pair<int, GridInfo> Grid::getRowInfo(const Cell* cell) const
{
  if (infoMapEmpty()) {
    logger_->error(DPL, 43, "No grid layers mapped.");
  }
  GridMapKey key = getGridMapKey(cell);
  auto layer = getInfoMap().find(key);
  if (layer == getInfoMap().end()) {
    // this means the cell is taller than any layer
    logger_->error(DPL,
                   44,
                   "Cell {} with height {} is taller than any row.",
                   cell->name(),
                   cell->height_);
  }
  return std::make_pair(cell->height_, layer->second);
}

GridMapKey Grid::getGridMapKey(const dbSite* site) const
{
  return getSiteToGrid().at(site);
}

GridMapKey Grid::getGridMapKey(const Cell* cell) const
{
  if (cell == nullptr) {
    logger_->error(DPL, 5211, "getGridMapKey cell is null");
  }
  auto site = cell->getSite();
  if (!cell->isStdCell()) {
    // non std cells can go to the first grid.
    return getSmallestNonHybridGridKey();
  }
  if (site == nullptr) {
    logger_->error(DPL, 4219, "Cell {} has no site.", cell->name());
  }
  return getGridMapKey(site);
}

GridInfo Grid::getGridInfo(const Cell* cell) const
{
  return getInfoMap().at(getGridMapKey(cell));
}

pair<int, int> Grid::gridY(int y, const GridInfo& grid_info) const
{
  const int sum_heights = grid_info.getSitesTotalHeight();
  int base_height_index = divFloor(y, sum_heights);
  int cur_height = base_height_index * sum_heights;
  int index = 0;
  const dbSite::RowPattern& grid_sites = grid_info.getSites();
  base_height_index *= grid_sites.size();
  while (cur_height < y && index < grid_sites.size()) {
    const auto site = grid_sites.at(index).site;
    if (cur_height + site->getHeight() > y) {
      break;
    }
    cur_height += site->getHeight();
    index++;
  }
  return {base_height_index + index, cur_height};
}

pair<int, int> Grid::gridEndY(int y, const GridInfo& grid_info) const
{
  const int sum_heights = grid_info.getSitesTotalHeight();
  int base_height_index = divFloor(y, sum_heights);
  int cur_height = base_height_index * sum_heights;
  int index = 0;
  const dbSite::RowPattern& grid_sites = grid_info.getSites();
  base_height_index *= grid_sites.size();
  while (cur_height < y && index < grid_sites.size()) {
    const auto site = grid_sites.at(index).site;
    cur_height += site->getHeight();
    index++;
  }
  return {base_height_index + index, cur_height};
}

int Grid::gridY(const Cell* cell) const
{
  return gridY(cell->y_, cell);
}

int Grid::gridY(const int y, const Cell* cell) const
{
  if (cell->isHybrid()) {
    auto grid_info = getGridInfo(cell);
    return gridY(y, grid_info).first;
  }

  return y / getRowHeight(cell);
}

void Grid::setGridPaddedLoc(Cell* cell, int x, int y) const
{
  cell->x_ = (x + padding_->padLeft(cell)) * getSiteWidth();
  if (cell->isHybrid()) {
    auto grid_info = getInfoMap().at(getGridMapKey(cell));
    int total_sites_height = grid_info.getSitesTotalHeight();
    const auto& sites = grid_info.getSites();
    const int sites_size = sites.size();
    int height = (y / sites_size) * total_sites_height;
    for (int s = 0; s < y % sites_size; s++) {
      height += sites[s].site->getHeight();
    }
    cell->y_ = height;
    if (cell->isHybridParent()) {
      debugPrint(
          logger_,
          DPL,
          "hybrid",
          1,
          "Offsetting cell {} to start at {} instead of {} -> offset: {}",
          cell->name(),
          cell->y_ + grid_info.getOffset(),
          cell->y_,
          grid_info.getOffset());
      cell->y_ += grid_info.getOffset();
    }
    return;
  }
  cell->y_ = y * getRowHeight(cell);
}

int Grid::gridPaddedEndX(const Cell* cell) const
{
  const int site_width = getSiteWidth();
  return divCeil(
      cell->x_ + cell->width_ + padding_->padRight(cell) * site_width,
      site_width);
}

int Grid::gridEndX(const Cell* cell) const
{
  return divCeil(cell->x_ + cell->width_, getSiteWidth());
}

int Grid::gridEndY(const Cell* cell) const
{
  return gridEndY(cell->y_ + cell->height_, cell);
}

int Grid::gridEndY(int y, const Cell* cell) const
{
  if (cell->isHybrid()) {
    auto grid_info = getGridInfo(cell);
    return gridY(y, grid_info).first;
  }
  int row_height = getRowHeight(cell);
  return divCeil(y, row_height);
}

bool Grid::cellFitsInCore(Cell* cell) const
{
  return gridPaddedWidth(cell) <= getRowSiteCount()
         && gridHeight(cell) <= getRowCount();
}

void Grid::examineRows(dbBlock* block)
{
  has_hybrid_rows_ = false;
  bool has_non_hybrid_rows = false;
  dbSite* first_site = nullptr;

  visitDbRows(block, [&](odb::dbRow* row) {
    dbSite* site = row->getSite();
    if (site->isHybrid()) {
      has_hybrid_rows_ = true;
    } else {
      has_non_hybrid_rows = true;
    }

    // Check all sites have equal width
    if (!first_site) {
      first_site = site;
      site_width_ = site->getWidth();
    } else if (site->getWidth() != site_width_) {
      logger_->error(DPL,
                     51,
                     "Site widths are not equal: {}={} != {}={}",
                     first_site->getName(),
                     site_width_,
                     site->getName(),
                     site->getWidth());
    }
  });

  if (!hasHybridRows() && !has_non_hybrid_rows) {
    logger_->error(DPL, 12, "no rows found.");
  }

  if (hasHybridRows() && has_non_hybrid_rows) {
    logger_->error(
        DPL, 49, "Mixing hybrid and non-hybrid rows is unsupported.");
  }

  row_height_ = std::numeric_limits<int>::max();
  visitDbRows(block, [&](odb::dbRow* db_row) {
    dbSite* site = db_row->getSite();
    row_height_ = std::min(row_height_, static_cast<int>(site->getHeight()));
  });
  row_site_count_ = divFloor(getCore().dx(), getSiteWidth());
  row_count_ = divFloor(getCore().dy(), getRowHeight());
}

}  // namespace dpl
