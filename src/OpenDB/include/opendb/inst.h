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
#include "dbtable2.h"
#include "master.h"
#include "net.h"
#include "parse.h"
#include "wire.h"

class Ath__iterm
{
 private:
  uint _netId;
  uint _flags;
  uint _instNameId;
  uint _mtermId;

 public:
  uint getNetId();
  void writeDB(FILE* fp);
  void readDB(FILE* fp);
  uint getInstNameId();

  friend class Ath__itermTable;
  friend class Ath__instTable;
};

class Ath__itermTable
{
 private:
  // DELETE AthArray<Ath__iterm*> *_array;
  AthPool<Ath__iterm>* _pool;

 public:
  Ath__itermTable(int n);
  ~Ath__itermTable();
  uint        addNewTerm(uint id, uint f, uint masterIndex, uint instId);
  Ath__iterm* makeTerm(uint id);
  Ath__iterm* getTerm(uint n);
  uint        getLastIndex();
  void        writeDB(FILE* fp);
  void        readDB(FILE* fp);

  friend class Ath__instTable;
};

class Ath__inst
{
 private:
  int  _x;
  int  _y;
  uint _nameId;
  uint _masterId;
  uint _firstTerm;
  uint _blockId : 10;
  uint _type : 5;
  uint _tmpMark1 : 1;
  uint _tmpMark2 : 1;
  uint _tmpMark3 : 1;
  char _orient[3];

 public:
  void reset();
  void resetMarkers();
  void writeDB(FILE* fp);
  void readDB(FILE* fp);
  void setFirstTerm(uint n);
  ;
  uint getFirstTerm();
  ;
  uint getMasterId();

  void setPosition(int x, int y, const char* orient, char* type);
  void setBlockId(uint num);
  void getTransform(Ath__trans* trans);
  void transXY(Ath__trans* trans);
  int  getX();
  int  getY();

  void setMark1();
  void resetMark1();
  bool isResetMark1();
  bool isSetMark1();

  void setId(uint n);

  uint makeZuiInst(Ath__master* m, Ath__zui* zui, Ath__hierType hier);

  friend class Ath__instTable;
};
class Ath__instTable
{
 private:
  Ath__dbtable2<Ath__inst>* _dbtable;
  Ath__masterTable*         _masterTable;

  Ath__itermTable* _itermTable;

  Ath__gridTable* _gridTable;

  Ath__netTable* _netTablePtr;
  uint           _tmpStart;
  uint           _tmpEnd;

 public:
  Ath__instTable(int iCnt, Ath__masterTable* mTable);
  ~Ath__instTable();
  void       setNetTablePtr(Ath__netTable* netTable);
  char*      getName(uint id);
  char*      getInstName(Ath__inst* inst);
  Ath__inst* addInst(char* iname, char* mname, uint flags = 0);

  Ath__inst*  makeInst(uint nameId);
  uint        getInstNameIds(char* mname, uint* A);
  Ath__inst*  getInst(char* name);
  uint        getInstTermId(char* iname, char* pname);
  uint        getItermId(Ath__iterm* iterm);
  void        setNet2Iterm2(uint termtag, uint nettag);
  void        setMark1(uint termtag);
  Ath__inst*  getInstPtr(uint termtag);
  uint        getTermCnt(uint masterId);
  Ath__iterm* getTerm(uint index);
  void        getInstBox(Ath__inst* inst, Ath__box* bb);
  void        makeInstBox(uint termTag, Ath__zui* zui);
  void        makeInstBox(Ath__inst* inst, Ath__zui* zui);
  char*       getMasterName(Ath__inst* inst);
  void        printInsts(const char* filename);
  uint        instCnt();
  void        getBbox(Ath__array1D<Ath__inst*>* instTable, Ath__box* box);
  void        getBbox(Ath__box* box, uint* minWidth, uint* minHeight);
  void        printStats(FILE* fp);
  int         readComponents(Ath__parser* parser,
                             char*        endWord,
                             int          skip,
                             uint         defUnits,
                             Ath__trans*  coordTrans = NULL);
  uint        getInstBoxes(Ath__zui* zui, Ath__hierType hier, bool skipBlocks);
  uint        getInstBoxes_linear(Ath__zui* zui, bool skipBlocks);
  uint        getInstBoxes_linear(Ath__inst* inst, Ath__box* bb);
  uint        getInstBlocks(Ath__zui* zui, Ath__hierType hier);
  Ath__inst*  getInstBox(Ath__zui* zui, uint nameId, uint nameType);
  void        defComponents(FILE*                     fp,
                            uint                      defUnits,
                            Ath__array1D<Ath__inst*>* instPtrTable);
  void        resetMark1(Ath__array1D<Ath__inst*>* instPtrTable);
  void        writeInstsDB(FILE* fp, char* instType);
  void        readInstsDB(FILE* fp, uint iCnt);
  uint        makeZuiInst(Ath__inst*    inst,
                          Ath__zui*     zui,
                          Ath__hierType hier,
                          bool          skipBlocks);

  uint       startInstIterator();
  Ath__inst* getNext();

  uint               getInstTermIndex(char* iname, char* pname);
  Ath__termDirection getTermDir2(uint instTermIndex);
  Ath__mterm*        getMtermPtr(Ath__iterm* iterm);
  Ath__mterm*        getMasterTermPtr(Ath__inst* inst, uint masterTermIndex);

  uint      createIterms();
  uint      createIterms(Ath__inst* inst);
  Ath__net* getNet(Ath__iterm* itermPtr);
  Ath__net* getNet(uint termId);

  Ath__master* getMaster(uint instId);
  Ath__master* getMaster(Ath__inst* inst);
  bool         isClockedCell(Ath__inst* inst);
  uint         depthFirstBack(Ath__inst* inst);
  uint         getIterms(Ath__array1D<Ath__iterm*>* termTable,
                         Ath__inst*                 inst,
                         Ath__termDirection         dir);
  uint         getInsts(Ath__array1D<Ath__inst*>* iTable,
                        Ath__net*                 net,
                        Ath__termDirection        dir);
  void         getMtermName(Ath__mterm* mterm, char* name);
  uint         getInstConnections(Ath__zui* zui, Ath__inst* start_inst);
  uint         getInsts(Ath__array1D<Ath__inst*>* iTable, Ath__net* net);
  uint         getIterms(Ath__array1D<Ath__iterm*>* termTable, Ath__inst* inst);
  Ath__inst*   getInstByTermId(uint termId);
  void         getItermBbox(Ath__iterm* iterm, Ath__box* bb);
  void         getInstBox(uint instId, Ath__box* bb);
  void         makeGrid(Ath__box* box, uint minCellWidth, uint minCellHeight);
};


