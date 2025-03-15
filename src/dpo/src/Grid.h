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

#include <optional>
#include <unordered_set>
#include <vector>

#include "network.h"
#include "odb/db.h"
namespace dpo {

struct Pixel
{
  Node* node = nullptr;
  double util = 0.0;
  bool is_valid = false;
  bool is_hopeless = false;
};

// Return value for grid searches.
class PixelPt
{
 public:
  PixelPt() = default;
  PixelPt(Pixel* pixel, int grid_x, int grid_y);
  Pixel* pixel = nullptr;
  int x{0};
  int y{0};
};

// The "Grid" is a 2D array of pixels.  The pixels represent the
// single height sites onto which multi-height sites are overlaid.
// The sites are assumed to be of a single width but the rows are of
// variable height in order to support hybrid rows.
class Grid
{
 public:
  void allocateGrid();
  // void initGrid(odb::dbDatabase* db, odb::dbBlock* block);
  void initBlock(odb::dbBlock* block) { core_ = block->getCoreArea(); }
  void init(utl::Logger* logger) { logger_ = logger; }
  void examineRows(odb::dbBlock* block);
  std::unordered_set<int> getRowCoordinates() const;

  int gridX(int x) const;
  int gridEndX(int x) const;

  int gridX(const Node* cell) const;
  int gridEndX(const Node* cell) const;
  int gridPaddedX(const Node* cell) const;
  int gridPaddedEndX(const Node* cell) const;

  int gridSnapDownY(int y) const;
  int gridRoundY(int y) const;
  int gridEndY(int y) const;

  // // Snap outwards to fully contain
  // GridRect gridCovering(const Rect& rect) const;
  // GridRect gridCovering(const Cell* cell) const;
  // GridRect gridCoveringPadded(const Cell* cell) const;

  // // Snap inwards to be fully contained
  // GridRect gridWithin(const DbuRect& rect) const;

  // GridY gridSnapDownY(const Cell* cell) const;
  // GridY gridRoundY(const Cell* cell) const;
  // GridY gridEndY(const Cell* cell) const;

  int gridYToDbu(int y) const;
  int gridXToDbu(int y) const;

  int gridWidth(const Node* cell) const;
  int gridHeight(const Node* cell) const;
  // GridY gridHeight(odb::dbMaster* master) const;
  // DbuY rowHeight(GridY index);
  void setGridPaddedLoc(Node* cell, int x, int y) const;

  void paintPixel(Node* cell, int grid_x, int grid_y);
  void erasePixel(const Node* cell, int grid_x, int grid_y);
  void paintPixel(Node* cell);
  void erasePixel(const Node* cell);
  // void visitCellPixels(Cell& cell,
  //                      bool padded,
  //                      const std::function<void(Pixel* pixel)>& visitor)
  //                      const;
  // void visitCellBoundaryPixels(
  //     Cell& cell,
  //     const std::function<
  //         void(Pixel* pixel, odb::Direction2D edge, GridX x, GridY y)>&
  //         visitor)
  //     const;

  int getRowCount() const { return row_count_; }
  int getRowSiteCount() const { return row_site_count_; }
  int getSiteWidth() const { return site_width_; }

  Pixel* gridPixel(int x, int y) const;
  // Pixel& pixel(GridY y, GridX x) { return pixels_[y.v][x.v]; }
  // const Pixel& pixel(GridY y, GridX x) const { return pixels_[y.v][x.v]; }

  void resize(int size) { pixels_.resize(size); }
  void resize(int y, int size) { pixels_[y].resize(size); }
  void clear();

  bool hasHybridRows() const { return has_hybrid_rows_; }

  // GridY getRowCount(DbuY row_height) const;

  odb::Rect getCore() const { return core_; }
  // bool cellFitsInCore(Cell* cell) const;

  // bool isMultiHeight(dbMaster* master) const;

  void report();

 private:
  // void markHopeless(dbBlock* block,
  //                   int max_displacement_x,
  //                   int max_displacement_y);
  // void markBlocked(dbBlock* block);
  void visitDbRows(odb::dbBlock* block,
                   const std::function<void(odb::dbRow*)>& func) const;

  using Pixels = std::vector<std::vector<Pixel>>;
  utl::Logger* logger_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  // std::shared_ptr<Padding> padding_;

  std::vector<std::vector<Pixel>> pixels_;
  // Contains all the rows' yLo plus the yHi of the last row.  The extra
  // value is useful for operations like region snapping to rows
  std::map<int, int> row_y_dbu_to_index_;
  std::vector<int> row_index_to_y_dbu_;         // index is GridY
  std::vector<int> row_index_to_pixel_height_;  // index is GridY

  bool has_hybrid_rows_ = false;
  odb::Rect core_;

  std::optional<int> uniform_row_height_;  // unset if hybrid
  int site_width_{0};

  int row_count_{0};
  int row_site_count_{0};
};

}  // namespace dpo
