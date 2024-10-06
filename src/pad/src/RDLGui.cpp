/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
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

#include "RDLGui.h"

#include "RDLRoute.h"
#include "RDLRouter.h"

namespace pad {

RDLGui::RDLGui()
{
  addDisplayControl(draw_vertex_, true);
  addDisplayControl(draw_edge_, true);
  addDisplayControl(draw_obs_, true);
  addDisplayControl(draw_targets_, true);
  addDisplayControl(draw_fly_wires_, true);
  addDisplayControl(draw_routes_, true);
  addDisplayControl(draw_route_obstructions_, true);
}

RDLGui::~RDLGui()
{
  if (router_ != nullptr) {
    router_->setRDLGui(nullptr);
  }
}

void RDLGui::drawObjects(gui::Painter& painter)
{
  if (router_ == nullptr) {
    return;
  }
  const bool draw_detail = painter.getPixelsPerDBU() * 1000 >= 1;

  const odb::Rect box = painter.getBounds();

  const auto& vertex_map = router_->getVertexMap();

  std::map<odb::dbITerm*, RDLRoute*> routes;
  for (const auto& route : router_->getRoutes()) {
    routes[route->getTerminal()] = route.get();
  }

  const bool draw_obs = draw_detail && checkDisplayControl(draw_obs_);
  if (draw_obs) {
    gui::Painter::Color obs_color = gui::Painter::cyan;
    obs_color.a = 127;
    painter.setPenAndBrush(obs_color, true);

    for (const auto& [rect, poly, ptr] : router_->getObstructions()) {
      painter.drawPolygon(poly);
    }
  }

  const bool draw_vertex = draw_detail && checkDisplayControl(draw_vertex_);
  const bool draw_edge = draw_detail && checkDisplayControl(draw_edge_);

  std::vector<GridGraph::vertex_descriptor> vertex;
  if (draw_vertex || draw_edge) {
    GridGraph::vertex_iterator v, vend;
    for (boost::tie(v, vend) = boost::vertices(router_->getGraph()); v != vend;
         ++v) {
      const odb::Point& pt = vertex_map.at(*v);
      if (box.contains({pt, pt})) {
        vertex.push_back(*v);
      }
    }
  }

  if (draw_vertex) {
    painter.setPenAndBrush(gui::Painter::red, true);

    for (const auto& v : vertex) {
      const odb::Point& pt = vertex_map.at(v);
      painter.drawCircle(pt.x(), pt.y(), 100);
    }
  }

  if (draw_edge) {
    gui::Painter::Color edge_color = gui::Painter::green;
    edge_color.a = 127;
    painter.setPenAndBrush(edge_color, true);

    for (const auto& v : vertex) {
      GridGraph::out_edge_iterator eit, eend;
      std::tie(eit, eend) = boost::out_edges(v, router_->getGraph());
      for (; eit != eend; eit++) {
        const odb::Point& pt0 = vertex_map.at(eit->m_source);
        const odb::Point& pt1 = vertex_map.at(eit->m_target);
        painter.drawLine(pt0, pt1);
      }
    }
  }

  const bool draw_flywires = checkDisplayControl(draw_fly_wires_);
  if (draw_flywires) {
    painter.setPenAndBrush(
        gui::Painter::yellow, true, gui::Painter::Brush::SOLID, 3);

    const auto& targets = router_->getRoutingTargets();

    for (const auto& [iterm, route] : routes) {
      if (route->isFailed() || route->isRouted()) {
        continue;
      }

      const auto& net_targets = targets.at(iterm->getNet());

      odb::dbITerm* dst_iterm = route->peakNextTerminal();
      const auto& src = net_targets.at(iterm);
      const auto& dst = net_targets.at(dst_iterm);
      painter.drawLine(src[0].center, dst[0].center);
    }

    painter.setPenAndBrush(
        gui::Painter::red, true, gui::Painter::Brush::SOLID, 3);
    for (const auto& route : router_->getFailedRoutes()) {
      for (auto* dst : route->getTerminals()) {
        painter.drawLine(route->getTerminal()->getBBox().center(),
                         dst->getBBox().center());
      }
    }
  }

  if (checkDisplayControl(draw_routes_)) {
    painter.setPenAndBrush(
        gui::Painter::green, true, gui::Painter::Brush::SOLID, 3);

    for (const auto& [iterm, route] : routes) {
      if (!route->isRouted()) {
        continue;
      }
      const auto& verticies = route->getRouteVerticies();
      for (size_t i = 1; i < verticies.size(); i++) {
        const odb::Point& src = vertex_map.at(verticies.at(i - 1));
        const odb::Point& dst = vertex_map.at(verticies.at(i));

        painter.drawLine(src, dst);
      }
    }
  }

  if (checkDisplayControl(draw_targets_)) {
    for (const auto& [net, iterm_targets] : router_->getRoutingTargets()) {
      for (const auto& [iterm, targets] : iterm_targets) {
        for (const auto& target : targets) {
          if (box.intersects(target.shape)) {
            painter.setPenAndBrush(
                gui::Painter::blue, true, gui::Painter::Brush::DIAGONAL);
            painter.drawRect(target.shape);
            painter.setPenAndBrush(gui::Painter::blue, true);
            painter.drawCircle(target.center.x(),
                               target.center.y(),
                               0.05 * target.shape.minDXDY());
          }
        }
      }
    }
  }

  if (checkDisplayControl(draw_routes_)) {
    painter.setPenAndBrush(
        gui::Painter::green, true, gui::Painter::Brush::SOLID, 3);

    for (const auto& [iterm, route] : routes) {
      if (!route->isRouted()) {
        continue;
      }
      const auto& verticies = route->getRouteVerticies();
      for (size_t i = 1; i < verticies.size(); i++) {
        const odb::Point& src = vertex_map.at(verticies.at(i - 1));
        const odb::Point& dst = vertex_map.at(verticies.at(i));

        painter.drawLine(src, dst);
      }
    }
  }

  if (checkDisplayControl(draw_route_obstructions_)) {
    for (const auto& [iterm, route] : routes) {
      if (!route->isRouted()) {
        continue;
      }
      const auto& verticies = route->getRouteVerticies();
      for (size_t i = 1; i < verticies.size(); i++) {
        const odb::Point& src = vertex_map.at(verticies.at(i - 1));
        const odb::Point& dst = vertex_map.at(verticies.at(i));

        painter.setPenAndBrush(
            gui::Painter::green, true, gui::Painter::Brush::NONE, 2);
        if (i == 1) {
          painter.drawRect(router_->getPointObstruction(src));
        }
        painter.drawRect(router_->getPointObstruction(dst));
        if (router_->is45DegreeEdge(src, dst)) {
          painter.drawPolygon(router_->getEdgeObstruction(src, dst));
        }
      }
    }
  }
}

void RDLGui::setRouter(RDLRouter* router)
{
  router_ = router;
  if (router_) {
    router_->setRDLGui(this);
  }
}

void RDLGui::pause(bool timeout) const
{
  gui::Gui::get()->redraw();

  if (timeout) {
    gui::Gui::get()->pause(gui_timeout_);
  } else {
    gui::Gui::get()->pause();
  }
}

}  // namespace pad
