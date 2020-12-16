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
#include "misc_global.h"
#include "util.h"

class Ath__nameBucket
{
 private:
  char* _name;
  uint  _tag;

 public:
  void set(char* name, uint tag);
  void deallocWord();

  friend class Ath__nameTable;
};

class Ath__nameTable
{
 private:
  AthHash<int>*             _hashTable;
  AthPool<Ath__nameBucket>* _bucketPool;
  // int *nameMap; // TODO

  void allocName(char* name, uint nameId, bool hash = false);
  uint addName(char* name, uint dataId);

 public:
  ~Ath__nameTable();
  Ath__nameTable(uint n, char* zero = NULL);

  void writeDB(FILE* fp, char* nameType);
  bool readDB(FILE* fp);
  void addData(uint poolId, uint dataId);

  uint  addNewName(char* name, uint dataId);
  char* getName(uint poolId);
  uint  getDataId(int poolId);
  uint  getTagId(char* name);
  uint  getDataId(char* name,
                  uint  ignoreFlag = 0,
                  uint  exitFlag   = 0,
                  int*  nn         = 0);
};


