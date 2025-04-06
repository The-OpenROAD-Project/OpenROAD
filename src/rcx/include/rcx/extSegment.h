// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

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
