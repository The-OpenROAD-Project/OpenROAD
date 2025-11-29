// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "graphics.h"

#include "gui/gui.h"
#include "odb/db.h"

namespace cgv {

Graphics::Graphics()
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::addEdges(std::vector<std::pair<odb::Point, odb::Point>> edges)
{
  edges_.insert(edges_.end(), edges.begin(), edges.end());

  auto gui = gui::Gui::get();

  gui->redraw();
}

// drawObjects is called to draw any layer independent objects
void Graphics::drawObjects(gui::Painter& painter)
{

  painter.setPen(gui::Painter::kGreen);
  painter.setBrush(gui::Painter::kGreen, gui::Painter::Brush::kDiagonal);
  
  for (auto e : edges_) {
    painter.drawLine(e.first, e.second);
    painter.drawX(e.first.x(), e.first.y(), 8);
  }
}

/* static */
bool Graphics::guiActive()
{
  return gui::Gui::enabled();
}

}  // namespace cgv
