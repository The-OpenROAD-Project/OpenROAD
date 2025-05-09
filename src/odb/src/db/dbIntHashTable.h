// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbPagedVector.h"
#include "odb/odb.h"

namespace odb {

class dbIStream;
class dbOStream;
template <class T>
class dbTable;

//////////////////////////////////////////////////////////
///
/// dbHashTable - hash table to hash named-objects.
///
/// Each object must have the following "named" fields:
///
///     uint          _id
///     dbId<T>       _next_entry
///
//////////////////////////////////////////////////////////
template <class T>
class dbIntHashTable
{
 public:
  enum Params
  {
    CHAIN_LENGTH = 4
  };

  // PERSISTANT-MEMBERS
  dbPagedVector<dbId<T>, 256, 8> _hash_tbl;
  uint _num_entries;

  // NON-PERSISTANT-MEMBERS
  dbTable<T>* _obj_tbl;

  void growTable();
  void shrinkTable();

  dbIntHashTable();
  dbIntHashTable(const dbIntHashTable<T>& t);
  ~dbIntHashTable();
  bool operator==(const dbIntHashTable<T>& rhs) const;
  bool operator!=(const dbIntHashTable<T>& rhs) const
  {
    return !operator==(rhs);
  }

  void setTable(dbTable<T>* table) { _obj_tbl = table; }

  T* find(uint id);
  int hasMember(uint id);
  void insert(T* object);
  void remove(T* object);
};

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbIntHashTable<T>& table);
template <class T>
dbIStream& operator>>(dbIStream& stream, dbIntHashTable<T>& table);

}  // namespace odb
