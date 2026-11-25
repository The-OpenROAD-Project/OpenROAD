// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <utility>
#include <vector>

#include "odb/geom.h"
#include "web/core.h"

namespace stt {

struct Tree;

// Simple general purpose render for a group of lines.
class LinesRenderer : public web::Renderer
{
 public:
  using LineSegment = std::pair<odb::Point, odb::Point>;
  using LineSegments = std::vector<LineSegment>;

  void highlight(const LineSegments& lines, const web::Painter::Color& color);
  void drawObjects(web::Painter& /* painter */) override;
  // singleton for debug functions
  static LinesRenderer* lines_renderer_;

 private:
  LineSegments lines_;
  web::Painter::Color color_;
};

void highlightSteinerTree(const Tree& tree, web::Gui* gui);

}  // namespace stt
