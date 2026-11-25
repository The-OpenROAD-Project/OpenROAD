// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbIterator.h"

namespace odb {

class _dbPolygon;

class dbPolygonItr : public dbIterator
{
 public:
  dbPolygonItr(dbTable<_dbPolygon, 8>* pbox_tbl) { pbox_tbl_ = pbox_tbl; }

  bool reversible() const override;
  bool orderReversed() const override;
  void reverse(dbObject* parent) override;
  uint32_t sequential() const override;
  uint32_t size(dbObject* parent) const override;
  uint32_t begin(dbObject* parent) const override;
  uint32_t end(dbObject* parent) const override;
  uint32_t next(uint32_t id, ...) const override;
  dbObject* getObject(uint32_t id, ...) override;

 private:
  dbTable<_dbPolygon, 8>* pbox_tbl_;
};

}  // namespace odb
