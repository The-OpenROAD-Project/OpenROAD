// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/odb.h"

namespace odb {

class dbObject;

class dbIterator
{
 public:
  virtual ~dbIterator() = default;

  virtual bool reversible() const = 0;
  virtual bool orderReversed() const = 0;
  virtual void reverse(dbObject* parent) = 0;
  virtual uint sequential() const = 0;
  virtual uint size(dbObject* parent) const = 0;
  virtual uint begin(dbObject* parent) const = 0;
  virtual uint end(dbObject* parent) const = 0;
  virtual uint next(uint id, ...) const = 0;
  virtual dbObject* getObject(uint id, ...) = 0;
};

}  // namespace odb
