///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array1.h"
#include "atypes.h"
#include "parse.h"
#include "util.h"

class Ath__tech;
class Ath__box;

class Ath__defPoint
{
 public:
  int  _xy[2];
  uint _width;
  uint _layer : 8;
  uint _via : 18;
  uint _new : 1;
  uint _prevXY : 1;

 public:
  void initValues();
  void setCoords(int x, int y);
  void setMidCoords(char* x, char* y);
  void setLayer(uint viaFlag, uint num, uint w);
  void print(FILE* fp, char* header = "");
};

class Ath__defPath
{
 private:
  Ath__array1D<Ath__defPoint*>* _pointArray;
  AthPool<Ath__defPoint>*       _memPool;
  Ath__box*                     _headBox;
  AthPool<Ath__box>*            _boxPoolPtr;
  FILE*                         _dbgFP;

 public:
  Ath__defPath(int n = 0);
  ~Ath__defPath();
  uint           getPointCnt() { return _pointArray->getCnt(); };
  void           setDbgFile(FILE* fp);
  int            addPoint(Ath__defPoint* p);
  int            addFirstPoint(Ath__defPoint* p);
  Ath__defPoint* createViaPoint(uint num, uint w);
  void           reuse(bool resetHead = true);
  Ath__defPoint* createPoint(char* x, char* y);
  Ath__defPoint* allocPoint();  // DEBUG
  Ath__defPoint* createFirstPoint(char* x, char* y, uint level, uint width);
  Ath__defPoint* getPoint(int ii);
  void           printPoint(FILE* fp, int idx = -1);
  void           print();
  int            getDefPathPoints(Ath__tech*   tech,
                                  Ath__parser* parser,
                                  int          start,
                                  int          makeSegs  = 0,
                                  uint         units     = 1,
                                  uint         minLength = 0,
                                  uint         nameId    = 0,
                                  uint         nameType  = 0);
  int       getNextDefPath(Ath__tech* tech, Ath__parser* parser, int* layerNum);
  void      setBoxPool(AthPool<Ath__box>* boxPool);
  void      setHeadBox(Ath__box* box);
  Ath__box* getHeadBox();
  int  getSegment(Ath__tech* tech, uint ii, int* xy1, int* xy2, int* layerNum);
  void makeCoords(uint           dir,
                  uint           wdir,
                  int*           xy1,
                  int*           xy2,
                  Ath__defPoint* p1,
                  Ath__defPoint* p2);
  Ath__box* makeSegments(Ath__tech* tech,
                         uint       minLength,
                         uint       units,
                         uint       nameId,
                         uint       nameType);
};


