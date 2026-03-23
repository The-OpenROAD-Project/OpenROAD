// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definPin.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ranges>
#include <vector>

#include "definTypes.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

void definPin::pinsBegin(int /* unused: n */)
{
  _block->getBusDelimiters(_left_bus, _right_bus);
  _bterm_cnt = 0;
  _update_cnt = 0;
  _ground_pins.clear();
  _supply_pins.clear();
}

void definPin::pinBegin(const char* name, const char* net_name)
{
  dbNet* net = _block->findNet(net_name);

  if (net == nullptr) {
    net = dbNet::create(_block, net_name);
  }

  const char* s = strstr(name, ".extra");

  if (s == nullptr) {
    if (_mode != defin::DEFAULT) {
      _cur_bterm = _block->findBTerm(name);
      if (_cur_bterm != nullptr) {
        _update_cnt++;
      }
    } else {
      _cur_bterm = dbBTerm::create(net, name);
      _bterm_cnt++;
    }
  } else  // extra pin statement
  {
    const char* busleft = strchr(s, _left_bus);
    const char* busright = strchr(s, _right_bus);
    char* bname = nullptr;

    // DEF 5.6
    if (busleft && busright) {
      // 5.6 PIN Name of form pinName.extraN[indexh]
      int len1 = (s - name);
      int len2 = (busright - busleft + 1);
      bname = (char*) malloc(len1 + len2 + 1);
      strncpy(bname, name, len1);
      strcat(bname, busleft);

    } else if (busleft) {
      ++_errors;
      _logger->warn(utl::ODB, 117, "PIN {} missing right bus character.", name);
      return;
    }

    else if (busright) {
      ++_errors;
      _logger->warn(utl::ODB, 118, "PIN {} missing left bus character.", name);
      return;
    } else {
      // PIN Name of form pinName.extraN
      int len = s - name;
      bname = (char*) malloc(len + 1);
      strncpy(bname, name, len);
      bname[len] = 0;
    }

    _cur_bterm = _block->findBTerm(bname);

    if (_cur_bterm == nullptr) {
      _cur_bterm = dbBTerm::create(net, name);
      _bterm_cnt++;
    }

    free((void*) bname);
  }
}

void definPin::pinSpecial()
{
  if (_cur_bterm == nullptr) {
    return;
  }

  _cur_bterm->setSpecial();
}

void definPin::pinUse(dbSigType type)
{
  if (_cur_bterm == nullptr) {
    return;
  }

  _cur_bterm->setSigType(type);
}

void definPin::pinDirection(dbIoType type)
{
  if (_cur_bterm == nullptr) {
    return;
  }

  _cur_bterm->setIoType(type);
}

bool definPin::checkPinDirection(dbIoType type)
{
  if (_cur_bterm == nullptr) {
    return true;
  }

  return _cur_bterm->getIoType() == type;
}

void definPin::pinPlacement(defPlacement status,
                            int x,
                            int y,
                            dbOrientType orient)
{
  if (_cur_bterm == nullptr) {
    return;
  }

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
    _logger->warn(utl::ODB,
                  119,
                  "error: Cannot specify effective width and minimum spacing "
                  "together.");
    ++_errors;
    return;
  }

  _min_spacing = dbdist(spacing);
  _has_min_spacing = true;
}

void definPin::pinEffectiveWidth(int width)
{
  if (_has_min_spacing) {
    _logger->warn(utl::ODB,
                  120,
                  "error: Cannot specify effective width and minimum spacing "
                  "together.");
    ++_errors;
    return;
  }

  _effective_width = dbdist(width);
  _has_effective_width = true;
}

void definPin::pinRect(const char* layer_name,
                       int x1,
                       int y1,
                       int x2,
                       int y2,
                       uint32_t mask)
{
  if (_cur_bterm == nullptr) {
    return;
  }

  _layer = _tech->findLayer(layer_name);

  if (_layer == nullptr) {
    _logger->warn(
        utl::ODB, 121, "error: undefined layer ({}) referenced", layer_name);
    ++_errors;
    return;
  }

  Rect r(dbdist(x1), dbdist(y1), dbdist(x2), dbdist(y2));
  _rects.emplace_back(_layer, r, mask);
}

void definPin::pinPolygon(std::vector<defPoint>& points, uint32_t mask)
{
  std::vector<Point> P;
  translate(points, P);
  _polygons.emplace_back(_layer, P, mask);
}

void definPin::pinGroundPin(const char* groundPin)
{
  if (_cur_bterm == nullptr) {
    return;
  }

  _ground_pins.emplace_back(_cur_bterm, std::string(groundPin));
}

void definPin::pinSupplyPin(const char* supplyPin)
{
  if (_cur_bterm == nullptr) {
    return;
  }

  _supply_pins.emplace_back(_cur_bterm, std::string(supplyPin));
}

void definPin::portBegin()
{
  _rects.clear();
  _polygons.clear();
  _has_min_spacing = false;
  _has_effective_width = false;
  _has_placement = false;
  _status = dbPlacementStatus::NONE;
  _orient = dbOrientType::R0;
  _orig_x = 0;
  _orig_y = 0;
}

void definPin::portEnd()
{
  dbBPin* pin = nullptr;

  if (!_rects.empty() || !_polygons.empty()) {
    pin = dbBPin::create(_cur_bterm);
    pin->setPlacementStatus(_status);

    if (_has_min_spacing) {
      pin->setMinSpacing(_min_spacing);
    }

    if (_has_effective_width) {
      pin->setEffectiveWidth(_effective_width);
    }
  }

  if (!_rects.empty()) {
    for (auto& rect : std::ranges::reverse_view(_rects)) {
      addRect(rect, pin);
    }
  }

  if (!_polygons.empty()) {
    for (Polygon& polygon : _polygons) {
      addPolygon(polygon, pin);
    }
  }
}

void definPin::pinEnd()
{
  _cur_bterm = nullptr;
}

void definPin::addRect(PinRect& r, dbBPin* pin)
{
  Point origin(0, 0);
  dbOrientType orient(_orient);
  dbTransform transform(orient, origin);
  transform.apply(r._rect);

  // Translate to placed location
  int xmin = r._rect.xMin() + _orig_x;
  int ymin = r._rect.yMin() + _orig_y;
  int xmax = r._rect.xMax() + _orig_x;
  int ymax = r._rect.yMax() + _orig_y;
  dbBox::create(pin, r._layer, xmin, ymin, xmax, ymax, r._mask);
}

void definPin::addPolygon(Polygon& p, dbBPin* pin)
{
  std::vector<Rect> R;

  p.decompose(R);

  std::vector<Rect>::iterator itr;

  for (itr = R.begin(); itr != R.end(); ++itr) {
    Rect& r = *itr;
    PinRect R(p._layer, r, p._mask);
    addRect(R, pin);
  }
}

void definPin::pinsEnd()
{
  dbSet<dbBTerm> bterms = _block->getBTerms();

  dbSet<dbBTerm>::iterator itr;

  for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
    dbBTerm* bterm = *itr;

    dbSet<dbBPin> bpins = bterm->getBPins();

    if (bpins.reversible() && bpins.orderReversed()) {
      bpins.reverse();
    }
  }

  std::vector<Pin>::iterator pitr;

  for (pitr = _ground_pins.begin(); pitr != _ground_pins.end(); ++pitr) {
    Pin& p = *pitr;

    dbBTerm* ground = _block->findBTerm(p._pin.c_str());

    if (ground != nullptr) {
      p._bterm->setGroundPin(ground);
    } else {
      ++_errors;
      _logger->warn(utl::ODB, 122, "error: Cannot find PIN {}", p._pin.c_str());
    }
  }

  for (pitr = _supply_pins.begin(); pitr != _supply_pins.end(); ++pitr) {
    Pin& p = *pitr;

    dbBTerm* supply = _block->findBTerm(p._pin.c_str());

    if (supply != nullptr) {
      p._bterm->setSupplyPin(supply);
    } else {
      ++_errors;
      _logger->warn(utl::ODB, 123, "error: Cannot find PIN {}", p._pin.c_str());
    }
  }
}

}  // namespace odb
