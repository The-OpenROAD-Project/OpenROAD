// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "graphics.h"

#include "gui/gui.h"

namespace exa {

Graphics::Graphics()
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::makeInstance(odb::dbInst* instance)
{
  instance_ = instance;

  auto gui = gui::Gui::get();

  gui->redraw();

  // This will cause the gui to pause and allow you to
  // zoom/pan/inspect the current state.  This method won't return
  // until the continue button is pressed in the GUI.
  gui->pause();
}

// drawObjects is called to draw any layer independent objects
void Graphics::drawObjects(gui::Painter& painter)
{
  if (!instance_) {
    return;
  }

  painter.setPen(gui::Painter::red);
  painter.setBrush(gui::Painter::red, gui::Painter::Brush::DIAGONAL);
  painter.drawRect(instance_->getBBox()->getBox());
}

/* static */
bool Graphics::guiActive()
{
  return gui::Gui::enabled();
}

}  // namespace exa
