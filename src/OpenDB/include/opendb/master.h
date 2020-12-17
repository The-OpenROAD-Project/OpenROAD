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

#include "array1.h"
#include "dbtable2.h"
#include "misc_global.h"
#include "parse.h"

class Ath__mterm
{
 private:
  uint               _id;
  Ath__termDirection _dir;
  uint               _type;
  uint               _flags;
  uint               _order;

 public:
  ~Ath__mterm();
  char*              getName();
  uint               getId();
  Ath__termDirection getDir();
  void               writeDB(FILE* fp);
  void               readDB(FILE* fp);
  uint               getOrder();

  friend class Ath__mtermTable;
  friend class Ath__masterTable;
};

class Ath__mtermTable
{
 private:
  Ath__dbtable2<Ath__mterm>* _dbtable;

  void initTypes();

 public:
  Ath__mtermTable(int n);
  ~Ath__mtermTable();
  char*       getName(uint id);
  uint        getTermId(char* name);
  Ath__mterm* getTermPtr(char* name);
  uint        startMtermIterator();
  Ath__mterm* getNextMterm();
  Ath__mterm* newTerm();
  Ath__mterm* addTerm(char*              pname,
                      char*              mname,
                      Ath__signalType    t,
                      Ath__termDirection d,
                      uint               f);

  Ath__mterm* makeMterm(uint nameId);

  Ath__mterm* getTermPtr(uint id);
  uint        getCnt();
  void        writeDB(FILE* fp);
  void        readDB(FILE* fp);

  friend class Ath__masterTable;
};
class Ath__master
{
 private:
  uint _nameId;
  uint _firstTerm;
  uint _termCnt;
  uint _height;
  uint _width;
  uint _type;
  uint _cellType;
  uint _clkFlag;

 public:
  void set(uint n);
  void setSize(uint h, uint w);
  void setType(uint n);
  void setCellType(uint n);

  uint getCellType();
  uint getWidth();
  uint getHeight();
  bool isClockedCell();

  void writeDB(FILE* fp);
  void readDB(FILE* fp);

  friend class Ath__masterTable;
};

class Ath__masterTable
{
 private:
  Ath__dbtable2<Ath__master>* _dbtable;
  Ath__mtermTable*            _mtermTable;
  Ath__array1D<Ath__mterm*>*  _indexTermTable;

 public:
  Ath__masterTable(int mCnt);
  ~Ath__masterTable();
  Ath__master* addMaster(char* name, uint flags);
  Ath__master* addMaster(uint nameId);
  Ath__master* getMaster(char* name);
  bool         isCoverMaster(char* name);
  char*        getName(Ath__master* m);
  char*        getName(uint id);
  uint         getTermCnt(uint mtag);
  void         addNewTerm(char*              pname,
                          char*              mname,
                          Ath__signalType    t,
                          Ath__termDirection d,
                          uint               flags);

  void addNewPinTag2(Ath__mterm* t, Ath__master* m, uint mtag, uint index);
  uint makePinTag2(char* pname, uint masterId, uint itag);
  Ath__termDirection getTermDir(uint termId);
  Ath__master*       getMaster(uint id);
  Ath__mterm*        getMtermPtr(uint termId);
  void               printStats(FILE* fp);
  void               writeMastersDB(FILE* fp, char* masterType);
  void               readMastersDB(FILE* fp, uint mCnt);

  Ath__signalType    getSignalType(char* use);
  Ath__termDirection getPinDirection(char* dir);
  int                readLefMacro(Ath__parser* parse, int skipSupply);

  uint         startMasterIterator();
  Ath__master* getNextMaster();
  uint         getMasterId(char* name);

  uint        orderMterms(Ath__termDirection dir,
                          Ath__master*       m,
                          uint               firstTermId,
                          uint               lastTermId,
                          uint               tCnt);
  void        orderMterms(Ath__master* m, uint firstTermId, uint lastTermId);
  Ath__mterm* getTermPtr(uint id);
  Ath__mterm* getTermPtr(char* pname, uint masterId);
  Ath__mterm* getMtermPtr(uint masterId, uint masterTermIndex);
  uint        getTermCnt();
  void        getMtermName(Ath__mterm* mterm, char* name);
};


