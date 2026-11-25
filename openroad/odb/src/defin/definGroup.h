// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include "definBase.h"

namespace odb {

class dbGroup;

class definGroup : public definBase
{
  dbGroup* cur_group_{nullptr};

 public:
  virtual void begin(const char* name);
  virtual void region(const char* region_name);
  virtual void inst(const char* inst);
  virtual void property(const char* name, const char* value);
  virtual void property(const char* name, int value);
  virtual void property(const char* name, double value);
  virtual void end();
};

}  // namespace odb
