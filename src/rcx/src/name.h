// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rcx/util.h"

namespace rcx {

class NameTable
{
 public:
  NameTable(uint32_t n, char* zero = nullptr);
  ~NameTable();

  uint32_t addNewName(const char* name, uint32_t dataId);
  const char* getName(uint32_t poolId);
  uint32_t getDataId(const char* name,
                     uint32_t ignoreFlag = 0,
                     uint32_t exitFlag = 0,
                     int* nn = nullptr);

 private:
  class NameBucket;

  uint32_t addName(const char* name, uint32_t dataId);
  uint32_t getDataId(int poolId);

  AthHash<int>* _hashTable;
  AthPool<NameBucket>* _bucketPool;
};

}  // namespace rcx
