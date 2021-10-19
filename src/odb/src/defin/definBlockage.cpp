///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "definBlockage.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "dbShape.h"
#include "definPolygon.h"
#include "utl/Logger.h"
namespace odb {

definBlockage::definBlockage()
{
  init();
}

definBlockage::~definBlockage()
{
}

void definBlockage::init()
{
  definBase::init();
}

void definBlockage::blockageRoutingBegin(const char* layer)
{
  _layer = _tech->findLayer(layer);
  _inst = NULL;
  _slots = false;
  _fills = false;
  _pushdown = false;
  _has_min_spacing = false;
  _has_effective_width = false;
  _min_spacing = 0;
  _effective_width = 0;

  if (_layer == NULL) {
    _logger->warn(
        utl::ODB, 88, "error: undefined layer ({}) referenced", layer);
    ++_errors;
  }
}

void definBlockage::blockageRoutingComponent(const char* comp)
{
  _inst = _block->findInst(comp);

  if (_inst == NULL) {
    _logger->warn(
        utl::ODB, 89, "error: undefined component ({}) referenced", comp);
    ++_errors;
  }
}

void definBlockage::blockageRoutingSlots()
{
  _slots = true;
}

void definBlockage::blockageRoutingFills()
{
  _fills = true;
}

void definBlockage::blockageRoutingPushdown()
{
  _pushdown = true;
}

void definBlockage::blockageRoutingMinSpacing(int spacing)
{
  _has_min_spacing = true;
  _min_spacing = spacing;
}

void definBlockage::blockageRoutingEffectiveWidth(int width)
{
  _has_effective_width = true;
  _effective_width = width;
}

void definBlockage::blockageRoutingRect(int x1, int y1, int x2, int y2)
{
  if (_layer == NULL)
    return;

  x1 = dbdist(x1);
  y1 = dbdist(y1);
  x2 = dbdist(x2);
  y2 = dbdist(y2);
  dbObstruction* o
      = dbObstruction::create(_block, _layer, x1, y1, x2, y2, _inst);

  if (_pushdown)
    o->setPushedDown();

  if (_fills)
    o->setFillObstruction();

  if (_slots)
    o->setSlotObstruction();

  if (_has_min_spacing)
    o->setMinSpacing(dbdist(_min_spacing));

  if (_has_effective_width)
    o->setEffectiveWidth(dbdist(_effective_width));
}

void definBlockage::blockageRoutingPolygon(const std::vector<Point>& points)
{
  if (_layer == NULL)
    return;

  definPolygon polygon(points);
  std::vector<Rect> R;
  polygon.decompose(R);

  std::vector<Rect>::iterator itr;

  for (itr = R.begin(); itr != R.end(); ++itr) {
    Rect& r = *itr;

    dbObstruction* o = dbObstruction::create(
        _block, _layer, r.xMin(), r.yMin(), r.xMax(), r.yMax(), _inst);
    if (_pushdown)
      o->setPushedDown();

    if (_fills)
      o->setFillObstruction();

    if (_slots)
      o->setSlotObstruction();
    if (_has_min_spacing)
      o->setMinSpacing(dbdist(_min_spacing));

    if (_has_effective_width)
      o->setEffectiveWidth(dbdist(_effective_width));
  }
}

void definBlockage::blockageRoutingEnd()
{
}

void definBlockage::blockagePlacementBegin()
{
  _layer = NULL;
  _inst = NULL;
  _slots = false;
  _fills = false;
  _pushdown = false;
  _soft = false;
  _max_density = 0.0;
}

void definBlockage::blockagePlacementComponent(const char* comp)
{
  _inst = _block->findInst(comp);

  if (_inst == NULL) {
    _logger->warn(
        utl::ODB, 90, "error: undefined component ({}) referenced", comp);
    ++_errors;
  }
}

void definBlockage::blockagePlacementPushdown()
{
  _pushdown = true;
}

void definBlockage::blockagePlacementSoft()
{
  _soft = true;
}

void definBlockage::blockagePlacementMaxDensity(double max_density)
{
  if (max_density >= 0 && max_density <= 100) {
    _max_density = max_density;
  } else {
    _logger->warn(
        utl::ODB,
        91,
        "warning: Blockage max density {} not in [0, 100] will be ignored",
        max_density);
  }
}

void definBlockage::blockagePlacementRect(int x1, int y1, int x2, int y2)
{
  x1 = dbdist(x1);
  y1 = dbdist(y1);
  x2 = dbdist(x2);
  y2 = dbdist(y2);
  dbBlockage* b = dbBlockage::create(_block, x1, y1, x2, y2, _inst);

  if (_pushdown) {
    b->setPushedDown();
  }

  if (_soft) {
    b->setSoft();
  }

  if (_max_density > 0) {
    b->setMaxDensity(_max_density);
  }
}

void definBlockage::blockagePlacementEnd()
{
}

}  // namespace odb
