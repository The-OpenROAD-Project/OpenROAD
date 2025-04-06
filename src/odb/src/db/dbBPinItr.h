// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {

class _dbBPin;
template <class T>
class dbTable;

class dbBPinItr : public dbIterator
{
  dbTable<_dbBPin>* _bpin_tbl;

 public:
  dbBPinItr(dbTable<_dbBPin>* bpin_tbl) { _bpin_tbl = bpin_tbl; }

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
