// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "odb/util.h"
#include "util.h"

namespace utl {
class Logger;
}

namespace rcx {

using utl::Logger;

class extViaModel
{
 public:
  const char* _viaName;
  double _res;
  uint _cutCnt;
  uint _dx;
  uint _dy;
  uint _topMet;
  uint _botMet;

 public:
  extViaModel(const char* name,
              double R,
              uint cCnt,
              uint dx,
              uint dy,
              uint top,
              uint bot)
  {
    _viaName = strdup(name);
    _res = R;
    _cutCnt = cCnt;
    _dx = dx;
    _dy = dy;
    _topMet = top;
    _botMet = bot;
  }
  extViaModel()
  {
    _viaName = nullptr;
    _res = 0;
    _cutCnt = 0;
    _dx = 0;
    _dy = 0;
    _topMet = 0;
    _botMet = 0;
  }
  void printViaRule(FILE* fp);
  void writeViaRule(FILE* fp);
  // dkf 09182024
  void printVia(FILE* fp, uint corner);
};
}  // namespace rcx
