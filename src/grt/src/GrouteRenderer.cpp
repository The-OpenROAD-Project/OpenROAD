// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "GrouteRenderer.h"

#include "Net.h"

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
          painter.drawCircle(pin.getOnGridPosition().x(),
                             pin.getOnGridPosition().y(),
                             (int) (groute_->getTileSize() / 1.5));
        }
      }
    }

    // draw guides
    NetRouteMap& routes = groute_->getRoutes();
    GRoute& groute = routes[const_cast<odb::dbNet*>(net)];
    for (GSegment& seg : groute) {
      int layer1 = seg.init_layer;
      int layer2 = seg.final_layer;
      if (layer1 != layer2) {
        continue;
      }
      odb::dbTechLayer* seg_layer = tech_->findRoutingLayer(layer1);
      if (seg_layer != layer) {
        continue;
      }
      // Draw rect because drawLine does not have a way to set the pen
      // thickness.
      odb::Rect rect = groute_->globalRoutingToBox(seg);
      painter.drawRect(rect);
    }
  }
}

void GrouteRenderer::clearRoute()
{
  nets_.clear();
  show_pin_locations_.clear();
  redraw();
}

}  // namespace grt
