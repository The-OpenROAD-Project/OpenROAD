// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include <memory>
#include <vector>

#include "FlexPA.h"
#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

namespace gui {
class Painter;
}

namespace drt {

class AbstractPAGraphics
{
 public:
  virtual void startPin(frBPin* pin,
                        frInstTerm* inst_term,
                        std::set<frInst*, frBlockObjectComp>* inst_class)
      = 0;

  virtual void startPin(frMPin* pin,
                        frInstTerm* inst_term,
                        std::set<frInst*, frBlockObjectComp>* inst_class)
      = 0;

  virtual void setAPs(const std::vector<std::unique_ptr<frAccessPoint>>& aps,
                      frAccessPointEnum lower_type,
                      frAccessPointEnum upper_type)
      = 0;

  virtual void setViaAP(const frAccessPoint* ap,
                        const frVia* via,
                        const std::vector<std::unique_ptr<frMarker>>& markers)
      = 0;

  virtual void setPlanarAP(
      const frAccessPoint* ap,
      const frPathSeg* seg,
      const std::vector<std::unique_ptr<frMarker>>& markers)
      = 0;

  virtual void setObjsAndMakers(
      const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      const std::vector<std::unique_ptr<frMarker>>& markers,
      FlexPA::PatternType type)
      = 0;

  // Show a message in the status bar
  virtual void status(const std::string& message) = 0;

  // From Renderer API
  virtual void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) = 0;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();
};

}  // namespace drt