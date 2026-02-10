// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "renderer.h"

#include <string>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "domain.h"
#include "grid.h"
#include "gui/gui.h"
#include "odb/dbTypes.h"
#include "pdn/PdnGen.hh"
#include "shape.h"
#include "straps.h"
#include "via.h"

namespace pdn {

const gui::Painter::Color PDNRenderer::ring_color_
    = gui::Painter::Color(gui::Painter::kRed, 100);
const gui::Painter::Color PDNRenderer::strap_color_
    = gui::Painter::Color(gui::Painter::kCyan, 100);
const gui::Painter::Color PDNRenderer::followpin_color_
    = gui::Painter::Color(gui::Painter::kGreen, 100);
const gui::Painter::Color PDNRenderer::via_color_
    = gui::Painter::Color(gui::Painter::kBlue, 100);
const gui::Painter::Color PDNRenderer::obstruction_color_
    = gui::Painter::Color(gui::Painter::kGray, 100);
const gui::Painter::Color PDNRenderer::repair_color_
    = gui::Painter::Color(gui::Painter::kLightGray, 100);
const gui::Painter::Color PDNRenderer::repair_outline_color_
    = gui::Painter::Color(gui::Painter::kYellow, 100);

PDNRenderer::PDNRenderer(PdnGen* pdn) : pdn_(pdn)
{
  addDisplayControl(grid_obs_text_, false);
  addDisplayControl(initial_obs_text_, false);
  addDisplayControl(obs_text_, false);
  addDisplayControl(vias_text_, true);
  addDisplayControl(followpins_text_, true);
  addDisplayControl(rings_text_, true);
  addDisplayControl(straps_text_, true);
  addDisplayControl(repair_text_, true);

  update();

  gui::Gui::get()->registerRenderer(this);
}

void PDNRenderer::update()
{
  shapes_.clear();
  initial_obstructions_.clear();
  grid_obstructions_.clear();
  vias_.clear();
  repair_.clear();

  if (!pdn_->getDomains().empty()) {
    auto* domain = pdn_->getDomains()[0];
    ShapeVectorMap initial_shapes;
    Grid::makeInitialObstructions(
        domain->getBlock(), initial_shapes, {}, {}, domain->getLogger());
    initial_obstructions_
        = Shape::convertVectorToObstructionTree(initial_shapes);
  }

  ShapeVectorMap shapes;
  ShapeVectorMap obs;
  std::vector<ViaPtr> vias;
  for (const auto& domain : pdn_->getDomains()) {
    for (auto* net : domain->getBlock()->getNets()) {
      Shape::populateMapFromDb(net, obs);
    }

    for (const auto& grid : domain->getGrids()) {
      grid->getGridLevelObstructions(obs);

      for (const auto& [layer, grid_shapes] : grid->getShapes()) {
        shapes[layer].insert(
            shapes[layer].end(), grid_shapes.begin(), grid_shapes.end());
      }

      grid->getVias(vias);

      for (const auto& repair :
           RepairChannelStraps::findRepairChannels(grid.get())) {
        RepairChannel channel;
        channel.source = repair.connect_to;
        channel.target = repair.target->getLayer();
        channel.rect = repair.area;
        channel.available_rect = repair.available_area;
        std::string nets;
        for (auto* net : repair.nets) {
          if (!nets.empty()) {
            nets += ",";
          }
          nets += net->getName();
        }
        channel.text = fmt::format("Repair:{}->{}:{}",
                                   channel.source->getName(),
                                   channel.target->getName(),
                                   nets);

        repair_.push_back(std::move(channel));
      }
    }
  }

  shapes_ = Shape::convertVectorToTree(shapes);
  grid_obstructions_ = Shape::convertVectorToObstructionTree(obs);
  vias_ = Via::convertVectorToTree(vias);

  redraw();
}

void PDNRenderer::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  const double net_name_margin = 0.8;
  const double net_name_increment = 5;

  const int min_shape = 1.0 / painter.getPixelsPerDBU();

  const odb::Rect paint_rect = painter.getBounds();

  if (checkDisplayControl(initial_obs_text_)) {
    painter.setPen(gui::Painter::kHighlight, true);
    painter.setBrush(gui::Painter::kTransparent);
    auto& shapes = initial_obstructions_[layer];
    for (auto it = shapes.qbegin(bgi::intersects(paint_rect));
         it != shapes.qend();
         it++) {
      const auto& shape = *it;
      painter.drawRect(shape->getObstruction());
    }
  }

  if (checkDisplayControl(grid_obs_text_)) {
    painter.setPen(gui::Painter::kHighlight, true);
    painter.setBrush(gui::Painter::kTransparent);
    auto& shapes = grid_obstructions_[layer];
    for (auto it = shapes.qbegin(bgi::intersects(paint_rect));
         it != shapes.qend();
         it++) {
      const auto& shape = *it;
      painter.drawRect(shape->getObstruction());
    }
  }

  const bool show_rings = checkDisplayControl(rings_text_);
  const bool show_followpins = checkDisplayControl(followpins_text_);
  const bool show_straps = checkDisplayControl(straps_text_);
  const bool show_obs = checkDisplayControl(obs_text_);
  auto& shapes = shapes_[layer];
  if (show_rings || show_followpins || show_straps) {
    for (auto it = shapes.qbegin(bgi::intersects(paint_rect));
         it != shapes.qend();
         it++) {
      const auto& shape = *it;

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
          painter.setPenAndBrush(gui::Painter::kHighlight, true);
      }
      const odb::Rect shape_rect = shape->getRect();
      if (shape_rect.minDXDY() < min_shape) {
        continue;
      }

      painter.drawRect(shape_rect);

      if (show_obs) {
        painter.setPen(gui::Painter::kHighlight, true);
        painter.setBrush(gui::Painter::kTransparent);
        painter.drawRect(shape->getObstruction());
      }

      const std::string net_name = shape->getDisplayText();
      const odb::Rect name_box = painter.stringBoundaries(
          0, 0, gui::Painter::Anchor::kBottomLeft, net_name);

      if (shape_rect.dx() * net_name_margin > name_box.dx()
          && shape_rect.dy() * net_name_margin > name_box.dy()) {
        painter.setPen(gui::Painter::kWhite, true);

        if (shape_rect.dx() > shape_rect.dy()) {
          // horizontal
          const int y = 0.5 * (shape_rect.yMin() + shape_rect.yMax());
          const int name_offset = name_box.dx() * net_name_increment / 2;
          for (int x = shape_rect.xMin() + name_offset;
               x < shape_rect.xMax() - name_offset;
               x += net_name_increment * name_box.dx()) {
            if (paint_rect.intersects(odb::Point(x, y))) {
              painter.drawString(x, y, gui::Painter::Anchor::kCenter, net_name);
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
              painter.drawString(x, y, gui::Painter::Anchor::kCenter, net_name);
            }
          }
        }
      }
    }
  }

  if (checkDisplayControl(vias_text_)) {
    for (auto it = vias_.qbegin(bgi::intersects(paint_rect));
         it != vias_.qend();
         it++) {
      const auto& via = *it;
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
          0, 0, gui::Painter::Anchor::kBottomLeft, via_name);
      if (area.dx() * net_name_margin > name_box.dx()
          && area.dy() * net_name_margin > name_box.dy()) {
        painter.setPen(gui::Painter::kWhite, true);
        const int x = 0.5 * (area.xMin() + area.xMax());
        const int y = 0.5 * (area.yMin() + area.yMax());
        painter.drawString(x, y, gui::Painter::Anchor::kCenter, via_name);
      }
    }
  }

  if (checkDisplayControl(repair_text_)) {
    for (const auto& repair : repair_) {
      if (layer == repair.source || layer == repair.target) {
        painter.setPenAndBrush(repair_color_, true);
        painter.drawRect(repair.rect);
        painter.setPenAndBrush(
            repair_outline_color_, true, gui::Painter::kNone);
        painter.drawRect(repair.available_rect);

        const odb::Rect name_box = painter.stringBoundaries(
            0, 0, gui::Painter::Anchor::kBottomLeft, repair.text);
        if (repair.rect.dx() * net_name_margin > name_box.dx()
            && repair.rect.dy() * net_name_margin > name_box.dy()) {
          painter.setPen(gui::Painter::kWhite, true);
          const int x = 0.5 * (repair.rect.xMin() + repair.rect.xMax());
          const int y = 0.5 * (repair.rect.yMin() + repair.rect.yMax());
          painter.drawString(x, y, gui::Painter::Anchor::kCenter, repair.text);
        }
      }
    }
  }
}

void PDNRenderer::drawObjects(gui::Painter& painter)
{
  gui::DiscreteLegend legend;
  legend.addLegendKey(ring_color_, "Ring");
  legend.addLegendKey(strap_color_, "Strap");
  legend.addLegendKey(followpin_color_, "Followpin");
  legend.addLegendKey(via_color_, "Via");
  legend.addLegendKey(obstruction_color_, "Obstruction");
  legend.addLegendKey(repair_color_, "Repair Area");
  legend.addLegendKey(repair_outline_color_, "Repair Area Outline");
  legend.draw(painter);
}

void PDNRenderer::pause()
{
  gui::Gui::get()->pause();
}

}  // namespace pdn
