// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {
class _dbChipBumpInst;

class dbChipBumpInstItr : public dbIterator
{
 public:
  dbChipBumpInstItr(dbTable<_dbChipBumpInst>* chip_bump_inst_tbl)
  {
    _chip_bump_inst_tbl = chip_bump_inst_tbl;
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
  dbTable<_dbChipBumpInst>* _chip_bump_inst_tbl;
};

}  // namespace odb
// Generator Code End Header