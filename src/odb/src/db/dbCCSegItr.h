// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {

class _dbCCSeg;

class dbCCSegItr : public dbIterator
{
  dbTable<_dbCCSeg, 4096>* _seg_tbl;

 public:
  dbCCSegItr(dbTable<_dbCCSeg, 4096>* seg_tbl) { _seg_tbl = seg_tbl; }

  bool reversible() const override;
  bool orderReversed() const override;
  void reverse(dbObject* parent) override;
  uint sequential() const override;
  uint size(dbObject* parent) const override;
  uint begin(dbObject* parent) const override;
  uint end(dbObject* parent) const override;
  uint next(uint id, ...) const override;
  dbObject* getObject(uint id, ...) override;
};

}  // namespace odb
