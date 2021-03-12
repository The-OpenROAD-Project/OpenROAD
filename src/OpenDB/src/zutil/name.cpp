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

#include "name.h"

void Ath__nameBucket::set(char* name, uint tag)
{
  int len = strlen(name);
  _name = new char[len + 1];
  strcpy(_name, name);
  _tag = tag;
}
void Ath__nameBucket::deallocWord()
{
  delete[] _name;
}

Ath__nameTable::~Ath__nameTable()
{
  delete _hashTable;
  delete _bucketPool;
}
Ath__nameTable::Ath__nameTable(uint n, char* zero)
{
  if (zero == NULL)
    zero = strdup("zeroName");

  _hashTable = new AthHash<int>(n, 0);
  _bucketPool = new AthPool<Ath__nameBucket>(false, 0);

  addNewName(zero, 0);
}
uint Ath__nameTable::addName(char* name, uint dataId)
{
  uint poolIndex = 0;
  Ath__nameBucket* b = _bucketPool->alloc(NULL, &poolIndex);
  b->set(name, dataId);

  _hashTable->add(b->_name, poolIndex);

  return poolIndex;
}

// ---------------------------------------------------
// save/read DB functions
// ---------------------------------------------------
void Ath__nameTable::writeDB(FILE* fp, char* nameType)
{
  fprintf(fp, "%s NAMES %d\n", nameType, _bucketPool->getCnt() - 1);
  for (uint i = 1; i < _bucketPool->getCnt(); i++)
    fprintf(fp, "%d %s\n", i, _bucketPool->get(i)->_name);
}
bool Ath__nameTable::readDB(FILE* fp)
{
  char type[16];
  char word[16];
  uint nameCnt;
  if (fscanf(fp, "%s %s %u\n", type, word, &nameCnt) != 3) {
    return false;
  }
  for (uint i = 0; i < nameCnt; i++) {
    char name[1024];
    uint nameId;
    if (fscanf(fp, "%u %s\n", &nameId, name) != 2) {
      return false;
    }
    allocName(name, nameId, true);
  }
  return true;
}
void Ath__nameTable::allocName(char* name, uint nameId, bool hash)
{
  uint poolIndex = 0;
  Ath__nameBucket* b = _bucketPool->alloc(NULL, &poolIndex);
  b->set(name, 0);

  if (poolIndex != nameId) {
    fprintf(stderr, "Mismatch between %d and %d name ids\n", poolIndex, nameId);
  }
  if (hash)
    _hashTable->add(b->_name, poolIndex);
}
void Ath__nameTable::addData(uint poolId, uint dataId)
{
  // when reading DB names, buckets have been allocated already

  Ath__nameBucket* b = _bucketPool->get(poolId);
  b->_tag = dataId;
  _hashTable->add(b->_name, poolId);
}

// ---------------------------------------------------------
// Hash Functions
// ---------------------------------------------------------
uint Ath__nameTable::addNewName(char* name, uint dataId)
{
  int n;
  if (_hashTable->get(name, n)) {
    return 0;  // This is NOT supposed to happen
  }
  // TODO: option to replace dataId

  return addName(name, dataId);
}
char* Ath__nameTable::getName(uint poolId)
{
  return _bucketPool->get(poolId)->_name;
}
uint Ath__nameTable::getDataId(int poolId)
{
  return _bucketPool->get(poolId)->_tag;
}
uint Ath__nameTable::getDataId(char* name,
                               uint ignoreFlag,
                               uint exitFlag,
                               int* nn)
{
  int n;
  if (_hashTable->get(name, n)) {
    if (nn)
      *nn = n;
    return getDataId(n);
  }

  if (ignoreFlag > 0)
    return 0;

  Ath__hashError(name, exitFlag);
  return 0;
}
uint Ath__nameTable::getTagId(char* name)
{
  int n;
  if (_hashTable->get(name, n))
    return n;

  return 0;
}
