// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbIterator.h"

namespace odb {
class _dbInst;

// User Code Begin classes
class _dbChip;
// User Code End classes

class dbChipBumpItr : public dbIterator
{
 public:
  dbChipBumpItr(dbTable<_dbInst>* inst_tbl) { inst_tbl_ = inst_tbl; }

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
  dbTable<_dbInst>* inst_tbl_;
  // User Code Begin Fields
  // First bump of the first region with bumps, starting at region_id
  // (inclusive) in chip region table order. 0 if there is none.
  static uint32_t firstBumpFrom(_dbChip* chip, uint32_t region_id);
  // User Code End Fields
};

}  // namespace odb
// Generator Code End Header