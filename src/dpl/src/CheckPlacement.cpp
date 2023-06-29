/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
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

#include <cmath>
#include <limits>

#include "dpl/Opendp.h"
#include "utl/Logger.h"

namespace dpl {

using odb::Direction2D;
using std::vector;

using utl::DPL;

void Opendp::checkPlacement(bool verbose, bool disallow_one_site_gaps)
{
  importDb();

  vector<Cell*> placed_failures;
  vector<Cell*> in_rows_failures;
  vector<Cell*> overlap_failures;
  vector<Cell*> one_site_gap_failures;
  vector<Cell*> site_align_failures;
  vector<Cell*> region_placement_failures;

  initGrid();
  groupAssignCellRegions();
  for (Cell& cell : cells_) {
    if (isStdCell(&cell)) {
      // Site alignment check
      if (cell.x_ % site_width_ != 0 || cell.y_ % row_height_ != 0) {
        site_align_failures.push_back(&cell);
      }
      if (!checkInRows(cell)) {
        in_rows_failures.push_back(&cell);
      }
      if (!checkRegionPlacement(&cell)) {
        region_placement_failures.push_back(&cell);
      }
    }
    // Placed check
    if (!isPlaced(&cell)) {
      placed_failures.push_back(&cell);
    }
    // Overlap check
    if (checkOverlap(cell)) {
      overlap_failures.push_back(&cell);
    }
  }
  // This loop is separate because it needs to be done after the overlap check
  // The overlap check assigns the overlap cell to its pixel
  // Thus, the one site gap check needs to be done after the overlap check
  // Otherwise, this check will miss the pixels that could have resulted in
  // one-site gap violations as null
  if (disallow_one_site_gaps) {
    for (Cell& cell : cells_) {
      // One site gap check
      if (checkOneSiteGaps(cell)) {
        one_site_gap_failures.push_back(&cell);
      }
    }
  }

  reportFailures(placed_failures, 3, "Placed", verbose);
  reportFailures(in_rows_failures, 4, "Placed in rows", verbose);
  reportFailures(
      overlap_failures, 5, "Overlap", verbose, [&](Cell* cell) -> void {
        reportOverlapFailure(cell);
      });
  reportFailures(site_align_failures, 6, "Site aligned", verbose);
  reportFailures(one_site_gap_failures, 7, "One site gap", verbose);
  reportFailures(region_placement_failures, 8, "Region placement", verbose);

  logger_->metric("design__violations",
                  placed_failures.size() + in_rows_failures.size()
                      + overlap_failures.size() + site_align_failures.size());
  if (placed_failures.size() + in_rows_failures.size() + overlap_failures.size()
          + site_align_failures.size()
          + (disallow_one_site_gaps ? one_site_gap_failures.size() : 0)
          + region_placement_failures.size()
      > 0) {
    logger_->error(DPL, 33, "detailed placement checks failed.");
  }
}

void Opendp::reportFailures(const vector<Cell*>& failures,
                            int msg_id,
                            const char* msg,
                            bool verbose) const
{
  reportFailures(failures, msg_id, msg, verbose, [&](Cell* cell) -> void {
    logger_->report(" {}", cell->name());
  });
}

void Opendp::reportFailures(
    const vector<Cell*>& failures,
    int msg_id,
    const char* msg,
    bool verbose,
    const std::function<void(Cell* cell)>& report_failure) const
{
  if (!failures.empty()) {
    logger_->warn(DPL, msg_id, "{} check failed ({}).", msg, failures.size());
    if (verbose) {
      for (Cell* cell : failures) {
        report_failure(cell);
      }
    }
  }
}

void Opendp::reportOverlapFailure(Cell* cell) const
{
  const Cell* overlap = checkOverlap(*cell);
  logger_->report(" {} overlaps {}", cell->name(), overlap->name());
}

bool Opendp::isPlaced(const Cell* cell)
{
  return cell->db_inst_->isPlaced();
}

bool Opendp::checkInRows(const Cell& cell) const
{
  auto grid_info = getRowInfo(&cell);
  int site_width = getSiteWidth(&cell);
  int x_ll = gridX(&cell, site_width);
  int x_ur = gridEndX(&cell, site_width);
  int y_ll = gridY(&cell, grid_info.first);
  int y_ur = gridEndY(&cell, grid_info.first);

  for (int y = y_ll; y < y_ur; y++) {
    for (int x = x_ll; x < x_ur; x++) {
      Pixel* pixel = gridPixel(grid_info.second.grid_index, x, y);
      if (pixel == nullptr  // outside core
          || !pixel->is_valid) {
        return false;
      }
    }
  }
  return true;
}

// COVER *, RING, PAD * - ignored

// CLASSes are grouped as follows
// CR = {CORE, CORE FEEDTHRU, CORE TIEHIGH, CORE TIELOW, CORE ANTENNACELL}
// WT = CORE WELLTAP
// SP = CORE SPACER, ENDCAP *
// BL = BLOCK *

//    CR WT BL SP
// CR  P  P  P  O
// WT  P  O  P  O
// BL  P  P  -  O
// SP  O  O  O  O
//
// P = no padded overlap
// O = no overlap (padding ignored)
// - = no overlap check (overlap allowed)
// The rules apply to both FIXED or PLACED instances

// Return the cell this cell overlaps.
Cell* Opendp::checkOverlap(Cell& cell) const
{
  debugPrint(
      logger_, DPL, "grid", 2, "checking overlap for cell {}", cell.name());
  Cell* overlap_cell = nullptr;
  visitCellPixels(cell, true, [&](Pixel* pixel) {
    Cell* pixel_cell = pixel->cell;
    if (pixel_cell) {
      if (pixel_cell != &cell && overlap(&cell, pixel_cell)) {
        overlap_cell = pixel_cell;
      }
    } else {
      pixel->cell = &cell;
    }
  });
  return overlap_cell;
}

bool Opendp::overlap(const Cell* cell1, const Cell* cell2) const
{
  // BLOCK/BLOCK overlaps allowed
  if (isBlock(cell1) && isBlock(cell2)) {
    return false;
  }

  bool padded = havePadding() && isOverlapPadded(cell1, cell2);
  Point ll1 = initialLocation(cell1, padded);
  Point ll2 = initialLocation(cell2, padded);
  Point ur1, ur2;
  if (padded) {
    ur1 = Point(ll1.getX() + paddedWidth(cell1), ll1.getY() + cell1->height_);
    ur2 = Point(ll2.getX() + paddedWidth(cell2), ll2.getY() + cell2->height_);
  } else {
    ur1 = Point(ll1.getX() + cell1->width_, ll1.getY() + cell1->height_);
    ur2 = Point(ll2.getX() + cell2->width_, ll2.getY() + cell2->height_);
  }
  return ll1.getX() < ur2.getX() && ur1.getX() > ll2.getX()
         && ll1.getY() < ur2.getY() && ur1.getY() > ll2.getY();
}

Cell* Opendp::checkOneSiteGaps(Cell& cell) const
{
  Cell* gap_cell = nullptr;
  auto row_info = getRowInfo(&cell);
  int index_in_grid = row_info.second.grid_index;
  visitCellBoundaryPixels(
      cell, true, [&](Pixel* pixel, const Direction2D& edge, int x, int y) {
        Cell* pixel_cell = pixel->cell;

        int abut_x = 0;

        switch (edge) {
          case Direction2D::West:
            abut_x = -1;
            break;
          case Direction2D::East:
            abut_x = 1;
            break;
        }
        if (0 != abut_x) {
          // check the abutting pixel
          Pixel* abut_pixel = gridPixel(index_in_grid, x + abut_x, y);
          bool abuttment_exists
              = ((abut_pixel != nullptr) && abut_pixel->cell != pixel_cell
                 && abut_pixel->cell != nullptr);
          if (!abuttment_exists) {
            // check the 1 site gap pixel
            Pixel* gap_pixel = gridPixel(index_in_grid, x + 2 * abut_x, y);
            if (gap_pixel && gap_pixel->cell != pixel_cell) {
              gap_cell = gap_pixel->cell;
            }
          }
        }
      });
  return gap_cell;
}

bool Opendp::checkRegionPlacement(const Cell* cell) const
{
  int x_begin = cell->x_;
  int x_end = x_begin + cell->width_;
  int y_begin = cell->y_;
  int y_end = y_begin + cell->height_;

  if (cell->region_) {
    return cell->region_->contains(odb::Rect(x_begin, y_begin, x_end, y_end))
           && checkRegionOverlap(cell,
                                 x_begin / site_width_,
                                 y_begin / cell->height_,
                                 x_end / site_width_,
                                 y_end / cell->height_);
  }
  return true;
}

bool Opendp::isOverlapPadded(const Cell* cell1, const Cell* cell2) const
{
  return isCrWtBlClass(cell1) && isCrWtBlClass(cell2)
         && !(isWtClass(cell1) && isWtClass(cell2));
}

bool Opendp::isCrWtBlClass(const Cell* cell) const
{
  dbMasterType type = cell->db_inst_->getMaster()->getType();
  // Use switch so if new types are added we get a compiler warning.
  switch (type) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
      return true;
    case dbMasterType::CORE_SPACER:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
      // These classes are completely ignored by the placer.
    case dbMasterType::COVER:
    case dbMasterType::COVER_BUMP:
    case dbMasterType::RING:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
    case dbMasterType::NONE:
      return false;
  }
  // gcc warniing
  return false;
}

bool Opendp::isWtClass(const Cell* cell) const
{
  dbMasterType type = cell->db_inst_->getMaster()->getType();
  return type == dbMasterType::CORE_WELLTAP;
}

}  // namespace dpl
