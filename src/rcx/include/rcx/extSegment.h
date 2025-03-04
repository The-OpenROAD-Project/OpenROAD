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

#pragma once

#include <dbUtil.h>

#include "extprocess.h"
#include "grids.h"
#include "odb/db.h"
#include "odb/dbExtControl.h"
#include "odb/dbShape.h"
#include "odb/odb.h"
#include "odb/util.h"

#ifndef _WIN32
#define D2I_ROUND (name) rint(name)
#else
#define D2I_ROUND (name) name
#endif

// #define NEW_GS_FLOW

#define HI_ACC_1
#ifdef HI_ACC_1
#define HI_ACC_2
#define DAVID_ACC_08_02
#define GS_CROSS_LINES_ONLY 1
#define BUG_LAYER_CNT 1
#endif

#include <map>

namespace utl {
class Logger;
}

namespace rcx {
using namespace odb;
class Wire;

class extMeasure;

using utl::Logger;

class extSegment  // assume cross-section on the z-direction
{
 public:
  uint _dir;
  int _xy;
  int _base;
  int _len;
  int _width;
  int _dist;
  int _dist_down;

  int _ll[2];
  int _ur[2];
  uint _met;
  int _metUnder;
  int _metOver;
  uint _id;

  Wire* _wire;
  Wire* _up;
  Wire* _down;

  // extSegment(uint dir, Wire *w2, int dist);
  //  ~extSegment();

  // extSegment();
  // extSegment(uint d, Wire *w, int xy, int len, Wire *up, Wire
  // *down, int metOver=-1, int metUnder=-1);
  void set(uint d,
           Wire* w,
           int xy,
           int len,
           Wire* up,
           Wire* down,
           int metOver = -1,
           int metUnder = -1);
  int GetDist(Wire* w1, Wire* w2);
  int setUpDown(bool up, Wire* w1);

  friend class extMeasure;
  friend class extRCModel;
};

}  // namespace rcx
