#pragma once

#include <memory>

#include "gui/gui.h"

namespace utl {
class Logger;
}

namespace gpl {

class InitialPlace;
class NesterovBase;
class NesterovPlace;
class PlacerBase;
class GCell;

// This class draws debugging graphics on the layout
class Graphics : public gui::Renderer
{
 public:
  // Debug InitialPlace
  Graphics(utl::Logger* logger,
           std::shared_ptr<PlacerBase> pb,
           InitialPlace* ip);

  // Debug NesterovPlace
  Graphics(utl::Logger* logger,
           NesterovPlace* np,
           std::shared_ptr<PlacerBase> pb,
           std::shared_ptr<NesterovBase> nb,
           bool draw_bins);

  // Draw the graphics; optionally pausing afterwards
  void cellPlot(bool pause = false);

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  virtual void drawObjects(gui::Painter& painter) override;
  virtual gui::Selected select(odb::dbTechLayer* layer,
                               const odb::Point& point) override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
  NesterovPlace* np_;
  InitialPlace* ip_;
  GCell* selected_;
  bool draw_bins_;
  utl::Logger* logger_;

  void drawNesterov(gui::Painter& painter);
  void drawInitial(gui::Painter& painter);
  void drawBounds(gui::Painter& painter);
  void reportSelected();
};

}  // namespace gpl
