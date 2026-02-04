// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "odb/util.h"
#include "rcx/util.h"

namespace utl {
class Logger;
}

namespace rcx {

class extViaModel
{
 public:
  const char* _viaName;
  double _res;
  uint32_t _cutCnt;
  uint32_t _dx;
  uint32_t _dy;
  uint32_t _topMet;
  uint32_t _botMet;

 public:
  extViaModel(const char* name,
              double R,
              uint32_t cCnt,
              uint32_t dx,
              uint32_t dy,
              uint32_t top,
              uint32_t bot)
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
  void printVia(FILE* fp, uint32_t corner);
};
}  // namespace rcx
