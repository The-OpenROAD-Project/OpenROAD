// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include "AbstractSteinerRenderer.h"
#include "gui/gui.h"

namespace rsz {

class SteinerRenderer : public gui::Renderer, public AbstractSteinerRenderer
{
 public:
  SteinerRenderer();

  void highlight(SteinerTree* tree) override;
  void drawObjects(gui::Painter& /* painter */) override;

 private:
  SteinerTree* tree_ = nullptr;
};

}  // namespace rsz
