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

#include "dbPagedVector.h"
#include "odb.h"

namespace odb {

class dbIStream;
class dbOStream;
class dbDiff;
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
  uint                           _num_entries;

  // NON-PERSISTANT-MEMBERS
  dbTable<T>* _obj_tbl;

  void growTable();
  void shrinkTable();

  dbHashTable();
  dbHashTable(const dbHashTable<T>& table);
  ~dbHashTable();
  bool operator==(const dbHashTable<T>& rhs) const;
  bool operator!=(const dbHashTable<T>& rhs) const { return !operator==(rhs); }
  void differences(dbDiff&               diff,
                   const char*           field,
                   const dbHashTable<T>& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  void setTable(dbTable<T>* table) { _obj_tbl = table; }
  T*   find(const char* name);
  int  hasMember(const char* name);
  void insert(T* object);
  void remove(T* object);
};

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbHashTable<T>& table);
template <class T>
dbIStream& operator>>(dbIStream& stream, dbHashTable<T>& table);

}  // namespace odb
