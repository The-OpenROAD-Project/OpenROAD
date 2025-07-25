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
 protected:
  dbTable<_dbPolygon, 8>* pbox_tbl_;

 public:
  dbPolygonItr(dbTable<_dbPolygon, 8>* pbox_tbl) { pbox_tbl_ = pbox_tbl; }

  bool reversible() override;
  bool orderReversed() override;
  void reverse(dbObject* parent) override;
  uint sequential() override;
  uint size(dbObject* parent) override;
  uint begin(dbObject* parent) override;
  uint end(dbObject* parent) override;
  uint next(uint id, ...) override;
  dbObject* getObject(uint id, ...) override;
};

}  // namespace odb
