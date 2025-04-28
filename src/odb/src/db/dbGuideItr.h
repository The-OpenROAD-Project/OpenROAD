// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {
class _dbGuide;

template <class T>
class dbTable;

class dbGuideItr : public dbIterator
{
 public:
  dbGuideItr(dbTable<_dbGuide>* guide_tbl) { _guide_tbl = guide_tbl; }

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
  dbTable<_dbGuide>* _guide_tbl;
};

}  // namespace odb
   // Generator Code End Header
