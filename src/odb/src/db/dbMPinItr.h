// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {

class _dbMPin;

class dbMPinItr : public dbIterator
{
 public:
  dbMPinItr(dbTable<_dbMPin, 4>* mpin_tbl) { _mpin_tbl = mpin_tbl; }

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
  dbTable<_dbMPin, 4>* _mpin_tbl;
};

}  // namespace odb
