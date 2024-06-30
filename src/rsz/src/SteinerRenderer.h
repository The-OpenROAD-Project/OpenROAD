// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
