//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#include "renderer.h"

#include "domain.h"
#include "grid.h"
#include "pdn/PdnGen.hh"

namespace pdn {

const gui::Painter::Color PDNRenderer::ring_color_
    = gui::Painter::Color(gui::Painter::red, 100);
const gui::Painter::Color PDNRenderer::strap_color_
    = gui::Painter::Color(gui::Painter::cyan, 100);
const gui::Painter::Color PDNRenderer::followpin_color_
    = gui::Painter::Color(gui::Painter::green, 100);
const gui::Painter::Color PDNRenderer::via_color_
    = gui::Painter::Color(gui::Painter::blue, 100);
const gui::Painter::Color PDNRenderer::obstruction_color_
    = gui::Painter::Color(gui::Painter::gray, 100);

PDNRenderer::PDNRenderer(PdnGen* pdn) : pdn_(pdn)
{
  addDisplayControl(obs_text_, false);
  addDisplayControl(vias_text_, true);
  addDisplayControl(followpins_text_, true);
  addDisplayControl(rings_text_, true);
  addDisplayControl(straps_text_, true);

  update();

  gui::Gui::get()->registerRenderer(this);
}

void PDNRenderer::update()
{
  shapes_.clear();
  vias_.clear();

  for (const auto& domain : pdn_->getDomains()) {
    for (const auto& grid : domain->getGrids()) {
      for (const auto& [layer, shapes] : grid->getShapes()) {
        auto& save_shapes = shapes_[layer];
        for (const auto shape : shapes) {
          save_shapes.insert(shape);
        }
      }

      std::vector<ViaPtr> vias;
      grid->getVias(vias);
      for (const auto& via : vias) {
        vias_.insert({via->getBox(), via});
      }
    }
  }

  redraw();
}

void PDNRenderer::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  const double net_name_margin = 0.8;
  const double net_name_increment = 5;

  const int min_shape = 1.0 / painter.getPixelsPerDBU();

  const odb::Rect paint_rect = painter.getBounds();
  Box paint_box(Point(paint_rect.xMin(), paint_rect.yMin()),
                Point(paint_rect.xMax(), paint_rect.yMax()));

  const bool show_rings = checkDisplayControl(rings_text_);
  const bool show_followpins = checkDisplayControl(followpins_text_);
  const bool show_straps = checkDisplayControl(straps_text_);
  const bool show_obs = checkDisplayControl(obs_text_);
  auto& shapes = shapes_[layer];
  if (show_rings || show_followpins || show_straps) {
    for (auto it = shapes.qbegin(bgi::intersects(paint_box));
         it != shapes.qend();
         it++) {
      const auto& shape = it->second;

      if (shape->getLayer() != layer) {
        continue;
      }

      switch (shape->getType()) {
        case odb::dbWireShapeType::RING:
          if (!show_rings) {
            continue;
          }
          painter.setPenAndBrush(ring_color_, true);
          break;
        case odb::dbWireShapeType::STRIPE:
          if (!show_straps) {
            continue;
          }
          painter.setPenAndBrush(strap_color_, true);
          break;
        case odb::dbWireShapeType::FOLLOWPIN:
          if (!show_followpins) {
            continue;
          }
          painter.setPenAndBrush(followpin_color_, true);
          break;
        default:
          painter.setPenAndBrush(gui::Painter::highlight, true);
      }
      const odb::Rect shape_rect = shape->getRect();
      if (shape_rect.minDXDY() < min_shape) {
        continue;
      }

      painter.drawRect(shape_rect);

      if (show_obs) {
        painter.setPen(gui::Painter::highlight, true);
        painter.setBrush(gui::Painter::transparent);
        painter.drawRect(shape->getObstruction());
      }

      const std::string net_name = shape->getDisplayText();
      const odb::Rect name_box = painter.stringBoundaries(
          0, 0, gui::Painter::Anchor::BOTTOM_LEFT, net_name);

      if (shape_rect.dx() * net_name_margin > name_box.dx()
          && shape_rect.dy() * net_name_margin > name_box.dy()) {
        painter.setPen(gui::Painter::white, true);

        if (shape_rect.dx() > shape_rect.dy()) {
          // horizontal
          const int y = 0.5 * (shape_rect.yMin() + shape_rect.yMax());
          const int name_offset = name_box.dx() * net_name_increment / 2;
          for (int x = shape_rect.xMin() + name_offset;
               x < shape_rect.xMax() - name_offset;
               x += net_name_increment * name_box.dx()) {
            if (paint_rect.intersects(odb::Point(x, y))) {
              painter.drawString(x, y, gui::Painter::Anchor::CENTER, net_name);
            }
          }
        } else {
          // vertical
          const int x = 0.5 * (shape_rect.xMin() + shape_rect.xMax());
          const int name_offset = name_box.dy() * net_name_increment / 2;
          for (int y = shape_rect.yMin() + name_offset;
               y < shape_rect.yMax() - name_offset;
               y += net_name_increment * name_box.dy()) {
            if (paint_rect.intersects(odb::Point(x, y))) {
              painter.drawString(x, y, gui::Painter::Anchor::CENTER, net_name);
            }
          }
        }
      }
    }
  }

  if (checkDisplayControl(vias_text_)) {
    for (auto it = vias_.qbegin(bgi::intersects(paint_box)); it != vias_.qend();
         it++) {
      const auto& via = it->second;
      if (layer->getNumber() < via->getLowerLayer()->getNumber()
          || layer->getNumber() > via->getUpperLayer()->getNumber()) {
        continue;
      }

      const odb::Rect& area = via->getArea();
      if (area.minDXDY() < min_shape) {
        continue;
      }

      painter.setPenAndBrush(via_color_, true);
      painter.drawRect(area);

      const std::string via_name = via->getDisplayText();
      const odb::Rect name_box = painter.stringBoundaries(
          0, 0, gui::Painter::Anchor::BOTTOM_LEFT, via_name);
      if (area.dx() * net_name_margin > name_box.dx()
          && area.dy() * net_name_margin > name_box.dy()) {
        painter.setPen(gui::Painter::white, true);
        const int x = 0.5 * (area.xMin() + area.xMax());
        const int y = 0.5 * (area.yMin() + area.yMax());
        painter.drawString(x, y, gui::Painter::Anchor::CENTER, via_name);
      }
    }
  }
}

void PDNRenderer::drawObjects(gui::Painter& painter)
{
}

}  // namespace pdn
