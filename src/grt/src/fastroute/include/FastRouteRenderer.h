// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "AbstractFastRouteRenderer.h"
#include "FastRoute.h"
#include "gui/gui.h"

namespace grt {

class FastRouteRenderer : public gui::Renderer, public AbstractFastRouteRenderer
{
 public:
  FastRouteRenderer(odb::dbTech* tech);
  void setGridVariables(int tile_size, int x_corner, int y_corner) override;
  void highlight(const FrNet* net) override;
  void setSteinerTree(const stt::Tree& stree) override;
  void setStTreeValues(const StTree& stree) override;
  void setIs3DVisualization(bool is3DVisualization) override;
  void setTreeStructure(TreeStructure treeStructure) override;

  void redrawAndPause() override;

  void drawObjects(gui::Painter& /* painter */) override;

 private:
  void drawTreeEdges(gui::Painter& painter);
  void drawCircleObjects(gui::Painter& painter);
  void drawLineObject(int x1,
                      int y1,
                      int layer1,
                      int x2,
                      int y2,
                      int layer2,
                      gui::Painter& painter);

  TreeStructure treeStructure_;

  // Steiner Tree by stt
  stt::Tree stree_;

  // Steiner tree by fastroute
  std::vector<TreeEdge> treeEdges_;
  bool is3DVisualization_;

  // net data of pins
  std::vector<int> pinX_;  // array of X coordinates of pins
  std::vector<int> pinY_;  // array of Y coordinates of pins
  std::vector<int> pinL_;  // array of L coordinates of pins

  odb::dbTech* tech_;
  int tile_size_, x_corner_, y_corner_;
};

}  // namespace grt
