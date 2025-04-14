// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbPagedVector.h"
#include "odb/dbId.h"
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
///     char *        _name
///     dbId<T>       _next_entry
///
//////////////////////////////////////////////////////////
template <class T>
class dbHashTable
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

  dbHashTable();
  dbHashTable(const dbHashTable<T>& table);

  bool operator==(const dbHashTable<T>& rhs) const;
  bool operator!=(const dbHashTable<T>& rhs) const { return !operator==(rhs); }

  void setTable(dbTable<T>* table) { _obj_tbl = table; }
  T* find(const char* name);
  int hasMember(const char* name);
  void insert(T* object);
  void remove(T* object);
};

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbHashTable<T>& table);
template <class T>
dbIStream& operator>>(dbIStream& stream, dbHashTable<T>& table);

}  // namespace odb
