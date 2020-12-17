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

#include "atypes.h"
#include "wire.h"

#include "array1.h"
#include "dbtable2.h"
#include "inst.h"
#include "misc_global.h"

class Ath__iterm;
class Ath__zui;

class Ath__net
{
 private:
  uint _nameId;
  uint _firstTerm;
  uint _termOutCnt : 8;  // tri-state: 256 max
  uint _termInOutCnt : 8;
  uint _termInCnt : 16;
  uint _firstBlockTerm : 28;
  uint _blockTermCnt : 4;

  Ath__box* _headBox;

 public:
  void setId(uint n);
  void reset();
  void writeDB(FILE* fp);
  void set(uint n2, uint n3, uint n4, uint n5, uint n6, uint n7);
  void addBtermIndex(uint n);
  void setBterms(uint n, uint cnt);
  uint getBtermCnt();
  uint getBtermFirstIndex();
  uint getNameId();
  uint getTermCnt();
  uint getFirstTerm();
  void addBox(Ath__box* bb);

  friend class Ath__netTable;
};
class Ath__btermTable;
class Ath__instTable;
class Ath__inst;

class Ath__netTable
{
 private:
  Ath__dbtable2<Ath__net>* _dbtable;

  AthArray<uint>* _itermTable;

  Ath__array1D<uint>* _itermTmpTable;
  Ath__btermTable*    _btermTablePtr;
  Ath__instTable*     _instTablePtr;
  Ath__gridTile*      _gridSystem;
  AthPool<Ath__box>*  _boxPool;
  uint                _defUnits;

 public:
  Ath__netTable(int nCnt);
  ~Ath__netTable();
  Ath__box* allocBox(AthPool<Ath__box>* pool);

  void setDefUnits(uint defUnits) { _defUnits = defUnits; };

  Ath__gridTile* getGridSystem();
  void           setGrid(Ath__gridTile* v);
  void           resetTmp();
  void           setItable(Ath__instTable* itable) { _instTablePtr = itable; }
  void        setIOtable(Ath__btermTable* btable) { _btermTablePtr = btable; }
  uint        addTmpTerm(uint itermId);
  Ath__net*   makeNet(uint nameId);
  uint        addNet(char* name, int pinCnt, uint flags, bool itermOrder);
  Ath__net*   getNetPtr(uint nameId);
  Ath__net*   getNet(char* name);
  Ath__net*   getNet(uint id);
  Ath__net*   checkNet(char* name);
  char*       getName(Ath__net* m);
  char*       getName(uint id);
  uint        getTermId(uint termIndex);
  Ath__inst*  getFirstInstPtr(Ath__net* netPtr);
  Ath__inst*  getInstPtr(uint termIndex);
  Ath__mterm* getMtermPtr(uint termIndex);
  Ath__mterm* getMasterTermPtr(Ath__inst* inst, uint masterTermIndex);
  uint orderNetTerms(uint nettag, Ath__termDirection dir, bool itermOrder);
  void orderNetTerms(Ath__net* netPtr, bool itermOrder);
  void printInstTerms(FILE* fp, Ath__net* net);
  void printNetInsts(const char* filename);
  void writeNetsDB(FILE* fp, char* netType, uint globalFlag, bool powerNetFlag);
  void printStats(FILE* fp);
  uint getAllNetBoxes(Ath__zui* zui, Ath__hierType hier, Ath__boxType box);
  uint getNetBoxes(Ath__zui*     zui,
                   uint          nameId,
                   Ath__hierType hier,
                   Ath__boxType  box);
  uint getInstBoxes(Ath__zui* zui, Ath__net* net);

  uint  printPoints();
  int   readNets(uint         nameType,
                 Ath__tech*   tech,
                 Ath__parser* parser,
                 char*        endWord,
                 int          skip,
                 int          skipTerms,
                 int          skipBoxes,
                 uint         minLen);
  uint  groupConnections(Ath__tech* techTable, Ath__quadTable* quad);
  uint  addObs(Ath__quadTable* quad);
  FILE* openNetFile(int cnt, uint ii);

  void defNets(FILE* fp, Ath__array1D<uint>* netNameIdArray, uint skipShapes);
  uint getInstPtrs(Ath__array1D<uint>*       netNameIdArray,
                   Ath__array1D<Ath__inst*>* instPtrTable);
  void readNetsDB(FILE*         fp,
                  uint          bCnt,
                  Ath__hierType hier,
                  bool          consumeOnly = false);
  Ath__box* readBoxList(FILE*         fp,
                        uint          netId,
                        Ath__hierType hier,
                        bool          consumeOnly = false);
  uint      getStats(uint  level,
                     uint* dir,
                     uint* minWidth,
                     uint* maxWidth,
                     uint  minLen);
  void      makePowerGrid(Ath__layerTable* layerTable, Ath__box* bb);
  uint      add2Grid(Ath__grid* grid);

  uint        startNetIterator();
  Ath__net*   getNextNet();
  Ath__iterm* getItermPtr(uint termIndex);
  void        printInstTerms2(FILE* fp, Ath__net* net);
  uint        getInstTermIndex(uint termIndex);
  uint        getIterms(Ath__array1D<Ath__iterm*>* itermTable, Ath__net* net);
  void        printInstTerms3(FILE* fp, Ath__net* net);
  uint        getInstConns(uint netId, Ath__zui* zui);
};


