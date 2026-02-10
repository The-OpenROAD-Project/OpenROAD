// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "Grid.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Coordinates.h"
#include "Objects.h"
#include "Padding.h"
#include "boost/polygon/polygon.hpp"  // NOLINT(misc-include-cleaner) Boost polygon headers require a specific include order.
#include "boost/polygon/polygon_90_set_data.hpp"
#include "boost/polygon/rectangle_concept.hpp"
#include "boost/polygon/rectangle_data.hpp"
#include "dpl/Opendp.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"
#include "utl/Logger.h"

namespace dpl {

using std::max;
using std::min;

using utl::DPL;

using odb::dbBox;
using odb::dbRow;

using utl::format_as;  // NOLINT(misc-unused-using-decls)

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

void Grid::visitDbRows(odb::dbBlock* block,
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
      pixel.blocked_layers = 0;
    }
  }

  row_sites_.clear();
  row_sites_.resize(row_count_.v);
}

void Grid::markHopeless(odb::dbBlock* block,
                        const int max_displacement_x,
                        const int max_displacement_y)
{
  namespace gtl = boost::polygon;
  using gtl::operators::operator+=;
  using gtl::operators::operator-=;

  gtl::polygon_90_set_data<int> hopeless;
  hopeless += gtl::rectangle_data<int>{0, 0, row_site_count_.v, row_count_.v};

  const odb::Rect core = getCore();

  // Fragmented row support; mark valid sites.
  visitDbRows(block, [&](odb::dbRow* db_row) {
    const odb::Point orig = db_row->getOrigin();

    const GridX x_start{(orig.x() - core.xMin()) / site_width_.v};
    const GridX x_end{x_start + db_row->getSiteCount()};
    const GridY y_row{gridSnapDownY(DbuY{orig.y() - core_.yMin()})};
    for (GridX x{x_start}; x < x_end; x++) {
      Pixel* pixel = gridPixel(x, y_row);
      pixel->is_valid = true;
    }
    row_sites_[y_row.v].add(
        {{x_start.v, x_end.v}, {{db_row->getSite(), db_row->getOrient()}}});

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

void Grid::markBlocked(odb::dbBlock* block)
{
  const odb::Rect core = getCore();
  auto addBlockedLayers
      = [&](odb::Rect wire_rect, odb::dbTechLayer* tech_layer) {
          if (tech_layer->getType() != odb::dbTechLayerType::Value::ROUTING) {
            return;
          }
          auto routing_level = tech_layer->getRoutingLevel();
          if (routing_level <= 1 || routing_level > 3) {  // considering M2, M3
            return;
          }
          if (wire_rect.getDir() == odb::horizontal) {
            return;
          }
          wire_rect.moveDelta(-core.xMin(), -core.yMin());
          GridRect grid_rect = gridCovering(wire_rect);
          GridRect core{.xlo = GridX{0},
                        .ylo = GridY{0},
                        .xhi = GridX{row_site_count_},
                        .yhi = GridY{row_count_}};
          grid_rect = grid_rect.intersect(core);
          for (GridY y = grid_rect.ylo; y < grid_rect.yhi; y++) {
            for (GridX x = grid_rect.xlo; x < grid_rect.xhi; x++) {
              auto pixel1 = gridPixel(x, y);
              if (pixel1) {
                pixel1->blocked_layers |= 1 << routing_level;
              }
            }
          }
        };

  for (auto net : block->getNets()) {
    if (!net->isSpecial()) {
      continue;
    }
    for (odb::dbSWire* swire : net->getSWires()) {
      for (odb::dbSBox* sbox : swire->getWires()) {
        if (sbox->isVia()) {
          // TODO: handle via
          continue;
        }
        if (sbox->getWireShapeType() == odb::dbWireShapeType::DRCFILL) {
          // TODO: handle patches
          continue;
        }
        odb::Rect wire_rect = sbox->getBox();
        odb::dbTechLayer* tech_layer = sbox->getTechLayer();
        addBlockedLayers(wire_rect, tech_layer);
      }
    }
  }
  for (odb::dbBlockage* blockage : block->getBlockages()) {
    if (blockage->isSoft()) {
      continue;
    }
    odb::Rect bbox = blockage->getBBox()->getBox();
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

void Grid::initGrid(odb::dbDatabase* db,
                    odb::dbBlock* block,
                    std::shared_ptr<Padding> padding,
                    int max_displacement_x,
                    int max_displacement_y)
{
  padding_ = std::move(padding);

  allocateGrid();

  markHopeless(block, max_displacement_x, max_displacement_y);

  markBlocked(block);
}

std::pair<odb::dbSite*, odb::dbOrientType> Grid::getShortestSite(GridX grid_x,
                                                                 GridY grid_y)
{
  odb::dbSite* selected_site = nullptr;
  odb::dbOrientType selected_orient;
  DbuY min_height{std::numeric_limits<int>::max()};

  const RowSitesMap& sites_map = row_sites_[grid_y.v];
  auto it = sites_map.find(grid_x.v);

  if (it != sites_map.end()) {
    for (const auto& [site, orient] : it->second) {
      DbuY site_height{site->getHeight()};
      if (site_height < min_height) {
        min_height = site_height;
        selected_site = site;
        selected_orient = orient;
      }
    }
  }

  return {selected_site, selected_orient};
}

std::optional<odb::dbOrientType>
Grid::getSiteOrientation(GridX x, GridY y, odb::dbSite* site) const
{
  const RowSitesMap& sites_map = row_sites_[y.v];
  auto interval_it = sites_map.find(x.v);

  if (interval_it == sites_map.end()) {
    return {};
  }

  const SiteToOrientation& sites_orient = interval_it->second;

  if (auto it = sites_orient.find(site); it != sites_orient.end()) {
    return it->second;
  }

  return {};
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
    const std::function<void(Pixel* pixel, bool padded)>& visitor) const
{
  odb::dbInst* inst = cell.getDbInst();
  auto obstructions = inst->getMaster()->getObstructions();
  bool have_obstructions = false;
  const odb::Rect core = getCore();

  for (dbBox* obs : obstructions) {
    if (obs->getTechLayer()->getType()
        == odb::dbTechLayerType::Value::OVERLAP) {
      have_obstructions = true;

      odb::Rect rect = obs->getBox();
      inst->getTransform().apply(rect);
      rect.moveDelta(-core.xMin(), -core.yMin());
      GridRect grid_rect = gridCovering(rect);

      for (GridX x = grid_rect.xlo; x < grid_rect.xhi; x++) {
        for (GridY y = grid_rect.ylo; y < grid_rect.yhi; y++) {
          Pixel* pixel = gridPixel(x, y);
          if (pixel) {
            visitor(pixel, false);
          }
        }
      }
    }
  }
  if (!have_obstructions) {
    const auto grid_box = gridCovering(&cell);
    auto pad_left = padded ? padding_->padLeft(&cell) : GridX{0};
    auto pad_right = padded ? padding_->padRight(&cell) : GridX{0};

    auto x_start = grid_box.xlo - pad_left;
    auto x_end = grid_box.xhi + pad_right;
    for (GridX x{x_start}; x < x_end; x++) {
      for (GridY y{grid_box.ylo}; y < grid_box.yhi; y++) {
        Pixel* pixel = gridPixel(x, y);
        if (pixel == nullptr) {
          continue;
        }
        const bool within_cell = x >= grid_box.xlo && x < grid_box.xhi;
        visitor(pixel, !within_cell);
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
  odb::dbInst* inst = cell.getDbInst();

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

  odb::dbMaster* master = inst->getMaster();
  auto obstructions = master->getObstructions();
  bool have_obstructions = false;
  const odb::Rect core = getCore();
  for (dbBox* obs : obstructions) {
    if (obs->getTechLayer()->getType()
        == odb::dbTechLayerType::Value::OVERLAP) {
      have_obstructions = true;

      odb::Rect rect = obs->getBox();
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
  paintPixel(cell, gridX(cell), gridSnapDownY(cell));
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

  // Clear cell occupancy and padding reservations for this cell
  for (GridX x = grid_rect.xlo; x < grid_rect.xhi; x++) {
    for (GridY y = grid_rect.ylo; y < grid_rect.yhi; y++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel == nullptr) {
        continue;
      }

      // Clear cell occupancy
      if (pixel->cell == cell) {
        pixel->cell = nullptr;
        pixel->util = 0;
      }

      // Clear padding reservations made by this cell
      if (pixel->padding_reserved_by == cell) {
        pixel->padding_reserved_by = nullptr;
      }
    }
  }
}

void Grid::paintPixel(Node* cell, GridX grid_x, GridY grid_y)
{
  // Paint the actual cell footprint (not including padding)
  GridX cell_x_end = grid_x + gridWidth(cell);
  GridY cell_y_end = gridEndY(gridYToDbu(grid_y) + cell->getHeight());

  // Mark actual cell pixels
  for (GridX x{grid_x}; x < cell_x_end; x++) {
    for (GridY y{grid_y}; y < cell_y_end; y++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel == nullptr) {
        continue;
      }
      pixel->cell = cell;
      pixel->util = 1.0;
    }
  }

  // Mark spacing reservations around the cell
  paintCellPadding(cell, grid_x, grid_y, cell_x_end, cell_y_end);
}
void Grid::paintCellPadding(Node* cell)
{
  auto grid_x_begin = gridX(cell);
  auto grid_y_begin = gridSnapDownY(cell);
  auto grid_x_end = grid_x_begin + gridWidth(cell);
  auto grid_y_end = gridEndY(gridYToDbu(grid_y_begin) + cell->getHeight());

  paintCellPadding(cell, grid_x_begin, grid_y_begin, grid_x_end, grid_y_end);
}
void Grid::paintCellPadding(Node* cell,
                            const GridX grid_x_begin,
                            const GridY grid_y_begin,
                            const GridX grid_x_end,
                            const GridY grid_y_end)
{
  GridX left_pad = padding_->padLeft(cell);
  GridX right_pad = padding_->padRight(cell);

  // Reserve left spacing pixels
  for (GridX x{grid_x_begin - left_pad}; x < grid_x_begin; x++) {
    for (GridY y{grid_y_begin}; y < grid_y_end; y++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel == nullptr) {
        continue;
      }
      pixel->padding_reserved_by = cell;
    }
  }

  // Reserve right spacing pixels
  for (GridX x{grid_x_end}; x < grid_x_end + right_pad; x++) {
    for (GridY y{grid_y_begin}; y < grid_y_end; y++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel == nullptr) {
        continue;
      }
      pixel->padding_reserved_by = cell;
    }
  }
}

GridX Grid::gridPaddedWidth(const Node* cell) const
{
  return GridX{divCeil(padding_->paddedWidth(cell).v, getSiteWidth().v)};
}

GridX Grid::gridWidth(const Node* cell) const
{
  return GridX{divCeil(cell->getWidth().v, getSiteWidth().v)};
}

GridY Grid::gridHeight(odb::dbMaster* master) const
{
  odb::Rect bbox;
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

GridRect Grid::gridCovering(const odb::Rect& rect) const
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
  auto it = std::upper_bound(  // NOLINT(modernize-use-ranges)
      row_index_to_y_dbu_.begin(),
      row_index_to_y_dbu_.end(),
      y);
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
  auto it = std::lower_bound(  // NOLINT(modernize-use-ranges)
      row_index_to_y_dbu_.begin(),
      row_index_to_y_dbu_.end(),
      y);
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

void Grid::examineRows(odb::dbBlock* block)
{
  block_ = block;
  has_hybrid_rows_ = false;
  bool has_non_hybrid_rows = false;
  odb::dbSite* first_site = nullptr;

  visitDbRows(block, [&](odb::dbRow* row) {
    odb::dbSite* site = row->getSite();
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

bool Grid::isMultiHeight(odb::dbMaster* master) const
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

int Grid::countValidPixels(GridX x_begin,
                           GridY y_begin,
                           GridX x_end,
                           GridY y_end) const
{
  int count = 0;
  for (GridY y = y_begin; y < y_end; y++) {
    for (GridX x = x_begin; x < x_end; x++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel != nullptr && pixel->is_valid) {
        ++count;
      }
    }
  }
  return count;
}

void Grid::applyCellContribution(Node* node,
                                 GridX x_begin,
                                 GridY y_begin,
                                 GridX x_end,
                                 GridY y_end,
                                 float scale)
{
  const int cell_pixel_count = countValidPixels(x_begin, y_begin, x_end, y_end);
  if (cell_pixel_count == 0) {
    return;
  }
  if (total_area_.empty()) {
    return;
  }

  const float cell_area
      = static_cast<float>(node->getWidth().v * node->getHeight().v);
  const float area_per_pixel
      = (cell_area / static_cast<float>(cell_pixel_count)) * scale;
  const float pins_per_pixel
      = (static_cast<float>(node->getNumPins()) / cell_pixel_count) * scale;

  const int grid_size = static_cast<int>(total_area_.size());
  for (GridY y = y_begin; y < y_end; y++) {
    for (GridX x = x_begin; x < x_end; x++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel == nullptr || !pixel->is_valid) {
        continue;
      }
      const int pixel_idx = (y.v * row_site_count_.v) + x.v;
      if (pixel_idx < 0 || pixel_idx >= grid_size) {
        continue;
      }
      total_area_[pixel_idx] += area_per_pixel;
      total_pins_[pixel_idx] += pins_per_pixel;
      total_area_[pixel_idx] = max(total_area_[pixel_idx], 0.0f);
      total_pins_[pixel_idx] = max(total_pins_[pixel_idx], 0.0f);
    }
  }
}

void Grid::computeUtilizationMap(Network* network,
                                 float area_weight,
                                 float pin_weight)
{
  // Get grid dimensions
  const int grid_size = row_count_.v * row_site_count_.v;

  if (grid_size == 0) {
    return;  // No grid to work with
  }

  // Store weights for incremental updates
  area_weight_ = area_weight;
  pin_weight_ = pin_weight;

  // Initialize member vectors for area and pin density accumulation
  total_area_.assign(grid_size, 0.0f);
  total_pins_.assign(grid_size, 0.0f);
  utilization_density_.assign(grid_size, 0.0f);

  // Iterate through all movable nodes in the network
  for (const auto& node_ptr : network->getNodes()) {
    Node* node = node_ptr.get();
    if (!node || node->isFixed() || node->getType() != Node::Type::CELL) {
      continue;  // Skip fixed cells and non-standard cells
    }

    const GridRect cell_grid = gridCovering(node);
    applyCellContribution(
        node, cell_grid.xlo, cell_grid.ylo, cell_grid.xhi, cell_grid.yhi, 1.0f);
  }

  normalizeUtilization();
}

void Grid::normalizeUtilization()
{
  const int grid_size = total_area_.size();
  if (grid_size == 0) {
    return;
  }

  // Find maximum values for normalization
  float max_area = 0.0f;
  float max_pins = 0.0f;

  // We iterate manually to find max to avoid multiple passes or copies
  for (float value : total_area_) {
    max_area = std::max(value, max_area);
  }
  for (float value : total_pins_) {
    max_pins = std::max(value, max_pins);
  }

  if (max_area <= 0.0f || max_pins <= 0.0f) {
    if (logger_ != nullptr) {
      logger_->error(
          DPL,
          1300,
          "Utilization normalization failed: max area {} max pins {}.",
          max_area,
          max_pins);
    } else {
      throw std::runtime_error(
          "Utilization normalization failed: zero area or pins detected.");
    }
  }
  last_max_area_ = max_area;
  last_max_pins_ = max_pins;

  // Calculate weighted power density for each pixel
  float max_util_density = 0.0f;
  for (int i = 0; i < grid_size; i++) {
    const float normalized_area = total_area_[i] / max_area;
    const float normalized_pins = total_pins_[i] / max_pins;
    const float val
        = (area_weight_ * normalized_area) + (pin_weight_ * normalized_pins);
    utilization_density_[i] = val;
    max_util_density = std::max(val, max_util_density);
  }

  // Final normalization of power density to [0, 1] range
  if (max_util_density > 0.0f) {
    for (float& density : utilization_density_) {
      density /= max_util_density;
    }
  }
  last_max_utilization_ = max_util_density > 0.0f ? max_util_density : 1.0f;
  utilization_dirty_ = false;
}

void Grid::updateUtilizationMap(Node* node, DbuX x, DbuY y, bool add)
{
  if (!node || node->isFixed() || node->getType() != Node::Type::CELL) {
    return;  // Skip invalid, fixed, or non-standard cells
  }

  // Calculate grid rectangle for the cell at the given position
  const GridX grid_x = gridX(x);
  const GridY grid_y = gridSnapDownY(y);
  const GridX grid_x_end = grid_x + gridWidth(node);
  const GridY grid_y_end = gridEndY(y + node->getHeight());

  const float sign = add ? 1.0f : -1.0f;
  applyCellContribution(node, grid_x, grid_y, grid_x_end, grid_y_end, sign);
  utilization_dirty_ = true;
}

float Grid::getUtilizationDensity(int pixel_idx) const
{
  // When the map is marked dirty we reuse the maxima from the last full
  // normalization to produce an approximate normalized value. This avoids
  // recomputing the entire map on every query while still reflecting the most
  // recent raw contributions.

  if (pixel_idx < 0
      || pixel_idx >= static_cast<int>(utilization_density_.size())) {
    return 0.0f;
  }

  if (!utilization_dirty_) {
    return utilization_density_[pixel_idx];
  }

  if (last_max_area_ <= 0.0f || last_max_pins_ <= 0.0f
      || last_max_utilization_ <= 0.0f) {
    return utilization_density_[pixel_idx];
  }

  const float normalized_area = total_area_[pixel_idx] / last_max_area_;
  const float normalized_pins = total_pins_[pixel_idx] / last_max_pins_;
  const float val
      = (area_weight_ * normalized_area) + (pin_weight_ * normalized_pins);
  return std::min(val / last_max_utilization_, 1.0f);
}

}  // namespace dpl
