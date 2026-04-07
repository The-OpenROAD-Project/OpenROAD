// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "odb/geom.h"

namespace odb {
class dbMarkerCategory;
}

namespace web {

class TileGenerator;

struct DRCViolation
{
  int index = 0;        // flat global index (for highlight lookup)
  std::string rule;     // sub-category / marker rule name
  std::string layer;    // tech layer name (empty if none)
  odb::Rect bbox;       // bounding box in DBU (for zoom)
  std::string comment;
  bool is_visited = false;
  std::vector<odb::Rect> rects;     // shape rects for highlight rendering
  std::vector<odb::Polygon> polys;  // shape polys for highlight rendering
  std::vector<odb::Cuboid> cuboids; // shape cuboids for 3D highlight rendering
};

struct DRCCategoryResult
{
  std::string name;
  int count = 0;
  std::vector<DRCViolation> violations;
};

struct DRCReportResult
{
  std::vector<DRCCategoryResult> categories;
  int total_count = 0;
};

class DRCReport
{
 public:
  explicit DRCReport(const TileGenerator* gen);

  DRCReportResult getReport() const;

 private:
  void collectViolations(odb::dbMarkerCategory* category,
                         std::vector<DRCViolation>& violations,
                         int& index) const;

  const TileGenerator* gen_;
};

}  // namespace web
