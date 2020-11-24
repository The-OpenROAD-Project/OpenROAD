#pragma once

#include <memory>

#include "gui/gui.h"

namespace fr {

class FlexDRWorker;
class drNet;
class frDebugSettings;

// This class draws debugging graphics on the layout
class FlexDRGraphics : public gui::Renderer
{
 public:
  // Debug InitialPlace
  FlexDRGraphics(frDebugSettings* settings);

  void startWorker(FlexDRWorker* worker);

  void startNet(drNet* net);

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  virtual void drawObjects(gui::Painter& painter) override;
  virtual void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  FlexDRWorker*    worker_;
  drNet*           net_;
  frDebugSettings* settings_;
};

}  // namespace dr
