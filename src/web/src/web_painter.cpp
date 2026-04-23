// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "web_painter.h"

#include <string>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace web {

WebPainter::WebPainter(const odb::Rect& bounds, double pixels_per_dbu)
    : gui::Painter(nullptr, bounds, pixels_per_dbu)
{
  pen_.color = kBlack;
  brush_.color = Color(0, 0, 0, 0);
  brush_.style = Brush::kNone;
  font_.name = "sans-serif";
  font_.size = 10;
}

void WebPainter::setPen(odb::dbTechLayer* /* layer */, bool cosmetic)
{
  // Headless: no per-layer color scheme here; use current pen color,
  // just flip the cosmetic flag to match semantics.
  pen_.cosmetic = cosmetic;
}

void WebPainter::setPen(const Color& color, bool cosmetic, int width)
{
  pen_.color = color;
  pen_.cosmetic = cosmetic;
  pen_.width = width;
}

void WebPainter::setBrush(odb::dbTechLayer* /* layer */, int alpha)
{
  // Headless: keep current brush color, override alpha if provided.
  if (alpha >= 0) {
    brush_.color.a = alpha;
  }
  brush_.style = Brush::kSolid;
}

void WebPainter::setBrush(const Color& color, const Brush& style)
{
  brush_.color = color;
  brush_.style = style;
}

void WebPainter::setFont(const Font& font)
{
  font_.name = font.name;
  font_.size = font.size;
}

void WebPainter::saveState()
{
  state_stack_.push_back({pen_, brush_, font_});
}

void WebPainter::restoreState()
{
  if (state_stack_.empty()) {
    return;
  }
  const SavedState& s = state_stack_.back();
  pen_ = s.pen;
  brush_ = s.brush;
  font_ = s.font;
  state_stack_.pop_back();
}

void WebPainter::drawOctagon(const odb::Oct& oct)
{
  // Represent an octagon as a polygon.
  ops_.emplace_back(
      DrawPolygonOp{.points = oct.getPoints(), .pen = pen_, .brush = brush_});
}

void WebPainter::drawRect(const odb::Rect& rect, int round_x, int round_y)
{
  ops_.emplace_back(DrawRectOp{.rect = rect,
                               .round_x = round_x,
                               .round_y = round_y,
                               .pen = pen_,
                               .brush = brush_});
}

void WebPainter::drawLine(const odb::Point& p1, const odb::Point& p2)
{
  ops_.emplace_back(DrawLineOp{.p1 = p1, .p2 = p2, .pen = pen_});
}

void WebPainter::drawCircle(int x, int y, int r)
{
  ops_.emplace_back(
      DrawCircleOp{.cx = x, .cy = y, .r = r, .pen = pen_, .brush = brush_});
}

void WebPainter::drawX(int x, int y, int size)
{
  ops_.emplace_back(DrawXOp{.cx = x, .cy = y, .size = size, .pen = pen_});
}

void WebPainter::drawPolygon(const odb::Polygon& polygon)
{
  ops_.emplace_back(DrawPolygonOp{
      .points = polygon.getPoints(), .pen = pen_, .brush = brush_});
}

void WebPainter::drawPolygon(const std::vector<odb::Point>& points)
{
  ops_.emplace_back(
      DrawPolygonOp{.points = points, .pen = pen_, .brush = brush_});
}

void WebPainter::drawString(int x,
                            int y,
                            Anchor anchor,
                            const std::string& s,
                            bool rotate_90)
{
  ops_.emplace_back(DrawStringOp{.x = x,
                                 .y = y,
                                 .anchor = anchor,
                                 .text = s,
                                 .rotate_90 = rotate_90,
                                 .pen = pen_,
                                 .font = font_});
}

odb::Rect WebPainter::stringBoundaries(int /* x */,
                                       int /* y */,
                                       Anchor /* anchor */,
                                       const std::string& /* s */)
{
  // Headless: we don't have a real font metrics engine.  Callers use this
  // for hover-test / placement; return an empty rect.
  return {};
}

void WebPainter::drawRuler(int /* x0 */,
                           int /* y0 */,
                           int /* x1 */,
                           int /* y1 */,
                           bool /* euclidian */,
                           const std::string& /* label */)
{
  // Rulers are a GUI-only construct; no-op in web.
}

}  // namespace web
