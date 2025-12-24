// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbIntHashTable.h"
#include "odb/dbId.h"

namespace odb {

inline uint32_t hash_int(uint32_t id)
{
  uint32_t key = id;
  key += ~(key << 15);
  key ^= (key >> 10);
  key += (key << 3);
  key ^= (key >> 6);
  key += ~(key << 11);
  key ^= (key >> 16);
  return key;
}

template <class T>
dbIntHashTable<T>::dbIntHashTable()
{
  obj_tbl_ = nullptr;
  num_entries_ = 0;
}

template <class T>
dbIntHashTable<T>::dbIntHashTable(const dbIntHashTable<T>& t)
    : hash_tbl_(t.hash_tbl_), num_entries_(t.num_entries_), obj_tbl_(t.obj_tbl_)
{
}

template <class T>
bool dbIntHashTable<T>::operator==(const dbIntHashTable<T>& rhs) const
{
  if (num_entries_ != rhs.num_entries_) {
    return false;
  }

  if (hash_tbl_ != rhs.hash_tbl_) {
    return false;
  }

  return true;
}

template <class T>
void dbIntHashTable<T>::growTable()
{
  uint32_t sz = hash_tbl_.size();
  dbId<T> entries;
  uint32_t i;

  for (i = 0; i < sz; ++i) {
    dbId<T> cur = hash_tbl_[i];

    while (cur != 0) {
      T* entry = obj_tbl_->getPtr(cur);
      dbId<T> next = entry->next_entry_;
      entry->next_entry_ = entries;
      entries = entry->getOID();
      cur = next;
    }

    hash_tbl_[i] = 0;
  }

  // double the size of the hash-table
  dbId<T> nullId;
  for (i = 0; i < sz; ++i) {
    hash_tbl_.push_back(nullId);
  }

  // reinsert the entries
  sz = hash_tbl_.size() - 1;
  dbId<T> cur = entries;

  while (cur != 0) {
    T* entry = obj_tbl_->getPtr(cur);
    dbId<T> next = entry->next_entry_;
    uint32_t hid = hash_int(entry->id_) & sz;
    dbId<T>& e = hash_tbl_[hid];
    entry->next_entry_ = e;
    e = entry->getOID();
    cur = next;
  }
}

template <class T>
void dbIntHashTable<T>::shrinkTable()
{
  uint32_t sz = hash_tbl_.size();
  dbId<T> entries;
  uint32_t i;

  for (i = 0; i < sz; ++i) {
    dbId<T> cur = hash_tbl_[i];

    while (cur != 0) {
      T* entry = obj_tbl_->getPtr(cur);
      dbId<T> next = entry->next_entry_;
      entry->next_entry_ = entries;
      entries = entry->getOID();
      cur = next;
    }

    hash_tbl_[i] = 0;
  }

  // TODO: add method to dbPagedVector to resize the table
  hash_tbl_.clear();
  sz >>= 1;

  // halve the size of the hash-table
  dbId<T> nullId;
  for (i = 0; i < sz; ++i) {
    hash_tbl_.push_back(nullId);
  }

  sz -= 1;
  // reinsert the entries
  dbId<T> cur = entries;

  while (cur != 0) {
    T* entry = obj_tbl_->getPtr(cur);
    dbId<T> next = entry->next_entry_;
    uint32_t hid = hash_int(entry->id_) & sz;
    dbId<T>& e = hash_tbl_[hid];
    entry->next_entry_ = e;
    e = entry->getOID();
    cur = next;
  }
}

template <class T>
void dbIntHashTable<T>::insert(T* object)
{
  ++num_entries_;
  uint32_t sz = hash_tbl_.size();

  if (sz == 0) {
    dbId<T> nullId;
    hash_tbl_.push_back(nullId);
    sz = 1;
  } else {
    uint32_t r = num_entries_ / sz;

    if (r > kChainLength) {
      growTable();
      sz = hash_tbl_.size();
    }
  }

  uint32_t hid = hash_int(object->id_) & (sz - 1);
  dbId<T>& e = hash_tbl_[hid];
  object->next_entry_ = e;
  e = object->getOID();
}

template <class T>
T* dbIntHashTable<T>::find(uint32_t id)
{
  uint32_t sz = hash_tbl_.size();

  if (sz == 0) {
    return nullptr;
  }

  uint32_t hid = hash_int(id) & (sz - 1);
  dbId<T> cur = hash_tbl_[hid];

  while (cur != 0) {
    T* entry = obj_tbl_->getPtr(cur);

    if (entry->id_ == id) {
      return entry;
    }

    cur = entry->next_entry_;
  }

  return nullptr;
}

template <class T>
int dbIntHashTable<T>::hasMember(uint32_t id)
{
  uint32_t sz = hash_tbl_.size();

  if (sz == 0) {
    return false;
  }

  uint32_t hid = hash_int(id) & (sz - 1);
  dbId<T> cur = hash_tbl_[hid];

  while (cur != 0) {
    T* entry = obj_tbl_->getPtr(cur);

    if (entry->id_ == id) {
      return true;
    }

    cur = entry->next_entry_;
  }

  return false;
}

template <class T>
void dbIntHashTable<T>::remove(T* object)
{
  uint32_t sz = hash_tbl_.size();
  uint32_t hid = hash_int(object->id_) & (sz - 1);
  dbId<T> cur = hash_tbl_[hid];
  dbId<T> prev;

  while (cur != 0) {
    T* entry = obj_tbl_->getPtr(cur);

    if (entry == object) {
      if (prev == 0) {
        hash_tbl_[hid] = entry->next_entry_;
      } else {
        T* p = obj_tbl_->getPtr(prev);
        p->next_entry_ = entry->next_entry_;
      }

      --num_entries_;

      uint32_t r = (num_entries_ + num_entries_ / 10) / sz;

      if ((r < (kChainLength >> 1)) && (sz > 1)) {
        shrinkTable();
      }

      return;
    }

    prev = cur;
    cur = entry->next_entry_;
  }
}

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbIntHashTable<T>& table)
{
  stream << table.hash_tbl_;
  stream << table.num_entries_;
  return stream;
}

template <class T>
dbIStream& operator>>(dbIStream& stream, dbIntHashTable<T>& table)
{
  stream >> table.hash_tbl_;
  stream >> table.num_entries_;
  return stream;
}

}  // namespace odb
