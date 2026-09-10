// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include "AbstractCugrRenderer.h"
#include "gui/gui.h"
#include "odb/db.h"

namespace grt {

class CugrRenderer : public gui::Renderer, public AbstractCugrRenderer
{
 public:
  CugrRenderer();

  void drawAndPause(CugrDebugFrame frame) override;

  void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;

 private:
  CugrDebugFrame frame_;
};

}  // namespace grt
