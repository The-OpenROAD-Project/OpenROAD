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

#include "definPin.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "db.h"
#include "dbShape.h"
#include "dbTransform.h"

namespace odb {

definPin::definPin()
    : _bterm_cnt(0),
      _cur_bterm(NULL),
      _status(dbPlacementStatus::NONE),
      _orient(dbOrientType::R0),
      _orig_x(0),
      _orig_y(0),
      _min_spacing(0),
      _effective_width(0),
      _left_bus('['),
      _right_bus(']'),
      _layer(0),
      _has_min_spacing(false),
      _has_effective_width(false),
      _has_placement(false)
{
  init();
}

definPin::~definPin()
{
}

void definPin::init()
{
  definBase::init();
}

void definPin::pinsBegin(int /* unused: n */)
{
  _block->getBusDelimeters(_left_bus, _right_bus);
  _bterm_cnt = 0;
  _ground_pins.clear();
  _supply_pins.clear();
}

void definPin::pinBegin(const char* name, const char* net_name)
{
  dbNet* net = _block->findNet(net_name);

  if (net == NULL)
    net = dbNet::create(_block, net_name);

  const char* s = strstr(name, ".extra");

  if (s == NULL) {
    _cur_bterm = dbBTerm::create(net, name);
    _bterm_cnt++;
  } else  // extra pin statement
  {
    const char* busleft  = strchr(s, _left_bus);
    const char* busright = strchr(s, _right_bus);
    char*       bname    = NULL;

    // DEF 5.6
    if (busleft && busright) {
      // 5.6 PIN Name of form pinName.extraN[indexh]
      int len1 = (s - name);
      int len2 = (busright - busleft + 1);
      bname    = (char*) malloc(len1 + len2 + 1);
      strncpy(bname, name, len1);
      strcat(bname, busleft);

    } else if (busleft) {
      ++_errors;
      notice(0, "PIN %s missing right bus character.\n", name);
      return;
    }

    else if (busright) {
      ++_errors;
      notice(0, "PIN %s missing left bus character.\n", name);
      return;
    } else {
      // PIN Name of form pinName.extraN
      int len = s - name;
      bname   = (char*) malloc(len + 1);
      strncpy(bname, name, len);
      bname[len] = 0;
    }

    _cur_bterm = _block->findBTerm(bname);

    if (_cur_bterm == NULL) {
      _cur_bterm = dbBTerm::create(net, name);
      _bterm_cnt++;
    }

    free((void*) bname);
  }

  _rects.clear();
  _polygons.clear();
  _has_min_spacing     = false;
  _has_effective_width = false;
  _has_placement       = false;
}

void definPin::pinSpecial()
{
  if (_cur_bterm == NULL)
    return;

  _cur_bterm->setSpecial();
}

void definPin::pinUse(dbSigType type)
{
  if (_cur_bterm == NULL)
    return;

  _cur_bterm->setSigType(type);
}

void definPin::pinDirection(dbIoType type)
{
  if (_cur_bterm == NULL)
    return;

  _cur_bterm->setIoType(type);
}

void definPin::pinPlacement(defPlacement status, int x, int y, dbOrientType orient)
{
  if (_cur_bterm == NULL)
    return;

  _orig_x = dbdist(x);
  _orig_y = dbdist(y);

  switch (status) {
    case DEF_PLACEMENT_FIXED:
      _status = dbPlacementStatus::FIRM;
      break;
    case DEF_PLACEMENT_COVER:
      _status = dbPlacementStatus::COVER;
      break;
    case DEF_PLACEMENT_PLACED:
      _status = dbPlacementStatus::PLACED;
      break;
    case DEF_PLACEMENT_UNPLACED:
      _status = dbPlacementStatus::UNPLACED;
      break;
  }

  _orient = orient;
  _has_placement = true;
}

void definPin::pinMinSpacing(int spacing)
{
  if (_has_effective_width) {
    notice(0,
           "error: Cannot specify effective width and minimum spacing "
           "together.\n");
    ++_errors;
    return;
  }

  _min_spacing     = dbdist(spacing);
  _has_min_spacing = true;
}

void definPin::pinEffectiveWidth(int width)
{
  if (_has_min_spacing) {
    notice(0,
           "error: Cannot specify effective width and minimum spacing "
           "together.\n");
    ++_errors;
    return;
  }

  _effective_width     = dbdist(width);
  _has_effective_width = true;
}

void definPin::pinRect(const char* layer_name, int x1, int y1, int x2, int y2)
{
  if (_cur_bterm == NULL)
    return;

  _layer = _tech->findLayer(layer_name);

  if (_layer == NULL) {
    notice(0, "error: undefined layer (%s) referenced\n", layer_name);
    ++_errors;
    return;
  }

  Rect r(dbdist(x1), dbdist(y1), dbdist(x2), dbdist(y2));
  _rects.push_back(PinRect(_layer, r));
}

void definPin::pinPolygon(std::vector<defPoint>& points)
{
  std::vector<Point> P;
  translate(points, P);
  _polygons.push_back(Polygon(_layer, P));
}

void definPin::pinGroundPin(const char* groundPin)
{
  if (_cur_bterm == NULL)
    return;

  _ground_pins.push_back(Pin(_cur_bterm, std::string(groundPin)));
}

void definPin::pinSupplyPin(const char* supplyPin)
{
  if (_cur_bterm == NULL)
    return;

  _supply_pins.push_back(Pin(_cur_bterm, std::string(supplyPin)));
}

void definPin::pinEnd()
{
  if (!_rects.empty()) {
    if (_has_placement == false) {
      _status = dbPlacementStatus::NONE;
      _orient = dbOrientType::R0;
      _orig_x = 0;
      _orig_y = 0;
    }

    std::vector<PinRect>::iterator itr;

    for (itr = _rects.begin(); itr != _rects.end(); ++itr)
      addRect(*itr);
  }

  if (!_polygons.empty()) {
    if (_has_placement == false) {
      _status = dbPlacementStatus::NONE;
      _orient = dbOrientType::R0;
      _orig_x = 0;
      _orig_y = 0;
    }

    std::vector<Polygon>::iterator itr;

    for (itr = _polygons.begin(); itr != _polygons.end(); ++itr)
      addPolygon(*itr);
  }

  _cur_bterm = NULL;
}

void definPin::addRect(PinRect& r)
{
  dbBPin* pin = dbBPin::create(_cur_bterm);
  pin->setPlacementStatus(_status);

  Point     origin(0, 0);
  dbOrientType orient(_orient);
  dbTransform  transform(orient, origin);
  transform.apply(r._rect);

  // Translate to placed location
  int xmin = r._rect.xMin() + _orig_x;
  int ymin = r._rect.yMin() + _orig_y;
  int xmax = r._rect.xMax() + _orig_x;
  int ymax = r._rect.yMax() + _orig_y;
  dbBox::create(pin, r._layer, xmin, ymin, xmax, ymax);
}

void definPin::addPolygon(Polygon& p)
{
  std::vector<Rect> R;

  p.decompose(R);

  std::vector<Rect>::iterator itr;

  for (itr = R.begin(); itr != R.end(); ++itr) {
    Rect& r = *itr;
    PinRect R(p._layer, r);
    addRect(R);
  }
}

void definPin::pinsEnd()
{
  dbSet<dbBTerm> bterms = _block->getBTerms();

  dbSet<dbBTerm>::iterator itr;

  for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
    dbBTerm* bterm = *itr;

    dbSet<dbBPin> bpins = bterm->getBPins();

    if (bpins.reversible() && bpins.orderReversed())
      bpins.reverse();
  }

  std::vector<Pin>::iterator pitr;

  for (pitr = _ground_pins.begin(); pitr != _ground_pins.end(); ++pitr) {
    Pin& p = *pitr;

    dbBTerm* ground = _block->findBTerm(p._pin.c_str());

    if (ground != NULL) {
      p._bterm->setGroundPin(ground);
    } else {
      ++_errors;
      notice(0, "error: Cannot find PIN %s\n", p._pin.c_str());
    }
  }

  for (pitr = _supply_pins.begin(); pitr != _supply_pins.end(); ++pitr) {
    Pin& p = *pitr;

    dbBTerm* supply = _block->findBTerm(p._pin.c_str());

    if (supply != NULL) {
      p._bterm->setSupplyPin(supply);
    } else {
      ++_errors;
      notice(0, "error: Cannot find PIN %s\n", p._pin.c_str());
    }
  }
}

}  // namespace odb
