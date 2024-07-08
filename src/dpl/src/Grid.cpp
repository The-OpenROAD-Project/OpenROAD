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

using utl::format_as;

PixelPt::PixelPt(Pixel* pixel1, GridX grid_x, GridY grid_y)
    : pixel(pixel1), x(grid_x), y(grid_y)
{
}

////////////////////////////////////////////////////////////////

GridInfo::GridInfo(const GridY row_count,
                   const GridX site_count,
                   const int grid_index,
                   const dbSite::RowPattern& sites)
    : row_count_(row_count),
      site_count_(site_count),
      grid_index_(grid_index),
      sites_(sites)
{
}

DbuY GridInfo::getSitesTotalHeight() const
{
  return std::accumulate(sites_.begin(),
                         sites_.end(),
                         DbuY{0},
                         [](DbuY sum, const dbSite::OrientedSite& entry) {
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

GridY Grid::calculateHybridSitesRowCount(dbSite* parent_hybrid_site) const
{
  auto row_pattern = parent_hybrid_site->getRowPattern();
  DbuY site_height{static_cast<int>(parent_hybrid_site->getHeight())};
  GridY rows_count = getRowCount(site_height);
  DbuY remaining_core_height{DbuY{core_.dy()}
                             - gridToDbu(rows_count, site_height)};

  rows_count = GridY{rows_count.v * static_cast<int>(row_pattern.size())};

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
  DbuY min_row_y_coordinate{std::numeric_limits<int>::max()};
  visitDbRows(block, [&](odb::dbRow* db_row) {
    auto site = db_row->getSite();
    DbuY row_base_y{db_row->getOrigin().y()};
    min_row_y_coordinate = min(min_row_y_coordinate, row_base_y);

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
    DbuY row_height{static_cast<int>(working_site->getHeight())};
    GridMapKey gmk = site_to_grid_key_.at(working_site);
    if (getInfoMap().find(gmk) == getInfoMap().end()) {
      if (working_site->hasRowPattern() || !working_site->isHybrid()) {
        GridInfo newGridInfo = {
            getRowCount(row_height),
            GridX{divFloor(core_.dx(), db_row->getSite()->getWidth())},
            getSiteToGrid().at(working_site).grid_index,
            dbSite::RowPattern{{working_site, db_row->getOrient()}},
        };
        if (working_site->isHybrid()) {
          DbuY row_offset
              = DbuY{db_row->getOrigin().y()} - min_row_y_coordinate;
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
            GridX{divFloor(core_.dx(), db_row->getSite()->getWidth())},
            getSiteToGrid().at(working_site).grid_index,
            parent_site->getRowPattern(),  // FIXME(mina1460): this is wrong! in
                                           // the case of HybridAB, and
                                           // HybridBA. each of A and B maps to
                                           // both, but the hybrid_sites_mapper
                                           // only record one of them, resulting
                                           // in the wrong RowPattern here.
        };
        if (working_site->isHybrid()) {
          DbuY row_offset
              = DbuY{db_row->getOrigin().y()} - min_row_y_coordinate;
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
      DbuY row_offset = DbuY{db_row->getOrigin().y()} - min_row_y_coordinate;
      auto grid_info = getInfoMap().at(gmk);
      if (grid_info.isHybrid()) {
        grid_info.setOffset(min(grid_info.getOffset(), row_offset));
        addInfoMap(gmk, grid_info);
      }
    }
  });
  grid_info_vector_.resize(getInfoMap().size());
  DbuY min_height{std::numeric_limits<int>::max()};
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
    const GridY layer_row_count = grid_info.getRowCount();
    const GridX layer_row_site_count = grid_info.getSiteCount();
    const int index = grid_info.getGridIndex();
    resize(index, layer_row_count);
    for (GridY j{0}; j < layer_row_count; j++) {
      resize(index, j, layer_row_site_count);
      const auto& grid_sites = grid_info.getSites();
      dbSite* row_site = nullptr;
      if (!grid_sites.empty()) {
        row_site = grid_sites[j.v % grid_sites.size()].site;
      }

      for (GridX k{0}; k < layer_row_site_count; k++) {
        Pixel& pixel = pixels_[index][j.v][k.v];
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
        0, 0, grid_info.getSiteCount().v, grid_info.getRowCount().v};
  }
  const Rect core = getCore();

  // Fragmented row support; mark valid sites.
  visitDbRows(block, [&](odb::dbRow* db_row) {
    const auto db_row_site = db_row->getSite();
    int current_row_site_count = db_row->getSiteCount();
    auto gmk = getGridMapKey(db_row_site);
    const auto& entry = getInfoMap().at(gmk);
    GridY current_row_count{entry.getRowCount()};
    int current_row_grid_index = entry.getGridIndex();
    const odb::Point orig = db_row->getOrigin();

    const int site_width = db_row_site->getWidth();
    const GridX x_start{(orig.x() - core.xMin()) / site_width};
    const GridX x_end{x_start + current_row_site_count};
    const GridY y_row{gridY(DbuY{orig.y() - core.yMin()}, entry).first};
    for (GridX x{x_start}; x < x_end; x++) {
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
    GridX max_row_site_count{divFloor(core.dx(), db_row_site->getWidth())};

    const GridX xl = max(GridX{0}, x_start - max_displacement_x + safety);
    const GridX xh
        = min(max_row_site_count, x_end + max_displacement_x - safety);

    const GridY yl = max(GridY{0}, y_row - max_displacement_y + safety);
    const GridY yh
        = min(current_row_count, y_row + max_displacement_y - safety);
    hopeless[current_row_grid_index]
        -= gtl::rectangle_data<int>{xl.v, yl.v, xh.v, yh.v};
  });

  for (odb::dbBlockage* blockage : block->getBlockages()) {
    if (blockage->isSoft()) {
      continue;
    }
    dbBox* bbox = blockage->getBBox();
    for (auto& [gmk, grid_info] : getInfoMap()) {
      GridX xlo = gridX(DbuX{bbox->xMin() - core.xMin()});
      GridX xhi = gridEndX(DbuX{bbox->xMax() - core.xMin()});
      auto [ylo, ignore_x] = gridY(DbuY{bbox->yMin() - core.yMin()}, grid_info);
      auto [yhi, ignore_y]
          = gridEndY(DbuY{bbox->yMax() - core.yMin()}, grid_info);

      // Clip to the core area
      xlo = max(GridX{0}, xlo);
      ylo = max(GridY{0}, ylo);
      xhi = min(grid_info.getSiteCount(), xhi);
      yhi = min(grid_info.getRowCount(), yhi);

      for (GridY y = ylo; y < yhi; y++) {
        for (GridX x = xlo; x < xhi; x++) {
          Pixel& pixel1 = pixel(grid_info.getGridIndex(), y, x);
          pixel1.is_valid = false;
        }
      }
    }
  }

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

Pixel* Grid::gridPixel(int grid_idx, GridX grid_x, GridY grid_y) const
{
  if (grid_idx < 0 || grid_idx >= grid_info_vector_.size()) {
    return nullptr;
  }
  const GridInfo* grid_info = grid_info_vector_[grid_idx];
  if (grid_x >= 0 && grid_x < grid_info->getSiteCount() && grid_y >= 0
      && grid_y < grid_info->getRowCount()) {
    return const_cast<Pixel*>(&pixels_[grid_idx][grid_y.v][grid_x.v]);
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
      GridX x_start = gridX(DbuX{rect.xMin() - core.xMin()});
      GridX x_end = gridEndX(DbuX{rect.xMax() - core.xMin()});
      GridY y_start = gridY(DbuY{rect.yMin() - core.yMin()}, &cell);
      GridY y_end = gridEndY(DbuY{rect.yMax() - core.yMin()}, &cell);

      // Since there is an obstruction, we need to visit all the pixels at all
      // layers (for all row heights)
      int grid_idx = 0;
      for (const auto& [target_GridMapKey, target_grid_info] : getInfoMap()) {
        const auto smallest_non_hybrid_grid_key = getSmallestNonHybridGridKey();
        GridY layer_y_start = map_ycoordinates(
            y_start, smallest_non_hybrid_grid_key, target_GridMapKey, true);
        GridY layer_y_end = map_ycoordinates(
            y_end, smallest_non_hybrid_grid_key, target_GridMapKey, false);
        if (layer_y_end == layer_y_start) {
          ++layer_y_end;
        }
        for (GridX x = x_start; x < x_end; x++) {
          for (GridY y{layer_y_start}; y < layer_y_end; y++) {
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
    GridX x_start = padded ? gridPaddedX(&cell) : gridX(&cell);
    GridX x_end = padded ? gridPaddedEndX(&cell) : gridEndX(&cell);
    GridY y_start = gridY(&cell);
    GridY y_end = gridEndY(&cell);
    auto src_gmk = getGridMapKey(&cell);
    for (const auto& layer_it : getInfoMap()) {
      GridX layer_x_start = x_start;
      GridX layer_x_end = x_end;
      GridY layer_y_start
          = map_ycoordinates(y_start, src_gmk, layer_it.first, true);
      GridY layer_y_end
          = map_ycoordinates(y_end, src_gmk, layer_it.first, false);
      if (layer_y_end == layer_y_start) {
        ++layer_y_end;
      }
      if (layer_x_end == layer_x_start) {
        ++layer_x_end;
      }

      for (GridX x{layer_x_start}; x < layer_x_end; x++) {
        for (GridY y{layer_y_start}; y < layer_y_end; y++) {
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
        void(Pixel* pixel, odb::Direction2D edge, GridX x, GridY y)>& visitor)
    const
{
  dbInst* inst = cell.db_inst_;
  const GridMapKey& gmk = getGridMapKey(&cell);
  GridInfo grid_info = getInfoMap().at(gmk);
  const int index_in_grid = grid_info.getGridIndex();

  auto visit = [&visitor, index_in_grid, this](const GridX x_start,
                                               const GridX x_end,
                                               const GridY y_start,
                                               const GridY y_end) {
    for (GridX x = x_start; x < x_end; x++) {
      Pixel* pixel = gridPixel(index_in_grid, x, y_start);
      if (pixel) {
        visitor(pixel, odb::Direction2D::North, x, y_start);
      }
      pixel = gridPixel(index_in_grid, x, y_end - 1);
      if (pixel) {
        visitor(pixel, odb::Direction2D::South, x, y_end - 1);
      }
    }
    for (GridY y = y_start; y < y_end; y++) {
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

      GridX x_start = gridX(DbuX{rect.xMin() - core.xMin()});
      GridX x_end = gridEndX(DbuX{rect.xMax() - core.xMin()});
      GridY y_start = gridY(DbuY{rect.yMin() - core.yMin()}, grid_info).first;
      GridY y_end = gridEndY(DbuY{rect.yMax() - core.yMin()}, grid_info).first;
      visit(x_start, x_end, y_start, y_end);
    }
  }
  if (!have_obstructions) {
    const GridX x_start = padded ? gridPaddedX(&cell) : gridX(&cell);
    const GridX x_end = padded ? gridPaddedEndX(&cell) : gridEndX(&cell);
    const GridY y_start = gridY(&cell);
    const GridY y_end = gridEndY(&cell);
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
    GridX x_end = gridPaddedEndX(cell);
    GridY y_end = gridEndY(cell);
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} isHybrid {}",
               cell->name(),
               cell->isHybrid());
    GridY y_start = gridY(cell);
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} in rows. Y start {} y end {}",
               cell->name(),
               y_start,
               y_end);

    for (const auto& [target_GridMapKey, target_grid_info] : getInfoMap()) {
      GridY layer_y_start
          = map_ycoordinates(y_start, gmk, target_GridMapKey, true);
      GridY layer_y_end
          = map_ycoordinates(y_end, gmk, target_GridMapKey, false);

      if (layer_y_end == layer_y_start) {
        ++layer_y_end;
      }

      for (GridX x = gridPaddedX(cell); x < x_end; x++) {
        for (GridY y = layer_y_start; y < layer_y_end; y++) {
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

GridY Grid::map_ycoordinates(GridY source_grid_coordinate,
                             const GridMapKey& source_grid_key,
                             const GridMapKey& target_grid_key,
                             const bool start) const
{
  if (source_grid_key == target_grid_key) {
    return source_grid_coordinate;
  }
  const auto& src_grid_info = getInfoMap().at(source_grid_key);
  const auto& target_grid_info = getInfoMap().at(target_grid_key);
  if (!src_grid_info.isHybrid()) {
    if (!target_grid_info.isHybrid()) {
      dbSite* src_site = src_grid_info.getSites()[0].site;
      DbuY original_step{static_cast<int>(src_site->getHeight())};
      dbSite* target_site = target_grid_info.getSites()[0].site;
      DbuY target_step{static_cast<int>(target_site->getHeight())};
      if (start) {
        return GridY{divFloor(
            gridToDbu(source_grid_coordinate, original_step).v, target_step.v)};
      }
      return GridY{divCeil(gridToDbu(source_grid_coordinate, original_step).v,
                           target_step.v)};
    }
    // count until we find it.  BUG?: why multiply by grid_index?
    return gridY(DbuY{source_grid_coordinate.v * source_grid_key.grid_index},
                 target_grid_info)
        .first;
  }
  // src is hybrid
  DbuY src_total_sites_height = src_grid_info.getSitesTotalHeight();
  const auto& src_sites = src_grid_info.getSites();
  const int src_sites_size = src_sites.size();
  DbuY src_height{(source_grid_coordinate.v / src_sites_size)
                  * src_total_sites_height.v};
  for (int s = 0; s < source_grid_coordinate.v % src_sites_size; s++) {
    src_height += src_sites[s].site->getHeight();
  }
  if (target_grid_info.isHybrid()) {
    // both are hybrids.
    return gridY(src_height, target_grid_info).first;
  }
  dbSite* target_site = target_grid_info.getSites()[0].site;
  const int target_step = target_site->getHeight();
  if (start) {
    return GridY{divFloor(src_height.v, target_step)};
  }
  return GridY{divCeil(src_height.v, target_step)};
}

void Grid::paintPixel(Cell* cell, GridX grid_x, GridY grid_y)
{
  assert(!cell->is_placed_);
  GridX x_end = grid_x + gridPaddedWidth(cell);
  GridY grid_height = gridHeight(cell);
  GridY y_end = grid_y + grid_height;
  GridMapKey gmk = getGridMapKey(cell);
  GridInfo grid_info = getInfoMap().at(gmk);
  const int index_in_grid = gmk.grid_index;
  setGridPaddedLoc(cell, grid_x, grid_y);
  cell->is_placed_ = true;
  for (GridX x{grid_x}; x < x_end; x++) {
    for (GridY y{grid_y}; y < y_end; y++) {
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
    GridX layer_x = grid_x;
    GridX layer_x_end = x_end;
    GridY layer_y = map_ycoordinates(grid_y, gmk, layer.first, true);
    GridY layer_y_end = map_ycoordinates(y_end, gmk, layer.first, false);
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
      layer_y_end = GridY{layer.second.getRowCount()};
    }

    for (GridX x = layer_x; x < layer_x_end; x++) {
      for (GridY y = layer_y; y < layer_y_end; y++) {
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

GridX Grid::gridPaddedWidth(const Cell* cell) const
{
  return GridX{divCeil(padding_->paddedWidth(cell).v, getSiteWidth().v)};
}

DbuY Grid::coordinateToHeight(GridY y_coordinate, GridMapKey gmk) const
{
  // gets a coordinate and its grid, and returns the height of the coordinate.
  // This is useful for hybrid sites
  const auto& grid_info = infoMap(gmk);
  if (grid_info.isHybrid()) {
    auto& grid_sites = grid_info.getSites();
    const DbuY total_height = grid_info.getSitesTotalHeight();
    GridY patterns_below{divFloor(y_coordinate.v, grid_sites.size())};
    DbuY remaining_rows_height = grid_info.getSitesTotalHeight();  // BUG?
    return gridToDbu(patterns_below, total_height) + remaining_rows_height;
  }
  return gridToDbu(y_coordinate, grid_info.getSitesTotalHeight());
}

GridY Grid::gridHeight(const Cell* cell) const
{
  DbuY row_height = getRowHeight(cell);
  return GridY{max(1, divCeil(cell->height_.v, row_height.v))};
}

GridX Grid::gridEndX(DbuX x) const
{
  return GridX{divCeil(x.v, getSiteWidth().v)};
}

GridX Grid::gridX(DbuX x) const
{
  return GridX{x.v / getSiteWidth().v};
}

GridX Grid::gridX(const Cell* cell) const
{
  return gridX(cell->x_);
}

GridX Grid::gridPaddedX(const Cell* cell) const
{
  return gridX(cell->x_ - gridToDbu(padding_->padLeft(cell), getSiteWidth()));
}

GridY Grid::getRowCount(DbuY row_height) const
{
  return GridY{divFloor(core_.dy(), row_height.v)};
}

DbuY Grid::getRowHeight(const Cell* cell) const
{
  DbuY row_height = getRowHeight();
  if (cell->isStdCell() || cell->isHybrid()) {
    row_height = cell->height_;
  }
  return row_height;
}

pair<DbuY, GridInfo> Grid::getRowInfo(const Cell* cell) const
{
  if (infoMapEmpty()) {
    logger_->error(DPL, 43, "No grid layers mapped.");
  }
  const GridMapKey key = getGridMapKey(cell);
  const auto layer = getInfoMap().find(key);
  if (layer == getInfoMap().end()) {
    // this means the cell is taller than any layer
    logger_->error(DPL,
                   44,
                   "Cell {} with height {} is taller than any row.",
                   cell->name(),
                   cell->height_);
  }
  return {cell->height_, layer->second};
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

pair<GridY, DbuY> Grid::gridY(DbuY y, const GridInfo& grid_info) const
{
  const DbuY sum_heights = grid_info.getSitesTotalHeight();
  GridY base_height_index{divFloor(y.v, sum_heights.v)};
  DbuY cur_height{base_height_index.v * sum_heights.v};
  int index = 0;
  const dbSite::RowPattern& grid_sites = grid_info.getSites();
  base_height_index
      = GridY{base_height_index.v * static_cast<int>(grid_sites.size())};
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

pair<GridY, DbuY> Grid::gridEndY(DbuY y, const GridInfo& grid_info) const
{
  const DbuY sum_heights = grid_info.getSitesTotalHeight();
  GridY base_height_index{divFloor(y.v, sum_heights.v)};
  DbuY cur_height{gridToDbu(base_height_index, sum_heights)};
  int index = 0;
  const dbSite::RowPattern& grid_sites = grid_info.getSites();
  base_height_index
      = GridY{base_height_index.v * static_cast<int>(grid_sites.size())};
  while (cur_height < y && index < grid_sites.size()) {
    const auto site = grid_sites.at(index).site;
    cur_height += site->getHeight();
    index++;
  }
  return {base_height_index + index, cur_height};
}

GridY Grid::gridY(const Cell* cell) const
{
  return gridY(cell->y_, cell);
}

GridY Grid::gridY(const DbuY y, const Cell* cell) const
{
  if (cell->isHybrid()) {
    auto grid_info = getGridInfo(cell);
    return gridY(y, grid_info).first;
  }

  return GridY{y.v / getRowHeight(cell).v};
}

void Grid::setGridPaddedLoc(Cell* cell, GridX x, GridY y) const
{
  cell->x_ = gridToDbu(x + padding_->padLeft(cell), getSiteWidth());
  if (cell->isHybrid()) {
    const auto& grid_info = getInfoMap().at(getGridMapKey(cell));
    DbuY total_sites_height = grid_info.getSitesTotalHeight();
    const auto& sites = grid_info.getSites();
    const int sites_size = sites.size();
    DbuY height{(y.v / sites_size) * total_sites_height.v};
    for (int s = 0; s < y.v % sites_size; s++) {
      height += DbuY{static_cast<int>(sites[s].site->getHeight())};
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
  cell->y_ = gridToDbu(y, getRowHeight(cell));
}

GridX Grid::gridPaddedEndX(const Cell* cell) const
{
  const DbuX site_width = getSiteWidth();
  const DbuX end_x
      = cell->xMax() + gridToDbu(padding_->padRight(cell), site_width);
  return GridX{divCeil(end_x.v, site_width.v)};
}

GridX Grid::gridEndX(const Cell* cell) const
{
  return GridX{divCeil((cell->x_ + cell->width_).v, getSiteWidth().v)};
}

GridY Grid::gridEndY(const Cell* cell) const
{
  return gridEndY(cell->y_ + cell->height_, cell);
}

GridY Grid::gridEndY(DbuY y, const Cell* cell) const
{
  if (cell->isHybrid()) {
    auto grid_info = getGridInfo(cell);
    return gridY(y, grid_info).first;
  }
  auto row_height = getRowHeight(cell);
  return GridY{divCeil(y.v, row_height.v)};
}

bool Grid::cellFitsInCore(Cell* cell) const
{
  return gridPaddedWidth(cell) <= getRowSiteCount()
         && gridHeight(cell) <= getRowCount();
}

void Grid::examineRows(dbBlock* block)
{
  block_ = block;
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
      site_width_ = DbuX{static_cast<int>(site->getWidth())};
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

  row_height_ = DbuY{std::numeric_limits<int>::max()};
  visitDbRows(block, [&](odb::dbRow* db_row) {
    const int site_height = db_row->getSite()->getHeight();
    row_height_ = DbuY{std::min(row_height_.v, site_height)};
  });
  row_site_count_ = GridX{divFloor(getCore().dx(), getSiteWidth().v)};
  row_count_ = GridY{divFloor(getCore().dy(), getRowHeight().v)};
}

std::unordered_set<int> Grid::getRowCoordinates() const
{
  std::unordered_set<int> coords;
  visitDbRows(block_, [&](odb::dbRow* row) {
    coords.insert(row->getOrigin().y() - core_.yMin());
  });
  return coords;
}

}  // namespace dpl
