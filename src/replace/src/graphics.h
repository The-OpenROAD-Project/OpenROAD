#pragma once

#include <memory>

#include "gui/gui.h"

namespace replace {

class NesterovBase;
class PlacerBase;

// This class draws debugging graphics on the layout
class Graphics : public gui::Renderer
{
 public:
  Graphics(std::shared_ptr<PlacerBase> pb,
           std::shared_ptr<NesterovBase> nb,
           bool draw_bins);

  // Draw the graphics; optionally pausing afterwards
  void cellPlot(bool pause = false);

  // From Renderer API
  virtual void drawObjects(gui::Painter& painter) override;

 private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
  bool draw_bins_;
};

}  // namespace replace
