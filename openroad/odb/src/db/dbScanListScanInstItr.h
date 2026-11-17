// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbIterator.h"

namespace odb {
class _dbScanInst;

class dbScanListScanInstItr : public dbIterator
{
 public:
  dbScanListScanInstItr(dbTable<_dbScanInst>* scan_inst_tbl)
  {
    scan_inst_tbl_ = scan_inst_tbl;
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
  dbTable<_dbScanInst>* scan_inst_tbl_;
};

}  // namespace odb
// Generator Code End Header