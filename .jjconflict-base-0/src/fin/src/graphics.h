// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "polygon.h"

namespace fin {

// This class draws debugging graphics on the layout
class Graphics : public gui::Renderer
{
 public:
  Graphics();

  void drawPolygon90Set(const Polygon90Set& set);

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;

  // Show a message in the status bar
  void status(const std::string& message);

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  std::vector<Rectangle> polygon_rects_;
};

}  // namespace fin
