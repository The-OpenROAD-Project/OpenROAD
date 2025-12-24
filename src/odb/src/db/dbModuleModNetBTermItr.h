// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbIterator.h"

namespace odb {
class _dbBTerm;

class dbModuleModNetBTermItr : public dbIterator
{
 public:
  dbModuleModNetBTermItr(dbTable<_dbBTerm>* bterm_tbl)
  {
    bterm_tbl_ = bterm_tbl;
  }

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
  dbTable<_dbBTerm>* bterm_tbl_;
};

}  // namespace odb
   // Generator Code End Header
