// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "IOPlacerRenderer.h"

#include <vector>

namespace ppl {

IOPlacerRenderer::IOPlacerRenderer()
    : painting_interval_(0), current_iteration_(0), is_no_pause_mode_(false)
{
  gui::Gui::get()->registerRenderer(this);
}

IOPlacerRenderer::~IOPlacerRenderer()
{
  gui::Gui::get()->unregisterRenderer(this);
}

bool IOPlacerRenderer::isDrawingNeeded() const
{
  return (current_iteration_ == 0
          || (current_iteration_ + 1) % painting_interval_ == 0);
}

void IOPlacerRenderer::setCurrentIteration(const int& current_iteration)
{
  current_iteration_ = current_iteration;
}

void IOPlacerRenderer::setPaintingInterval(const int& painting_interval)
{
  painting_interval_ = painting_interval;
}

void IOPlacerRenderer::setIsNoPauseMode(const bool& is_no_pause_mode)
{
  is_no_pause_mode_ = is_no_pause_mode;
}

void IOPlacerRenderer::setSinks(
    const std::vector<std::vector<InstancePin>>& sinks)
{
  sinks_ = sinks;
}

void IOPlacerRenderer::setPinAssignment(const std::vector<IOPin>& assignment)
{
  pin_assignment_ = assignment;
}

void IOPlacerRenderer::drawObjects(gui::Painter& painter)
{
  painter.setPen(gui::Painter::yellow, true);

  if (isDrawingNeeded()) {
    for (int pin_idx = 0; pin_idx < sinks_.size(); pin_idx++) {
      for (int sink_idx = 0; sink_idx < sinks_[pin_idx].size(); sink_idx++) {
        odb::Point pin_position = pin_assignment_[pin_idx].getPosition();
        odb::Point sink_position = sinks_[pin_idx][sink_idx].getPos();
        painter.drawLine(pin_position, sink_position);
      }
    }
  }
}

void IOPlacerRenderer::redrawAndPause()
{
  if (isDrawingNeeded()) {
    auto* gui = gui::Gui::get();
    gui->redraw();

    int wait_time = is_no_pause_mode_ ? 1000 : 0;  // in milliseconds

    gui->pause(wait_time);
  }
}

}  // namespace ppl
