// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AbstractDRGraphics.h"
#include "frBaseTypes.h"
#include "gui/gui.h"

namespace odb {
class dbDatabase;
class Point;
}  // namespace odb

namespace drt {

class FlexGridGraph;
class FlexWavefrontGrid;
class FlexDRWorker;
class drNet;
class frDesign;
class frBlockObject;
struct RouterConfiguration;

// This class draws debugging graphics on the layout
class FlexDRGraphics : public gui::Renderer, public AbstractDRGraphics
{
 public:
  // Debug detailed routing
  FlexDRGraphics(frDebugSettings* settings,
                 frDesign* design,
                 odb::dbDatabase* db,
                 Logger* logger);

  void startWorker(FlexDRWorker* worker) override;

  void startIter(int iter, RouterConfiguration* router_cfg) override;

  void endWorker(int iter) override;

  void startNet(drNet* net) override;

  void midNet(drNet* net) override;

  void endNet(drNet* net) override;

  void searchNode(const FlexGridGraph* grid_graph,
                  const FlexWavefrontGrid& grid) override;

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;
  void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;
  const char* getDisplayControlGroupName() override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

  void init() override;

  void show(bool checkStopConditions) override;

  void update();

  void pause(drNet* net);

  void debugWholeDesign() override;

  void drawObj(frBlockObject* fig, gui::Painter& painter, int layerNum);

 private:
  FlexDRWorker* worker_;
  const frDesign* design_;
  drNet* net_;
  const FlexGridGraph* grid_graph_;
  frDebugSettings* settings_;
  int current_iter_;
  frLayerNum last_pt_layer_;
  gui::Gui* gui_;
  Logger* logger_;
  int dbu_per_uu_;
  bool drawWholeDesign_ = false;
  // maps odb layerIdx -> tr layerIdx, with -1 for no equivalent
  std::vector<frLayerNum> layer_map_;
  std::vector<std::vector<odb::Point>> points_by_layer_;

  // Names for the custom visibility controls in the gui
  static const char* graph_edges_visible_;
  static const char* grid_cost_edges_visible_;
  static const char* blocked_edges_visible_;
  static const char* route_guides_visible_;
  static const char* routing_objs_visible_;
  static const char* route_shape_cost_visible_;
  static const char* marker_cost_visible_;
  static const char* fixed_shape_cost_visible_;
  static const char* maze_search_visible_;
  static const char* current_net_only_visible_;

  void drawMarker(int xl, int yl, int xh, int yh, gui::Painter& painter);
};

}  // namespace drt
