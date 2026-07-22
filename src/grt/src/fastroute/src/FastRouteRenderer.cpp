// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "FastRouteRenderer.h"

#include <cmath>
#include <vector>

#include "AbstractFastRouteRenderer.h"
#include "DataType.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "stt/SteinerTreeBuilder.h"

namespace grt {

FastRouteRenderer::FastRouteRenderer(odb::dbTech* tech)
    : treeStructure_(TreeStructure::steinerTreeByStt),
      is3DVisualization_(false),
      tech_(tech),
      tile_size_(0),
      x_corner_(0),
      y_corner_(0)
{
  gui::Gui::get()->registerRenderer(this);
}

void FastRouteRenderer::setGridVariables(int tile_size,
                                         int x_corner,
                                         int y_corner)
{
  tile_size_ = tile_size;
  x_corner_ = x_corner;
  y_corner_ = y_corner;
}

void FastRouteRenderer::redrawAndPause()
{
  auto* gui = gui::Gui::get();
  gui->redraw();
  gui->pause();
}

void FastRouteRenderer::setTreeStructure(TreeStructure treeStructure)
{
  treeStructure_ = treeStructure;
}
void FastRouteRenderer::highlight(const FrNet* net)
{
  pinX_ = net->getPinX();
  pinY_ = net->getPinY();
  pinL_ = net->getPinL();
}
void FastRouteRenderer::setSteinerTree(const stt::Tree& stree)
{
  stree_ = stree;
}

void FastRouteRenderer::setStTreeValues(const StTree& stree)
{
  treeEdges_.clear();
  const int num_edges = stree.num_edges();
  for (int edgeID = 0; edgeID < num_edges; edgeID++) {
    treeEdges_.push_back(stree.edges[edgeID]);
  }
}
void FastRouteRenderer::setIs3DVisualization(bool is3DVisualization)
{
  is3DVisualization_ = is3DVisualization;
}

void FastRouteRenderer::drawLineObject(int x1,
                                       int y1,
                                       int layer1,
                                       int x2,
                                       int y2,
                                       int layer2,
                                       gui::Painter& painter)
{
  if (layer1 == layer2) {
    if (is3DVisualization_) {
      odb::dbTechLayer* layer = tech_->findRoutingLayer(layer1);
      painter.setPen(layer);
      painter.setBrush(layer);
    } else {
      painter.setPen(gui::Painter::kCyan);
      painter.setBrush(gui::Painter::kCyan);
    }
    painter.setPenWidth(700);
    painter.drawLine(x1, y1, x2, y2);
  }
}
void FastRouteRenderer::drawTreeEdges(gui::Painter& painter)
{
  int lastL = 0;
  for (const TreeEdge& treeEdge : treeEdges_) {
    if (treeEdge.len == 0) {
      continue;
    }

    int routeLen = treeEdge.route.routelen;
    const std::vector<GPoint3D>& grids = treeEdge.route.grids;
    int lastX = tile_size_ * (grids[0].x + 0.5) + x_corner_;
    int lastY = tile_size_ * (grids[0].y + 0.5) + y_corner_;

    if (is3DVisualization_) {
      lastL = grids[0].layer;
    }

    for (int i = 1; i <= routeLen; i++) {
      const int xreal = tile_size_ * (grids[i].x + 0.5) + x_corner_;
      const int yreal = tile_size_ * (grids[i].y + 0.5) + y_corner_;

      if (is3DVisualization_) {
        drawLineObject(
            lastX, lastY, lastL + 1, xreal, yreal, grids[i].layer + 1, painter);
        lastL = grids[i].layer;
      } else {
        drawLineObject(
            lastX, lastY, -1, xreal, yreal, -1, painter);  // -1 to 2D Trees
      }
      lastX = xreal;
      lastY = yreal;
    }
  }
}
void FastRouteRenderer::drawCircleObjects(gui::Painter& painter)
{
  painter.setPenWidth(700);
  for (auto i = 0; i < pinX_.size(); i++) {
    const int xreal = tile_size_ * (pinX_[i] + 0.5) + x_corner_;
    const int yreal = tile_size_ * (pinY_[i] + 0.5) + y_corner_;

    odb::dbTechLayer* layer = tech_->findRoutingLayer(pinL_[i] + 1);
    painter.setPen(layer);
    painter.setBrush(layer);
    painter.drawCircle(xreal, yreal, 1500);
  }
}

void FastRouteRenderer::drawObjects(gui::Painter& painter)
{
  if (treeStructure_ == TreeStructure::steinerTreeByStt) {
    painter.setPen(gui::Painter::kWhite);
    painter.setBrush(gui::Painter::kWhite);
    painter.setPenWidth(700);

    for (int i = 0; i < stree_.branchCount(); i++) {
      const int x1 = tile_size_ * (stree_.branch[i].x + 0.5) + x_corner_;
      const int y1 = tile_size_ * (stree_.branch[i].y + 0.5) + y_corner_;
      const int n = stree_.branch[i].n;
      const int x2 = tile_size_ * (stree_.branch[n].x + 0.5) + x_corner_;
      const int y2 = tile_size_ * (stree_.branch[n].y + 0.5) + y_corner_;
      const int len = abs(x1 - x2) + abs(y1 - y2);
      if (len > 0) {
        painter.drawLine(x1, y1, x2, y2);
      }
    }

    drawCircleObjects(painter);
  } else if (treeStructure_ == TreeStructure::steinerTreeByFastroute) {
    drawTreeEdges(painter);

    drawCircleObjects(painter);
  }
}

}  // namespace grt
