// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "Coordinates.h"
#include "Objects.h"
#include "Opendp.h"

namespace dpl {

using odb::dbOrientType;
using odb::dbSite;

struct GridIntervalX
{
  GridX lo;
  GridX hi;
};

struct GridIntervalY
{
  GridY lo;
  GridY hi;
};

struct Pixel
{
  Node* cell = nullptr;
  Group* group = nullptr;
  double util = 0.0;
  bool is_valid = false;     // false for dummy cells
  bool is_hopeless = false;  // too far from sites for diamond search
  std::map<dbSite*, dbOrientType> sites;
};

// Return value for grid searches.
class PixelPt
{
 public:
  PixelPt() = default;
  PixelPt(Pixel* pixel, GridX grid_x, GridY grid_y);
  Pixel* pixel = nullptr;
  GridX x{0};
  GridY y{0};
};

// The "Grid" is a 2D array of pixels.  The pixels represent the
// single height sites onto which multi-height sites are overlaid.
// The sites are assumed to be of a single width but the rows are of
// variable height in order to support hybrid rows.
class Grid
{
 public:
  void init(Logger* logger) { logger_ = logger; }
  void initBlock(dbBlock* block) { core_ = block->getCoreArea(); }
  void initGrid(dbDatabase* db,
                dbBlock* block,
                std::shared_ptr<Padding> padding,
                int max_displacement_x,
                int max_displacement_y);
  void allocateGrid();
  void examineRows(dbBlock* block);
  std::unordered_set<int> getRowCoordinates() const;

  GridX gridX(DbuX x) const;
  GridX gridEndX(DbuX x) const;

  GridX gridX(const Node* cell) const;
  GridX gridEndX(const Node* cell) const;
  GridX gridPaddedX(const Node* cell) const;
  GridX gridPaddedEndX(const Node* cell) const;

  GridY gridSnapDownY(DbuY y) const;
  GridY gridRoundY(DbuY y) const;
  GridY gridEndY(DbuY y) const;

  // Snap outwards to fully contain
  GridRect gridCovering(const Rect& rect) const;
  GridRect gridCovering(const Node* cell) const;
  GridRect gridCoveringPadded(const Node* cell) const;

  // Snap inwards to be fully contained
  GridRect gridWithin(const DbuRect& rect) const;

  GridY gridSnapDownY(const Node* cell) const;
  GridY gridRoundY(const Node* cell) const;
  GridY gridEndY(const Node* cell) const;

  DbuY gridYToDbu(GridY y) const;

  GridX gridPaddedWidth(const Node* cell) const;
  GridY gridHeight(const Node* cell) const;
  GridY gridHeight(odb::dbMaster* master) const;
  DbuY rowHeight(GridY index);

  void paintPixel(Node* cell, GridX grid_x, GridY grid_y);
  void erasePixel(Node* cell);
  void visitCellPixels(Node& cell,
                       bool padded,
                       const std::function<void(Pixel* pixel)>& visitor) const;
  void visitCellBoundaryPixels(
      Node& cell,
      const std::function<
          void(Pixel* pixel, odb::Direction2D edge, GridX x, GridY y)>& visitor)
      const;

  GridY getRowCount() const { return row_count_; }
  GridX getRowSiteCount() const { return row_site_count_; }
  DbuX getSiteWidth() const { return site_width_; }

  Pixel* gridPixel(GridX x, GridY y) const;
  Pixel& pixel(GridY y, GridX x) { return pixels_[y.v][x.v]; }
  const Pixel& pixel(GridY y, GridX x) const { return pixels_[y.v][x.v]; }

  void resize(int size) { pixels_.resize(size); }
  void resize(GridY size) { pixels_.resize(size.v); }
  void resize(GridY y, GridX size) { pixels_[y.v].resize(size.v); }
  void clear();

  bool hasHybridRows() const { return has_hybrid_rows_; }

  GridY getRowCount(DbuY row_height) const;

  Rect getCore() const { return core_; }
  bool cellFitsInCore(Node* cell) const;

  bool isMultiHeight(dbMaster* master) const;

 private:
  void markHopeless(dbBlock* block,
                    int max_displacement_x,
                    int max_displacement_y);
  void markBlocked(dbBlock* block);
  void visitDbRows(dbBlock* block,
                   const std::function<void(odb::dbRow*)>& func) const;

  using Pixels = std::vector<std::vector<Pixel>>;
  Logger* logger_ = nullptr;
  dbBlock* block_ = nullptr;
  std::shared_ptr<Padding> padding_;
  Pixels pixels_;
  // Contains all the rows' yLo plus the yHi of the last row.  The extra
  // value is useful for operations like region snapping to rows
  std::map<DbuY, GridY> row_y_dbu_to_index_;
  std::vector<DbuY> row_index_to_y_dbu_;         // index is GridY
  std::vector<DbuY> row_index_to_pixel_height_;  // index is GridY

  bool has_hybrid_rows_ = false;
  Rect core_;

  std::optional<DbuY> uniform_row_height_;  // unset if hybrid
  DbuX site_width_{0};

  GridY row_count_{0};
  GridX row_site_count_{0};
};

}  // namespace dpl
