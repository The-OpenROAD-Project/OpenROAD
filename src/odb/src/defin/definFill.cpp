// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definFill.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"

namespace odb {

definFill::definFill()
{
  _cur_layer = nullptr;
}

definFill::~definFill()
{
}

void definFill::init()
{
  definBase::init();
  _cur_layer = nullptr;
}

void definFill::fillBegin(const char* layer, bool needs_opc, int mask_number)
{
  _cur_layer = _tech->findLayer(layer);

  if (_cur_layer == nullptr) {
    _logger->warn(
        utl::ODB, 95, "error: undefined layer ({}) referenced", layer);
    ++_errors;
  }
  _mask_number = mask_number;
  _needs_opc = needs_opc;
}

void definFill::fillRect(int x1, int y1, int x2, int y2)
{
  x1 = dbdist(x1);
  y1 = dbdist(y1);
  x2 = dbdist(x2);
  y2 = dbdist(y2);
  dbFill::create(_block, _needs_opc, _mask_number, _cur_layer, x1, y1, x2, y2);
}

void definFill::fillPolygon(std::vector<Point>& /* unused: points */)
{
}

void definFill::fillEnd()
{
}

}  // namespace odb
