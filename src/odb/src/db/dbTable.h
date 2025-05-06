// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

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

class dbTablePage final : public dbObjectPage
{
 public:
  char _objects[1];
};

template <class T>
class dbTable final : public dbObjectTable, public dbIterator
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
