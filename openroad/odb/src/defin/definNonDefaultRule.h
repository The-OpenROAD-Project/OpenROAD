// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "definBase.h"
#include "odb/geom.h"

namespace odb {

class dbTechNonDefaultRule;
class dbTechLayerRule;

class definNonDefaultRule : public definBase
{
  dbTechNonDefaultRule* _cur_rule{nullptr};
  dbTechLayerRule* _cur_layer_rule{nullptr};

 public:
  virtual void beginRule(const char* name);
  virtual void hardSpacing();
  virtual void via(const char* name);
  virtual void viaRule(const char* rule);
  virtual void minCuts(const char* layer, int count);
  virtual void beginLayerRule(const char* layer, int width);
  virtual void spacing(int s);
  virtual void wireExt(int e);
  virtual void endLayerRule();
  virtual void property(const char* name, const char* value);
  virtual void property(const char* name, int value);
  virtual void property(const char* name, double value);
  virtual void endRule();
};

}  // namespace odb
