// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {
class _dbChipInst;

class dbChipInstItr : public dbIterator
{
 public:
  dbChipInstItr(dbTable<_dbChipInst>* chip_inst_tbl)
  {
    _chip_inst_tbl = chip_inst_tbl;
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
  dbTable<_dbChipInst>* _chip_inst_tbl;
};

}  // namespace odb
// Generator Code End Header