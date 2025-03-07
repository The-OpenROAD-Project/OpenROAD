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
namespace dpo {
int divFloor(const int dividend, const int divisor)
{
  return dividend / divisor;
}
int divCeil(const int dividend, const int divisor)
{
  return ceil(static_cast<double>(dividend) / divisor);
}

inline int gridToDbu(int x, int scale)
{
  return x * scale;
}
void Grid::allocateGrid()
{
  // Make pixel grid
  if (pixels_.empty()) {
    resize(row_count_);
    for (int y{0}; y < row_count_; y++) {
      resize(y, row_site_count_);
    }
  }

  for (int y{0}; y < row_count_; y++) {
    for (int x{0}; x < row_site_count_; x++) {
      Pixel& pixel = pixels_[y][x];
      pixel.node = nullptr;
      pixel.util = 0.0;
      pixel.is_valid = false;
      pixel.is_hopeless = false;
    }
  }
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
    const int dbu_y{y_lo};
    row_y_dbu_to_index_[dbu_y] = 0;
    row_y_dbu_to_index_[dbu_y + site->getHeight()] = 0;

    // Check all sites have equal width
    if (!first_site) {
      first_site = site;
      site_width_ = site->getWidth();
    } else if (site->getWidth() != site_width_) {
      logger_->error(utl::DPO,
                     51,
                     "Site widths are not equal: {}={} != {}={}",
                     first_site->getName(),
                     site_width_,
                     site->getName(),
                     site->getWidth());
    }
  });

  if (!hasHybridRows() && !has_non_hybrid_rows) {
    logger_->error(utl::DPO, 12, "no rows found.");
  }

  if (hasHybridRows() && has_non_hybrid_rows) {
    logger_->error(
        utl::DPO, 49, "Mixing hybrid and non-hybrid rows is unsupported.");
  }

  int index{0};
  int prev_y{0};
  for (auto& [dbu_y, grid_y] : row_y_dbu_to_index_) {
    grid_y = index++;
    row_index_to_y_dbu_.push_back(dbu_y);
    row_index_to_pixel_height_.push_back(dbu_y - prev_y);
    prev_y = dbu_y;
  }

  if (!hasHybridRows()) {
    uniform_row_height_ = int{std::numeric_limits<int>::max()};
    visitDbRows(block, [&](odb::dbRow* db_row) {
      const int site_height = db_row->getSite()->getHeight();
      uniform_row_height_ = std::min(uniform_row_height_.value(), site_height);
    });
  }
  row_site_count_ = divFloor(getCore().dx(), getSiteWidth());
  row_count_ = static_cast<int>(row_y_dbu_to_index_.size() - 1);
}

std::unordered_set<int> Grid::getRowCoordinates() const
{
  std::unordered_set<int> coords;
  visitDbRows(block_, [&](odb::dbRow* row) {
    coords.insert(row->getOrigin().y() - core_.yMin());
  });
  return coords;
}
void Grid::visitDbRows(odb::dbBlock* block,
                       const std::function<void(odb::dbRow*)>& func) const
{
  for (auto row : block->getRows()) {
    // dpl doesn't deal with pads
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      func(row);
    }
  }
}

int Grid::gridYToDbu(int y) const
{
  if (y == row_index_to_y_dbu_.size()) {
    return core_.yMax();
  }
  return row_index_to_y_dbu_.at(y);
}
int Grid::gridXToDbu(int x) const
{
  return x * getSiteWidth();
}
void Grid::setGridPaddedLoc(Node* cell, int x, int y) const
{
  cell->setLeft(gridToDbu(x, getSiteWidth()));
  cell->setBottom(gridYToDbu(y));
}
int Grid::gridHeight(const Node* cell) const
{
  if (uniform_row_height_) {
    int row_height = uniform_row_height_.value();
    return std::max(1, divCeil(cell->getHeight(), row_height));
  }
  return 1;
}
Pixel* Grid::gridPixel(int grid_x, int grid_y) const
{
  if (grid_x >= 0 && grid_x < row_site_count_ && grid_y >= 0
      && grid_y < row_count_) {
    return const_cast<Pixel*>(&pixels_[grid_y][grid_x]);
  }
  return nullptr;
}

int Grid::gridWidth(const Node* cell) const
{
  return divCeil(cell->getWidth(), getSiteWidth());
}

int Grid::gridEndX(int x) const
{
  return divCeil(x, getSiteWidth());
}

int Grid::gridX(int x) const
{
  return x / getSiteWidth();
}

int Grid::gridX(const Node* cell) const
{
  return gridX(cell->getLeft());
}
int Grid::gridSnapDownY(int y) const
{
  auto it = std::upper_bound(
      row_index_to_y_dbu_.begin(), row_index_to_y_dbu_.end(), y);
  if (it == row_index_to_y_dbu_.begin()) {
    return 0;
  }
  --it;
  return static_cast<int>(it - row_index_to_y_dbu_.begin());
}

int Grid::gridRoundY(int y) const
{
  const auto grid_y = gridSnapDownY(y);
  if (grid_y < row_index_to_y_dbu_.size() - 1) {
    const auto grid_next = grid_y + 1;
    if (std::abs(row_index_to_y_dbu_[grid_y] - y)
        >= std::abs(row_index_to_y_dbu_[grid_next] - y)) {
      return grid_next;
    }
  }
  return grid_y;
}

int Grid::gridEndY(int y) const
{
  auto it = std::lower_bound(
      row_index_to_y_dbu_.begin(), row_index_to_y_dbu_.end(), y);
  if (it == row_index_to_y_dbu_.end()) {
    return static_cast<int>(row_index_to_y_dbu_.size());
  }
  return static_cast<int>(it - row_index_to_y_dbu_.begin());
}

void Grid::paintPixel(Node* cell, int grid_x, int grid_y)
{
  int x_end = grid_x + gridWidth(cell);
  int y_end = grid_y + gridHeight(cell);
  for (int x{grid_x}; x < x_end; x++) {
    for (int y{grid_y}; y < y_end; y++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel == nullptr) {
        continue;
      }
      pixel->node = cell;
    }
  }
}
void Grid::paintPixel(Node* cell)
{
  auto grid_x = gridX(cell);
  auto grid_y = gridRoundY(cell->getBottom());
  paintPixel(cell, grid_x, grid_y);
}
void Grid::erasePixel(const Node* cell, int grid_x, int grid_y)
{
  int x_end = grid_x + gridWidth(cell);
  int y_end = grid_y + gridHeight(cell);
  for (int x{grid_x}; x < x_end; x++) {
    for (int y{grid_y}; y < y_end; y++) {
      Pixel* pixel = gridPixel(x, y);
      if (pixel == nullptr) {
        continue;
      }
      if (pixel->node == cell) {
        pixel->node = nullptr;
      }
    }
  }
}
void Grid::erasePixel(const Node* cell)
{
  auto grid_x = gridX(cell);
  auto grid_y = gridRoundY(cell->getBottom());
  erasePixel(cell, grid_x, grid_y);
}
void Grid::report()
{
  for (int y = 0; y < row_count_; y++) {
    for (int x = 0; x < row_site_count_; x++) {
      auto pixel = gridPixel(x, y);
      if (pixel == nullptr || pixel->node == nullptr) {
        continue;
      }
      logger_->report("Found node at ({} {}) which is ({} {})",
                      x,
                      y,
                      gridToDbu(x, getSiteWidth()),
                      gridYToDbu(y));
    }
  }
}
}  // namespace dpo