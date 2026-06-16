// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <variant>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace web {

// Captured draw style at the time a shape is emitted.  Separate pen and
// brush state matches gui::Painter's semantics.
struct PenState
{
  gui::Painter::Color color{0, 0, 0, 255};
  int width = 1;
  bool cosmetic = false;
};

struct BrushState
{
  gui::Painter::Color color{0, 0, 0, 0};  // alpha 0 means "no fill"
  gui::Painter::Brush style = gui::Painter::Brush::kNone;
};

struct FontState
{
  std::string name;
  int size = 10;
};

// Tagged draw operations recorded by WebPainter.  DBU coordinates throughout;
// the rasterizer converts to pixels using the tile's bounds / scale.
struct DrawRectOp
{
  odb::Rect rect;
  int round_x = 0;
  int round_y = 0;
  PenState pen;
  BrushState brush;
};

struct DrawLineOp
{
  odb::Point p1;
  odb::Point p2;
  PenState pen;
};

struct DrawCircleOp
{
  int cx = 0;
  int cy = 0;
  int r = 0;
  PenState pen;
  BrushState brush;
};

struct DrawXOp
{
  int cx = 0;
  int cy = 0;
  int size = 0;
  PenState pen;
};

struct DrawPolygonOp
{
  std::vector<odb::Point> points;
  PenState pen;
  BrushState brush;
};

struct DrawStringOp
{
  int x = 0;
  int y = 0;
  gui::Painter::Anchor anchor = gui::Painter::kCenter;
  std::string text;
  bool rotate_90 = false;
  PenState pen;
  FontState font;
};

using DrawOp = std::variant<DrawRectOp,
                            DrawLineOp,
                            DrawCircleOp,
                            DrawXOp,
                            DrawPolygonOp,
                            DrawStringOp>;

// Records draw calls as DrawOps so existing drawObjects() code
// (gpl::GraphicsImpl, etc.) can be reused unmodified for web rendering.
class WebPainter : public gui::Painter
{
 public:
  WebPainter(const odb::Rect& bounds, double pixels_per_dbu);
  ~WebPainter() override = default;

  const std::vector<DrawOp>& ops() const { return ops_; }

  // --- gui::Painter overrides ---
  // Pull in the non-virtual helper overloads that the three-arg
  // setPen/setBrush/drawRect/drawLine overrides below would otherwise
  // hide (C++ name hiding on derived overrides).
  using gui::Painter::drawLine;
  using gui::Painter::drawRect;
  using gui::Painter::drawRuler;
  using gui::Painter::drawString;
  using gui::Painter::setBrush;
  using gui::Painter::setPen;

  Color getPenColor() override { return pen_.color; }
  void setPen(odb::dbTechLayer* layer, bool cosmetic) override;
  void setPen(const Color& color, bool cosmetic, int width) override;
  void setPenWidth(int width) override { pen_.width = width; }

  void setBrush(odb::dbTechLayer* layer, int alpha) override;
  void setBrush(const Color& color, const Brush& style) override;

  void setFont(const Font& font) override;

  void saveState() override;
  void restoreState() override;

  void drawOctagon(const odb::Oct& oct) override;
  void drawRect(const odb::Rect& rect, int round_x, int round_y) override;
  void drawLine(const odb::Point& p1, const odb::Point& p2) override;
  void drawCircle(int x, int y, int r) override;
  void drawX(int x, int y, int size) override;
  void drawPolygon(const odb::Polygon& polygon) override;
  void drawPolygon(const std::vector<odb::Point>& points) override;
  void drawString(int x,
                  int y,
                  Anchor anchor,
                  const std::string& s,
                  bool rotate_90) override;
  odb::Rect stringBoundaries(int x,
                             int y,
                             Anchor anchor,
                             const std::string& s) override;
  void drawRuler(int x0,
                 int y0,
                 int x1,
                 int y1,
                 bool euclidian,
                 const std::string& label) override;

 private:
  struct SavedState
  {
    PenState pen;
    BrushState brush;
    FontState font;
  };

  PenState pen_;
  BrushState brush_;
  FontState font_;
  std::vector<SavedState> state_stack_;
  std::vector<DrawOp> ops_;
};

}  // namespace web
