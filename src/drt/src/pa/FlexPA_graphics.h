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

#include "FlexPA.h"
#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"
#include "gui/gui.h"

namespace odb {
class dbDatabase;
class dbTechLayer;
}  // namespace odb

namespace drt {

class frDesign;
class frPin;
class frBPin;
class frInstTerm;
class frBlock;
class frMaster;
class frAccessPoint;
class frVia;
class frMarker;
class frInst;
class frPathSeg;
class frConnFig;

// This class draws debugging graphics on the layout
class FlexPAGraphics : public gui::Renderer
{
 public:
  // Debug pin access
  FlexPAGraphics(frDebugSettings* settings,
                 frDesign* design,
                 odb::dbDatabase* db,
                 Logger* logger,
                 RouterConfiguration* router_cfg);

  void startPin(frBPin* pin,
                frInstTerm* inst_term,
                std::set<frInst*, frBlockObjectComp>* inst_class);

  void startPin(frMPin* pin,
                frInstTerm* inst_term,
                std::set<frInst*, frBlockObjectComp>* inst_class);

  void setAPs(const std::vector<std::unique_ptr<frAccessPoint>>& aps,
              frAccessPointEnum lower_type,
              frAccessPointEnum upper_type);

  void setViaAP(const frAccessPoint* ap,
                const frVia* via,
                const std::vector<std::unique_ptr<frMarker>>& markers);

  void setPlanarAP(const frAccessPoint* ap,
                   const frPathSeg* seg,
                   const std::vector<std::unique_ptr<frMarker>>& markers);

  void setObjsAndMakers(
      const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      const std::vector<std::unique_ptr<frMarker>>& markers,
      FlexPA::PatternType type);

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  Logger* logger_;
  frDebugSettings* settings_;
  frInst* inst_;           // from settings_->pinName
  std::string term_name_;  // from settings_->pinName
  gui::Gui* gui_;
  frPin* pin_;
  frInstTerm* inst_term_;
  frBlock* top_block_;
  std::vector<frAccessPoint> aps_;
  // maps odb layerIdx -> tr layerIdx, with -1 for no equivalent
  std::vector<std::pair<frLayerNum, std::string>> layer_map_;
  const frAccessPoint* pa_ap_;
  std::vector<const frVia*> pa_vias_;
  std::vector<const frPathSeg*> pa_segs_;
  const std::vector<std::unique_ptr<frMarker>>* pa_markers_;
  std::vector<std::pair<Rect, frLayerNum>> shapes_;
};

}  // namespace drt
