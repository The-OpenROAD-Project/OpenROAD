/* Author: Matt Liberty */
/*
 * Copyright (c) 2020, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <memory>

#include "frBaseTypes.h"
#include "gui/gui.h"

namespace odb {
class dbDatabase;
}

namespace fr {

class frPoint;
class FlexGridGraph;
class FlexWavefrontGrid;
class FlexDRWorker;
class drNet;
class frDesign;
class frBlockObject;

// This class draws debugging graphics on the layout
class FlexDRGraphics : public gui::Renderer
{
 public:
  // Debug detailed routing
  FlexDRGraphics(frDebugSettings* settings,
                 frDesign* design,
                 odb::dbDatabase* db,
                 Logger* logger);

  void startWorker(FlexDRWorker* worker);

  void startIter(int iter);

  void startNet(drNet* net);

  void endNet(drNet* net);

  void searchNode(const FlexGridGraph* grid_graph,
                  const FlexWavefrontGrid& grid);

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  virtual void drawObjects(gui::Painter& painter) override;
  virtual void drawLayer(odb::dbTechLayer* layer,
                         gui::Painter& painter) override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

  static void init();

  void update();

  void pause(drNet* net);

  void debugWholeDesign();

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
  std::vector<std::vector<frPoint>> points_by_layer_;

  // Names for the custom visibility controls in the gui
  static const char* grid_graph_visible_;
  static const char* route_guides_visible_;
  static const char* routing_objs_visible_;
  static const char* route_shape_cost_visible_;
  static const char* marker_cost_visible_;
  static const char* fixed_shape_cost_visible_;

  void drawMarker(int xl, int yl, int xh, int yh, gui::Painter& painter);
};

}  // namespace fr
