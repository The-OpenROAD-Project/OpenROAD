// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {

class _dbTechLayer;
template <class T>
class dbTable;

class dbTechLayerItr : public dbIterator
{
  dbTable<_dbTechLayer>* _layer_tbl;

 public:
  dbTechLayerItr(dbTable<_dbTechLayer>* layer_tbl) { _layer_tbl = layer_tbl; }

  bool reversible() override;
  bool orderReversed() override;
  void reverse(dbObject* parent) override;
  uint sequential() override;
  uint size(dbObject* parent) override;
  uint begin(dbObject* parent) override;
  uint end(dbObject* parent) override;
  uint next(uint id, ...) override;
  dbObject* getObject(uint id, ...) override;
};

}  // namespace odb
