// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <iterator>

#include "odb.h"

namespace odb {

class dbObject;
class dbObjectTable;

class dbIterator
{
 public:
  virtual bool reversible() = 0;
  virtual bool orderReversed() = 0;
  virtual void reverse(dbObject* parent) = 0;
  virtual uint sequential() = 0;
  virtual uint size(dbObject* parent) = 0;
  virtual uint begin(dbObject* parent) = 0;
  virtual uint end(dbObject* parent) = 0;
  virtual uint next(uint id, ...) = 0;
  virtual dbObject* getObject(uint id, ...) = 0;
  virtual ~dbIterator() = default;
};

}  // namespace odb
