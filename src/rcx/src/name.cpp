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

namespace rcx {

class NameTable::NameBucket
{
 public:
  void set(const char* name, uint tag);
  void deallocWord();

 private:
  const char* _name;
  uint _tag;

  friend class NameTable;
};

static void Ath__hashError(const char* msg, int exitFlag)
{
  fprintf(stderr, "Cannot find %s in hash table\n", msg);
  fprintf(stderr, "\nexiting ...\n");

  if (exitFlag > 0) {
    exit(1);
  }
}

void NameTable::NameBucket::set(const char* name, uint tag)
{
  int len = strlen(name);
  char* name_copy = new char[len + 1];
  strcpy(name_copy, name);
  _name = name_copy;
  _tag = tag;
}
void NameTable::NameBucket::deallocWord()
{
  delete[] _name;
}

NameTable::~NameTable()
{
  delete _hashTable;
  delete _bucketPool;
}

NameTable::NameTable(uint n, char* zero)
{
  if (zero == nullptr) {
    zero = strdup("zeroName");
  }

  _hashTable = new rcx::AthHash<int>(n, 0);
  _bucketPool = new odb::AthPool<NameBucket>(0);

  addNewName(zero, 0);
}

uint NameTable::addName(const char* name, uint dataId)
{
  int poolIndex = 0;
  NameBucket* b = _bucketPool->alloc(nullptr, &poolIndex);
  b->set(name, dataId);

  _hashTable->add(b->_name, poolIndex);

  return poolIndex;
}

// ---------------------------------------------------------
// Hash Functions
// ---------------------------------------------------------
uint NameTable::addNewName(const char* name, uint dataId)
{
  int n;
  if (_hashTable->get(name, n)) {
    return 0;  // This is NOT supposed to happen
  }
  // TODO: option to replace dataId

  return addName(name, dataId);
}

const char* NameTable::getName(uint poolId)
{
  return _bucketPool->get(poolId)->_name;
}

uint NameTable::getDataId(int poolId)
{
  return _bucketPool->get(poolId)->_tag;
}

uint NameTable::getDataId(const char* name,
                          uint ignoreFlag,
                          uint exitFlag,
                          int* nn)
{
  int n;
  if (_hashTable->get(name, n)) {
    if (nn) {
      *nn = n;
    }
    return getDataId(n);
  }

  if (ignoreFlag > 0) {
    return 0;
  }

  Ath__hashError(name, exitFlag);
  return 0;
}

}  // namespace rcx
