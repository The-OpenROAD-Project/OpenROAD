// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "definBase.h"
#include "odb/odb.h"

namespace odb {

class dbTechLayer;

class definFill : public definBase
{
  dbTechLayer* _cur_layer;
  uint _mask_number;
  bool _needs_opc;

 public:
  // Fill interface methods
  virtual void fillBegin(const char* layer, bool needs_opc, int mask_number);
  virtual void fillRect(int x1, int y1, int x2, int y2);
  virtual void fillPolygon(std::vector<Point>& points);
  virtual void fillEnd();

  definFill();
  ~definFill() override;
  void init() override;
};

}  // namespace odb
