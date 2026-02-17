// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "graphics.h"

#include <cstdio>
#include <string>

#include "boost/polygon/polygon.hpp"
#include "gui/gui.h"
#include "odb/geom.h"
#include "polygon.h"

namespace fin {

Graphics::Graphics()
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::drawPolygon90Set(const Polygon90Set& set)
{
  // It is much faster to decompose the set to rectangles once using boost
  // than trying to have Qt draw the polygons directly.
  polygon_rects_.clear();
  get_rectangles(polygon_rects_, set);
  gui::Gui::get()->redraw();
  gui::Gui::get()->pause();
}

void Graphics::drawObjects(gui::Painter& painter)
{
  painter.setPen(gui::Painter::kTransparent);
  auto color = gui::Painter::kYellow;
  color.a = 180;
  painter.setBrush(color);

  for (auto& rect : polygon_rects_) {
    odb::Rect db_rect(xl(rect), yl(rect), xh(rect), yh(rect));
    painter.drawRect(db_rect);
  }
}

void Graphics::status(const std::string& message)
{
  gui::Gui::get()->status(message);
}

/* static */
bool Graphics::guiActive()
{
  return gui::Gui::enabled();
}

}  // namespace fin
