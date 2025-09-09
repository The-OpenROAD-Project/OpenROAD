// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "odb/util.h"
#include "util.h"

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

class NameBucket
{
 private:
  char* _name;
  uint _tag;

 public:
  void set(char* name, uint tag);
  void deallocWord();

  friend class NameTable;
};

class NameTable
{
 private:
  AthHash<int>* _hashTable;
  odb::AthPool<NameBucket>* _bucketPool;
  // int *nameMap; // TODO

  void allocName(char* name, uint nameId, bool hash = false);
  uint addName(char* name, uint dataId);

 public:
  ~NameTable();
  NameTable(uint n, char* zero = nullptr);

  void writeDB(FILE* fp, char* nameType);
  bool readDB(FILE* fp);
  void addData(uint poolId, uint dataId);

  uint addNewName(char* name, uint dataId);
  char* getName(uint poolId);
  uint getDataId(int poolId);
  uint getTagId(char* name);
  uint getDataId(char* name,
                 uint ignoreFlag = 0,
                 uint exitFlag = 0,
                 int* nn = 0);
};

}  // namespace rcx
