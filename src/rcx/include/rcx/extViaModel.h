
///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
// derivative file from extRCap.h

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
    _viaName = NULL;
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
