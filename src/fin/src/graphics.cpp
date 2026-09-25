// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "graphics.h"

#include <cstdio>
#include <string>

#include "boost/polygon/polygon.hpp"
#include "odb/geom.h"
#include "polygon.h"
#include "web/core.h"

namespace fin {

Graphics::Graphics()
{
  web::Gui::get()->registerRenderer(this);
}

void Graphics::drawPolygon90Set(const Polygon90Set& set)
{
  // It is much faster to decompose the set to rectangles once using boost
  // than trying to have Qt draw the polygons directly.
  polygon_rects_.clear();
  get_rectangles(polygon_rects_, set);
  web::Gui::get()->redraw();
  web::Gui::get()->pause();
}

void Graphics::drawObjects(web::Painter& painter)
{
  painter.setPen(web::Painter::kTransparent);
  auto color = web::Painter::kYellow;
  color.a = 180;
  painter.setBrush(color);

  for (auto& rect : polygon_rects_) {
    odb::Rect db_rect(xl(rect), yl(rect), xh(rect), yh(rect));
    painter.drawRect(db_rect);
  }
}

void Graphics::status(const std::string& message)
{
  web::Gui::get()->status(message);
}

/* static */
bool Graphics::guiActive()
{
  return web::Gui::enabled();
}

}  // namespace fin
