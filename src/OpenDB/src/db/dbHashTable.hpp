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

#include "dbHashTable.h"
#include "dbCore.h"

namespace odb {

inline unsigned int hash_string(const char* str)
{
  unsigned int hash = 0;
  int          c;

  while ((c = *str++) != '\0')
    hash = c + (hash << 6) + (hash << 16) - hash;

  return hash;
}

template <class T>
dbHashTable<T>::dbHashTable()
{
  _obj_tbl     = NULL;
  _num_entries = 0;
}

template <class T>
dbHashTable<T>::dbHashTable(const dbHashTable<T>& t)
    : _hash_tbl(t._hash_tbl), _num_entries(t._num_entries), _obj_tbl(t._obj_tbl)
{
}

template <class T>
dbHashTable<T>::~dbHashTable()
{
}

template <class T>
bool dbHashTable<T>::operator==(const dbHashTable<T>& rhs) const
{
  if (_num_entries != rhs._num_entries)
    return false;

  if (_hash_tbl != rhs._hash_tbl)
    return false;

  return true;
}

template <class T>
void dbHashTable<T>::growTable()
{
  uint    sz = _hash_tbl.size();
  dbId<T> entries;
  uint    i;

  for (i = 0; i < sz; ++i) {
    dbId<T> cur = _hash_tbl[i];

    while (cur != 0) {
      T*      entry      = _obj_tbl->getPtr(cur);
      dbId<T> next       = entry->_next_entry;
      entry->_next_entry = entries;
      entries            = entry->getOID();
      cur                = next;
    }

    _hash_tbl[i] = 0;
  }

  // double the size of the hash-table
  dbId<T> nullId;
  for (i = 0; i < sz; ++i)
    _hash_tbl.push_back(nullId);

  // reinsert the entries
  sz          = _hash_tbl.size() - 1;
  dbId<T> cur = entries;

  while (cur != 0) {
    T*       entry     = _obj_tbl->getPtr(cur);
    dbId<T>  next      = entry->_next_entry;
    uint     hid       = hash_string(entry->_name) & sz;
    dbId<T>& e         = _hash_tbl[hid];
    entry->_next_entry = e;
    e                  = entry->getOID();
    cur                = next;
  }
}

template <class T>
void dbHashTable<T>::shrinkTable()
{
  uint    sz = _hash_tbl.size();
  dbId<T> entries;
  uint    i;

  for (i = 0; i < sz; ++i) {
    dbId<T> cur = _hash_tbl[i];

    while (cur != 0) {
      T*      entry      = _obj_tbl->getPtr(cur);
      dbId<T> next       = entry->_next_entry;
      entry->_next_entry = entries;
      entries            = entry->getOID();
      cur                = next;
    }

    _hash_tbl[i] = 0;
  }

  // TODO: add method to dbPagedVector to resize the table
  _hash_tbl.clear();
  sz >>= 1;

  // halve the size of the hash-table
  dbId<T> nullId;
  for (i = 0; i < sz; ++i)
    _hash_tbl.push_back(nullId);

  sz -= 1;
  // reinsert the entries
  dbId<T> cur = entries;

  while (cur != 0) {
    T*       entry     = _obj_tbl->getPtr(cur);
    dbId<T>  next      = entry->_next_entry;
    uint     hid       = hash_string(entry->_name) & sz;
    dbId<T>& e         = _hash_tbl[hid];
    entry->_next_entry = e;
    e                  = entry->getOID();
    cur                = next;
  }
}

template <class T>
void dbHashTable<T>::insert(T* object)
{
  ++_num_entries;
  uint sz = _hash_tbl.size();

  if (sz == 0) {
    dbId<T> nullId;
    _hash_tbl.push_back(nullId);
    sz = 1;
  } else {
    uint r = _num_entries / sz;

    if (r > CHAIN_LENGTH) {
      growTable();
      sz = _hash_tbl.size();
    }
  }

  uint     hid        = hash_string(object->_name) & (sz - 1);
  dbId<T>& e          = _hash_tbl[hid];
  object->_next_entry = e;
  e                   = object->getOID();
}

template <class T>
T* dbHashTable<T>::find(const char* name)
{
  uint sz = _hash_tbl.size();

  if (sz == 0)
    return 0;

  uint    hid = hash_string(name) & (sz - 1);
  dbId<T> cur = _hash_tbl[hid];

  while (cur != 0) {
    T* entry = _obj_tbl->getPtr(cur);

    if (strcmp(entry->_name, name) == 0)
      return entry;

    cur = entry->_next_entry;
  }

  return NULL;
}

template <class T>
int dbHashTable<T>::hasMember(const char* name)
{
  uint sz = _hash_tbl.size();

  if (sz == 0)
    return false;

  uint    hid = hash_string(name) & (sz - 1);
  dbId<T> cur = _hash_tbl[hid];

  while (cur != 0) {
    T* entry = _obj_tbl->getPtr(cur);

    if (strcmp(entry->_name, name) == 0)
      return true;

    cur = entry->_next_entry;
  }

  return false;
}

template <class T>
void dbHashTable<T>::remove(T* object)
{
  uint    sz  = _hash_tbl.size();
  uint    hid = hash_string(object->_name) & (sz - 1);
  dbId<T> cur = _hash_tbl[hid];
  dbId<T> prev;

  while (cur != 0) {
    T* entry = _obj_tbl->getPtr(cur);

    if (entry == object) {
      if (prev == 0)
        _hash_tbl[hid] = entry->_next_entry;
      else {
        T* p           = _obj_tbl->getPtr(prev);
        p->_next_entry = entry->_next_entry;
      }

      --_num_entries;

      uint r = (_num_entries + _num_entries / 10) / sz;

      if ((r < (CHAIN_LENGTH >> 1)) && (sz > 1))
        shrinkTable();

      return;
    }

    prev = cur;
    cur  = entry->_next_entry;
  }
}

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbHashTable<T>& table)
{
  stream << table._hash_tbl;
  stream << table._num_entries;
  return stream;
}

template <class T>
dbIStream& operator>>(dbIStream& stream, dbHashTable<T>& table)
{
  stream >> table._hash_tbl;
  stream >> table._num_entries;
  return stream;
}

template <class T>
void dbHashTable<T>::differences(dbDiff&               diff,
                                 const char*           field,
                                 const dbHashTable<T>& rhs) const
{
  diff.report("<> %s", field);
  diff.increment();
  DIFF_FIELD(_num_entries)
  DIFF_VECTOR(_hash_tbl);
  diff.decrement();
}

template <class T>
void dbHashTable<T>::out(dbDiff& diff, char side, const char* field) const
{
  diff.report("%c %s", side, field);
  diff.increment();
  DIFF_OUT_FIELD(_num_entries)
  DIFF_OUT_VECTOR(_hash_tbl);
  diff.decrement();
}

}  // namespace odb
