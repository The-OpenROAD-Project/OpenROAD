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

#include "dpl/Opendp.h"

namespace dpl {

using odb::dbOrientType;
using odb::dbSite;

struct Pixel
{
  Cell* cell;
  Group* group_;
  double util;
  dbOrientType orient_;
  bool is_valid;     // false for dummy cells
  bool is_hopeless;  // too far from sites for diamond search
  dbSite* site;      // site that this pixel is
};

// Return value for grid searches.
class PixelPt
{
 public:
  PixelPt() = default;
  PixelPt(Pixel* pixel, int grid_x, int grid_y);
  Pixel* pixel = nullptr;
  Point pt;  // grid locataion
};

class GridInfo
{
 public:
  GridInfo(const int row_count,
           const int site_count,
           const int grid_index,
           const dbSite::RowPattern& sites)
      : row_count_(row_count),
        site_count_(site_count),
        grid_index_(grid_index),
        sites_(sites)
  {
  }

  int getRowCount() const { return row_count_; }
  int getSiteCount() const { return site_count_; }
  int getGridIndex() const { return grid_index_; }
  int getOffset() const { return offset_; }

  void setOffset(int offset) { offset_ = offset; }

  const dbSite::RowPattern& getSites() const { return sites_; }

  bool isHybrid() const
  {
    return sites_.size() > 1 || sites_[0].site->hasRowPattern();
  }
  int getSitesTotalHeight() const
  {
    return std::accumulate(sites_.begin(),
                           sites_.end(),
                           0,
                           [](int sum, const dbSite::OrientedSite& entry) {
                             return sum + entry.site->getHeight();
                           });
  }

 private:
  const int row_count_;
  const int site_count_;
  const int grid_index_;
  int offset_ = 0;
  // will have one site only for non-hybrid and hybrid parent cells.
  // For hybrid children, this will have all the sites
  const dbSite::RowPattern sites_;
};

struct GridMapKey
{
  int grid_index;
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
                Padding* padding,
                int max_displacement_x,
                int max_displacement_y);
  void examineRows(dbBlock* block);

  GridMapKey getGridMapKey(const dbSite* site) const;
  GridMapKey getGridMapKey(const Cell* cell) const;
  int gridX(int x) const;
  int gridX(const Cell* cell) const;
  int gridEndX(int x) const;
  int gridEndX(const Cell* cell) const;
  int gridPaddedX(const Cell* cell) const;
  int gridPaddedEndX(const Cell* cell) const;
  int gridY(int y, const Cell* cell) const;
  int gridY(const Cell* cell) const;
  int gridEndY(int y, const Cell* cell) const;
  int gridEndY(const Cell* cell) const;
  pair<int, int> gridY(int y, const dbSite::RowPattern& grid_sites) const;
  pair<int, int> gridEndY(int y, const dbSite::RowPattern& grid_sites) const;
  GridInfo getGridInfo(const Cell* cell) const;
  int gridPaddedWidth(const Cell* cell) const;
  int gridHeight(const Cell* cell) const;
  void setGridPaddedLoc(Cell* cell, int x, int y) const;
  int coordinateToHeight(int y_coordinate, GridMapKey gmk) const;

  void paintPixel(Cell* cell, int grid_x, int grid_y);
  void erasePixel(Cell* cell);
  void visitCellPixels(Cell& cell,
                       bool padded,
                       const std::function<void(Pixel* pixel)>& visitor) const;
  void visitCellBoundaryPixels(
      Cell& cell,
      bool padded,
      const std::function<
          void(Pixel* pixel, odb::Direction2D edge, int x, int y)>& visitor)
      const;

  int getRowCount() const { return row_count_; }
  int getRowSiteCount() const { return row_site_count_; }
  int getRowHeight() const { return row_height_; }
  int getRowHeight(const Cell* cell) const;
  int getSiteWidth() const { return site_width_; }
  void setRowHeight(int height) { row_height_ = height; }
  void setSiteWidth(int width) { site_width_ = width; }
  int map_ycoordinates(int source_grid_coordinate,
                       const GridMapKey& source_grid_key,
                       const GridMapKey& target_grid_key,
                       const bool start) const;

  Pixel* gridPixel(int grid_idx, int x, int y) const;
  Pixel& pixel(int g, int y, int x) { return pixels_[g][y][x]; }
  const Pixel& pixel(int g, int y, int x) const { return pixels_[g][y][x]; }

  bool empty() const { return pixels_.empty(); }
  void resize(int size) { pixels_.resize(size); }
  void resize(int g, int size) { pixels_[g].resize(size); }
  void resize(int g, int y, int size) { pixels_[g][y].resize(size); }
  void clear() { pixels_.clear(); }

  void clearInfo() { grid_info_vector_.clear(); }
  void resizeInfo(int size) { grid_info_vector_.resize(size); }
  void setInfo(int idx, const GridInfo* info) { grid_info_vector_[idx] = info; }

  GridInfo& infoMap(const GridMapKey& key) { return grid_info_map_.at(key); }
  const GridInfo& infoMap(const GridMapKey& key) const
  {
    return grid_info_map_.at(key);
  }
  bool infoMapEmpty() const { return grid_info_map_.empty(); }
  const map<GridMapKey, GridInfo>& getInfoMap() const { return grid_info_map_; }
  void addInfoMap(const GridMapKey& key, const GridInfo& info)
  {
    grid_info_map_.emplace(key, info);
  }
  void clearInfoMap() { grid_info_map_.clear(); }

  void clearHybridParent() { hybrid_parent_.clear(); }
  const std::unordered_map<dbSite*, dbSite*>& getHybridParent() const
  {
    return hybrid_parent_;
  }

  void addHybridParent(dbSite* child, dbSite* parent)
  {
    hybrid_parent_[child] = parent;
  }

  const map<const dbSite*, GridMapKey>& getSiteToGrid() const
  {
    return site_to_grid_key_;
  }

  void addSiteToGrid(dbSite* site, const GridMapKey& key)
  {
    site_to_grid_key_[site] = key;
  }

  GridMapKey getSmallestNonHybridGridKey() const
  {
    return smallest_non_hybrid_grid_key_;
  }

  void setSmallestNonHybridGridKey(const GridMapKey& key)
  {
    smallest_non_hybrid_grid_key_ = key;
  }

  bool getHasHybridRows() const { return has_hybrid_rows_; }
  void setHasHybridRows(bool has) { has_hybrid_rows_ = has; }

  int getRowCount(int row_height) const;

  Rect getCore() const { return core_; }
  bool cellFitsInCore(Cell* cell) const;

 private:
  int calculateHybridSitesRowCount(dbSite* parent_hybrid_site) const;
  void initGridLayersMap(dbDatabase* db, dbBlock* block);

  Logger* logger_ = nullptr;
  Padding* padding_ = nullptr;
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

  int row_height_ = 0;  // dbu
  int site_width_ = 0;  // dbu

  int row_count_ = 0;
  int row_site_count_ = 0;
};

}  // namespace dpl
