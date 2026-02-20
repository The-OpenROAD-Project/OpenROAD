// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "db/obj/frBlock.h"
#include "frBaseTypes.h"
#include "gui/gui.h"
#include "ta/AbstractTAGraphics.h"
#include "ta/FlexTA.h"

namespace odb {
class dbDatabase;
class dbTechLayer;
}  // namespace odb

namespace drt {

class frDesign;
class frNet;

// This class draws debugging graphics on the layout
class FlexTAGraphics : public gui::Renderer, public AbstractTAGraphics
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
  void endIter(int iter) override;

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
