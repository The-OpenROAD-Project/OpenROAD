// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {
class _dbModBTerm;

class dbModuleModBTermItr : public dbIterator
{
 public:
  dbModuleModBTermItr(dbTable<_dbModBTerm>* modbterm_tbl)
  {
    modbterm_tbl_ = modbterm_tbl;
  }

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
  dbTable<_dbModBTerm>* modbterm_tbl_;
};

}  // namespace odb
   // Generator Code End Header
