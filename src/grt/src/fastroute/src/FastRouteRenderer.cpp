// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "FastRouteRenderer.h"

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
      painter.setPen(gui::Painter::cyan);
      painter.setBrush(gui::Painter::cyan);
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
    const std::vector<short>& gridsX = treeEdge.route.gridsX;
    const std::vector<short>& gridsY = treeEdge.route.gridsY;
    const std::vector<short>& gridsL = treeEdge.route.gridsL;
    int lastX = tile_size_ * (gridsX[0] + 0.5) + x_corner_;
    int lastY = tile_size_ * (gridsY[0] + 0.5) + y_corner_;

    if (is3DVisualization_) {
      lastL = gridsL[0];
    }

    for (int i = 1; i <= routeLen; i++) {
      const int xreal = tile_size_ * (gridsX[i] + 0.5) + x_corner_;
      const int yreal = tile_size_ * (gridsY[i] + 0.5) + y_corner_;

      if (is3DVisualization_) {
        drawLineObject(
            lastX, lastY, lastL + 1, xreal, yreal, gridsL[i] + 1, painter);
        lastL = gridsL[i];
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
    painter.setPen(gui::Painter::white);
    painter.setBrush(gui::Painter::white);
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
