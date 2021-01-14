/////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "graphics.h"

#include <algorithm>
#include <cstdio>
#include <limits>

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
  painter.setPen(gui::Painter::transparent);
  auto color = gui::Painter::yellow;
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
  return gui::Gui::get() != nullptr;
}

}  // namespace fin
