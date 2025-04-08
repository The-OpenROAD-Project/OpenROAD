// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {
class _dbModInst;

template <class T>
class dbTable;

class dbModuleModInstItr : public dbIterator
{
 public:
  dbModuleModInstItr(dbTable<_dbModInst>* modinst_tbl)
  {
    _modinst_tbl = modinst_tbl;
  }

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
  dbTable<_dbModInst>* _modinst_tbl;
};

}  // namespace odb
   // Generator Code End Header
