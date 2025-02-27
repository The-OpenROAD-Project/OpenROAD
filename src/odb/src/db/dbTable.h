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

#include <vector>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/ZException.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {

class dbIStream;
class dbOStream;

class dbTablePage : public dbObjectPage
{
 public:
  char _objects[1];
};

template <class T>
class dbTable : public dbObjectTable, public dbIterator
{
 public:
  dbTable(_dbDatabase* db,
          dbObject* owner,
          dbObjectTable* (dbObject::*m)(dbObjectType),
          dbObjectType type,
          uint page_size = 128,
          uint page_shift = 7);

  ~dbTable() override;

  // returns the number of instances of "T" allocated
  uint size() const { return _alloc_cnt; }

  // Create a "T", calls T( _dbDatabase * )
  T* create();

  // Destroy instance of "T", calls destructor
  void destroy(T*);

  // clear the table
  void clear();

  uint pageSize() const { return _page_mask + 1; }

  // Get the object of this id
  T* getPtr(dbId<T> id) const;

  bool validId(dbId<T> id) const;

  void collectMemInfo(MemInfo& info);

  bool operator==(const dbTable<T>& rhs) const;
  bool operator!=(const dbTable<T>& table) const;

  // dbIterator interface methods
  bool reversible() override;
  bool orderReversed() override;
  void reverse(dbObject* parent) override;
  uint sequential() override;
  uint size(dbObject* parent) override;
  uint begin(dbObject* parent) override;
  uint end(dbObject* parent) override;
  uint next(uint id, ...) override;
  dbObject* getObject(uint id, ...) override;
  bool validObject(uint id, ...) override { return validId(id); }

 private:
  void resizePageTbl();
  void newPage();
  void pushQ(uint& Q, _dbFreeObject* e);
  _dbFreeObject* popQ(uint& Q);
  void findTop();
  void findBottom();

  void readPage(dbIStream& stream, dbTablePage* page);
  void writePage(dbOStream& stream, const dbTablePage* page) const;

  _dbFreeObject* getFreeObj(dbId<T> id);

  // PERSISTANT-DATA
  uint _page_mask;      // bit-mask to get page-offset
  uint _page_shift;     // number of bits to shift to determine page-no
  uint _top_idx;        // largest id which has been allocated.
  uint _bottom_idx;     // smallest id which has been allocated.
  uint _page_cnt;       // high-water mark of page-table
  uint _page_tbl_size;  // length of the page table
  uint _alloc_cnt;      // number of object allocated
  uint _free_list;      // objects on freelist

  // NON-PERSISTANT-DATA
  dbTablePage** _pages;  // page-table

  template <class U>
  friend dbOStream& operator<<(dbOStream& stream, const dbTable<U>& table);

  template <class U>
  friend dbIStream& operator>>(dbIStream& stream, dbTable<U>& table);
};

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbTable<T>& table);

template <class T>
dbIStream& operator>>(dbIStream& stream, dbTable<T>& table);

// Useful if you want to write the table in a named scope
template <class T>
struct NamedTable
{
  NamedTable(const char* name, const dbTable<T>* table)
      : name(name), table(table)
  {
  }
  const char* name;
  const dbTable<T>* table;
};

template <class T>
dbOStream& operator<<(dbOStream& stream, const NamedTable<T>& named_table);

}  // namespace odb
