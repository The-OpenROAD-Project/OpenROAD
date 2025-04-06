// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "definBase.h"

namespace odb {

class dbTechLayer;

class definComponentMaskShift : public definBase
{
 public:
  definComponentMaskShift();

  void init() override;

  void addLayer(const char* layer_name);
  void setLayers();

 private:
  std::vector<dbTechLayer*> _layers;
};

}  // namespace odb
