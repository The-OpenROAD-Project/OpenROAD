// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "rcx/extSegment.h"

#include "dbUtil.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

#ifdef HI_ACC_1
#define FRINGE_UP_DOWN
#endif
// #define CHECK_SAME_NET
// #define MIN_FOR_LOOPS

namespace rcx {

using utl::RCX;
using namespace odb;

/*
extSegment::extSegment(uint d, Wire *w2, int dist)
{
    _wire= w2;
    _dir= d;
    _dist= dist;
}
*/
/* Working
 extSegment::extSegment(uint dir, Wire *w, int xy, int len, Wire *up,
 Wire *down, int metOver, int metUnder)
 {
     _dir = dir;
     _wire = w;

     _base = w->getBase();
     _width = w->getWidth();
     _xy = xy;
     _len = len;

     uint d = !dir;
     _ll[d] = xy;
     _ur[d] = xy + len;
     _ll[dir] = w->getBase();
     _ur[dir] = _ll[dir] + w->getWidth();

     _up = up;
     _down = down;
     _dist= GetDist(_wire, _up);
     _dist_down = GetDist(_down, _wire);

     _metUnder= metUnder;
     _metOver= metOver;
 }
 */
void extSegment::set(uint dir,
                     Wire* w,
                     int xy,
                     int len,
                     Wire* up,
                     Wire* down,
                     int metOver,
                     int metUnder)
{
  _dir = dir;
  _wire = w;

  _base = w->getBase();
  _width = w->getWidth();
  _xy = xy;
  _len = len;

  uint d = !dir;
  _ll[d] = xy;
  _ur[d] = xy + len;
  _ll[dir] = w->getBase();
  _ur[dir] = _ll[dir] + w->getWidth();

  _up = up;
  _down = down;
  _dist = GetDist(_wire, _up);
  _dist_down = GetDist(_down, _wire);

  _metUnder = metUnder;
  _metOver = metOver;
}
int extSegment::setUpDown(bool up, Wire* w1)
{
  if (up) {
    _up = w1;
    _dist = GetDist(_wire, _up);
    return _dist;
  }
  _down = w1;
  _dist_down = GetDist(_down, _wire);
  return _dist_down;
}
int extSegment::GetDist(Wire* w1, Wire* w2)
{
  if (w2 == nullptr)
    return -1;
  if (w1 == nullptr)
    return -1;
  return w2->getBase() - (w1->getBase() + w1->getWidth());
}
// void extSegment::setMets(int metover, int metUnder);
}  // namespace rcx
