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
///     char *        name_
///     dbId<T>       next_entry_
///
//////////////////////////////////////////////////////////
template <class T, uint32_t page_size>
class dbHashTable
{
 public:
  void growTable();
  void shrinkTable();

  dbHashTable();
  dbHashTable(const dbHashTable<T, page_size>& table);

  bool operator==(const dbHashTable<T, page_size>& rhs) const;
  bool operator!=(const dbHashTable<T, page_size>& rhs) const
  {
    return !operator==(rhs);
  }

  void setTable(dbTable<T, page_size>* table) { obj_tbl_ = table; }
  T* find(const char* name);
  int hasMember(const char* name);
  void insert(T* object);
  void remove(T* object);

  // PERSISTANT-MEMBERS
  dbPagedVector<dbId<T>, 256, 8> hash_tbl_;
  uint32_t num_entries_;

  // NON-PERSISTANT-MEMBERS
  dbTable<T, page_size>* obj_tbl_;

  static constexpr int kChainLength = 4;
};

template <class T, uint32_t page_size>
dbOStream& operator<<(dbOStream& stream,
                      const dbHashTable<T, page_size>& table);
template <class T, uint32_t page_size>
dbIStream& operator>>(dbIStream& stream, dbHashTable<T, page_size>& table);

}  // namespace odb
