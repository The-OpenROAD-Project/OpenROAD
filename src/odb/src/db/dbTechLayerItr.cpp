// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechLayerItr.h"

#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "odb/dbObject.h"
#include "odb/odb.h"

namespace odb {

bool dbTechLayerItr::reversible() const
{
  return false;
}

bool dbTechLayerItr::orderReversed() const
{
  return false;
}

void dbTechLayerItr::reverse(dbObject* /* unused: parent */)
{
}

uint dbTechLayerItr::sequential() const
{
  return 0;
}

uint dbTechLayerItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbTechLayerItr::begin(parent); id != dbTechLayerItr::end(parent);
       id = dbTechLayerItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbTechLayerItr::begin(dbObject* parent) const
{
  _dbTech* tech = (_dbTech*) parent;
  return (uint) tech->bottom_;
}

uint dbTechLayerItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbTechLayerItr::next(uint id, ...) const
{
  _dbTechLayer* layer = _layer_tbl->getPtr(id);
  return layer->upper_;
}

dbObject* dbTechLayerItr::getObject(uint id, ...)
{
  return _layer_tbl->getPtr(id);
}

}  // namespace odb
