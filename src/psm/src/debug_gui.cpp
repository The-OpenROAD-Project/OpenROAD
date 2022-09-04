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

DebugGui::DebugGui(PDNSim* pdnsim) : pdnsim_(pdnsim), bump_layer_(-1)
{
  addDisplayControl("nodes", true);
  addDisplayControl("bumps", true);
  gui::Gui::get()->registerRenderer(this);
}

void DebugGui::drawObjects(gui::Painter& painter)
{
  const bool bumps = checkDisplayControl("bumps");
  if (bumps) {
    painter.setPen(gui::Painter::white, /* cosmetic */ true);
    painter.setBrush(gui::Painter::transparent);
    for (auto& bump : bumps_) {
      painter.drawCircle(bump.x, bump.y, 50000);
    }
  }
}

void DebugGui::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  painter.setPen(gui::Painter::white, /* cosmetic */ true);

  const bool nodes = checkDisplayControl("nodes");
  if (nodes) {
    PDNSim::IRDropByPoint ir_drop;
    pdnsim_->getIRDropForLayer(layer, ir_drop);
    for (auto& [pt, v] : ir_drop) {
      painter.drawCircle(pt.getX(), pt.getY(), 1000);
    }
  }

  const bool bumps = checkDisplayControl("bumps");
  if (bumps && layer->getRoutingLevel() == bump_layer_) {
    for (auto& bump : bumps_) {
      painter.drawCircle(bump.x, bump.y, 5000);
    }
  }
}

void DebugGui::setBumps(const std::vector<IRSolver::BumpData>& bumps,
                        int bump_layer)
{
  bumps_ = bumps;
  bump_layer_ = bump_layer;
}

}  // namespace psm
