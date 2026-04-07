// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "drc_report.h"

#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "tile_generator.h"

namespace web {

DRCReport::DRCReport(const TileGenerator* gen) : gen_(gen)
{
}

void DRCReport::collectViolations(odb::dbMarkerCategory* category,
                                  std::vector<DRCViolation>& violations,
                                  int& index) const
{
  for (odb::dbMarker* marker : category->getMarkers()) {
    DRCViolation v;
    v.index = index++;
    v.rule = category->getName();

    odb::dbTechLayer* layer = marker->getTechLayer();
    if (layer) {
      v.layer = layer->getName();
    }

    v.bbox = marker->getBBox();
    v.comment = marker->getComment();
    v.is_visited = marker->isVisited();

    for (const auto& shape : marker->getShapes()) {
      if (std::holds_alternative<odb::Rect>(shape)) {
        v.rects.push_back(std::get<odb::Rect>(shape));
      } else if (std::holds_alternative<odb::Polygon>(shape)) {
        v.polys.push_back(std::get<odb::Polygon>(shape));
      } else if (std::holds_alternative<odb::Cuboid>(shape)) {
        v.cuboids.push_back(std::get<odb::Cuboid>(shape));
      }
      // Point and Line shapes: bbox is used as fallback below
    }

    // Ensure at least one shape for rendering
    if (v.rects.empty() && v.polys.empty() && v.cuboids.empty()
        && !v.bbox.isInverted()) {
      v.rects.push_back(v.bbox);
    }

    violations.push_back(std::move(v));
  }

  // Recurse into sub-categories
  for (odb::dbMarkerCategory* sub : category->getMarkerCategories()) {
    collectViolations(sub, violations, index);
  }
}

DRCReportResult DRCReport::getReport() const
{
  DRCReportResult result;

  odb::dbChip* chip = gen_->getChip();
  if (!chip) {
    return result;
  }

  int index = 0;
  for (odb::dbMarkerCategory* category : chip->getMarkerCategories()) {
    DRCCategoryResult cat_result;
    cat_result.name = category->getName();
    collectViolations(category, cat_result.violations, index);
    cat_result.count = static_cast<int>(cat_result.violations.size());
    result.total_count += cat_result.count;
    result.categories.push_back(std::move(cat_result));
  }

  return result;
}

}  // namespace web
