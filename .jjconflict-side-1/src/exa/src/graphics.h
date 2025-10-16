// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "gui/gui.h"
#include "observer.h"
#include "odb/db.h"

// This implements the Observer interface and renders to the GUI

namespace exa {

class Graphics : public gui::Renderer, public Observer
{
 public:
  Graphics();

  // From Observer
  void makeInstance(odb::dbInst* instance) override;

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;

  static bool guiActive();

 private:
  odb::dbInst* instance_{nullptr};
};

}  // namespace exa
