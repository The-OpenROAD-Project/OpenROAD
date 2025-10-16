// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "odb/util.h"
#include "rcx/util.h"

namespace rcx {

using uint = unsigned int;

class NameTable
{
 public:
  NameTable(uint n, char* zero = nullptr);
  ~NameTable();

  uint addNewName(const char* name, uint dataId);
  const char* getName(uint poolId);
  uint getDataId(const char* name,
                 uint ignoreFlag = 0,
                 uint exitFlag = 0,
                 int* nn = nullptr);

 private:
  class NameBucket;

  uint addName(const char* name, uint dataId);
  uint getDataId(int poolId);

  AthHash<int>* _hashTable;
  odb::AthPool<NameBucket>* _bucketPool;
};

}  // namespace rcx
