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

#include "FlexTA.h"
#include "frBaseTypes.h"
#include "gui/gui.h"

namespace odb {
class dbDatabase;
class dbTechLayer;
}  // namespace odb

namespace drt {

class frDesign;
class frNet;

// This class draws debugging graphics on the layout
class FlexTAGraphics : public gui::Renderer
{
 public:
  // Debug track allocation
  FlexTAGraphics(frDebugSettings* settings,
                 frDesign* design,
                 odb::dbDatabase* db);

  // Show a message in the status bar
  void status(const std::string& message);

  // Draw iroutes for one guide
  void drawIrouteGuide(frNet* net,
                       odb::dbTechLayer* layer,
                       gui::Painter& painter);

  // From Renderer API
  void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;

  // Update status and optionally pause
  void endIter(int iter);

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  frDebugSettings* settings_;
  gui::Gui* gui_;
  frBlock* top_block_;
  // maps odb layerIdx -> tr layerIdx, with -1 for no equivalent
  std::vector<frLayerNum> layer_map_;
  frNet* net_;
};

}  // namespace drt
