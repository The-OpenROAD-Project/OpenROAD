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
///     char *        _name
///     dbId<T>       _next_entry
///
//////////////////////////////////////////////////////////
template <class T, uint page_size>
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
  dbTable<T, page_size>* _obj_tbl;

  void growTable();
  void shrinkTable();

  dbHashTable();
  dbHashTable(const dbHashTable<T, page_size>& table);

  bool operator==(const dbHashTable<T, page_size>& rhs) const;
  bool operator!=(const dbHashTable<T, page_size>& rhs) const
  {
    return !operator==(rhs);
  }

  void setTable(dbTable<T, page_size>* table) { _obj_tbl = table; }
  T* find(const char* name);
  int hasMember(const char* name);
  void insert(T* object);
  void remove(T* object);
};

template <class T, uint page_size>
dbOStream& operator<<(dbOStream& stream,
                      const dbHashTable<T, page_size>& table);
template <class T, uint page_size>
dbIStream& operator>>(dbIStream& stream, dbHashTable<T, page_size>& table);

}  // namespace odb
