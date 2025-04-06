// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "definComponentMaskShift.h"

#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

definComponentMaskShift::definComponentMaskShift()
{
  init();
}

void definComponentMaskShift::init()
{
  definBase::init();
  _layers.clear();
}

void definComponentMaskShift::addLayer(const char* layer)
{
  odb::dbTechLayer* db_layer = _tech->findLayer(layer);
  if (db_layer == nullptr) {
    _logger->warn(
        utl::ODB, 425, "error: undefined layer ({}) referenced", layer);
    ++_errors;
  } else {
    _layers.push_back(db_layer);
  }
}

void definComponentMaskShift::setLayers()
{
  _block->setComponentMaskShift(_layers);
}

}  // namespace odb
