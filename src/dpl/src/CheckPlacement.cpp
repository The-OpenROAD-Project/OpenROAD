// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include "PlacementDRC.h"
#include "dpl/Opendp.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/Padding.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "odb/isotropy.h"
#include "utl/Logger.h"
namespace dpl {

using odb::Direction2D;
using std::vector;

using utl::DPL;

using utl::format_as;  // NOLINT(misc-unused-using-decls)

void Opendp::checkPlacement(const bool verbose,
                            const std::string& report_file_name,
                            const bool fixed_only)
{
  importDb(fixed_only);
  adjustNodesOrient();
  initGrid();
  groupAssignCellRegions();

  CheckPlacementFailures failures;
  const auto& row_coords = grid_->getRowCoordinates();
  for (auto& cell : network_->getNodes()) {
    if (cell->getType() != Node::CELL) {
      continue;
    }
    checkCellPlacement(cell.get(), row_coords, failures);
  }
  // This loop is separate because it needs to be done after the overlap check
  // The overlap check assigns the overlap cell to its pixel
  // Thus, the one site gap check needs to be done after the overlap check
  // Otherwise, this check will miss the pixels that could have resulted in
  // one-site gap violations as null
  if (disallow_one_site_gaps_) {
    for (auto& cell : network_->getNodes()) {
      // One site gap check
      if (cell->getType() == Node::CELL && checkOneSiteGaps(*cell)) {
        failures.one_site_gap.push_back(cell.get());
      }
    }
  }
  const size_t violations
      = reportCheckPlacement(failures, verbose, fixed_only, report_file_name);
  if (fixed_only) {
    // Filler and decap placement reuse a populated network without
    // re-importing, so do not leave one without the movable cells behind.
    importClear();
  }
  if (violations == 0) {
    return;
  }
  if (fixed_only) {
    logger_->error(DPL,
                   41,
                   "placement checks failed for fixed instances during check "
                   "placement.");
  } else {
    logger_->error(
        DPL, 33, "detailed placement checks failed during check placement.");
  }
}

void Opendp::checkCellPlacement(Node* cell,
                                const std::unordered_set<int>& row_coords,
                                CheckPlacementFailures& failures)
{
  if (cell->isStdCell()) {
    // Site alignment check
    if (cell->getLeft() % grid_->getSiteWidth() != 0
        || row_coords.find(cell->getBottom().v) == row_coords.end()) {
      failures.site_align.push_back(cell);
      return;
    }

    if (!checkInRows(*cell)) {
      failures.in_rows.push_back(cell);
    }
    if (!checkRegionPlacement(cell)) {
      failures.region_placement.push_back(cell);
    }
  }
  // Placed check
  if (!isPlaced(cell)) {
    failures.placed.push_back(cell);
  }
  // Overlap check
  if (checkOverlap(*cell)) {
    failures.overlap.push_back(cell);
  }
  // Padding check
  if (!drc_engine_->checkPadding(cell)) {
    failures.padding.emplace_back(cell);
  }
  grid_->paintCellPadding(cell);
  // EdgeSpacing check
  if (!drc_engine_->checkEdgeSpacing(cell)) {
    failures.edge_spacing.emplace_back(cell);
  }
  if (!drc_engine_->checkBlockedLayers(cell)) {
    failures.blocked_layers.emplace_back(cell);
  }
}

// Saves markers, writes the report and logs the per-check warnings.
// Returns the number of violations that make the check fail.
size_t Opendp::reportCheckPlacement(const CheckPlacementFailures& failures,
                                    const bool verbose,
                                    const bool fixed_only,
                                    const std::string& report_file_name)
{
  saveFailures(failures);
  if (!report_file_name.empty()) {
    writeJsonReport(report_file_name);
  }
  reportFailures(failures.placed, 3, "Placed", verbose);
  reportFailures(failures.in_rows, 4, "Placed in rows", verbose);
  reportFailures(
      failures.overlap, 5, "Overlap", verbose, [&](Node* cell) -> void {
        reportOverlapFailure(cell);
      });
  reportFailures(failures.padding, 11, "Padding", verbose);
  reportFailures(failures.site_align, 6, "Site aligned", verbose);
  reportFailures(failures.one_site_gap, 7, "One site gap", verbose);
  reportFailures(failures.region_placement, 8, "Region placement", verbose);
  reportFailures(
      failures.edge_spacing, 9, "LEF58_CELLEDGESPACINGTABLE", verbose);
  reportFailures(failures.blocked_layers, 10, "Blocked layers", verbose);

  const size_t metric_violations
      = failures.placed.size() + failures.in_rows.size()
        + failures.overlap.size() + failures.padding.size()
        + failures.site_align.size();
  // A fixed-only run validates the floorplan, so it reports under its own
  // metric instead of overwriting the detailed placement one.
  logger_->metric(fixed_only ? "floorplan__violations" : "design__violations",
                  metric_violations);

  return metric_violations
         + (disallow_one_site_gaps_ ? failures.one_site_gap.size() : 0)
         + failures.region_placement.size() + failures.edge_spacing.size()
         + failures.blocked_layers.size();
}

void Opendp::saveViolations(const std::vector<Node*>& failures,
                            odb::dbMarkerCategory* category,
                            const std::string& violation_type) const
{
  for (auto failure : failures) {
    odb::dbMarker* marker = odb::dbMarker::create(category);
    if (!marker) {
      break;
    }
    int xMin = (failure->getLeft() + core_.xMin()).v;
    int yMin = (failure->getBottom() + core_.yMin()).v;
    int xMax = (failure->getLeft() + failure->getWidth() + core_.xMin()).v;
    int yMax = (failure->getBottom() + failure->getHeight() + core_.yMin()).v;

    if (violation_type == "overlap") {
      const Node* o_cell = checkOverlap(*failure);
      if (!o_cell) {
        logger_->error(DPL,
                       48,
                       "Could not find overlapping cell for cell {}",
                       failure->name());
      }
      odb::Rect o_rect(o_cell->getLeft().v,
                       o_cell->getBottom().v,
                       o_cell->getLeft().v + o_cell->getWidth().v,
                       o_cell->getBottom().v + o_cell->getHeight().v);
      odb::Rect f_rect(failure->getLeft().v,
                       failure->getBottom().v,
                       failure->getLeft().v + failure->getWidth().v,
                       failure->getBottom().v + failure->getHeight().v);

      odb::Rect overlap_rect;
      o_rect.intersection(f_rect, overlap_rect);

      xMin = overlap_rect.xMin() + core_.xMin();
      yMin = overlap_rect.yMin() + core_.yMin();
      xMax = overlap_rect.xMax() + core_.xMin();
      yMax = overlap_rect.yMax() + core_.yMin();

      marker->addSource(o_cell->getDbInst());
    }
    marker->addShape(odb::Rect{xMin, yMin, xMax, yMax});
    marker->addSource(failure->getDbInst());
  }
}

void Opendp::saveFailures(const CheckPlacementFailures& failures)
{
  if (failures.placed.empty() && failures.in_rows.empty()
      && failures.overlap.empty() && failures.padding.empty()
      && failures.one_site_gap.empty() && failures.site_align.empty()
      && failures.region_placement.empty() && failures.placement.empty()
      && failures.edge_spacing.empty() && failures.blocked_layers.empty()) {
    return;
  }

  auto* tool_category = odb::dbMarkerCategory::createOrReplace(block_, "DPL");
  if (!failures.placed.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Placement failures");
    category->setDescription("Cells that were not placed.");
    saveViolations(failures.placed, category);
  }
  if (!failures.in_rows.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(tool_category,
                                                           "In_rows_failures");
    category->setDescription(
        "Cells that were not assigned to rows in the grid.");
    saveViolations(failures.in_rows, category);
  }
  if (!failures.overlap.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(tool_category,
                                                           "Overlap_failures");
    category->setDescription("Cells that are overlapping with other cells.");
    saveViolations(failures.overlap, category, "overlap");
  }
  if (!failures.padding.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(tool_category,
                                                           "Padding_failures");
    category->setDescription("Cells that violate the padding rules.");
    saveViolations(failures.padding, category);
  }
  if (!failures.one_site_gap.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "One_site_gap_failures");
    category->setDescription(
        "Cells that violate the one site gap spacing rules.");
    saveViolations(failures.one_site_gap, category);
  }
  if (!failures.site_align.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Site_alignment_failures");
    category->setDescription(
        "Cells that are not aligned with placement sites.");
    saveViolations(failures.site_align, category);
  }
  if (!failures.region_placement.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Region_placement_failures");
    category->setDescription(
        "Cells that violate the region placement constraints.");
    saveViolations(failures.region_placement, category);
  }
  if (!failures.placement.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Placement_failures");
    category->setDescription("Cells that DPL failed to place.");
    saveViolations(failures.placement, category);
  }
  if (!failures.edge_spacing.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Cell_edge_spacing_failures");
    category->setDescription(
        "Cells that violate the LEF58_CELLEDGESPACINGTABLE.");
    saveViolations(failures.edge_spacing, category);
  }
  if (!failures.blocked_layers.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Blocked_layers_failures");
    category->setDescription("Cells that violate the blocked layers.");
    saveViolations(failures.blocked_layers, category);
  }
}

void Opendp::writeJsonReport(const std::string& filename)
{
  auto* tool_category = block_->findMarkerCategory("DPL");
  if (tool_category) {
    tool_category->writeJSON(filename);
  }
}

void Opendp::reportFailures(const vector<Node*>& failures,
                            const int msg_id,
                            const char* msg,
                            const bool verbose) const
{
  reportFailures(failures, msg_id, msg, verbose, [&](Node* cell) -> void {
    logger_->report(" {}", cell->name());
  });
}

void Opendp::reportFailures(
    const vector<Node*>& failures,
    const int msg_id,
    const char* msg,
    const bool verbose,
    const std::function<void(Node* cell)>& report_failure) const
{
  if (!failures.empty()) {
    logger_->warn(DPL, msg_id, "{} check failed ({}).", msg, failures.size());
    if (verbose) {
      for (Node* cell : failures) {
        report_failure(cell);
      }
    }
  }
}

void Opendp::reportOverlapFailure(Node* cell) const
{
  const Node* overlap = checkOverlap(*cell);
  logger_->report(" {} ({}) overlaps {} ({})",
                  cell->name(),
                  cell->getDbInst()->getMaster()->getName(),
                  overlap->name(),
                  overlap->getDbInst()->getMaster()->getName());
}

/* static */
bool Opendp::isPlaced(const Node* cell)
{
  return cell->getDbInst()->isPlaced();
}

bool Opendp::checkInRows(const Node& cell) const
{
  const auto grid_rect = grid_->gridCovering(&cell);
  debugPrint(logger_,
             DPL,
             "hybrid",
             1,
             "Checking cell {} with site {} and "
             "height {} in rows. Y start {} y end {}",
             cell.name(),
             cell.getSite()->getName(),
             cell.getHeight(),
             grid_rect.ylo,
             grid_rect.yhi);

  for (GridY y = grid_rect.ylo; y < grid_rect.yhi; y++) {
    const bool first_row = (y == grid_rect.ylo);
    for (GridX x = grid_rect.xlo; x < grid_rect.xhi; x++) {
      const Pixel* pixel = grid_->gridPixel(x, y);
      // outside core or invalid
      if (pixel == nullptr || !pixel->is_valid) {
        return false;
      }
      if (first_row && !grid_->getSiteOrientation(x, y, cell.getSite())) {
        return false;
      }
    }
  }
  return !cell.getMaster()->isMultiRow()
         || checkRowPowerCompatible(&cell, grid_rect.ylo);
}

// Return the cell this cell overlaps.
const Node* Opendp::checkOverlap(Node& cell) const
{
  debugPrint(
      logger_, DPL, "grid", 2, "checking overlap for cell {}", cell.name());
  const Node* overlap_cell = nullptr;
  grid_->visitCellPixels(cell, false, [&](Pixel* pixel, bool padded) {
    const Node* pixel_cell = pixel->cell;
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

bool Opendp::overlap(const Node* cell1, const Node* cell2) const
{
  // BLOCK/BLOCK overlaps allowed
  if (cell1->isBlock() && cell2->isBlock()) {
    return false;
  }

  const DbuPt ll1 = initialLocation(cell1, false);
  const DbuPt ll2 = initialLocation(cell2, false);
  DbuPt ur1, ur2;
  ur1 = DbuPt(ll1.x + cell1->getWidth().v, ll1.y + cell1->getHeight().v);
  ur2 = DbuPt(ll2.x + cell2->getWidth().v, ll2.y + cell2->getHeight().v);
  return ll1.x < ur2.x && ur1.x > ll2.x && ll1.y < ur2.y && ur1.y > ll2.y;
}

Node* Opendp::checkOneSiteGaps(Node& cell) const
{
  Node* gap_cell = nullptr;
  grid_->visitCellBoundaryPixels(
      cell, [&](Pixel* pixel, const Direction2D& edge, GridX x, GridY y) {
        GridX abut_x{0};

        switch (static_cast<Direction2D::Value>(edge)) {
          case Direction2D::West:
            abut_x = GridX{-1};
            break;
          case Direction2D::East:
            abut_x = GridX{1};
            break;
          case Direction2D::North:
          case Direction2D::South:
            return;
        }
        // check the abutting pixel
        const Pixel* abut_pixel = grid_->gridPixel(x + abut_x, y);
        const bool site_exists = (abut_pixel && abut_pixel->is_valid);
        const bool abuttment_exists = (abut_pixel && abut_pixel->cell);
        if (site_exists && !abuttment_exists) {
          // check the 1 site gap pixel
          const Pixel* gap_pixel = grid_->gridPixel(x + GridX{2 * abut_x.v}, y);
          if (gap_pixel) {
            gap_cell = gap_pixel->cell;
          }
        }
      });
  return gap_cell;
}

bool Opendp::checkRegionPlacement(const Node* cell) const
{
  const DbuX x_begin = cell->getLeft();
  const DbuX x_end = x_begin + cell->getWidth();
  const DbuY y_begin = cell->getBottom();
  const DbuY y_end = y_begin + cell->getHeight();

  if (cell->getRegion()) {
    const DbuX site_width = grid_->getSiteWidth();
    return cell->getRegion()->contains(
               odb::Rect(x_begin.v, y_begin.v, x_end.v, y_end.v))
           && checkRegionOverlap(cell,
                                 GridX{x_begin.v / site_width.v},
                                 GridY{y_begin.v / cell->getHeight().v},
                                 GridX{x_end.v / site_width.v},
                                 GridY{y_end.v / cell->getHeight().v});
  }
  return true;
}

}  // namespace dpl
