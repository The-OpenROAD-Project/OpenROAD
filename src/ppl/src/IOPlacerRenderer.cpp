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

#include "ppl/IOPlacerRenderer.h"

namespace ppl {

IOPlacerRenderer::IOPlacerRenderer()
{
  gui::Gui::get()->registerRenderer(this);
}

void IOPlacerRenderer::setPinAssignment(const std::vector<IOPin>& assignment)
{
  pin_assignment_ = assignment;
}

void IOPlacerRenderer::drawObjects(gui::Painter& painter)
{
  painter.setPen(gui::Painter::yellow);
  painter.setBrush(gui::Painter::yellow);
  painter.setPenWidth(100);

  for(auto pin : pin_assignment_) {
    odb::Point position = pin.getPosition();
    const int x_pin = position.getX();
    const int y_pin = position.getY();
    const int radius = 1000;
    painter.drawCircle(x_pin, y_pin, radius); 
  }

}

void IOPlacerRenderer::redrawAndPause()
{
  auto* gui = gui::Gui::get();
  gui->redraw();
  gui->pause();
}

}