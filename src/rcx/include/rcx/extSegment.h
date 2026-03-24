// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>

#include "odb/db.h"
#include "odb/dbExtControl.h"
#include "odb/dbShape.h"
#include "odb/util.h"
#include "rcx/dbUtil.h"
#include "rcx/extprocess.h"
#include "rcx/grids.h"

#ifndef _WIN32
#define D2I_ROUND (name) rint(name)
#else
#define D2I_ROUND (name) name
#endif

#define HI_ACC_1
#ifdef HI_ACC_1
#define HI_ACC_2
#define DAVID_ACC_08_02
#define GS_CROSS_LINES_ONLY 1
#define BUG_LAYER_CNT 1
#endif

namespace utl {
class Logger;
}

namespace rcx {
class Wire;

class extMeasure;

class extSegment  // assume cross-section on the z-direction
{
 public:
  uint32_t _dir;
  int _xy;
  int _base;
  int _len;
  int _width;
  int _dist;
  int _dist_down;

  int _ll[2];
  int _ur[2];
  uint32_t _met;
  int _metUnder;
  int _metOver;
  uint32_t _id;

  Wire* _wire;
  Wire* _up;
  Wire* _down;

  // extSegment(uint32_t dir, Wire *w2, int dist);
  //  ~extSegment();

  // extSegment();
  // extSegment(uint32_t d, Wire *w, int xy, int len, Wire *up, Wire
  // *down, int metOver=-1, int metUnder=-1);
  void set(uint32_t d,
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
