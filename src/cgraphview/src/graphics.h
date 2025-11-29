// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "gui/gui.h"
#include "observer.h"
#include "odb/db.h"
#include "odb/geom.h"

// This implements the Observer interface and renders to the GUI

namespace cgv {

class Graphics : public gui::Renderer, public Observer
{
 public:
  Graphics();

  // From Observer
  void addEdges(std::vector<std::pair<odb::Point, odb::Point>> edges) override;

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;

  static bool guiActive();

 private:
  std::vector<std::pair<odb::Point, odb::Point>> edges_;
};

}  // namespace cgv
