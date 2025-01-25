// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include <memory>
#include <vector>

#include "frBaseTypes.h"

namespace gui {
class Painter;
}

namespace odb {
class dbDatabase;
class Point;
class dbTechLayer;
}  // namespace odb

namespace drt {

class FlexGridGraph;
class FlexWavefrontGrid;
class FlexDRWorker;
class drNet;
class frDesign;
class frBlockObject;
struct RouterConfiguration;

class AbstractDRGraphics
{
 public:
  virtual void startWorker(FlexDRWorker* worker) = 0;

  virtual void startIter(int iter, RouterConfiguration* router_cfg) = 0;

  virtual void endWorker(int iter) = 0;

  virtual void startNet(drNet* net) = 0;

  virtual void midNet(drNet* net) = 0;

  virtual void endNet(drNet* net) = 0;

  virtual void searchNode(const FlexGridGraph* grid_graph,
                          const FlexWavefrontGrid& grid)
      = 0;

  // Show a message in the status bar
  virtual void status(const std::string& message) = 0;

  // From Renderer API
  virtual void drawObjects(gui::Painter& painter) = 0;
  virtual void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) = 0;
  virtual const char* getDisplayControlGroupName() = 0;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

  virtual void init() = 0;

  virtual void show(bool checkStopConditions) = 0;

  virtual void update() = 0;

  virtual void pause(drNet* net) = 0;

  virtual void debugWholeDesign() = 0;

  virtual void drawObj(frBlockObject* fig, gui::Painter& painter, int layerNum)
      = 0;
};

}  // namespace drt
