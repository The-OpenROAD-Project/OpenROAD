// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frMPin.h"
#include "frBaseTypes.h"
#include "global.h"
#include "gui/gui.h"
#include "pa/AbstractPAGraphics.h"
#include "pa/FlexPA.h"
#include "pa/FlexPA_unique.h"

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
class FlexPAGraphics : public gui::Renderer, public AbstractPAGraphics
{
 public:
  // Debug pin access
  FlexPAGraphics(frDebugSettings* settings,
                 frDesign* design,
                 odb::dbDatabase* db,
                 utl::Logger* logger,
                 RouterConfiguration* router_cfg);

  void startPin(frBPin* pin,
                frInstTerm* inst_term,
                UniqueClass* inst_class) override;

  void startPin(frMPin* pin,
                frInstTerm* inst_term,
                UniqueClass* inst_class) override;

  void setAPs(const std::vector<std::unique_ptr<frAccessPoint>>& aps,
              frAccessPointEnum lower_type,
              frAccessPointEnum upper_type) override;

  void setViaAP(const frAccessPoint* ap,
                const frVia* via,
                const std::vector<std::unique_ptr<frMarker>>& markers) override;

  void setPlanarAP(
      const frAccessPoint* ap,
      const frPathSeg* seg,
      const std::vector<std::unique_ptr<frMarker>>& markers) override;

  void setObjsAndMakers(
      const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      const std::vector<std::unique_ptr<frMarker>>& markers,
      FlexPA::PatternType type) override;

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  utl::Logger* logger_;
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
  std::vector<std::pair<odb::Rect, frLayerNum>> shapes_;
};

}  // namespace drt
