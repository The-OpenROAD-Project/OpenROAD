// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechLayerItr.h"

#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"

namespace odb {

bool dbTechLayerItr::reversible()
{
  return false;
}

bool dbTechLayerItr::orderReversed()
{
  return false;
}

void dbTechLayerItr::reverse(dbObject* /* unused: parent */)
{
}

uint dbTechLayerItr::sequential()
{
  return 0;
}

uint dbTechLayerItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbTechLayerItr::begin(parent); id != dbTechLayerItr::end(parent);
       id = dbTechLayerItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbTechLayerItr::begin(dbObject* parent)
{
  _dbTech* tech = (_dbTech*) parent;
  return (uint) tech->_bottom;
}

uint dbTechLayerItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbTechLayerItr::next(uint id, ...)
{
  _dbTechLayer* layer = _layer_tbl->getPtr(id);
  return layer->_upper;
}

dbObject* dbTechLayerItr::getObject(uint id, ...)
{
  return _layer_tbl->getPtr(id);
}

}  // namespace odb
