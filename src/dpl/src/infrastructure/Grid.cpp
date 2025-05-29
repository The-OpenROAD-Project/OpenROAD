// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "Grid.h"

#include <boost/polygon/polygon.hpp>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

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
    Node& cell,
    bool padded,
    const std::function<void(Pixel* pixel)>& visitor) const
{
  dbInst* inst = cell.getDbInst();
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
    Node& cell,
    const std::function<
        void(Pixel* pixel, odb::Direction2D edge, GridX x, GridY y)>& visitor)
    const
{
  dbInst* inst = cell.getDbInst();

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
    const auto grid_rect = gridCovering(&cell);
    debugPrint(logger_,
               DPL,
               "hybrid",
               1,
               "Checking cell {} isHybrid {} in rows. Y start {} y end {}",
               cell.getDbInst()->getName(),
               cell.isHybrid(),
               grid_rect.ylo,
               grid_rect.yhi);

    visit(grid_rect.xlo, grid_rect.xhi, grid_rect.ylo, grid_rect.yhi);
  }
}

void Grid::paintPixel(Node* cell)
{
  paintPixel(cell, gridPaddedX(cell), gridSnapDownY(cell));
}

void Grid::erasePixel(Node* cell)
{
  const auto grid_rect = gridCoveringPadded(cell);
  debugPrint(logger_,
             DPL,
             "hybrid",
             1,
             "Checking cell {} isHybrid {}",
             cell->getDbInst()->getName(),
             cell->isHybrid());
  debugPrint(logger_,
             DPL,
             "hybrid",
             1,
             "Checking cell {} in rows. Y start {} y end {}",
             cell->getDbInst()->getName(),
             grid_rect.ylo,
             grid_rect.yhi);

  for (GridX x = grid_rect.xlo; x < grid_rect.xhi; x++) {
    for (GridY y = grid_rect.ylo; y < grid_rect.yhi; y++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel == nullptr || pixel->cell != cell) {
        continue;
      }
      pixel->cell = nullptr;
      pixel->util = 0;
    }
  }
}

void Grid::paintPixel(Node* cell, GridX grid_x, GridY grid_y)
{
  GridX x_end = grid_x + gridPaddedWidth(cell);
  GridY y_end = gridEndY(gridYToDbu(grid_y) + cell->getHeight());

  for (GridX x{grid_x}; x < x_end; x++) {
    for (GridY y{grid_y}; y < y_end; y++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel == nullptr) {
        // This can happen if cell padding is larger than the grid.
        continue;
      }
      pixel->cell = cell;
      pixel->util = 1.0;
    }
  }
}

GridX Grid::gridPaddedWidth(const Node* cell) const
{
  return GridX{divCeil(padding_->paddedWidth(cell).v, getSiteWidth().v)};
}

GridY Grid::gridHeight(odb::dbMaster* master) const
{
  Rect bbox;
  master->getPlacementBoundary(bbox);
  if (uniform_row_height_) {
    DbuY row_height = uniform_row_height_.value();
    return GridY{max(1, divCeil(bbox.dy(), row_height.v))};
  }
  auto site = master->getSite();
  if (!site->hasRowPattern()) {
    return GridY{1};
  }

  return GridY{static_cast<int>(site->getRowPattern().size())};
}

GridY Grid::gridHeight(const Node* cell) const
{
  if (uniform_row_height_) {
    DbuY row_height = uniform_row_height_.value();
    return GridY{max(1, divCeil(cell->getHeight().v, row_height.v))};
  }
  if (!cell->getDbInst()) {
    return GridY{1};
  }
  auto site = cell->getDbInst()->getMaster()->getSite();
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

GridX Grid::gridX(const Node* cell) const
{
  return gridX(cell->getLeft());
}

GridX Grid::gridPaddedX(const Node* cell) const
{
  return gridX(cell->getLeft()
               - gridToDbu(padding_->padLeft(cell), getSiteWidth()));
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

GridRect Grid::gridCovering(const Node* cell) const
{
  return {.xlo = gridX(cell),
          .ylo = gridSnapDownY(cell),
          .xhi = gridEndX(cell),
          .yhi = gridEndY(cell)};
}

GridRect Grid::gridCoveringPadded(const Node* cell) const
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

GridY Grid::gridSnapDownY(const Node* cell) const
{
  return gridSnapDownY(cell->getBottom());
}

GridY Grid::gridRoundY(const Node* cell) const
{
  return gridRoundY(cell->getBottom());
}

DbuY Grid::gridYToDbu(GridY y) const
{
  if (y == row_index_to_y_dbu_.size()) {
    return DbuY{core_.yMax()};
  }
  return row_index_to_y_dbu_.at(y.v);
}

GridX Grid::gridPaddedEndX(const Node* cell) const
{
  const DbuX site_width = getSiteWidth();
  const DbuX end_x = cell->getLeft() + cell->getWidth()
                     + gridToDbu(padding_->padRight(cell), site_width);
  return GridX{divCeil(end_x.v, site_width.v)};
}

GridX Grid::gridEndX(const Node* cell) const
{
  return GridX{
      divCeil((cell->getLeft() + cell->getWidth()).v, getSiteWidth().v)};
}

GridY Grid::gridEndY(const Node* cell) const
{
  return gridEndY(cell->getBottom() + cell->getHeight());
}

bool Grid::cellFitsInCore(Node* cell) const
{
  return gridPaddedWidth(cell) <= getRowSiteCount()
         && cell->getHeight().v <= core_.dy();
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
      site_width_ = DbuX{site->getWidth()};
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

  GridY index{0};
  DbuY prev_y{0};
  for (auto& [dbu_y, grid_y] : row_y_dbu_to_index_) {
    grid_y = index++;
    row_index_to_y_dbu_.push_back(dbu_y);
    row_index_to_pixel_height_.push_back(dbu_y - prev_y);
    prev_y = dbu_y;
  }
  uniform_row_height_.reset();
  bool is_uniform = true;
  visitDbRows(block, [&](odb::dbRow* db_row) {
    if (!is_uniform) {
      return;
    }
    const int site_height = db_row->getSite()->getHeight();
    if (uniform_row_height_.has_value()) {
      // check if the bigger of both; the new and old heights, is a multiple of
      // the smaller
      const auto smaller = std::min(site_height, uniform_row_height_.value().v);
      const auto larger = std::max(site_height, uniform_row_height_.value().v);
      if (larger % smaller != 0) {
        // not uniform
        uniform_row_height_.reset();
        is_uniform = false;
      } else {
        // uniform
        uniform_row_height_ = DbuY{smaller};
      }
    } else {
      uniform_row_height_ = DbuY{site_height};
    }
  });
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
    return master->getHeight() > uniform_row_height_.value();
  }

  return master->getSite()->hasRowPattern();
}

DbuY Grid::rowHeight(GridY index)
{
  return row_index_to_pixel_height_.at(index.v);
}

}  // namespace dpl
