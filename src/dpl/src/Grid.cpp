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

void Grid::clear()
{
  pixels_.clear();
  row_y_dbu_to_index_.clear();
  row_index_to_y_dbu_.clear();
}

void Grid::visitDbRows(dbBlock* block,
                       const std::function<void(dbRow*)>& func) const
{
  for (auto row : block->getRows()) {
    // dpl doesn't deal with pads
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      func(row);
    }
  }
}

void Grid::allocateGrid()
{
  // Make pixel grid
  if (pixels_.empty()) {
    resize(row_count_);
    for (GridY y{0}; y < row_count_; y++) {
      resize(y, row_site_count_);
    }
  }

  for (GridY y{0}; y < row_count_; y++) {
    for (GridX x{0}; x < row_site_count_; x++) {
      Pixel& pixel = pixels_[y.v][x.v];
      pixel.cell = nullptr;
      pixel.group = nullptr;
      pixel.util = 0.0;
      pixel.is_valid = false;
      pixel.is_hopeless = false;
    }
  }
}

void Grid::markHopeless(dbBlock* block,
                        const int max_displacement_x,
                        const int max_displacement_y)
{
  namespace gtl = boost::polygon;
  using gtl::operators::operator+=;
  using gtl::operators::operator-=;

  gtl::polygon_90_set_data<int> hopeless;
  hopeless += gtl::rectangle_data<int>{0, 0, row_site_count_.v, row_count_.v};

  const Rect core = getCore();

  // Fragmented row support; mark valid sites.
  visitDbRows(block, [&](odb::dbRow* db_row) {
    const odb::Point orig = db_row->getOrigin();

    const GridX x_start{(orig.x() - core.xMin()) / site_width_.v};
    const GridX x_end{x_start + db_row->getSiteCount()};
    const GridY y_row{gridSnapDownY(DbuY{orig.y() - core_.yMin()})};
    for (GridX x{x_start}; x < x_end; x++) {
      Pixel* pixel = gridPixel(x, y_row);
      pixel->is_valid = true;
      pixel->sites[db_row->getSite()] = db_row->getOrient();
    }

    // The safety margin is to avoid having only a very few sites
    // within the diamond search that may still lead to failures.
    const int safety = 20;
    const GridX xl = max(GridX{0}, x_start - max_displacement_x + safety);
    const GridX xh = min(row_site_count_, x_end + max_displacement_x - safety);

    const GridY yl = max(GridY{0}, y_row - max_displacement_y + safety);
    const GridY yh = min(row_count_, y_row + max_displacement_y - safety);
    hopeless -= gtl::rectangle_data<int>{xl.v, yl.v, xh.v, yh.v};
  });

  std::vector<gtl::rectangle_data<int>> rects;
  hopeless.get_rectangles(rects);
  for (const auto& rect : rects) {
    for (int y = gtl::yl(rect); y < gtl::yh(rect); y++) {
      for (int x = gtl::xl(rect); x < gtl::xh(rect); x++) {
        pixels_[y][x].is_hopeless = true;
      }
    }
  }
}

void Grid::markBlocked(dbBlock* block)
{
  const Rect core = getCore();
  for (odb::dbBlockage* blockage : block->getBlockages()) {
    if (blockage->isSoft()) {
      continue;
    }
    Rect bbox = blockage->getBBox()->getBox();
    bbox.moveDelta(-core.xMin(), -core.yMin());
    GridRect grid_rect = gridCovering(bbox);

    // Clip to the core area
    GridRect core{.xlo = GridX{0},
                  .ylo = GridY{0},
                  .xhi = GridX{row_site_count_},
                  .yhi = GridY{row_count_}};
    grid_rect = grid_rect.intersect(core);

    for (GridY y = grid_rect.ylo; y < grid_rect.yhi; y++) {
      for (GridX x = grid_rect.xlo; x < grid_rect.xhi; x++) {
        Pixel& pixel1 = pixel(y, x);
        pixel1.is_valid = false;
      }
    }
  }
}

void Grid::initGrid(dbDatabase* db,
                    dbBlock* block,
                    std::shared_ptr<Padding> padding,
                    int max_displacement_x,
                    int max_displacement_y)
{
  padding_ = std::move(padding);

  allocateGrid();

  markHopeless(block, max_displacement_x, max_displacement_y);

  markBlocked(block);
}

Pixel* Grid::gridPixel(GridX grid_x, GridY grid_y) const
{
  if (grid_x >= 0 && grid_x < row_site_count_ && grid_y >= 0
      && grid_y < row_count_) {
    return const_cast<Pixel*>(&pixels_[grid_y.v][grid_x.v]);
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
      rect.moveDelta(-core.xMin(), -core.yMin());
      GridRect grid_rect = gridCovering(rect);

      for (GridX x = grid_rect.xlo; x < grid_rect.xhi; x++) {
        for (GridY y = grid_rect.ylo; y < grid_rect.yhi; y++) {
          Pixel* pixel = gridPixel(x, y);
          if (pixel) {
            visitor(pixel);
          }
        }
      }
    }
  }
  if (!have_obstructions) {
    const auto grid_box
        = padded ? gridCoveringPadded(&cell) : gridCovering(&cell);
    for (GridX x{grid_box.xlo}; x < grid_box.xhi; x++) {
      for (GridY y{grid_box.ylo}; y < grid_box.yhi; y++) {
        Pixel* pixel = gridPixel(x, y);
        if (pixel) {
          visitor(pixel);
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

  auto visit = [&visitor, this](const GridX x_start,
                                const GridX x_end,
                                const GridY y_start,
                                const GridY y_end) {
    for (GridX x = x_start; x < x_end; x++) {
      Pixel* pixel = gridPixel(x, y_start);
      if (pixel) {
        visitor(pixel, odb::Direction2D::North, x, y_start);
      }
      pixel = gridPixel(x, y_end - 1);
      if (pixel) {
        visitor(pixel, odb::Direction2D::South, x, y_end - 1);
      }
    }
    for (GridY y = y_start; y < y_end; y++) {
      Pixel* pixel = gridPixel(x_start, y);
      if (pixel) {
        visitor(pixel, odb::Direction2D::West, x_start, y);
      }
      pixel = gridPixel(x_end - 1, y);
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
      rect.moveDelta(-core.xMin(), -core.yMin());

      GridRect grid_rect = gridCovering(rect);
      visit(grid_rect.xlo, grid_rect.xhi, grid_rect.ylo, grid_rect.yhi);
    }
  }
  if (!have_obstructions) {
    const auto grid_rect
        = padded ? gridCoveringPadded(&cell) : gridCovering(&cell);
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} isHybrid {} in rows. Y start {} y end {}",
               cell.name(),
               cell.isHybrid(),
               grid_rect.ylo,
               grid_rect.yhi);

    visit(grid_rect.xlo, grid_rect.xhi, grid_rect.ylo, grid_rect.yhi);
  }
}

void Grid::erasePixel(Cell* cell)
{
  if (!(cell->isFixed() || !cell->is_placed_)) {
    const auto grid_rect = gridCoveringPadded(cell);
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} isHybrid {}",
               cell->name(),
               cell->isHybrid());
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} in rows. Y start {} y end {}",
               cell->name(),
               grid_rect.ylo,
               grid_rect.yhi);

    for (GridX x = grid_rect.xlo; x < grid_rect.xhi; x++) {
      for (GridY y = grid_rect.ylo; y < grid_rect.yhi; y++) {
        Pixel* pixel = gridPixel(x, y);
        if (nullptr == pixel) {
          continue;
        }
        pixel->cell = nullptr;
        pixel->util = 0;
      }
    }
    cell->is_placed_ = false;
    cell->hold_ = false;
  }
}

void Grid::paintPixel(Cell* cell, GridX grid_x, GridY grid_y)
{
  assert(!cell->is_placed_);
  GridX x_end = grid_x + gridPaddedWidth(cell);
  GridY grid_height = gridHeight(cell);
  GridY y_end = grid_y + grid_height;

  setGridPaddedLoc(cell, grid_x, grid_y);
  cell->is_placed_ = true;

  for (GridX x{grid_x}; x < x_end; x++) {
    for (GridY y{grid_y}; y < y_end; y++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel->cell) {
        logger_->error(
            DPL, 13, "Cannot paint grid because it is already occupied.");
      } else {
        pixel->cell = cell;
        pixel->util = 1.0;
      }
    }
  }

  cell->orient_ = gridPixel(grid_x, grid_y)
                      ->sites.at(cell->db_inst_->getMaster()->getSite());
}

GridX Grid::gridPaddedWidth(const Cell* cell) const
{
  return GridX{divCeil(padding_->paddedWidth(cell).v, getSiteWidth().v)};
}

GridY Grid::gridHeight(const Cell* cell) const
{
  if (uniform_row_height_) {
    DbuY row_height = uniform_row_height_.value();
    return GridY{max(1, divCeil(cell->height_.v, row_height.v))};
  }
  if (!cell->db_inst_) {
    return GridY{1};
  }
  auto site = cell->db_inst_->getMaster()->getSite();
  if (!site->hasRowPattern()) {
    return GridY{1};
  }

  return GridY{static_cast<int>(site->getRowPattern().size())};
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

GridRect Grid::gridCovering(const Rect& rect) const
{
  return {.xlo = gridX(DbuX{rect.xMin()}),
          .ylo = gridSnapDownY(DbuY{rect.yMin()}),
          .xhi = gridEndX(DbuX{rect.xMax()}),
          .yhi = gridEndY(DbuY{rect.yMax()})};
}

GridRect Grid::gridCovering(const Cell* cell) const
{
  return {.xlo = gridX(cell),
          .ylo = gridSnapDownY(cell),
          .xhi = gridEndX(cell),
          .yhi = gridEndY(cell)};
}

GridRect Grid::gridCoveringPadded(const Cell* cell) const
{
  return {.xlo = gridPaddedX(cell),
          .ylo = gridSnapDownY(cell),
          .xhi = gridPaddedEndX(cell),
          .yhi = gridEndY(cell)};
}

GridRect Grid::gridWithin(const DbuRect& rect) const
{
  return {.xlo = dbuToGridCeil(rect.xl, getSiteWidth()),
          .ylo = gridEndY(rect.yl),
          .xhi = dbuToGridFloor(rect.xh, getSiteWidth()),
          .yhi = gridSnapDownY(rect.yh)};
}

GridY Grid::gridSnapDownY(DbuY y) const
{
  auto it = std::upper_bound(
      row_index_to_y_dbu_.begin(), row_index_to_y_dbu_.end(), y.v);
  if (it == row_index_to_y_dbu_.begin()) {
    return GridY{0};
  }
  --it;
  return GridY{static_cast<int>(it - row_index_to_y_dbu_.begin())};
}

GridY Grid::gridRoundY(DbuY y) const
{
  const auto grid_y = gridSnapDownY(y);
  if (grid_y < row_index_to_y_dbu_.size() - 1) {
    const auto grid_next = grid_y + 1;
    if (std::abs(row_index_to_y_dbu_[grid_y.v].v - y.v)
        >= std::abs(row_index_to_y_dbu_[grid_next.v].v - y.v)) {
      return grid_next;
    }
  }
  return grid_y;
}

GridY Grid::gridEndY(DbuY y) const
{
  auto it = std::lower_bound(
      row_index_to_y_dbu_.begin(), row_index_to_y_dbu_.end(), y.v);
  if (it == row_index_to_y_dbu_.end()) {
    return GridY{static_cast<int>(row_index_to_y_dbu_.size())};
  }
  return GridY{static_cast<int>(it - row_index_to_y_dbu_.begin())};
}

GridY Grid::gridSnapDownY(const Cell* cell) const
{
  return gridSnapDownY(cell->y_);
}

GridY Grid::gridRoundY(const Cell* cell) const
{
  return gridRoundY(cell->y_);
}

DbuY Grid::gridYToDbu(GridY y) const
{
  if (y == row_index_to_y_dbu_.size()) {
    return DbuY{core_.yMax()};
  }
  return row_index_to_y_dbu_.at(y.v);
}

void Grid::setGridPaddedLoc(Cell* cell, GridX x, GridY y) const
{
  cell->x_ = gridToDbu(x + padding_->padLeft(cell), getSiteWidth());
  cell->y_ = gridYToDbu(y);
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
  return gridEndY(cell->y_ + cell->height_);
}

bool Grid::cellFitsInCore(Cell* cell) const
{
  return gridPaddedWidth(cell) <= getRowSiteCount()
         && cell->height_.v <= core_.dy();
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

    const int y_lo = row->getOrigin().getY() - core_.yMin();
    const DbuY dbu_y{y_lo};
    row_y_dbu_to_index_[dbu_y] = GridY{0};
    row_y_dbu_to_index_[dbu_y + site->getHeight()] = GridY{0};

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

  GridY index{0};
  DbuY prev_y{0};
  for (auto& [dbu_y, grid_y] : row_y_dbu_to_index_) {
    grid_y = index++;
    row_index_to_y_dbu_.push_back(dbu_y);
    row_index_to_pixel_height_.push_back(dbu_y - prev_y);
    prev_y = dbu_y;
  }

  if (!hasHybridRows()) {
    uniform_row_height_ = DbuY{std::numeric_limits<int>::max()};
    visitDbRows(block, [&](odb::dbRow* db_row) {
      const int site_height = db_row->getSite()->getHeight();
      uniform_row_height_
          = std::min(uniform_row_height_.value(), DbuY{site_height});
    });
  }
  row_site_count_ = GridX{divFloor(getCore().dx(), getSiteWidth().v)};
  row_count_ = GridY{static_cast<int>(row_y_dbu_to_index_.size() - 1)};
}

std::unordered_set<int> Grid::getRowCoordinates() const
{
  std::unordered_set<int> coords;
  visitDbRows(block_, [&](odb::dbRow* row) {
    coords.insert(row->getOrigin().y() - core_.yMin());
  });
  return coords;
}

bool Grid::isMultiHeight(dbMaster* master) const
{
  if (uniform_row_height_) {
    return master->getHeight() != uniform_row_height_.value();
  }

  return master->getSite()->hasRowPattern();
}

DbuY Grid::rowHeight(GridY index)
{
  return row_index_to_pixel_height_.at(index.v);
}

}  // namespace dpl
