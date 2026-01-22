// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "RDLGui.h"

#include <cstddef>
#include <map>
#include <tuple>
#include <vector>

#include "RDLRoute.h"
#include "RDLRouter.h"
#include "gui/gui.h"
#include "odb/geom.h"

namespace pad {

RDLGui::RDLGui()
{
  addDisplayControl(kDrawVertex, true);
  addDisplayControl(kDrawEdge, true);
  addDisplayControl(kDrawObs, true);
  addDisplayControl(kDrawTargets, true);
  addDisplayControl(kDrawFlyWires, true);
  addDisplayControl(kDrawRoutes, true);
  addDisplayControl(kDrawRouteObstructions, true);
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

  const bool draw_obs = draw_detail && checkDisplayControl(kDrawObs);
  gui::Painter::Color obs_color = gui::Painter::kCyan;
  obs_color.a = 127;
  if (draw_obs) {
    painter.setPenAndBrush(obs_color, true);

    for (const auto& [rect, poly, ptr, src] : router_->getObstructions()) {
      painter.drawPolygon(poly);
    }
  }

  const bool draw_vertex = draw_detail && checkDisplayControl(kDrawVertex);
  const bool draw_edge = draw_detail && checkDisplayControl(kDrawEdge);

  std::vector<GridGraph::vertex_descriptor> vertex;
  if (draw_vertex || draw_edge) {
    GridGraph::vertex_iterator v, vend;
    for (boost::tie(v, vend) = boost::vertices(router_->getGraph()); v != vend;
         ++v) {
      const auto find_pt = vertex_map.find(*v);
      if (find_pt == vertex_map.end()) {
        continue;
      }
      const odb::Point& pt = find_pt->second;
      if (box.contains({pt, pt})) {
        vertex.push_back(*v);
      }
    }
  }

  if (draw_vertex) {
    painter.setPenAndBrush(gui::Painter::kRed, true);

    for (const auto& v : vertex) {
      const odb::Point& pt = vertex_map.at(v);
      painter.drawCircle(pt.x(), pt.y(), 100);
    }
  }

  gui::Painter::Color edge_color = gui::Painter::kGreen;
  edge_color.a = 127;
  if (draw_edge) {
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

  const bool draw_flywires = checkDisplayControl(kDrawFlyWires);
  if (draw_flywires) {
    painter.setPenAndBrush(
        gui::Painter::kYellow, true, gui::Painter::Brush::kSolid, 3);

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
        gui::Painter::kRed, true, gui::Painter::Brush::kSolid, 3);
    for (const auto& route : router_->getFailedRoutes()) {
      for (auto* dst : route->getTerminals()) {
        painter.drawLine(route->getTerminal()->getBBox().center(),
                         dst->getBBox().center());
      }
    }
  }

  if (checkDisplayControl(kDrawRoutes)) {
    painter.setPenAndBrush(
        gui::Painter::kGreen, true, gui::Painter::Brush::kSolid, 3);

    for (const auto& [iterm, route] : routes) {
      if (!route->isRouted()) {
        continue;
      }
      const auto& route_pts = route->getRoutePoints();
      for (size_t i = 1; i < route_pts.size(); i++) {
        const odb::Point& src = route_pts[i - 1];
        const odb::Point& dst = route_pts[i];

        painter.drawLine(src, dst);
      }
    }
  }

  if (checkDisplayControl(kDrawTargets)) {
    for (const auto& [net, iterm_targets] : router_->getRoutingTargets()) {
      for (const auto& [iterm, targets] : iterm_targets) {
        for (const auto& target : targets) {
          if (box.intersects(target.shape)) {
            painter.setPenAndBrush(
                gui::Painter::kBlue, true, gui::Painter::Brush::kDiagonal);
            painter.drawRect(target.shape);
            painter.setPenAndBrush(gui::Painter::kBlue, true);
            painter.drawCircle(target.center.x(),
                               target.center.y(),
                               0.05 * target.shape.minDXDY());
          }
        }
      }
    }
  }

  if (checkDisplayControl(kDrawRoutes)) {
    painter.setPenAndBrush(
        gui::Painter::kGreen, true, gui::Painter::Brush::kSolid, 3);

    for (const auto& [iterm, route] : routes) {
      if (!route->isRouted()) {
        continue;
      }
      const auto& route_pts = route->getRoutePoints();
      for (size_t i = 1; i < route_pts.size(); i++) {
        const odb::Point& src = route_pts[i - 1];
        const odb::Point& dst = route_pts[i];

        painter.drawLine(src, dst);
      }
    }
  }

  if (checkDisplayControl(kDrawRouteObstructions)) {
    for (const auto& [iterm, route] : routes) {
      if (!route->isRouted()) {
        continue;
      }
      const auto& route_pts = route->getRoutePoints();
      for (size_t i = 1; i < route_pts.size(); i++) {
        const odb::Point& src = route_pts[i - 1];
        const odb::Point& dst = route_pts[i];

        painter.setPenAndBrush(
            gui::Painter::kGreen, true, gui::Painter::Brush::kNone, 2);
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

  painter.setPenAndBrush(snap_color_, true, gui::Painter::Brush::kSolid, 2);
  for (const auto& [pt0, pt1] : snap_) {
    painter.drawLine(pt0, pt1);
  }

  gui::DiscreteLegend legend;
  legend.addLegendKey(gui::Painter::kRed, "Vertex");
  legend.addLegendKey(edge_color, "Edge");
  legend.addLegendKey(obs_color, "Obstruction");
  legend.addLegendKey(gui::Painter::kBlue, "Target");
  legend.addLegendKey(gui::Painter::kYellow, "Flywire");
  legend.addLegendKey(gui::Painter::kGreen, "Route");
  legend.draw(painter);
}

void RDLGui::addSnap(const odb::Point& pt0, const odb::Point& pt1)
{
  snap_.emplace(pt0, pt1);
}

void RDLGui::zoomToSnap(bool preview)
{
  if (snap_.empty()) {
    return;
  }

  odb::Rect zoom;
  zoom.mergeInit();
  for (const auto& [p0, p1] : snap_) {
    zoom.merge(odb::Rect(p0, p1));
  }

  if (preview) {
    snap_color_ = gui::Painter::kGray;
  } else {
    snap_color_ = gui::Painter::kWhite;
  }

  odb::Rect zoomto;
  zoom.bloat(zoom.maxDXDY(), zoomto);
  gui::Gui::get()->zoomTo(zoomto);
}

void RDLGui::clearSnap()
{
  snap_.clear();
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
    gui::Gui::get()->pause(kGuiTimeout);
  } else {
    gui::Gui::get()->pause();
  }
}

}  // namespace pad
