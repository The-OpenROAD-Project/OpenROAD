///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, IC BENCH, Dimitris Fotakis
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
  if (w2 == NULL)
    return -1;
  if (w1 == NULL)
    return -1;
  return w2->getBase() - (w1->getBase() + w1->getWidth());
}
// void extSegment::setMets(int metover, int metUnder);
}  // namespace rcx
