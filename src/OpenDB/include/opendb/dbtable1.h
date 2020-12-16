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

#include "array2d.h"
#include "atypes.h"
#include "itype.h"
#include "name.h"
#include "util.h"

template <class T>
class Ath__dbtable
{
 private:
  Ath__array2d<T*>* _array;
  Ath__nameTable*   _nameTable;
  AthPool<T>*       _pool;

 public:
  Ath__dbtable(uint cnt, bool, bool memDbg = false)
  {
    /*
    _array= NULL;
    if (use2darray)
    */
    _array = new Ath__array2d<T*>(32, 512, 256, 8);

    _nameTable = new Ath__nameTable(cnt);
    _pool      = new AthPool<T>(memDbg, cnt);
  }
  Ath__dbtable()
  {
    if (_array != NULL)
      delete _array;
    delete _nameTable;
    delete _pool;
  }

 private:
 public:
  char* getName(uint id) { return _nameTable->getName(id); }
  uint  getBankCnt() { return _array->getBankCnt(); }
  uint  getCnt(uint ii) { return _array->getCnt(ii); }

  T* get(uint ii, uint jj) { return _array->get(ii, jj); }
  T* getPtr(uint mtag)
  {
    uint num;
    uint type = getType(mtag, &num);

    return _array->get(type, num);
  }
  uint getType(uint tag)
  {
    Ath__itype w;
    w.setAll(tag);

    return w.getType();
  }
  uint getType(uint tag, uint* num)
  {
    Ath__itype w;
    w.setAll(tag);

    uint index;
    return w.get(&index, num);
  }
  uint getTypeAndIndex(uint tag, uint* num)
  {
    Ath__itype w;
    w.setAll(tag);

    uint index;
    return w.get(&index, num, 0);
  }
  uint getTypeNum(uint tag, uint* num)
  {
    Ath__itype w;
    w.setAll(tag);

    uint index;
    return w.get(&index, num);
  }
  uint getIndex(uint tag)
  {
    Ath__itype w;
    w.setAll(tag);

    uint index;
    uint num;
    w.get(&index, &num);
    return index;
  }
  uint getIndex(char* master_pin_name)
  {
    return getIndex(getTag(master_pin_name));
  }
  uint getNextTag(uint cnt, uint* t)
  {
    Ath__itype w;
    uint       type = w.cnt2type(cnt);

    w.setType(type);
    w.set(0, _array->getCnt(type));

    *t = type;

    return w.getAll();
  }
  uint getNextTag(uint type)
  {
    Ath__itype w;

    w.setType(type);
    w.set(0, _array->getCnt(type));

    return w.getAll();
  }
  uint getTermTag(uint type_num, uint index)
  {
    Ath__itype w;
    w.setAll(type_num);
    w.setIndex(index);
    return w.getAll();
  }
  uint getTermTag(uint type_num, char* name)
  {
    uint index = getIndex(name);

    Ath__itype w;
    w.setAll(type_num);
    w.setIndex(index);

    return w.getAll();
  }

  uint addNewName(char* name, uint tag)
  {
    return _nameTable->addNewName(name, tag);
  }
  void writeNamesDB(FILE* fp, char* nameType)
  {
    _nameTable->writeDB(fp, nameType);
  }
  T* getNewByTag(uint nameId, uint tag)
  {
    _nameTable->addNewName(reinterpret_cast<char*>(static_cast<long>(nameId)),
                           tag);

    uint num;
    uint type = getTypeNum(tag, &num);

    uint addr;
    T*   a = _pool->alloc(&addr);
    uint n = _array->add(type, a);
    if (n != num) {
      fprintf(stderr, "ERROR: db: getNewByType netTag=%u", tag);
    }
    return a;
  }
  T* getNewByType(char* name, uint type, uint* nameId)
  {
    uint tag = getNextTag(type);

    T* a = _pool->alloc();

    *nameId = _nameTable->addNewName(name, tag);

    _array->add(type, a);

    return a;
  }
  T* getNew(char* name, uint cnt, uint* nameId, uint skipName = 0)
  {
    uint type;
    uint tag = getNextTag(cnt, &type);

    uint addr;
    T*   a = _pool->alloc(NULL, &addr);

    if (skipName == 0) {
      *nameId = _nameTable->addNewName(name, tag);
      _array->add(type, a);
    }
    return a;
  }
  uint getTag(char* name) { return _nameTable->getDataId(name); }
  T*   getPtr(char* name)
  {
    uint num;
    uint type = getType(_nameTable->getDataId(name), &num);
    if (type + num == 0)
      return NULL;

    return _array->get(type, num);
  }
  T* checkPtr(char* name)
  {
    uint num;
    uint type = getType(_nameTable->getDataId(name, 1, 1), &num);
    if (type + num == 0)
      return NULL;

    return _array->get(type, num);
  }
  T* getPtr(char* name, uint* itag)
  {
    uint tag = _nameTable->getDataId(name);

    uint num;
    uint type = getType(tag, &num);

    *itag = tag;
    return _array->get(type, num);
  }
  T* getPtrByNameId(uint id) { return getPtr(getTag(_nameTable->getName(id))); }

  uint getTag(uint id) { return getTag(_nameTable->getName(id)); }

  uint getTotalCnt()
  {
    uint cnt = 0;
    for (uint ii = 0; ii < _array->getBankCnt(); ii++)
      cnt += _array->getCnt(ii);
    return cnt;
  }
  void readNamesDB(FILE* fp) { _nameTable->readDB(fp); }
  uint writeDBheader(FILE* fp, char* keyword, char* obj_type)
  {
    uint cnt = getTotalCnt();

    fprintf(fp, "%s %s %d\n", keyword, obj_type, cnt);

    return cnt;
  }
  uint readDBheader(FILE* fp, char* keyword, char* obj_type, uint)
  {
    int cnt;

    fprintf(fp, "%s %s %d\n", keyword, obj_type, &cnt);

    return cnt;
  }
  uint startIterator() { return _array->startIteratorAll(); }
  T*   getNext()
  {
    T* a = NULL;
    if (_array->getNext(&a) > 0)
      return a;
    else
      return NULL;
  }
};


