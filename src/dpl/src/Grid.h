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

#pragma once

#include <unordered_set>

#include "Coordinates.h"
#include "dpl/Opendp.h"

namespace dpl {

using odb::dbOrientType;
using odb::dbSite;

struct PixelPoint
{
  GridX x;
  GridY y;
};

struct DbuPoint
{
  DbuX x;
  DbuY y;
};

struct Pixel
{
  Cell* cell = nullptr;
  Group* group = nullptr;
  double util = 0.0;
  dbOrientType orient_;
  bool is_valid = false;     // false for dummy cells
  bool is_hopeless = false;  // too far from sites for diamond search
  dbSite* site = nullptr;    // site that this pixel is
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

class GridInfo
{
 public:
  GridInfo(GridY row_count,
           GridX site_count,
           int grid_index,
           const dbSite::RowPattern& sites);

  bool isHybrid() const;
  int getGridIndex() const { return grid_index_; }
  DbuY getOffset() const { return offset_; }
  GridY getRowCount() const { return row_count_; }
  GridX getSiteCount() const { return site_count_; }
  const dbSite::RowPattern& getSites() const { return sites_; }
  DbuY getSitesTotalHeight() const;

  void setOffset(DbuY offset) { offset_ = offset; }

 private:
  const GridY row_count_;
  const GridX site_count_;
  const int grid_index_;
  DbuY offset_{0};
  // will have one site only for non-hybrid and hybrid parent cells.
  // For hybrid children, this will have all the sites
  const dbSite::RowPattern sites_;
};

struct GridMapKey
{
  int grid_index{0};
  bool operator<(const GridMapKey& other) const
  {
    return grid_index < other.grid_index;
  }
  bool operator==(const GridMapKey& other) const
  {
    return grid_index == other.grid_index;
  }
};

// The "Grid" is now an array of 2D grids. The new dimension is to support
// multi-height cells. Each unique row height creates a new grid that is used in
// legalization. The first index is the grid index (corresponding to row
// height), second index is the row index, and third index is the site index.
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
  void examineRows(dbBlock* block);
  std::unordered_set<int> getRowCoordinates() const;

  GridMapKey getGridMapKey(const dbSite* site) const;
  GridMapKey getGridMapKey(const Cell* cell) const;
  GridX gridX(DbuX x) const;
  GridX gridX(const Cell* cell) const;
  GridX gridEndX(DbuX x) const;
  GridX gridEndX(const Cell* cell) const;
  GridX gridPaddedX(const Cell* cell) const;
  GridX gridPaddedEndX(const Cell* cell) const;
  GridY gridY(DbuY y, const Cell* cell) const;
  GridY gridY(const Cell* cell) const;
  GridY gridEndY(DbuY y, const Cell* cell) const;
  GridY gridEndY(const Cell* cell) const;
  pair<GridY, DbuY> gridY(DbuY y, const GridInfo& grid_info) const;
  pair<GridY, DbuY> gridEndY(DbuY y, const GridInfo& grid_info) const;
  GridInfo getGridInfo(const Cell* cell) const;
  GridX gridPaddedWidth(const Cell* cell) const;
  GridY gridHeight(const Cell* cell) const;
  void setGridPaddedLoc(Cell* cell, GridX x, GridY y) const;
  DbuY coordinateToHeight(GridY y_coordinate, GridMapKey gmk) const;

  void paintPixel(Cell* cell, GridX grid_x, GridY grid_y);
  void erasePixel(Cell* cell);
  void visitCellPixels(Cell& cell,
                       bool padded,
                       const std::function<void(Pixel* pixel)>& visitor) const;
  void visitCellBoundaryPixels(
      Cell& cell,
      bool padded,
      const std::function<
          void(Pixel* pixel, odb::Direction2D edge, GridX x, GridY y)>& visitor)
      const;

  GridY getRowCount() const { return row_count_; }
  GridX getRowSiteCount() const { return row_site_count_; }
  DbuY getRowHeight() const { return row_height_; }
  DbuY getRowHeight(const Cell* cell) const;
  DbuX getSiteWidth() const { return site_width_; }
  GridY map_ycoordinates(GridY source_grid_coordinate,
                         const GridMapKey& source_grid_key,
                         const GridMapKey& target_grid_key,
                         bool start) const;

  Pixel* gridPixel(int grid_idx, GridX x, GridY y) const;
  Pixel& pixel(int g, GridY y, GridX x) { return pixels_[g][y.v][x.v]; }
  const Pixel& pixel(int g, GridY y, GridX x) const
  {
    return pixels_[g][y.v][x.v];
  }

  void resize(int size) { pixels_.resize(size); }
  void resize(int g, GridY size) { pixels_[g].resize(size.v); }
  void resize(int g, GridY y, GridX size) { pixels_[g][y.v].resize(size.v); }
  void clear() { pixels_.clear(); }

  GridInfo& infoMap(const GridMapKey& key) { return grid_info_map_.at(key); }
  const GridInfo& infoMap(const GridMapKey& key) const
  {
    return grid_info_map_.at(key);
  }
  bool infoMapEmpty() const { return grid_info_map_.empty(); }
  const map<GridMapKey, GridInfo>& getInfoMap() const { return grid_info_map_; }

  const std::unordered_map<dbSite*, dbSite*>& getHybridParent() const;

  GridMapKey getSmallestNonHybridGridKey() const;

  bool hasHybridRows() const { return has_hybrid_rows_; }

  GridY getRowCount(DbuY row_height) const;
  std::pair<DbuY, GridInfo> getRowInfo(const Cell* cell) const;

  Rect getCore() const { return core_; }
  bool cellFitsInCore(Cell* cell) const;

 private:
  GridY calculateHybridSitesRowCount(dbSite* parent_hybrid_site) const;
  void initGridLayersMap(dbDatabase* db, dbBlock* block);
  void addSiteToGrid(dbSite* site, const GridMapKey& key);
  const map<const dbSite*, GridMapKey>& getSiteToGrid() const;
  void setSmallestNonHybridGridKey(const GridMapKey& key);
  void addHybridParent(dbSite* child, dbSite* parent);
  void addInfoMap(const GridMapKey& key, const GridInfo& info);
  void visitDbRows(dbBlock* block,
                   const std::function<void(odb::dbRow*)>& func) const;

  Logger* logger_ = nullptr;
  dbBlock* block_ = nullptr;
  std::shared_ptr<Padding> padding_;
  std::vector<std::vector<std::vector<Pixel>>> pixels_;
  std::vector<const GridInfo*> grid_info_vector_;
  map<GridMapKey, GridInfo> grid_info_map_;
  std::unordered_map<dbSite*, dbSite*> hybrid_parent_;  // child -> parent

  // This map is used to map each unique site to a grid. The key is always
  // unique, but the value is not unique in the case of hybrid sites
  // (alternating rows)
  map<const dbSite*, GridMapKey> site_to_grid_key_;
  GridMapKey smallest_non_hybrid_grid_key_;
  bool has_hybrid_rows_ = false;
  Rect core_;

  DbuY row_height_{0};
  DbuX site_width_{0};

  GridY row_count_{0};
  GridX row_site_count_{0};
};

}  // namespace dpl
