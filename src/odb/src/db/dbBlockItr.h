// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {

class _dbBlock;

class dbBlockItr : public dbIterator
{
 public:
  dbBlockItr(dbTable<_dbBlock>* block_tbl) { block_tbl_ = block_tbl; }

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
  dbTable<_dbBlock>* block_tbl_;
};

}  // namespace odb
