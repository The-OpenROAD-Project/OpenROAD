// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include "AbstractSteinerRenderer.h"
#include "web/core.h"

namespace est {

class SteinerRenderer : public web::Renderer, public AbstractSteinerRenderer
{
 public:
  SteinerRenderer();

  void highlight(SteinerTree* tree) override;
  void drawObjects(web::Painter& /* painter */) override;

 private:
  SteinerTree* tree_ = nullptr;
};

}  // namespace est
