// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "graphics.h"

#include "odb/db.h"
#include "web/core.h"

namespace exa {

Graphics::Graphics()
{
  web::Gui::get()->registerRenderer(this);
}

void Graphics::makeInstance(odb::dbInst* instance)
{
  instance_ = instance;

  auto gui = web::Gui::get();

  gui->redraw();

  // This will cause the gui to pause and allow you to
  // zoom/pan/inspect the current state.  This method won't return
  // until the continue button is pressed in the GUI.
  gui->pause();
}

// drawObjects is called to draw any layer independent objects
void Graphics::drawObjects(web::Painter& painter)
{
  if (!instance_) {
    return;
  }

  painter.setPen(web::Painter::kRed);
  painter.setBrush(web::Painter::kRed, web::Painter::Brush::kDiagonal);
  painter.drawRect(instance_->getBBox()->getBox());
}

/* static */
bool Graphics::guiActive()
{
  return web::Gui::enabled();
}

}  // namespace exa
