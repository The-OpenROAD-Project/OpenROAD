
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

#include "dbUtil.h"
#include "rcx/extMeasureRC.h"
#include "rcx/extRCap.h"
#include "rcx/extSegment.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;
using namespace odb;

int extMeasureRC::GetDx1Dx2(Wire* w1, Wire* w2, int& dx2)
{
  int dx1 = w2->getXY() - w1->getXY();
  dx2 = w2->getLen() - w1->getLen() + dx1;
  return dx1;
}
int extMeasureRC::GetDistance(Wire* w1, Wire* w2)
{
  if (w2->getBase() >= w1->getBase()) {
    int dx1 = w2->getBase() - (w1->getBase() + w1->getWidth());
    return dx1;
  } else {
    int dx1 = w1->getBase() - (w2->getBase() + w2->getWidth());
    return dx1;
  }
}
int extMeasureRC::GetDx1Dx2(int xy1, int len1, Wire* w2, int& dx2)
{
  int dx1 = w2->getXY() - xy1;
  dx2 = w2->getLen() - len1 + dx1;
  return dx1;
}
int extMeasureRC::GetDx1Dx2(int xy1, int len1, extSegment* w2, int& dx2)
{
  int dx1 = w2->_xy - xy1;
  dx2 = w2->_len - len1 + dx1;
  return dx1;
}
bool extMeasureRC::OverlapOnly(int xy1, int len1, int xy2, int len2)
{
  if (xy1 >= xy2 && xy1 <= xy2 + len2)
    return true;
  if (xy2 >= xy1 && xy2 <= xy1 + len1)
    return true;
  if (xy1 + len1 >= xy2 && xy1 + len1 <= xy2 + len2)
    return true;
  if (xy2 + len2 >= xy1 && xy2 + len2 <= xy1 + len1)
    return true;

  return false;
}
bool extMeasureRC::Enclosed(int x1, int x2, int y1, int y2)
{
  // segmet (x1,x2) vs. (y1,y2)
  if (x1 >= y1 && x1 <= y2 && x2 >= y1 && x2 <= y2)
    return true;

  return false;
}
uint extMeasureRC::FindSegments(bool lookUp,
                                uint dir,
                                int maxDist,
                                Wire* w1,
                                int xy1,
                                int len1,
                                Wire* w2_next,
                                Ath__array1D<extSegment*>* segTable)
{
  if (w2_next == NULL)
    return 0;

  int dist = GetDistance(w1, w2_next);
  if (dist > maxDist) {
    extSegment* s = _seqmentPool->alloc();
    s->set(dir, w1, xy1, len1, NULL, NULL);

    segTable->add(s);
    return 0;
  }
  if (xy1 + len1 < w2_next->getXY())  // no overlap and w2 too far on the right
  {
    Wire* next_up_down = lookUp ? w2_next->getUpNext() : w2_next->getDownNext();
    FindSegments(lookUp, dir, maxDist, w1, xy1, len1, next_up_down, segTable);
    return 0;
  }

  Wire* prev = NULL;
  Wire* w2 = w2_next;
  for (; w2 != NULL; w2 = w2->getNext()) {
    if (OverlapOnly(xy1, len1, w2->getXY(), w2->getLen()))
      break;

    if (prev != NULL && Enclosed(xy1, xy1 + len1, prev->getXY(), w2->getXY()))
      return 0;

    prev = w2;
  }
  if (w2 == NULL) {
    Wire* next_up_down = lookUp ? w2_next->getUpNext() : w2_next->getDownNext();
    FindSegments(lookUp, dir, maxDist, w1, xy1, len1, next_up_down, segTable);
    return 0;
  }
  uint cnt = 0;
  int xy2 = w2->getXY() + w2->getLen();
  int dx2;
  int dx1 = GetDx1Dx2(xy1, len1, w2, dx2);

  if (dx1 <= 0) {    // Covered Left
    if (dx2 >= 0) {  // covered Right

      extSegment* s = _seqmentPool->alloc();
      s->set(dir, w1, xy1, len1, NULL, NULL);

      segTable->add(s);
      s->setUpDown(lookUp, w2);
    } else {  // not covered right
      extSegment* s = _seqmentPool->alloc();
      s->set(dir, w1, xy1, xy2 - xy1, NULL, NULL);
      s->setUpDown(lookUp, w2);
      segTable->add(s);

      Wire* next = w2->getNext();
      if (next != NULL
          && next->getXY() <= w1->getXY() + w1->getLen()) {  // overlap
        FindSegments(lookUp, dir, maxDist, w1, xy2, -dx2, next, segTable);
      } else {
        next = lookUp ? w2_next->getUpNext() : w2_next->getDownNext();
        FindSegments(lookUp, dir, maxDist, w1, xy2, -dx2, next, segTable);
      }
    }
  } else {  // Open Left
    Wire* next = lookUp ? w2_next->getUpNext() : w2_next->getDownNext();
    FindSegments(
        lookUp, dir, maxDist, w1, xy1, dx1, next, segTable);  // white space
    if (dx2 >= 0) {                                           // covered Right
      extSegment* s = _seqmentPool->alloc();
      s->set(dir, w1, w2->getXY(), xy1 + len1 - w2->getXY(), NULL, NULL);
      segTable->add(s);
      s->setUpDown(lookUp, w2);
    } else {  // not covered right
      extSegment* s = _seqmentPool->alloc();
      s->set(dir, w1, w2->getXY(), w2->getLen(), NULL, NULL);
      segTable->add(s);
      s->setUpDown(lookUp, w2);

      Wire* next = w2->getNext();
      if (next != NULL
          && next->getXY() <= w1->getXY() + w1->getLen()) {  // overlap
        FindSegments(lookUp, dir, maxDist, w1, xy2, -dx2, next, segTable);
      } else {
        next = lookUp ? w2_next->getUpNext() : w2_next->getDownNext();
        FindSegments(lookUp, dir, maxDist, w1, xy2, -dx2, next, segTable);
      }
    }
  }
  return cnt;
}

}  // namespace rcx
