/*
 * Copyright (c) 2022, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "debug_gui.h"

#include "psm/pdnsim.h"

namespace psm {

DebugGui::DebugGui(PDNSim* pdnsim) : pdnsim_(pdnsim)
{
  addDisplayControl(nodes_text_, true);
  addDisplayControl(source_text_, true);
  gui::Gui::get()->registerRenderer(this);
}

void DebugGui::drawObjects(gui::Painter& painter)
{
  if (checkDisplayControl(source_text_)) {
    painter.setPen(gui::Painter::white, /* cosmetic */ true);
    painter.setBrush(gui::Painter::transparent);
    for (auto& source : sources_) {
      painter.drawCircle(source.x, source.y, 10 * source.size);
    }
  }
}

void DebugGui::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  const int nodesize = 1000;

  painter.setPen(gui::Painter::white, /* cosmetic */ true);

  const odb::Rect test_box = painter.stringBoundaries(
      0, 0, gui::Painter::Anchor::BOTTOM_LEFT, "1.000V");
  const bool add_text = test_box.minDXDY() < nodesize;

  const bool nodes = checkDisplayControl(nodes_text_);
  if (nodes) {
    PDNSim::IRDropByPoint ir_drop;
    pdnsim_->getIRDropForLayer(layer, ir_drop);
    for (auto& [pt, v] : ir_drop) {
      painter.drawCircle(pt.getX(), pt.getY(), nodesize);
      if (add_text) {
        painter.drawString(pt.getX(),
                           pt.getY(),
                           gui::Painter::Anchor::CENTER,
                           fmt::format("{:.3f}V", v));
      }
    }
  }

  if (checkDisplayControl(source_text_)) {
    for (auto& source : sources_) {
      if (layer->getRoutingLevel() == source.layer) {
        painter.drawCircle(source.x, source.y, source.size);
      }
    }
  }
}

void DebugGui::setSources(const std::vector<IRSolver::SourceData>& sources)
{
  sources_ = sources;
}

}  // namespace psm
