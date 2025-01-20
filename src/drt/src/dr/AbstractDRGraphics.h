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
