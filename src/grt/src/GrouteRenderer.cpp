// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "GrouteRenderer.h"

#include "Net.h"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace grt {

GrouteRenderer::GrouteRenderer(GlobalRouter* groute, odb::dbTech* tech)
    : groute_(groute), tech_(tech)
{
  gui::Gui::get()->registerRenderer(this);
}

void GrouteRenderer::highlightRoute(odb::dbNet* net, bool show_pin_locations)
{
  nets_.insert(net);
  show_pin_locations_[net] = show_pin_locations;
  redraw();
}

void GrouteRenderer::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  painter.setPen(layer);
  painter.setBrush(layer);

  for (odb::dbNet* net : nets_) {
    Net* gr_net = groute_->getNet(net);
    if (show_pin_locations_[net]) {
      // draw on grid pin locations
      for (const Pin& pin : gr_net->getPins()) {
        if (pin.getConnectionLayer() == layer->getRoutingLevel()) {
          float circle_factor = show_segments_[net] ? 8 : 1.5;
          painter.drawCircle(pin.getOnGridPosition().x(),
                             pin.getOnGridPosition().y(),
                             (int) (groute_->getTileSize() / circle_factor));
        }
      }
    }

    // draw guides
    NetRouteMap& routes = groute_->getRoutes();
    GRoute& groute = routes[net];
    for (GSegment& seg : groute) {
      int layer1 = seg.init_layer;
      int layer2 = seg.final_layer;
      if (layer1 != layer2) {
        odb::dbTechLayer* via_layer1 = tech_->findRoutingLayer(layer1);
        odb::dbTechLayer* via_layer2 = tech_->findRoutingLayer(layer2);
        if (via_layer1 == layer) {
          drawViaRect(seg, via_layer1, painter);
        } else if (via_layer2 == layer) {
          drawViaRect(seg, via_layer2, painter);
        } else {
          continue;
        }
      }
      odb::dbTechLayer* seg_layer = tech_->findRoutingLayer(layer1);
      if (seg_layer != layer) {
        continue;
      }
      odb::Point pt1(seg.init_x, seg.init_y);
      odb::Point pt2(seg.final_x, seg.final_y);
      painter.setPenWidth(layer->getMinWidth() * 2);
      painter.drawLine(pt1, pt2);
    }
  }
}

void GrouteRenderer::clearRoute()
{
  nets_.clear();
  show_pin_locations_.clear();
  redraw();
}

void GrouteRenderer::drawViaRect(const GSegment& seg,
                                 odb::dbTechLayer* layer,
                                 gui::Painter& painter)
{
  int width = layer->getMinWidth() * 2;
  odb::Point ll(seg.init_x - width, seg.init_y - width);
  odb::Point ur(seg.init_x + width, seg.init_y + width);
  odb::Rect via_rect(ll, ur);
  painter.drawRect(via_rect);
}

}  // namespace grt
