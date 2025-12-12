// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {
class _dbChipConn;

class dbChipConnItr : public dbIterator
{
 public:
  dbChipConnItr(dbTable<_dbChipConn>* chip_conn_tbl)
  {
    chip_conn_tbl_ = chip_conn_tbl;
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
  dbTable<_dbChipConn>* chip_conn_tbl_;
};

}  // namespace odb
// Generator Code End Header