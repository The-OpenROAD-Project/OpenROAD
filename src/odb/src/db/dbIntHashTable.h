// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbPagedVector.h"
#include "odb/dbId.h"

namespace odb {

class dbIStream;
class dbOStream;

//////////////////////////////////////////////////////////
///
/// dbHashTable - hash table to hash named-objects.
///
/// Each object must have the following "named" fields:
///
///     uint32_t          id_
///     dbId<T>       next_entry_
///
//////////////////////////////////////////////////////////
template <class T>
class dbIntHashTable
{
 public:
  dbIntHashTable();
  dbIntHashTable(const dbIntHashTable<T>& t);

  bool operator==(const dbIntHashTable<T>& rhs) const;
  bool operator!=(const dbIntHashTable<T>& rhs) const
  {
    return !operator==(rhs);
  }

  void growTable();
  void shrinkTable();
  void setTable(dbTable<T>* table) { obj_tbl_ = table; }

  T* find(uint32_t id);
  int hasMember(uint32_t id);
  void insert(T* object);
  void remove(T* object);

  // PERSISTANT-MEMBERS
  dbPagedVector<dbId<T>, 256, 8> hash_tbl_;
  uint32_t num_entries_;

  // NON-PERSISTANT-MEMBERS
  dbTable<T>* obj_tbl_;

  static constexpr int kChainLength = 4;
};

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbIntHashTable<T>& table);
template <class T>
dbIStream& operator>>(dbIStream& stream, dbIntHashTable<T>& table);

}  // namespace odb
