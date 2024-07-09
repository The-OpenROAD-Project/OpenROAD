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

#include "dpl/Opendp.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>

#include "DplObserver.h"
#include "Grid.h"
#include "Objects.h"
#include "Padding.h"
#include "dpl/OptMirror.h"
#include "odb/util.h"
#include "utl/Logger.h"

namespace dpl {

using std::round;
using std::string;

using utl::DPL;

using odb::dbMasterType;
using odb::Rect;

using utl::format_as;

////////////////////////////////////////////////////////////////

bool Opendp::isMultiRow(const Cell* cell) const
{
  return db_master_map_.at(cell->db_inst_->getMaster()).is_multi_row;
}

////////////////////////////////////////////////////////////////

Opendp::Opendp()
{
  Cell::dummy_cell.is_placed_ = true;
}

Opendp::~Opendp() = default;

void Opendp::init(dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
  padding_ = std::make_shared<Padding>();
  grid_ = std::make_unique<Grid>();
  grid_->init(logger);
}

void Opendp::setPaddingGlobal(const int left, const int right)
{
  padding_->setPaddingGlobal(GridX{left}, GridX{right});
}

void Opendp::setPadding(dbInst* inst, const int left, const int right)
{
  padding_->setPadding(inst, GridX{left}, GridX{right});
}

void Opendp::setPadding(dbMaster* master, const int left, const int right)
{
  padding_->setPadding(master, GridX{left}, GridX{right});
}

void Opendp::setDebug(std::unique_ptr<DplObserver>& observer)
{
  debug_observer_ = std::move(observer);
}

void Opendp::detailedPlacement(const int max_displacement_x,
                               const int max_displacement_y,
                               const std::string& report_file_name,
                               const bool disallow_one_site_gaps)
{
  importDb();

  if (have_fillers_) {
    logger_->warn(DPL, 37, "Use remove_fillers before detailed placement.");
  }

  if (max_displacement_x == 0 || max_displacement_y == 0) {
    // defaults
    max_displacement_x_ = 500;
    max_displacement_y_ = 100;
  } else {
    max_displacement_x_ = max_displacement_x;
    max_displacement_y_ = max_displacement_y;
  }
  disallow_one_site_gaps_ = disallow_one_site_gaps;
  if (!have_one_site_cells_) {
    // If 1-site fill cell is not detected && no disallow_one_site_gaps flag:
    // warn the user then continue as normal
    if (!disallow_one_site_gaps_) {
      logger_->warn(DPL,
                    38,
                    "No 1-site fill cells detected.  To remove 1-site gaps use "
                    "the -disallow_one_site_gaps flag.");
    }
  }
  odb::WireLengthEvaluator eval(block_);
  hpwl_before_ = eval.hpwl();
  detailedPlacement();
  // Save displacement stats before updating instance DB locations.
  findDisplacementStats();
  updateDbInstLocations();
  if (!placement_failures_.empty()) {
    logger_->info(DPL,
                  34,
                  "Detailed placement failed on the following {} instances:",
                  placement_failures_.size());
    for (auto cell : placement_failures_) {
      logger_->info(DPL, 35, " {}", cell->name());
    }

    if (!report_file_name.empty()) {
      writeJsonReport(
          report_file_name, {}, {}, {}, {}, {}, {}, placement_failures_);
    }
    logger_->error(DPL, 36, "Detailed placement failed.");
  }
}

void Opendp::updateDbInstLocations()
{
  for (Cell& cell : cells_) {
    if (!cell.isFixed() && cell.isStdCell()) {
      dbInst* db_inst_ = cell.db_inst_;
      // Only move the instance if necessary to avoid triggering callbacks.
      if (db_inst_->getOrient() != cell.orient_) {
        db_inst_->setOrient(cell.orient_);
      }
      const DbuX x = grid_->getCore().xMin() + cell.x_;
      const DbuY y = grid_->getCore().yMin() + cell.y_;
      int inst_x, inst_y;
      db_inst_->getLocation(inst_x, inst_y);
      if (x != inst_x || y != inst_y) {
        db_inst_->setLocation(x.v, y.v);
      }
    }
  }
}

void Opendp::reportLegalizationStats() const
{
  logger_->report("Placement Analysis");
  logger_->report("---------------------------------");
  logger_->report("total displacement   {:10.1f} u",
                  block_->dbuToMicrons(displacement_sum_));
  logger_->metric("design__instance__displacement__total",
                  block_->dbuToMicrons(displacement_sum_));
  logger_->report("average displacement {:10.1f} u",
                  block_->dbuToMicrons(displacement_avg_));
  logger_->metric("design__instance__displacement__mean",
                  block_->dbuToMicrons(displacement_avg_));
  logger_->report("max displacement     {:10.1f} u",
                  block_->dbuToMicrons(displacement_max_));
  logger_->metric("design__instance__displacement__max",
                  block_->dbuToMicrons(displacement_max_));
  logger_->report("original HPWL        {:10.1f} u",
                  block_->dbuToMicrons(hpwl_before_));
  odb::WireLengthEvaluator eval(block_);
  const double hpwl_legal = eval.hpwl();
  logger_->report("legalized HPWL       {:10.1f} u",
                  block_->dbuToMicrons(hpwl_legal));
  logger_->metric("route__wirelength__estimated",
                  block_->dbuToMicrons(hpwl_legal));
  const int hpwl_delta
      = (hpwl_before_ == 0.0)
            ? 0.0
            : round((hpwl_legal - hpwl_before_) / hpwl_before_ * 100);
  logger_->report("delta HPWL           {:10} %", hpwl_delta);
  logger_->report("");
}

////////////////////////////////////////////////////////////////

void Opendp::findDisplacementStats()
{
  displacement_avg_ = 0;
  displacement_sum_ = 0;
  displacement_max_ = 0;

  for (const Cell& cell : cells_) {
    const int displacement = disp(&cell);
    displacement_sum_ += displacement;
    if (displacement > displacement_max_) {
      displacement_max_ = displacement;
    }
  }
  if (!cells_.empty()) {
    displacement_avg_ = displacement_sum_ / cells_.size();
  } else {
    displacement_avg_ = 0.0;
  }
}

////////////////////////////////////////////////////////////////

void Opendp::optimizeMirroring()
{
  OptimizeMirroring opt(logger_, db_);
  opt.run();
}

int Opendp::disp(const Cell* cell) const
{
  const DbuPt init = initialLocation(cell, false);
  return sumXY(abs(init.x - cell->x_), abs(init.y - cell->y_));
}

int Opendp::padGlobalLeft() const
{
  return padding_->padGlobalLeft().v;
}

int Opendp::padGlobalRight() const
{
  return padding_->padGlobalRight().v;
}

int Opendp::padLeft(dbInst* inst) const
{
  return padding_->padLeft(inst).v;
}

int Opendp::padRight(dbInst* inst) const
{
  return padding_->padRight(inst).v;
}

void Opendp::initGrid()
{
  grid_->initGrid(
      db_, block_, padding_, max_displacement_x_, max_displacement_y_);
}

void Opendp::deleteGrid()
{
  grid_->clear();
}

void Opendp::findOverlapInRtree(const bgBox& queryBox,
                                vector<bgBox>& overlaps) const
{
  overlaps.clear();
  regions_rtree_.query(boost::geometry::index::intersects(queryBox),
                       std::back_inserter(overlaps));
}

void Opendp::setFixedGridCells()
{
  for (Cell& cell : cells_) {
    if (cell.isFixed()) {
      grid_->visitCellPixels(
          cell, true, [&](Pixel* pixel) { setGridCell(cell, pixel); });
    }
  }
}

void Opendp::setGridCell(Cell& cell, Pixel* pixel)
{
  pixel->cell = &cell;
  pixel->util = 1.0;
  if ((&cell)->isBlock()) {
    // Try the is_hopeless strategy to get off of a block
    pixel->is_hopeless = true;
  }
}

void Opendp::groupAssignCellRegions()
{
  for (Group& group : groups_) {
    int64_t total_site_area = 0;
    const DbuX site_width = grid_->getSiteWidth();
    if (!group.cells_.empty()) {
      auto group_cell = group.cells_.at(0);
      const DbuRect core = grid_->getCore();
      const GridX max_row_site_count = dbuToGridFloor(core.dx(), site_width);
      const DbuY row_height = grid_->getRowHeight(group_cell);
      const GridY row_count = dbuToGridFloor(core.dy(), row_height);
      const int64_t site_area
          = row_height.v * static_cast<int64_t>(site_width.v);
      const auto gmk = grid_->getGridMapKey(group_cell);
      const auto& grid_info = grid_->getInfoMap().at(gmk);

      for (GridX x{0}; x < max_row_site_count; x++) {
        for (GridY y{0}; y < row_count; y++) {
          const Pixel* pixel = grid_->gridPixel(grid_info.getGridIndex(), x, y);
          if (pixel->is_valid && pixel->group == &group) {
            total_site_area += site_area;
          }
        }
      }
    }

    int64_t cell_area = 0;
    for (Cell* cell : group.cells_) {
      cell_area += cell->area();

      for (Rect& rect : group.region_boundaries) {
        if (isInside(cell, rect)) {
          cell->region_ = &rect;
        }
      }
      if (cell->region_ == nullptr) {
        cell->region_ = group.region_boundaries.data();
      }
    }
    group.util = (total_site_area != 0)
                     ? static_cast<double>(cell_area) / total_site_area
                     : 0.0;
  }
}

void Opendp::groupInitPixels2()
{
  for (auto& layer : grid_->getInfoMap()) {
    const GridInfo& grid_info = layer.second;
    const GridY row_count = layer.second.getRowCount();
    const GridX row_site_count = layer.second.getSiteCount();
    const auto& grid_sites = layer.second.getSites();
    for (GridX x{0}; x < row_site_count; x++) {
      for (GridY y{0}; y < row_count; y++) {
        const int row_height
            = grid_sites[y.v % grid_sites.size()].site->getHeight();
        const DbuX site_width = grid_->getSiteWidth();
        const Rect sub(x.v * site_width.v,
                       y.v * row_height,
                       (x + 1).v * site_width.v,
                       (y + 1).v * row_height);
        Pixel* pixel = grid_->gridPixel(grid_info.getGridIndex(), x, y);
        for (Group& group : groups_) {
          for (Rect& rect : group.region_boundaries) {
            if (!isInside(sub, rect) && checkOverlap(sub, rect)) {
              pixel->util = 0.0;
              pixel->cell = &Cell::dummy_cell;
              pixel->is_valid = false;
            }
          }
        }
      }
    }
  }
}

/* static */
bool Opendp::isInside(const Rect& cell, const Rect& box)
{
  return cell.xMin() >= box.xMin() && cell.xMax() <= box.xMax()
         && cell.yMin() >= box.yMin() && cell.yMax() <= box.yMax();
}

bool Opendp::checkOverlap(const Rect& cell, const Rect& box)
{
  return box.xMin() < cell.xMax() && box.xMax() > cell.xMin()
         && box.yMin() < cell.yMax() && box.yMax() > cell.yMin();
}

void Opendp::groupInitPixels()
{
  for (const auto& layer : grid_->getInfoMap()) {
    const GridInfo& grid_info = layer.second;
    for (GridX x{0}; x < grid_info.getSiteCount(); x++) {
      for (GridY y{0}; y < grid_info.getRowCount(); y++) {
        Pixel* pixel = grid_->gridPixel(grid_info.getGridIndex(), x, y);
        pixel->util = 0.0;
      }
    }
  }
  for (Group& group : groups_) {
    if (group.cells_.empty()) {
      logger_->warn(DPL, 42, "No cells found in group {}. ", group.name);
      continue;
    }
    const DbuY row_height = group.cells_[0]->height_;
    const GridMapKey gmk = grid_->getGridMapKey(group.cells_[0]);
    const GridInfo& grid_info = grid_->getInfoMap().at(gmk);
    const int grid_index = grid_info.getGridIndex();
    const DbuX site_width = grid_->getSiteWidth();
    for (const DbuRect rect : group.region_boundaries) {
      debugPrint(logger_,
                 DPL,
                 "detailed",
                 1,
                 "Group {} region [x{} y{}] [x{} y{}]",
                 group.name,
                 rect.xl,
                 rect.yl,
                 rect.xh,
                 rect.yh);
      const GridY row_start{dbuToGridCeil(rect.yl, row_height)};
      const GridY row_end{dbuToGridFloor(rect.yh, row_height)};

      for (GridY k{row_start}; k < row_end; k++) {
        const GridX col_start{dbuToGridCeil(rect.xl, site_width)};
        const GridX col_end{dbuToGridFloor(rect.xh, site_width)};

        for (GridX l{col_start}; l < col_end; l++) {
          Pixel* pixel = grid_->gridPixel(grid_index, l, k);
          pixel->util += 1.0;
        }
        if (rect.xl % site_width != 0) {
          Pixel* pixel = grid_->gridPixel(grid_index, col_start, k);
          pixel->util
              -= (rect.xl % site_width).v / static_cast<double>(site_width.v);
        }
        if (rect.xh % site_width != 0) {
          Pixel* pixel = grid_->gridPixel(grid_index, col_end - 1, k);
          pixel->util -= ((site_width - rect.xh) % site_width).v
                         / static_cast<double>(site_width.v);
        }
      }
    }
    for (const DbuRect rect : group.region_boundaries) {
      const GridY row_start{dbuToGridCeil(rect.yl, row_height)};
      const GridY row_end{dbuToGridFloor(rect.yh, row_height)};

      for (GridY k{row_start}; k < row_end; k++) {
        const GridX col_start = dbuToGridCeil(rect.xl, site_width);
        const GridX col_end = dbuToGridFloor(rect.xh, site_width);

        // Assign group to each pixel.
        for (GridX l{col_start}; l < col_end; l++) {
          Pixel* pixel = grid_->gridPixel(grid_index, l, k);
          if (pixel->util == 1.0) {
            pixel->group = &group;
            pixel->is_valid = true;
            pixel->util = 1.0;
          } else if (pixel->util > 0.0 && pixel->util < 1.0) {
            pixel->cell = &Cell::dummy_cell;
            pixel->util = 0.0;
            pixel->is_valid = false;
          }
        }
      }
    }
  }
}

int divRound(const int dividend, const int divisor)
{
  return round(static_cast<double>(dividend) / divisor);
}

int divCeil(const int dividend, const int divisor)
{
  return ceil(static_cast<double>(dividend) / divisor);
}

int divFloor(const int dividend, const int divisor)
{
  return dividend / divisor;
}

}  // namespace dpl
