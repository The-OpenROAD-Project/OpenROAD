// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "CugrRenderer.h"

#include <algorithm>
#include <string>
#include <utility>

#include "AbstractCugrRenderer.h"
#include "grt/GRoute.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace grt {

CugrRenderer::CugrRenderer()
{
  gui::Gui::get()->registerRenderer(this);
}

void CugrRenderer::drawAndPause(CugrDebugFrame frame)
{
  frame_ = std::move(frame);

  auto* gui = gui::Gui::get();
  gui->status("CUGR: " + stageLabel(frame_.stage, frame_.iteration));
  gui->redraw();
  gui->pause();
}

void CugrRenderer::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  const int level = layer->getRoutingLevel();
  // setPen resets the width, so it goes before setPenWidth.
  painter.setPen(layer);
  painter.setBrush(layer);
  painter.setPenWidth(std::max(frame_.gcell_size / 8, 1));

  const int via_half = std::max(frame_.gcell_size / 6, 1);
  for (const GSegment& seg : frame_.route) {
    if (seg.init_layer == seg.final_layer) {
      if (seg.init_layer == level) {
        painter.drawLine(odb::Point(seg.init_x, seg.init_y),
                         odb::Point(seg.final_x, seg.final_y));
      }
    } else if (seg.init_layer == level || seg.final_layer == level) {
      // Mark the transition on both layers it connects.
      painter.drawRect(odb::Rect(seg.init_x - via_half,
                                 seg.init_y - via_half,
                                 seg.init_x + via_half,
                                 seg.init_y + via_half));
    }
  }

  const int pin_radius = std::max(frame_.gcell_size / 3, 1);
  for (const odb::Point3D& pin : frame_.pins) {
    if (pin.z() == level) {
      painter.drawCircle(pin.x(), pin.y(), pin_radius);
    }
  }
}

}  // namespace grt
