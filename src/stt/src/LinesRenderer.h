// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <utility>
#include <vector>

#include "gui/gui.h"

namespace stt {

struct Tree;

// Simple general purpose render for a group of lines.
class LinesRenderer : public gui::Renderer
{
 public:
  using LineSegment = std::pair<odb::Point, odb::Point>;
  using LineSegments = std::vector<LineSegment>;

  void highlight(const LineSegments& lines, const gui::Painter::Color& color);
  void drawObjects(gui::Painter& /* painter */) override;
  // singleton for debug functions
  static LinesRenderer* lines_renderer;

 private:
  LineSegments lines_;
  gui::Painter::Color color_;
};

void highlightSteinerTree(const Tree& tree, gui::Gui* gui);

}  // namespace stt
