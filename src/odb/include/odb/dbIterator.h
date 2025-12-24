// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

namespace odb {

class dbObject;

class dbIterator
{
 public:
  virtual ~dbIterator() = default;

  virtual bool reversible() const = 0;
  virtual bool orderReversed() const = 0;
  virtual void reverse(dbObject* parent) = 0;
  virtual uint32_t sequential() const = 0;
  virtual uint32_t size(dbObject* parent) const = 0;
  virtual uint32_t begin(dbObject* parent) const = 0;
  virtual uint32_t end(dbObject* parent) const = 0;
  virtual uint32_t next(uint32_t id, ...) const = 0;
  virtual dbObject* getObject(uint32_t id, ...) = 0;
};

}  // namespace odb
