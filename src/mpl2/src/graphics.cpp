///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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
#include "graphics.h"

#include "object.h"
#include "utl/Logger.h"

namespace mpl2 {

Graphics::Graphics(int dbu, utl::Logger* logger) : dbu_(dbu), logger_(logger)
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::saStep(const std::vector<SoftMacro>& macros)
{
  soft_macros_ = macros;
  hard_macros_.clear();

  gui::Gui::get()->status("SoftMacro");
  gui::Gui::get()->redraw();
  gui::Gui::get()->pause();
}

void Graphics::saStep(const std::vector<HardMacro>& macros)
{
  hard_macros_ = macros;
  soft_macros_.clear();

  gui::Gui::get()->status("HardMacro");
  gui::Gui::get()->redraw();
  gui::Gui::get()->pause();
}

void Graphics::drawObjects(gui::Painter& painter)
{
  painter.setPen(gui::Painter::yellow, true);
  painter.setBrush(gui::Painter::gray);

  int i = 0;
  for (const auto& macro : soft_macros_) {
    const int lx = dbu_ * macro.getX();
    const int ly = dbu_ * macro.getY();
    const int ux = dbu_ * (lx + macro.getWidth());
    const int uy = dbu_ * (ly + macro.getHeight());
    odb::Rect bbox(lx, ly, ux, uy);

    painter.drawRect(bbox);
    painter.drawString(bbox.xCenter(),
                       bbox.yCenter(),
                       gui::Painter::CENTER,
                       std::to_string(i++));
  }

  i = 0;
  for (const auto& macro : hard_macros_) {
    const int lx = dbu_ * macro.getX();
    const int ly = dbu_ * macro.getY();
    const int ux = dbu_ * (macro.getX() + macro.getWidth());
    const int uy = dbu_ * (macro.getY() + macro.getHeight());
    odb::Rect bbox(lx, ly, ux, uy);

    painter.drawRect(bbox);
    painter.drawString(bbox.xCenter(),
                       bbox.yCenter(),
                       gui::Painter::CENTER,
                       std::to_string(i++));
  }
}

}  // namespace mpl2
