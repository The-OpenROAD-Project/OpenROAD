// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "Graphics.hh"

namespace rsz {

Graphics::Graphics()
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::setNet(odb::dbNet* net)
{
  net_ = net;
  if (!net) {
    subdivide_ignore_ = false;
  }
}

void Graphics::stopOnSubdivideStep(const bool stop)
{
  stop_on_subdivide_step_ = stop;
}

void Graphics::subdivideStart(odb::dbNet* net)
{
  lines_.clear();
  if (net_) {
    subdivide_ignore_ = (net != net_);
  }
}

void Graphics::subdivide(const odb::Line& line)
{
  if (subdivide_ignore_) {
    return;
  }
  lines_.emplace_back(line);
  if (stop_on_subdivide_step_) {
    gui::Gui::get()->redraw();
    gui::Gui::get()->pause();
  }
}

void Graphics::subdivideDone()
{
  if (!subdivide_ignore_) {
    gui::Gui::get()->redraw();
    gui::Gui::get()->pause();
  }
}

void Graphics::drawObjects(gui::Painter& painter)
{
  painter.setPen(gui::Painter::red, true);
  for (const odb::Line& line : lines_) {
    painter.drawLine(line.pt0(), line.pt1());
  }
}

}  // namespace rsz
