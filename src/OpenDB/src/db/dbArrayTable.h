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

#include "ZException.h"
#include "dbCore.h"
#include "dbVector.h"
#include "odb.h"

namespace odb {

class dbIStream;
class dbOStream;

class dbArrayTablePage : public dbObjectPage
{
 public:
  char _objects[1];
};

//
// dbArrayTable - Specialized class to allocate a fixed array of objects.
//                The array is indexing is only in terms of database IDs.
//                Also, this class does not free-pages when they become
//                empty, because an array may cross the page boundary.
//
template <class T>
class dbArrayTable : public dbObjectTable
{
 public:
  // PERSISTANT-DATA
  uint _page_mask;          // bit-mask to get page-offset
  uint _page_shift;         // number of bits to shift to determine page-no
  uint _page_cnt;           // high-water mark of page-table
  uint _page_tbl_size;      // length of the page table
  uint _alloc_cnt;          // number of object allocated
  uint _objects_per_alloc;  // number of object to allocate per array
  uint _free_list;          // objects on freelist

  // NON-PERSISTANT-DATA
  dbArrayTablePage** _pages;  // page-table

  void           resizePageTbl();
  void           newPage();
  void           pushQ(uint& Q, _dbFreeObject* e);
  _dbFreeObject* popQ(uint& Q);
  void           unlinkQ(uint& Q, _dbFreeObject* e);

  dbArrayTable(_dbDatabase* db,
               dbObject*    owner,
               GetObjTbl_t  m,
               dbObjectType type,
               uint         array_size,
               uint         page_size  = 128,
               uint         page_shift = 7);

  // Make a copy of a table.
  // The copy is identical including the ordering of all free-lists.
  dbArrayTable(_dbDatabase* db, dbObject* owner, const dbArrayTable<T>&);

  virtual ~dbArrayTable();

  // returns the number of instances of "T" allocated
  uint size() const { return _alloc_cnt; }

  // returns the number of objects per array
  uint arraySize() const { return _objects_per_alloc; }

  // Create a "T", calls T( _dbDatabase * )
  dbId<T> createArray();

  // Destroy instance of "T", calls destructor
  void destroyArray(dbId<T> t);

  // clear the table
  void clear();

  // clear the table and set the array-size
  // void reset( int array_size );

  uint page_size() const { return _page_mask + 1; }

  // Get the object of this id
  T* getPtr(uint id) const
  {
    uint page   = id >> _page_shift;
    uint offset = id & _page_mask;
    ZASSERT((id != 0) && (page < _page_cnt));
    T* p = (T*) &(_pages[page]->_objects[offset * sizeof(T)]);
    ZASSERT(p->_oid & DB_ALLOC_BIT);
    return p;
  }

  bool validId(uint id) const
  {
    uint page   = id >> _page_shift;
    uint offset = id & _page_mask;

    if ((id != 0) && (page < _page_cnt)) {
      T* p = (T*) &(_pages[page]->_objects[offset * sizeof(T)]);
      return (p->_oid & DB_ALLOC_BIT) == DB_ALLOC_BIT;
    }

    return false;
  }

  //
  // Get the object of this id
  // This method is the same as getPtr() but is is
  // used to get objects on the free-list.
  //
  T* getFreeObj(uint id)
  {
    uint page   = id >> _page_shift;
    uint offset = id & _page_mask;
    ZASSERT((id != 0) && (page < _page_cnt));
    T* p = (T*) &(_pages[page]->_objects[offset * sizeof(T)]);
    ZASSERT((p->_oid & DB_ALLOC_BIT) == 0);
    return p;
  }

  bool operator==(const dbArrayTable<T>& table) const;
  bool operator!=(const dbArrayTable<T>& table) const;
  void differences(dbDiff& diff, const dbArrayTable<T>& rhs) const;
  void out(dbDiff& diff, char side) const;

  void      readPage(dbIStream& stream, dbArrayTablePage* page);
  void      writePage(dbOStream& stream, const dbArrayTablePage* page) const;
  void      getObjects(std::vector<T*>& objects);
  dbObject* getObject(uint id, ...) { return getPtr(id); }

 private:
  T*   create();
  void destroy(T* t);
  void copy_pages(const dbArrayTable<T>&);
  void copy_page(uint page_id, dbArrayTablePage* page);
};

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbArrayTable<T>& table);

template <class T>
dbIStream& operator>>(dbIStream& stream, dbArrayTable<T>& table);

}  // namespace odb
