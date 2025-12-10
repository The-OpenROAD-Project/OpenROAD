// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {

class _dbPolygon;

class dbPolygonItr : public dbIterator
{
 public:
  dbPolygonItr(dbTable<_dbPolygon, 8>* pbox_tbl) { pbox_tbl_ = pbox_tbl; }

  bool reversible() const override;
  bool orderReversed() const override;
  void reverse(dbObject* parent) override;
  uint sequential() const override;
  uint size(dbObject* parent) const override;
  uint begin(dbObject* parent) const override;
  uint end(dbObject* parent) const override;
  uint next(uint id, ...) const override;
  dbObject* getObject(uint id, ...) override;

 private:
  dbTable<_dbPolygon, 8>* pbox_tbl_;
};

}  // namespace odb
