// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "LinesRenderer.h"

#include "stt/SteinerTreeBuilder.h"

namespace stt {

LinesRenderer* LinesRenderer::lines_renderer = nullptr;

void LinesRenderer::highlight(const LineSegments& lines,
                              const gui::Painter::Color& color)
{
  lines_ = lines;
  color_ = color;
}

void LinesRenderer::drawObjects(gui::Painter& painter)
{
  if (!lines_.empty()) {
    painter.setPen(color_, true);
    for (const auto& [pt1, pt2] : lines_) {
      painter.drawLine(pt1, pt2);
    }
  }
}

void highlightSteinerTree(const Tree& tree, gui::Gui* gui)
{
  if (gui::Gui::enabled()) {
    if (LinesRenderer::lines_renderer == nullptr) {
      LinesRenderer::lines_renderer = new LinesRenderer();
      gui->registerRenderer(LinesRenderer::lines_renderer);
    }
    LinesRenderer::LineSegments lines;
    for (int i = 0; i < tree.branchCount(); i++) {
      const stt::Branch& branch = tree.branch[i];
      const int x1 = branch.x;
      const int y1 = branch.y;
      const stt::Branch& neighbor = tree.branch[branch.n];
      const int x2 = neighbor.x;
      const int y2 = neighbor.y;
      lines.emplace_back(odb::Point(x1, y1), odb::Point(x2, y2));
    }
    LinesRenderer::lines_renderer->highlight(lines, gui::Painter::red);
  }
}

}  // namespace stt
