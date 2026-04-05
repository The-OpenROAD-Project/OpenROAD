// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "name.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rcx/util.h"

namespace rcx {

class NameTable::NameBucket
{
 public:
  void set(const char* name, uint32_t tag);
  void deallocWord();

 private:
  const char* _name;
  uint32_t _tag;

  friend class NameTable;
};

static void hashError(const char* msg, int exitFlag)
{
  fprintf(stderr, "Cannot find %s in hash table\n", msg);
  fprintf(stderr, "\nexiting ...\n");

  if (exitFlag > 0) {
    exit(1);
  }
}

void NameTable::NameBucket::set(const char* name, uint32_t tag)
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

NameTable::NameTable(uint32_t n, char* zero)
{
  bool free_zero = false;
  if (zero == nullptr) {
    zero = strdup("zeroName");
    free_zero = true;
  }

  _hashTable = new AthHash<int, false>(n);
  _bucketPool = new AthPool<NameBucket>(0);

  addNewName(zero, 0);
  if (free_zero) {
    free(zero);
  }
}

uint32_t NameTable::addName(const char* name, uint32_t dataId)
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
uint32_t NameTable::addNewName(const char* name, uint32_t dataId)
{
  int n;
  if (_hashTable->get(name, n)) {
    return 0;  // This is NOT supposed to happen
  }
  // TODO: option to replace dataId

  return addName(name, dataId);
}

const char* NameTable::getName(uint32_t poolId)
{
  return _bucketPool->get(poolId)->_name;
}

uint32_t NameTable::getDataId(int poolId)
{
  return _bucketPool->get(poolId)->_tag;
}

uint32_t NameTable::getDataId(const char* name,
                              uint32_t ignoreFlag,
                              uint32_t exitFlag,
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

  hashError(name, exitFlag);
  return 0;
}

}  // namespace rcx
