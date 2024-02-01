///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
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

#include "util.h"

#include <map>
#include <numeric>
#include <string>

#include "db.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"

namespace odb {

using std::string;
using std::vector;

RUDYCalculator::RUDYCalculator(dbBlock* block) : block_(block)
{
  gridBlock_ = block_->getDieArea();
  if (gridBlock_.area() == 0) {
    return;
  }
  // TODO: Match the wire width with the paper definition
  wireWidth_ = block_->getTech()->findRoutingLayer(1)->getWidth();

  odb::dbTechLayer* tech_layer = block_->getTech()->findRoutingLayer(3);
  odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
  if (track_grid == nullptr) {
    return;
  }
  int track_spacing, track_init, num_tracks;
  track_grid->getAverageTrackSpacing(track_spacing, track_init, num_tracks);
  int upper_rightX = gridBlock_.xMax();
  int upper_rightY = gridBlock_.yMax();
  int tile_size = pitches_in_tile_ * track_spacing;
  int x_grids = upper_rightX / tile_size;
  int y_grids = upper_rightY / tile_size;
  setGridConfig(gridBlock_, x_grids, y_grids);
}

void RUDYCalculator::setGridConfig(odb::Rect block, int tileCntX, int tileCntY)
{
  gridBlock_ = block;
  tileCntX_ = tileCntX;
  tileCntY_ = tileCntY;
  makeGrid();
}

void RUDYCalculator::makeGrid()
{
  const int block_width = gridBlock_.dx();
  const int block_height = gridBlock_.dy();
  const int gridLx = gridBlock_.xMin();
  const int gridLy = gridBlock_.yMin();
  const int tile_width = block_width / tileCntX_;
  const int tile_height = block_height / tileCntY_;

  grid_.resize(tileCntX_);
  int curX = gridLx;
  for (auto& gridColumn : grid_) {
    gridColumn.resize(tileCntY_);
    int curY = gridLy;
    for (auto& grid : gridColumn) {
      grid.setRect(curX, curY, curX + tile_width, curY + tile_height);
      curY += tile_height;
    }
    curX += tile_width;
  }
}

void RUDYCalculator::calculateRUDY()
{
  // refer: https://ieeexplore.ieee.org/document/4211973
  const int tile_width = gridBlock_.dx() / tileCntX_;
  const int tile_height = gridBlock_.dy() / tileCntY_;

  for (auto net : block_->getNets()) {
    if (!net->getSigType().isSupply()) {
      const auto net_rect = net->getTermBBox();
      processIntersectionSignalNet(net_rect, tile_width, tile_height);
    } else {
      for (odb::dbSWire* swire : net->getSWires()) {
        for (odb::dbSBox* s : swire->getWires()) {
          if (s->isVia()) {
            continue;
          }
          odb::Rect wire_rect = s->getBox();
          processIntersectionGenericObstruction(
              wire_rect, tile_width, tile_height, 1);
        }
      }
    }
  }

  for (odb::dbInst* instance : block_->getInsts()) {
    odb::dbMaster* master = instance->getMaster();
    if (master->isBlock()) {
      processMacroObstruction(master, instance);
    }
  }
}

void RUDYCalculator::processMacroObstruction(odb::dbMaster* macro,
                                             odb::dbInst* instance)
{
  const int tile_width = gridBlock_.dx() / tileCntX_;
  const int tile_height = gridBlock_.dy() / tileCntY_;
  for (odb::dbBox* obstr_box : macro->getObstructions()) {
    const odb::Point origin = instance->getOrigin();
    odb::dbTransform transform(instance->getOrient(), origin);
    odb::Rect macro_obstruction = obstr_box->getBox();
    transform.apply(macro_obstruction);
    const auto obstr_area = macro_obstruction.area();
    if (obstr_area == 0) {
      continue;
    }
    processIntersectionGenericObstruction(
        macro_obstruction, tile_width, tile_height, 2);
  }
}

void RUDYCalculator::processIntersectionGenericObstruction(
    odb::Rect obstruction_rect,
    const int tile_width,
    const int tile_height,
    const int nets_per_tile)
{
  // Calculate the intersection range
  const int minXIndex
      = std::max(0, (obstruction_rect.xMin() - gridBlock_.xMin()) / tile_width);
  const int maxXIndex
      = std::min(tileCntX_ - 1,
                 (obstruction_rect.xMax() - gridBlock_.xMin()) / tile_width);
  const int minYIndex = std::max(
      0, (obstruction_rect.yMin() - gridBlock_.yMin()) / tile_height);
  const int maxYIndex
      = std::min(tileCntY_ - 1,
                 (obstruction_rect.yMax() - gridBlock_.yMin()) / tile_height);

  // Iterate over the tiles in the calculated range
  for (int x = minXIndex; x <= maxXIndex; ++x) {
    for (int y = minYIndex; y <= maxYIndex; ++y) {
      Tile& tile = getEditableTile(x, y);
      const auto tileBox = tile.getRect();
      if (obstruction_rect.overlaps(tileBox)) {
        const auto hpwl = static_cast<float>(tileBox.dx() + tileBox.dy());
        const auto wireArea = hpwl * wireWidth_;
        const auto tileArea = tileBox.area();
        const auto obstr_congestion = wireArea / tileArea;
        const auto intersectArea = obstruction_rect.intersect(tileBox).area();

        const auto tileObstrBoxRatio
            = static_cast<float>(intersectArea) / static_cast<float>(tileArea);
        const auto rudy
            = obstr_congestion * tileObstrBoxRatio * 100 * nets_per_tile;
        tile.addRUDY(rudy);
      }
    }
  }
}

void RUDYCalculator::processIntersectionSignalNet(const odb::Rect net_rect,
                                                  const int tile_width,
                                                  const int tile_height)
{
  const auto netArea = net_rect.area();
  if (netArea == 0) {
    // TODO: handle nets with 0 area from getTermBBox()
    return;
  }
  const auto hpwl = static_cast<float>(net_rect.dx() + net_rect.dy());
  const auto wireArea = hpwl * wireWidth_;
  const auto netCongestion = wireArea / netArea;

  // Calculate the intersection range
  const int minXIndex
      = std::max(0, (net_rect.xMin() - gridBlock_.xMin()) / tile_width);
  const int maxXIndex = std::min(
      tileCntX_ - 1, (net_rect.xMax() - gridBlock_.xMin()) / tile_width);
  const int minYIndex
      = std::max(0, (net_rect.yMin() - gridBlock_.yMin()) / tile_height);
  const int maxYIndex = std::min(
      tileCntY_ - 1, (net_rect.yMax() - gridBlock_.yMin()) / tile_height);

  // Iterate over the tiles in the calculated range
  for (int x = minXIndex; x <= maxXIndex; ++x) {
    for (int y = minYIndex; y <= maxYIndex; ++y) {
      Tile& tile = getEditableTile(x, y);
      const auto tileBox = tile.getRect();
      if (net_rect.overlaps(tileBox)) {
        const auto intersectArea = net_rect.intersect(tileBox).area();
        const auto tileArea = tileBox.area();
        const auto tileNetBoxRatio
            = static_cast<float>(intersectArea) / static_cast<float>(tileArea);
        const auto rudy = netCongestion * tileNetBoxRatio * 100;
        tile.addRUDY(rudy);
      }
    }
  }
}

std::pair<int, int> RUDYCalculator::getGridSize() const
{
  if (grid_.empty()) {
    return {0, 0};
  }
  return {grid_.size(), grid_.at(0).size()};
}

void RUDYCalculator::Tile::setRect(int lx, int ly, int ux, int uy)
{
  rect_ = odb::Rect(lx, ly, ux, uy);
}

void RUDYCalculator::Tile::addRUDY(float rudy)
{
  rudy_ += rudy;
}

static void buildRow(dbBlock* block,
                     const string& name,
                     dbSite* site,
                     int start_x,
                     int end_x,
                     int y,
                     dbOrientType& orient,
                     dbRowDir& direction,
                     int min_row_width)
{
  const int site_width = site->getWidth();
  const int new_row_num_sites = (end_x - start_x) / site_width;
  const int new_row_width = new_row_num_sites * site_width;

  if (new_row_num_sites > 0 && new_row_width >= min_row_width) {
    dbRow::create(block,
                  name.c_str(),
                  site,
                  start_x,
                  y,
                  orient,
                  direction,
                  new_row_num_sites,
                  site_width);
  }
}

static void cutRow(dbBlock* block,
                   dbRow* row,
                   vector<dbBox*>& row_blockages,
                   int min_row_width,
                   int halo_x,
                   int halo_y)
{
  string row_name = row->getName();
  Rect row_bb = row->getBBox();

  dbSite* row_site = row->getSite();
  const int site_width = row_site->getWidth();
  dbOrientType orient = row->getOrient();
  dbRowDir direction = row->getDirection();

  const int curr_min_row_width = min_row_width + 2 * site_width;

  vector<dbBox*> row_blockage_bboxs = row_blockages;
  vector<std::pair<int, int>> row_blockage_xs;
  row_blockage_xs.reserve(row_blockages.size());
  for (dbBox* row_blockage_bbox : row_blockages) {
    row_blockage_xs.emplace_back(row_blockage_bbox->xMin(),
                                 row_blockage_bbox->xMax());
  }

  std::sort(row_blockage_xs.begin(), row_blockage_xs.end());

  int start_origin_x = row_bb.xMin();
  int start_origin_y = row_bb.yMin();
  int row_sub_idx = 1;
  for (std::pair<int, int> blockage : row_blockage_xs) {
    const int blockage_x0 = blockage.first;
    const int new_row_end_x
        = makeSiteLoc(blockage_x0 - halo_x, site_width, true, start_origin_x);
    buildRow(block,
             row_name + "_" + std::to_string(row_sub_idx),
             row_site,
             start_origin_x,
             new_row_end_x,
             start_origin_y,
             orient,
             direction,
             curr_min_row_width);
    row_sub_idx++;
    const int blockage_x1 = blockage.second;
    start_origin_x
        = makeSiteLoc(blockage_x1 + halo_x, site_width, false, start_origin_x);
  }
  // Make last row
  buildRow(block,
           row_name + "_" + std::to_string(row_sub_idx),
           row_site,
           start_origin_x,
           row_bb.xMax(),
           start_origin_y,
           orient,
           direction,
           curr_min_row_width);
  // Remove current row
  dbRow::destroy(row);
}

static bool overlaps(dbBox* blockage, dbRow* row, int halo_x, int halo_y)
{
  Rect rowBB = row->getBBox();

  // Check if Y has overlap first since rows are long and skinny
  const int blockage_lly = blockage->yMin() - halo_y;
  const int blockage_ury = blockage->yMax() + halo_y;
  const int row_lly = rowBB.yMin();
  const int row_ury = rowBB.yMax();

  if (blockage_lly >= row_ury || row_lly >= blockage_ury) {
    return false;
  }

  const int blockage_llx = blockage->xMin() - halo_x;
  const int blockage_urx = blockage->xMax() + halo_x;
  const int row_llx = rowBB.xMin();
  const int row_urx = rowBB.xMax();

  if (blockage_llx >= row_urx || row_llx >= blockage_urx) {
    return false;
  }

  return true;
}

int makeSiteLoc(int x, double site_width, bool at_left_from_macro, int offset)
{
  double site_x = (x - offset) / site_width;
  int site_x1 = at_left_from_macro ? floor(site_x) : ceil(site_x);
  return site_x1 * site_width + offset;
}

template <typename T>
bool hasOverflow(T a, T b)
{
  if ((b > 0 && a > std::numeric_limits<T>::max() - b)
      || (b < 0 && a < std::numeric_limits<T>::lowest() - b)) {
    return true;
  }
  return false;
}

void cutRows(dbBlock* block,
             const int min_row_width,
             const vector<dbBox*>& blockages,
             int halo_x,
             int halo_y,
             utl::Logger* logger)
{
  if (blockages.empty()) {
    return;
  }
  auto rows = block->getRows();
  const int initial_rows_count = rows.size();
  const std::int64_t initial_sites_count
      = std::accumulate(rows.begin(),
                        rows.end(),
                        (std::int64_t) 0,
                        [&](std::int64_t sum, dbRow* row) {
                          return sum + (std::int64_t) row->getSiteCount();
                        });

  std::map<dbRow*, int> placed_row_insts;
  for (dbInst* inst : block->getInsts()) {
    if (!inst->isFixed()) {
      continue;
    }
    if (inst->getMaster()->isCoreAutoPlaceable()
        && !inst->getMaster()->isBlock()) {
      const Rect inst_bbox = inst->getBBox()->getBox();
      for (dbRow* row : block->getRows()) {
        const Rect row_bbox = row->getBBox();
        if (row_bbox.contains(inst_bbox)) {
          placed_row_insts[row]++;
        }
      }
    }
  }

  // Gather rows needing to be cut up front
  for (dbRow* row : block->getRows()) {
    std::vector<dbBox*> row_blockages;
    for (dbBox* blockage : blockages) {
      if (overlaps(blockage, row, halo_x, halo_y)) {
        row_blockages.push_back(blockage);
      }
    }
    // Cut row around macros
    if (!row_blockages.empty()) {
      if (placed_row_insts.find(row) != placed_row_insts.end()) {
        logger->warn(utl::ODB,
                     386,
                     "{} contains {} placed instances and will not be cut.",
                     row->getName(),
                     placed_row_insts[row]);
      } else {
        cutRow(block, row, row_blockages, min_row_width, halo_x, halo_y);
      }
    }
  }

  const std::int64_t final_sites_count
      = std::accumulate(rows.begin(),
                        rows.end(),
                        (std::int64_t) 0,
                        [&](std::int64_t sum, dbRow* row) {
                          return sum + (std::int64_t) row->getSiteCount();
                        });

  logger->info(utl::ODB,
               303,
               "The initial {} rows ({} sites) were cut with {} shapes for a "
               "total of {} rows ({} sites).",
               initial_rows_count,
               initial_sites_count,
               blockages.size(),
               block->getRows().size(),
               final_sites_count);
}

std::string generateMacroPlacementString(dbBlock* block)
{
  std::string macro_placement;

  const float dbu = block->getTech()->getDbUnitsPerMicron();
  float x = 0.0f;
  float y = 0.0f;

  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->isBlock()) {
      x = (inst->getLocation().x()) / dbu;
      y = (inst->getLocation().y()) / dbu;

      macro_placement += fmt::format(
          "place_macro -macro_name {} -location {{{} {}}} -orientation {}\n",
          inst->getName(),
          x,
          y,
          inst->getOrient().getString());
    }
  }

  return macro_placement;
}

}  // namespace odb
