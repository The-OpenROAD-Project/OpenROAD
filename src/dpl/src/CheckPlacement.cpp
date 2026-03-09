// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include <cmath>
#include <functional>
#include <string>
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
                            const std::string& report_file_name)
{
  importDb();
  adjustNodesOrient();

  std::vector<Node*> placed_failures;
  std::vector<Node*> in_rows_failures;
  std::vector<Node*> overlap_failures;
  std::vector<Node*> padding_failures;
  std::vector<Node*> one_site_gap_failures;
  std::vector<Node*> site_align_failures;
  std::vector<Node*> region_placement_failures;
  std::vector<Node*> edge_spacing_failures;
  std::vector<Node*> blocked_layers_failures;

  initGrid();
  groupAssignCellRegions();
  const auto& row_coords = grid_->getRowCoordinates();
  for (auto& cell : network_->getNodes()) {
    if (cell->getType() != Node::CELL) {
      continue;
    }
    if (cell->isStdCell()) {
      // Site alignment check
      if (cell->getLeft() % grid_->getSiteWidth() != 0
          || row_coords.find(cell->getBottom().v) == row_coords.end()) {
        site_align_failures.push_back(cell.get());
        continue;
      }

      if (!checkInRows(*cell)) {
        in_rows_failures.push_back(cell.get());
      }
      if (!checkRegionPlacement(cell.get())) {
        region_placement_failures.push_back(cell.get());
      }
    }
    // Placed check
    if (!isPlaced(cell.get())) {
      placed_failures.push_back(cell.get());
    }
    // Overlap check
    if (checkOverlap(*cell)) {
      overlap_failures.push_back(cell.get());
    }
    // Padding check
    if (!drc_engine_->checkPadding(cell.get())) {
      padding_failures.emplace_back(cell.get());
    }
    grid_->paintCellPadding(cell.get());
    // EdgeSpacing check
    if (!drc_engine_->checkEdgeSpacing(cell.get())) {
      edge_spacing_failures.emplace_back(cell.get());
    }
    if (!drc_engine_->checkBlockedLayers(cell.get())) {
      blocked_layers_failures.emplace_back(cell.get());
    }
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
        one_site_gap_failures.push_back(cell.get());
      }
    }
  }
  saveFailures(placed_failures,
               in_rows_failures,
               overlap_failures,
               padding_failures,
               one_site_gap_failures,
               site_align_failures,
               region_placement_failures,
               {},
               edge_spacing_failures,
               blocked_layers_failures);
  if (!report_file_name.empty()) {
    writeJsonReport(report_file_name);
  }
  reportFailures(placed_failures, 3, "Placed", verbose);
  reportFailures(in_rows_failures, 4, "Placed in rows", verbose);
  reportFailures(
      overlap_failures, 5, "Overlap", verbose, [&](Node* cell) -> void {
        reportOverlapFailure(cell);
      });
  reportFailures(padding_failures, 11, "Padding", verbose);
  reportFailures(site_align_failures, 6, "Site aligned", verbose);
  reportFailures(one_site_gap_failures, 7, "One site gap", verbose);
  reportFailures(region_placement_failures, 8, "Region placement", verbose);
  reportFailures(
      edge_spacing_failures, 9, "LEF58_CELLEDGESPACINGTABLE", verbose);
  reportFailures(blocked_layers_failures, 10, "Blocked layers", verbose);
  logger_->metric("design__violations",
                  placed_failures.size() + in_rows_failures.size()
                      + overlap_failures.size() + padding_failures.size()
                      + site_align_failures.size());

  if (placed_failures.size() + in_rows_failures.size() + overlap_failures.size()
          + padding_failures.size() + site_align_failures.size()
          + (disallow_one_site_gaps_ ? one_site_gap_failures.size() : 0)
          + region_placement_failures.size() + edge_spacing_failures.size()
          + blocked_layers_failures.size()
      > 0) {
    logger_->error(DPL, 33, "detailed placement checks failed.");
  }
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

void Opendp::saveFailures(const vector<Node*>& placed_failures,
                          const vector<Node*>& in_rows_failures,
                          const vector<Node*>& overlap_failures,
                          const vector<Node*>& padding_failures,
                          const vector<Node*>& one_site_gap_failures,
                          const vector<Node*>& site_align_failures,
                          const vector<Node*>& region_placement_failures,
                          const vector<Node*>& placement_failures,
                          const vector<Node*>& edge_spacing_failures,
                          const vector<Node*>& blocked_layers_failures)
{
  if (placed_failures.empty() && in_rows_failures.empty()
      && overlap_failures.empty() && padding_failures.empty()
      && one_site_gap_failures.empty() && site_align_failures.empty()
      && region_placement_failures.empty() && placement_failures.empty()
      && edge_spacing_failures.empty() && blocked_layers_failures.empty()) {
    return;
  }

  auto* tool_category = odb::dbMarkerCategory::createOrReplace(block_, "DPL");
  if (!placed_failures.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Placement failures");
    category->setDescription("Cells that were not placed.");
    saveViolations(placed_failures, category);
  }
  if (!in_rows_failures.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(tool_category,
                                                           "In_rows_failures");
    category->setDescription(
        "Cells that were not assigned to rows in the grid.");
    saveViolations(in_rows_failures, category);
  }
  if (!overlap_failures.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(tool_category,
                                                           "Overlap_failures");
    category->setDescription("Cells that are overlapping with other cells.");
    saveViolations(overlap_failures, category, "overlap");
  }
  if (!padding_failures.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(tool_category,
                                                           "Padding_failures");
    category->setDescription("Cells that violate the padding rules.");
    saveViolations(padding_failures, category);
  }
  if (!one_site_gap_failures.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "One_site_gap_failures");
    category->setDescription(
        "Cells that violate the one site gap spacing rules.");
    saveViolations(one_site_gap_failures, category);
  }
  if (!site_align_failures.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Site_alignment_failures");
    category->setDescription(
        "Cells that are not aligned with placement sites.");
    saveViolations(site_align_failures, category);
  }
  if (!region_placement_failures.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Region_placement_failures");
    category->setDescription(
        "Cells that violate the region placement constraints.");
    saveViolations(region_placement_failures, category);
  }
  if (!placement_failures.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Placement_failures");
    category->setDescription("Cells that DPL failed to place.");
    saveViolations(placement_failures, category);
  }
  if (!edge_spacing_failures.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Cell_edge_spacing_failures");
    category->setDescription(
        "Cells that violate the LEF58_CELLEDGESPACINGTABLE.");
    saveViolations(edge_spacing_failures, category);
  }
  if (!blocked_layers_failures.empty()) {
    auto category = odb::dbMarkerCategory::createOrReplace(
        tool_category, "Blocked_layers_failures");
    category->setDescription("Cells that violate the blocked layers.");
    saveViolations(blocked_layers_failures, category);
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
  return true;
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
        const bool abuttment_exists = (abut_pixel && abut_pixel->cell);
        if (!abuttment_exists) {
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
