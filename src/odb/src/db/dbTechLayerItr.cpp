// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechLayerItr.h"

#include <cstdint>

#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "odb/dbObject.h"

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

uint32_t dbTechLayerItr::sequential() const
{
  return 0;
}

uint32_t dbTechLayerItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbTechLayerItr::begin(parent); id != dbTechLayerItr::end(parent);
       id = dbTechLayerItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbTechLayerItr::begin(dbObject* parent) const
{
  _dbTech* tech = (_dbTech*) parent;
  return (uint32_t) tech->bottom_;
}

uint32_t dbTechLayerItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbTechLayerItr::next(uint32_t id, ...) const
{
  _dbTechLayer* layer = layer_tbl_->getPtr(id);
  return layer->upper_;
}

dbObject* dbTechLayerItr::getObject(uint32_t id, ...)
{
  return layer_tbl_->getPtr(id);
}

}  // namespace odb
