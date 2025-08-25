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
    _chip_conn_tbl = chip_conn_tbl;
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
  dbTable<_dbChipConn>* _chip_conn_tbl;
};

}  // namespace odb
// Generator Code End Header