// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "boost/integer/static_log2.hpp"
#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbIterator.h"
#include "odb/dbObject.h"

namespace odb {

class dbIStream;
class dbOStream;

class dbTablePage final : public dbObjectPage
{
 public:
  char objects_[1];
};

template <class T, uint32_t page_size /* = 128 */>
class dbTable final : public dbObjectTable, public dbIterator
{
  static_assert((page_size & (page_size - 1)) == 0,
                "page_size must be a power of two");

  // number of bits to shift to determine page number
  static constexpr int kPageShift = boost::static_log2<page_size>::value;

  // bit-mask to get page-offset
  static constexpr uint32_t kPageMask = page_size - 1;

 public:
  dbTable(_dbDatabase* db,
          dbObject* owner,
          dbObjectTable* (dbObject::*m)(dbObjectType),
          dbObjectType type);

  ~dbTable() override;

  // returns the number of instances of "T" allocated
  uint32_t size() const { return alloc_cnt_; }

  // Create a "T", calls T( _dbDatabase * )
  T* create();

  // Destroy instance of "T", calls destructor
  void destroy(T*);

  // clear the table
  void clear();

  uint32_t pageSize() const { return kPageMask + 1; }

  // Get the object of this id
  T* getPtr(dbId<T> id) const;

  bool validId(dbId<T> id) const;

  void collectMemInfo(MemInfo& info);

  bool operator==(const dbTable<T, page_size>& rhs) const;
  bool operator!=(const dbTable<T, page_size>& table) const;

  // dbIterator interface methods
  bool reversible() const override;
  bool orderReversed() const override;
  void reverse(dbObject* parent) override;
  uint32_t sequential() const override;
  uint32_t size(dbObject* parent) const override;
  uint32_t begin(dbObject* parent) const override;
  uint32_t end(dbObject* parent) const override;
  uint32_t next(uint32_t id, ...) const override;
  dbObject* getObject(uint32_t id, ...) override;
  bool validObject(uint32_t id, ...) override { return validId(id); }

 private:
  void resizePageTbl();
  void newPage();
  void pushQ(uint32_t& Q, _dbFreeObject* e);
  _dbFreeObject* popQ(uint32_t& Q);
  void findTop();
  void findBottom();

  void readPage(dbIStream& stream, dbTablePage* page);
  void writePage(dbOStream& stream, const dbTablePage* page) const;

  _dbFreeObject* getFreeObj(dbId<T> id);

  template <class U, uint32_t page_size2>
  friend dbOStream& operator<<(dbOStream& stream,
                               const dbTable<U, page_size2>& table);

  template <class U, uint32_t page_size2>
  friend dbIStream& operator>>(dbIStream& stream,
                               dbTable<U, page_size2>& table);

  // PERSISTANT-DATA
  uint32_t top_idx_;        // largest id which has been allocated.
  uint32_t bottom_idx_;     // smallest id which has been allocated.
  uint32_t page_cnt_;       // high-water mark of page-table
  uint32_t page_tbl_size_;  // length of the page table
  uint32_t alloc_cnt_;      // number of object allocated
  uint32_t free_list_;      // objects on freelist

  // NON-PERSISTANT-DATA
  dbTablePage** pages_;  // page-table
};

template <class T, uint32_t page_size>
dbOStream& operator<<(dbOStream& stream, const dbTable<T, page_size>& table);

template <class T, uint32_t page_size>
dbIStream& operator>>(dbIStream& stream, dbTable<T, page_size>& table);

// Useful if you want to write the table in a named scope
template <class T, uint32_t page_size>
struct NamedTable
{
  NamedTable(const char* name, const dbTable<T, page_size>* table)
      : name(name), table(table)
  {
  }
  const char* name;
  const dbTable<T, page_size>* table;
};

template <class T, uint32_t page_size>
dbOStream& operator<<(dbOStream& stream,
                      const NamedTable<T, page_size>& named_table);

}  // namespace odb

#include "dbTable.inc"  // IWYU pragma: export
