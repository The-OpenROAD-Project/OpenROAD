#include "stt/LinesRenderer.h"

#include "stt/SteinerTreeBuilder.h"

namespace stt {

LinesRenderer* LinesRenderer::lines_renderer = nullptr;

void LinesRenderer::highlight(
    std::vector<std::pair<odb::Point, odb::Point>>& lines,
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
    std::vector<std::pair<odb::Point, odb::Point>> lines;
    for (int i = 0; i < tree.branchCount(); i++) {
      const stt::Branch& branch = tree.branch[i];
      int x1 = branch.x;
      int y1 = branch.y;
      const stt::Branch& neighbor = tree.branch[branch.n];
      int x2 = neighbor.x;
      int y2 = neighbor.y;
      lines.emplace_back(odb::Point(x1, y1), odb::Point(x2, y2));
    }
    LinesRenderer::lines_renderer->highlight(lines, gui::Painter::red);
  }
}

}  // namespace stt
