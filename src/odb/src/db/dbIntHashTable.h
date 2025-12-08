// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbPagedVector.h"
#include "odb/dbId.h"
#include "odb/odb.h"

namespace odb {

class dbIStream;
class dbOStream;

//////////////////////////////////////////////////////////
///
/// dbHashTable - hash table to hash named-objects.
///
/// Each object must have the following "named" fields:
///
///     uint          id_
///     dbId<T>       next_entry_
///
//////////////////////////////////////////////////////////
template <class T>
class dbIntHashTable
{
 public:
  dbIntHashTable();
  dbIntHashTable(const dbIntHashTable<T>& t);
  ~dbIntHashTable();
  bool operator==(const dbIntHashTable<T>& rhs) const;
  bool operator!=(const dbIntHashTable<T>& rhs) const
  {
    return !operator==(rhs);
  }

  void growTable();
  void shrinkTable();
  void setTable(dbTable<T>* table) { obj_tbl_ = table; }

  T* find(uint id);
  int hasMember(uint id);
  void insert(T* object);
  void remove(T* object);

  // PERSISTANT-MEMBERS
  dbPagedVector<dbId<T>, 256, 8> hash_tbl_;
  uint num_entries_;

  // NON-PERSISTANT-MEMBERS
  dbTable<T>* obj_tbl_;

  static constexpr int kChainLength = 4;
};

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbIntHashTable<T>& table);
template <class T>
dbIStream& operator>>(dbIStream& stream, dbIntHashTable<T>& table);

}  // namespace odb
