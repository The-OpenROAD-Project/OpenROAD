/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#include "IOPlacerRenderer.h"

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