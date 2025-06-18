// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <boost/integer/static_log2.hpp>
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

template <class T, uint page_size /* = 128 */>
class dbTable final : public dbObjectTable, public dbIterator
{
  static_assert((page_size & (page_size - 1)) == 0,
                "page_size must be a power of two");

  // number of bits to shift to determine page number
  constexpr static int page_shift = boost::static_log2<page_size>::value;

  // bit-mask to get page-offset
  constexpr static uint page_mask = page_size - 1;

 public:
  dbTable(_dbDatabase* db,
          dbObject* owner,
          dbObjectTable* (dbObject::*m)(dbObjectType),
          dbObjectType type);

  ~dbTable() override;

  // returns the number of instances of "T" allocated
  uint size() const { return _alloc_cnt; }

  // Create a "T", calls T( _dbDatabase * )
  T* create();

  // Destroy instance of "T", calls destructor
  void destroy(T*);

  // clear the table
  void clear();

  uint pageSize() const { return page_mask + 1; }

  // Get the object of this id
  T* getPtr(dbId<T> id) const;

  bool validId(dbId<T> id) const;

  void collectMemInfo(MemInfo& info);

  bool operator==(const dbTable<T, page_size>& rhs) const;
  bool operator!=(const dbTable<T, page_size>& table) const;

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
  uint _top_idx;        // largest id which has been allocated.
  uint _bottom_idx;     // smallest id which has been allocated.
  uint _page_cnt;       // high-water mark of page-table
  uint _page_tbl_size;  // length of the page table
  uint _alloc_cnt;      // number of object allocated
  uint _free_list;      // objects on freelist

  // NON-PERSISTANT-DATA
  dbTablePage** _pages;  // page-table

  template <class U, uint page_size2>
  friend dbOStream& operator<<(dbOStream& stream,
                               const dbTable<U, page_size2>& table);

  template <class U, uint page_size2>
  friend dbIStream& operator>>(dbIStream& stream,
                               dbTable<U, page_size2>& table);
};

template <class T, uint page_size>
dbOStream& operator<<(dbOStream& stream, const dbTable<T, page_size>& table);

template <class T, uint page_size>
dbIStream& operator>>(dbIStream& stream, dbTable<T, page_size>& table);

// Useful if you want to write the table in a named scope
template <class T, uint page_size>
struct NamedTable
{
  NamedTable(const char* name, const dbTable<T, page_size>* table)
      : name(name), table(table)
  {
  }
  const char* name;
  const dbTable<T, page_size>* table;
};

template <class T, uint page_size>
dbOStream& operator<<(dbOStream& stream,
                      const NamedTable<T, page_size>& named_table);

}  // namespace odb
