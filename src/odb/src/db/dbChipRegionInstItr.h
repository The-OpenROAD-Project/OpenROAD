// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {
class _dbChipRegionInst;

class dbChipRegionInstItr : public dbIterator
{
 public:
  dbChipRegionInstItr(dbTable<_dbChipRegionInst>* chip_region_inst_tbl)
  {
    _chip_region_inst_tbl = chip_region_inst_tbl;
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
  dbTable<_dbChipRegionInst>* _chip_region_inst_tbl;
};

}  // namespace odb
// Generator Code End Header