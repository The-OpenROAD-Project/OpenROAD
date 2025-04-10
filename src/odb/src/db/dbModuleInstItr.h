// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {
class _dbInst;

template <class T>
class dbTable;

class dbModuleInstItr : public dbIterator
{
 public:
  dbModuleInstItr(dbTable<_dbInst>* inst_tbl) { _inst_tbl = inst_tbl; }

  bool reversible() override;
  bool orderReversed() override;
  void reverse(dbObject* parent) override;
  uint sequential() override;
  uint size(dbObject* parent) override;
  uint begin(dbObject* parent) override;
  uint end(dbObject* parent) override;
  uint next(uint id, ...) override;
  dbObject* getObject(uint id, ...) override;

 private:
  dbTable<_dbInst>* _inst_tbl;
};

}  // namespace odb
   // Generator Code End Header
