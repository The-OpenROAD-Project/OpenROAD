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

#include "box.h"
#include "layer.h"
#include "misc_global.h"

class Ath__parser;
class Ath__box;
class Ath__layerTable;

class Ath__masterVia
{
 private:
  uint      _nameId;
  uint      _num;
  uint      _cutNum;
  uint      _width;
  uint      _length;
  double    _resPerCut;  // per square
  double    _res;        // per square
  double    _ccCap;      // cap pcoupling per nm
  double    _totCap;     // cap pcoupling per nm
  Ath__box* _cutBoxList;
  uint      _cutCnt;
  Ath__box* _layerBoxList[2];
  uint      _layer[2];
  uint      _level[2];
  char*     foreign;
  uint      _signalFlag : 1;
  uint      _defFlag : 1;
  uint      _defaultFlag : 1;

 public:
  void      reset();
  void      printVia(FILE* fp);
  void      writeBoxListDB(Ath__box* list, char* name, FILE* fp);
  Ath__box* readBoxList(FILE* fp);
  void      writeDB(FILE* fp);
  void      readDB(FILE* fp);

  friend class Ath__masterViaTable;
};

class Ath__masterViaTable
{
 private:
  Ath__nameTable*            _nameTable;
  AthArray<Ath__masterVia*>* _viaArray;
  AthPool<Ath__masterVia>*   _pool;
  Ath__layerTable*           _layerTablePtr;
  uint                       _level2via[1024];
  uint                       _via2level[1024];

  // for lef reader
  int _lefViaWordCnt;
  int _newViaCnt;

  // int *nameMap; // TODO

  friend class Ath__tech;

 public:
  ~Ath__masterViaTable();
  Ath__masterViaTable(uint n);
  void            setLayerTablePtr(Ath__layerTable* v);
  uint            getViaCnt();
  Ath__masterVia* allocVia(uint nameId, uint num);
  Ath__masterVia* allocVia(char* name);
  uint            newVia(uint defaultVia, char* name);
  void            writeDB(FILE* fp);
  void            readDB(FILE* fp);
  void            setLayer(uint viaId, char* layer);
  void            checkViaLayers(FILE* fp, uint viaId);
  char*           getName(uint id);
  uint            getOtherLevel(Ath__masterVia* via, uint level);
  uint            switchLevel(uint viaId, char* layerName);
  uint            switchLevel(uint viaId, uint level);
  Ath__masterVia* getVia(int id);
  uint            getViaId(char* name, uint ignoreFlag = 0, uint exitFlag = 0);
  Ath__masterVia* getVia(char* name);
  int             readVia(Ath__parser* parser, char* endWord, int units);
  void            printVias(FILE* fp);
  void            checkCounters();
};


