// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "definBase.h"
#include "definTypes.h"

namespace odb {

class dbRegion;

class definRegion : public definBase
{
  dbRegion* _cur_region = nullptr;

 public:
  virtual void begin(const char* name);
  virtual void boundary(int x1, int y1, int x2, int y2);
  virtual void type(defRegionType type);
  virtual void property(const char* name, const char* value);
  virtual void property(const char* name, int value);
  virtual void property(const char* name, double value);
  virtual void end();
};

}  // namespace odb
