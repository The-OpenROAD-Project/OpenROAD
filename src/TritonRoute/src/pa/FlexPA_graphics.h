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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <memory>
#include <vector>

#include "gui/gui.h"
#include "frBaseTypes.h"

namespace odb {
  class dbDatabase;
}

namespace fr {

class frDesign;
class frPin;
class frInstTerm;
class frBlock;
class frAccessPoint;
class frVia;
class frMarker;

// This class draws debugging graphics on the layout
class FlexPAGraphics : public gui::Renderer
{
 public:
  // Debug pin acess
  FlexPAGraphics(frDebugSettings* settings,
                 frDesign* design,
                 odb::dbDatabase* db,
                 Logger* logger);

  void startPin(frPin* pin, frInstTerm* inst_term);

  void setAPs(const std::vector<std::unique_ptr<frAccessPoint>>& aps,
              frAccessPointEnum lower_type, frAccessPointEnum upper_type);

  void setViaAP(const frAccessPoint* ap, const frVia* via,
                const std::vector<std::unique_ptr<frMarker>>& markers);
  void setMarkersAndShapes(const std::vector<std::unique_ptr<frMarker>>& markers);

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  virtual void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  Logger*          logger_;
  frDebugSettings* settings_;
  gui::Gui*        gui_;
  frPin*           pin_;
  frInstTerm*      inst_term_;
  frBlock*         top_block_;
  std::vector<frAccessPoint> aps_;
  // maps odb layerIdx -> tr layerIdx, with -1 for no equivalent
  std::vector<frLayerNum> layer_map_;
  const frAccessPoint* pa_ap_;
  const frVia* pa_via_;
  const std::vector<std::unique_ptr<frMarker>>* pa_markers_;
  std::vector<std::pair<frBox, frLayerNum>>     shapes;
};

}  // namespace fr
