// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <type_traits>

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

// Opt-in, per slot type, to writing this table's pages field-major (see
// dbTable::writeBlock). Nothing about the in-memory layout changes: the same
// bytes are written in a different order, so ids, free lists and iteration
// order are untouched, and only the byte stream moves. Tables opt in one at
// a time because the win is proportional to how much of the artifact they
// are, and because a smaller change is a reviewable one.
//
// A slot type opts in by declaring `static constexpr bool kFieldMajorTable
// = true;`. It has to be a member of the type rather than a specialization
// of a trait: dbTable<T> is instantiated in whichever translation unit
// happens to serialize the block, and a specialization declared in T's own
// header is not necessarily visible there, so the writer and the reader
// could be instantiated with different answers -- which is a silently
// corrupt file, and was exactly the first bug this code had.
template <class T>
constexpr bool dbFieldMajorTable()
{
  if constexpr (requires { T::kFieldMajorTable; }) {
    return T::kFieldMajorTable;
  } else {
    return false;
  }
}

// Opt-in, per slot type, to storing the table's fields as arrays -- one per
// field, page_size entries each, owned by the page -- instead of inside the
// slots. A slot keeps only what dbObject needs to be an address the public
// API can hand out and to find its page; everything else moves to a column,
// reached through an accessor that returns a reference, so a call site says
// `iterm->net()` where it said `iterm->net_`.
//
// A type opts in the same way it opts into the field-major file format:
// `static constexpr bool kSoaTable = true;`, plus the columns themselves and
// the initFields/clearFields pair dbTable calls in place of running a
// constructor or destructor over slot storage.
template <class T>
constexpr bool dbSoaTable()
{
  if constexpr (requires { T::kSoaTable; }) {
    return T::kSoaTable;
  } else {
    return false;
  }
}

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
  // The free list, in whichever storage this table uses: the slot bytes a
  // _dbFreeObject overlays, or two columns that mean nothing while a slot
  // is free. Everything above these three works the same either way.
  uint32_t freeNext(T* t) const;
  uint32_t freePrev(T* t) const;
  void setFreeLinks(T* t, uint32_t next, uint32_t prev);

  void pushQ(uint32_t& Q, T* t);
  T* popQ(uint32_t& Q);
  void findTop();
  void findBottom();

  void readPage(dbIStream& stream, dbTablePage* page);
  void writePage(dbOStream& stream, const dbTablePage* page) const;

  // Field-major form of the same bytes, a group of pages at a time. The
  // group -- not the page -- is the unit, because a page is as small as 128
  // slots and the compression a field-major layout buys grows with the run
  // length. Enlarging the page instead would renumber every object, since
  // an id is `page_addr | slot`.
  // A block whose records are nearly all distinct lengths gets columns one
  // byte wide, which buys nothing and costs a header entry each; past this
  // many classes the block is written verbatim instead. The bound is on
  // pointlessness, not on cost -- the writer buckets records by length, so
  // many classes are not themselves expensive.
  static constexpr size_t kMaxLengthClasses = 64;

  static constexpr uint32_t kSlotsPerBlock = 8192;
  static constexpr uint32_t kPagesPerBlock
      = (kSlotsPerBlock / page_size) ? (kSlotsPerBlock / page_size) : 1;

  // Records are sized by sizeof(T) for the purpose of choosing a boundary;
  // a type whose records carry a heap payload will overrun this, which is
  // why the bound is generous rather than tight.
  static constexpr uint64_t kMaxBlockBytes = 4u << 20;

  uint32_t blockPages(uint32_t first_page) const;
  void writeBlocks(dbOStream& stream) const;
  void readBlocks(dbIStream& stream);
  void writeBlock(dbOStream& stream, uint32_t first_page, uint32_t pages) const;
  void readBlock(dbIStream& stream, uint32_t first_page, uint32_t pages);

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
