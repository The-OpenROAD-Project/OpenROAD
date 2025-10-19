// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "definBase.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {

class dbObject;
class dbBTerm;

class definPinProps : public definBase
{
 private:
  dbObject* _cur_obj;

 public:
  virtual void begin(const char* inst, const char* term);
  virtual void property(const char* name, const char* value);
  virtual void property(const char* name, int value);
  virtual void property(const char* name, double value);
  virtual void end();

  definPinProps();
  ~definPinProps() override;
};

}  // namespace odb
