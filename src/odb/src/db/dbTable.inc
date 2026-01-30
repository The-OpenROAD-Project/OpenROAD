// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"
#include "odb/dbStream.h"
#include "utl/Logger.h"

namespace odb {

//
// Get the object of this id
// This method is the same as getPtr() but is is
// use to get objects on the free-list.
//
template <class T, uint32_t page_size>
inline _dbFreeObject* dbTable<T, page_size>::getFreeObj(dbId<T> id)
{
  const uint32_t page = (uint32_t) id >> kPageShift;
  const uint32_t offset = (uint32_t) id & kPageMask;

  assert(((uint32_t) id != 0) && (page < page_cnt_));
  T* p = (T*) &(pages_[page]->objects_[offset * sizeof(T)]);
  assert((p->oid_ & kAllocBit) == 0);
  return (_dbFreeObject*) p;
}

template <class T, uint32_t page_size>
inline T* dbTable<T, page_size>::getPtr(dbId<T> id) const
{
  const uint32_t page = (uint32_t) id >> kPageShift;
  const uint32_t offset = (uint32_t) id & kPageMask;

  assert(((uint32_t) id != 0) && (page < page_cnt_));
  T* p = (T*) &(pages_[page]->objects_[offset * sizeof(T)]);
  assert(p->oid_ & kAllocBit);
  return p;
}

template <class T, uint32_t page_size>
inline bool dbTable<T, page_size>::validId(dbId<T> id) const
{
  const uint32_t page = (uint32_t) id >> kPageShift;
  const uint32_t offset = (uint32_t) id & kPageMask;

  if (((uint32_t) id != 0) && (page < page_cnt_)) {
    T* p = (T*) &(pages_[page]->objects_[offset * sizeof(T)]);
    return (p->oid_ & kAllocBit) == kAllocBit;
  }

  return false;
}

template <class T, uint32_t page_size>
inline void dbTable<T, page_size>::pushQ(uint32_t& Q, _dbFreeObject* e)
{
  e->prev_ = 0;
  e->next_ = Q;
  const uint32_t head_id = e->getImpl()->getOID();
  if (Q != 0) {
    getFreeObj(Q)->prev_ = head_id;
  }
  Q = head_id;
}

template <class T, uint32_t page_size>
inline _dbFreeObject* dbTable<T, page_size>::popQ(uint32_t& Q)
{
  _dbFreeObject* e = getFreeObj(Q);
  Q = e->next_;

  if (Q) {
    _dbFreeObject* head = getFreeObj(Q);
    head->prev_ = 0;
  }

  return e;
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::clear()
{
  for (uint32_t i = 0; i < page_cnt_; ++i) {
    dbTablePage* page = pages_[i];
    const T* t = (T*) page->objects_;
    const T* e = &t[pageSize()];

    for (; t < e; t++) {
      if (t->oid_ & kAllocBit) {
        t->~T();
      }
    }

    free(page);
  }

  delete[] pages_;

  bottom_idx_ = 0;
  top_idx_ = 0;
  page_cnt_ = 0;
  page_tbl_size_ = 0;
  alloc_cnt_ = 0;
  free_list_ = 0;
  pages_ = nullptr;
}

template <class T, uint32_t page_size>
dbTable<T, page_size>::dbTable(_dbDatabase* db,
                               dbObject* owner,
                               dbObjectTable* (dbObject::*m)(dbObjectType),
                               const dbObjectType type)
    : dbObjectTable(db, owner, m, type, sizeof(T))
{
  bottom_idx_ = 0;
  top_idx_ = 0;
  page_cnt_ = 0;
  page_tbl_size_ = 0;
  alloc_cnt_ = 0;
  free_list_ = 0;
  pages_ = nullptr;
}

template <class T, uint32_t page_size>
dbTable<T, page_size>::~dbTable()
{
  clear();
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::resizePageTbl()
{
  dbTablePage** old_tbl = pages_;
  const uint32_t old_tbl_size = page_tbl_size_;
  page_tbl_size_ *= 2;

  pages_ = new dbTablePage*[page_tbl_size_];

  uint32_t i;
  for (i = 0; i < old_tbl_size; ++i) {
    pages_[i] = old_tbl[i];
  }

  for (; i < page_tbl_size_; ++i) {
    pages_[i] = nullptr;
  }

  delete[] old_tbl;
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::newPage()
{
  const uint32_t size = (pageSize() * sizeof(T)) + sizeof(dbObjectPage);
  dbTablePage* page = (dbTablePage*) safe_malloc(size);
  memset(page, 0, size);

  const uint32_t page_id = page_cnt_;

  if (page_tbl_size_ == 0) {
    pages_ = new dbTablePage*[1];
    page_tbl_size_ = 1;
  } else if (page_tbl_size_ == page_cnt_) {
    resizePageTbl();
  }

  ++page_cnt_;
  page->table_ = this;
  page->page_addr_ = page_id << kPageShift;
  page->alloc_cnt_ = 0;
  pages_[page_id] = page;

  // The objects are put on the list in reverse order, so they can be removed
  // in low-to-high order.
  if (page_id == 0) {
    T* b = (T*) page->objects_;
    T* t = &b[kPageMask];

    for (; t >= b; --t) {
      _dbFreeObject* o = (_dbFreeObject*) t;
      o->oid_ = (uint32_t) ((char*) t - (char*) b);

      if (t != b) {  // don't link zero-object
        pushQ(free_list_, o);
      }
    }
  } else {
    T* b = (T*) page->objects_;
    T* t = &b[kPageMask];

    for (; t >= b; --t) {
      _dbFreeObject* o = (_dbFreeObject*) t;
      o->oid_ = (uint32_t) ((char*) t - (char*) b);
      pushQ(free_list_, o);
    }
  }
}

template <class T, uint32_t page_size>
T* dbTable<T, page_size>::create()
{
  ++alloc_cnt_;

  if (free_list_ == 0) {
    newPage();
  }

  _dbFreeObject* o = popQ(free_list_);
  const uint32_t oid = o->oid_;
  new (o) T(db_);
  T* t = (T*) o;
  t->oid_ = oid | kAllocBit;

  dbTablePage* page = (dbTablePage*) t->getObjectPage();
  page->alloc_cnt_++;

  const uint32_t id = t->getOID();

  top_idx_ = std::max(id, top_idx_);

  if ((bottom_idx_ == 0) || (id < bottom_idx_)) {
    bottom_idx_ = id;
  }

  return t;
}

#define ADS_DB_TABLE_BOTTOM_SEARCH_FAILED 0
#define ADS_DB_TABLE_TOP_SEARCH_FAILED 0

// find the new bottom_idx...
template <class T, uint32_t page_size>
inline void dbTable<T, page_size>::findBottom()
{
  if (alloc_cnt_ == 0) {
    bottom_idx_ = 0;
    return;
  }

  uint32_t page_id = bottom_idx_ >> kPageShift;
  dbTablePage* page = pages_[page_id];

  // if page is still valid, find the next allocated object
  if (page->valid_page()) {
    uint32_t offset = bottom_idx_ & kPageMask;
    T* b = (T*) page->objects_;
    T* s = &b[offset + 1];
    T* e = &b[pageSize()];
    for (; s < e; s++) {
      if (s->oid_ & kAllocBit) {
        offset = s - b;
        bottom_idx_ = (page_id << kPageShift) + offset;
        return;
      }
    }

    // if s == e, then something is corrupted...
    assert(ADS_DB_TABLE_BOTTOM_SEARCH_FAILED);
  }

  for (++page_id;; ++page_id) {
    assert(page_id < page_cnt_);
    page = pages_[page_id];

    if (page->valid_page()) {
      break;
    }
  }

  T* b = (T*) page->objects_;
  T* s = b;
  T* e = &s[pageSize()];

  for (; s < e; s++) {
    if (s->oid_ & kAllocBit) {
      const uint32_t offset = s - b;
      bottom_idx_ = (page_id << kPageShift) + offset;
      return;
    }
  }

  // if s == e, then something is corrupted...
  assert(ADS_DB_TABLE_BOTTOM_SEARCH_FAILED);
}

// find the new top_idx...
template <class T, uint32_t page_size>
inline void dbTable<T, page_size>::findTop()
{
  if (alloc_cnt_ == 0) {
    top_idx_ = 0;
    return;
  }

  uint32_t page_id = top_idx_ >> kPageShift;
  dbTablePage* page = pages_[page_id];

  // if page is still valid, find the next allocated object
  if (page->valid_page()) {
    uint32_t offset = top_idx_ & kPageMask;
    T* b = (T*) page->objects_;
    T* s = &b[offset - 1];

    for (; s >= b; s--) {
      if (s->oid_ & kAllocBit) {
        offset = s - b;
        top_idx_ = (page_id << kPageShift) + offset;
        return;
      }
    }

    // if s < e, then something is corrupted...
    assert(ADS_DB_TABLE_TOP_SEARCH_FAILED);
  }

  for (--page_id;; --page_id) {
    assert(page_id >= 0);
    page = pages_[page_id];

    if (page->valid_page()) {
      break;
    }
  }

  T* b = (T*) page->objects_;
  T* s = &b[kPageMask];

  for (; s >= b; s--) {
    if (s->oid_ & kAllocBit) {
      const uint32_t offset = s - b;
      top_idx_ = (page_id << kPageShift) + offset;
      return;
    }
  }

  // if s < e, then something is corrupted...
  assert(ADS_DB_TABLE_TOP_SEARCH_FAILED);
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::destroy(T* t)
{
  --alloc_cnt_;

  assert(t->getOID() != 0);
  assert(t->getTable() == this);
  assert(t->oid_ & kAllocBit);

  dbTablePage* page = (dbTablePage*) t->getObjectPage();
  _dbFreeObject* o = (_dbFreeObject*) t;

  page->alloc_cnt_--;
  const uint32_t oid = t->oid_;
  t->~T();  // call destructor
  o->oid_ = oid & ~kAllocBit;

  const uint32_t offset = t - (T*) page->objects_;
  const uint32_t id = page->page_addr_ + offset;

  // Add to freelist
  pushQ(free_list_, o);

  if (id == bottom_idx_) {
    findBottom();
  }

  if (id == top_idx_) {
    findTop();
  }
}

template <class T, uint32_t page_size>
bool dbTable<T, page_size>::reversible() const
{
  return false;
}

template <class T, uint32_t page_size>
bool dbTable<T, page_size>::orderReversed() const
{
  return false;
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::reverse(dbObject* /* unused: parent */)
{
}

template <class T, uint32_t page_size>
uint32_t dbTable<T, page_size>::sequential() const
{
  return top_idx_;
}

template <class T, uint32_t page_size>
uint32_t dbTable<T, page_size>::size(dbObject* /* unused: parent */) const
{
  return size();
}

template <class T, uint32_t page_size>
uint32_t dbTable<T, page_size>::begin(dbObject* /* unused: parent */) const
{
  return bottom_idx_;
}

template <class T, uint32_t page_size>
uint32_t dbTable<T, page_size>::end(dbObject* /* unused: parent */) const
{
  return 0;
}

template <class T, uint32_t page_size>
uint32_t dbTable<T, page_size>::next(uint32_t id, ...) const
{
  assert(id != 0);
  ++id;

  if (id > top_idx_) {
    return 0;
  }

  uint32_t page_id = id >> kPageShift;
  dbTablePage* page = pages_[page_id];
  uint32_t offset = id & kPageMask;

next_obj:
  T* p = (T*) &(page->objects_[offset * sizeof(T)]);
  T* e = (T*) &(page->objects_[pageSize() * sizeof(T)]);

  for (; p < e; ++p) {
    if (p->oid_ & kAllocBit) {
      offset = p - (T*) page->objects_;
      const uint32_t n = (page_id << kPageShift) + offset;
      assert(n <= top_idx_);
      return n;
    }
  }

  // search for next non empty page
  for (++page_id; page_id < page_cnt_; ++page_id) {
    page = pages_[page_id];

    if (page->valid_page()) {
      offset = 0;
      goto next_obj;
    }
  }

  return 0;
}

template <class T, uint32_t page_size>
dbObject* dbTable<T, page_size>::getObject(uint32_t id, ...)
{
  return getPtr(id);
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::writePage(dbOStream& stream,
                                      const dbTablePage* page) const
{
  const T* t = (T*) page->objects_;
  const T* e = &t[pageSize()];

  for (; t < e; t++) {
    if (t->oid_ & kAllocBit) {
      const char allocated = 1;
      stream << allocated;
      stream << *t;
    } else {
      const char allocated = 0;
      stream << allocated;
      _dbFreeObject* o = (_dbFreeObject*) t;
      stream << o->next_;
      stream << o->prev_;
    }
  }
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::readPage(dbIStream& stream, dbTablePage* page)
{
  T* t = (T*) page->objects_;
  T* e = &t[pageSize()];
  page->alloc_cnt_ = 0;

  for (; t < e; t++) {
    char allocated;
    stream >> allocated;

    if (!allocated) {
      t->oid_ = (uint32_t) ((char*) t - page->objects_);
      _dbFreeObject* o = (_dbFreeObject*) t;
      stream >> o->next_;
      stream >> o->prev_;
    } else {
      new (t) T(db_);
      uint32_t oid = uint32_t((char*) t - page->objects_) | kAllocBit;
      t->oid_ = oid;  // Set the oid so the stream code can call the dbObject
                      // methods.
      page->alloc_cnt_++;
      stream >> *t;
      // Check that the streaming code did not overwrite the oid
      assert(t->oid_ == oid);
    }
  }
}

template <class T, uint32_t page_size>
dbOStream& operator<<(dbOStream& stream,
                      const NamedTable<T, page_size>& named_table)
{
  dbOStreamScope scope(
      stream,
      fmt::format("{}({})", named_table.name, named_table.table->size()));
  stream << *named_table.table;
  return stream;
}

template <class T, uint32_t page_size>
dbOStream& operator<<(dbOStream& stream, const dbTable<T, page_size>& table)
{
  stream << table.top_idx_;
  stream << table.bottom_idx_;
  stream << table.page_cnt_;
  stream << table.page_tbl_size_;
  stream << table.alloc_cnt_;
  stream << table.free_list_;

  for (uint32_t i = 0; i < table.page_cnt_; ++i) {
    const dbTablePage* page = table.pages_[i];
    table.writePage(stream, page);
  }

  stream << table.prop_list_;

  return stream;
}

template <class T, uint32_t page_size>
dbIStream& operator>>(dbIStream& stream, dbTable<T, page_size>& table)
{
  table.clear();
  _dbDatabase* db = stream.getDatabase();
  if (!db->isSchema(kSchemaTableMaskShift)) {
    uint32_t page_mask;
    uint32_t page_shift;
    stream >> page_mask;
    stream >> page_shift;
    if (page_mask != table.kPageMask || page_shift != table.kPageShift) {
      utl::Logger* logger = db->getLogger();
      logger->error(utl::ODB,
                    477,
                    "dbTable mask/shift mismatch {}/{} vs {}/{}",
                    page_mask,
                    page_shift,
                    table.kPageMask,
                    table.kPageShift);
    }
  }
  stream >> table.top_idx_;
  stream >> table.bottom_idx_;
  stream >> table.page_cnt_;
  stream >> table.page_tbl_size_;
  stream >> table.alloc_cnt_;
  stream >> table.free_list_;

  if (table.page_tbl_size_ == 0) {
    table.pages_ = nullptr;
    assert(table.page_cnt_ == 0);
  } else {
    table.pages_ = new dbTablePage*[table.page_tbl_size_];
  }

  uint32_t i;
  for (i = 0; i < table.page_cnt_; ++i) {
    uint32_t size = (table.pageSize() * sizeof(T)) + sizeof(dbObjectPage);
    dbTablePage* page = (dbTablePage*) safe_malloc(size);
    memset(page, 0, size);
    page->page_addr_ = i << table.kPageShift;
    page->table_ = &table;
    table.pages_[i] = page;
    table.readPage(stream, page);
  }

  for (; i < table.page_tbl_size_; ++i) {
    table.pages_[i] = nullptr;
  }

  stream >> table.prop_list_;

  return stream;
}

template <class T, uint32_t page_size>
bool dbTable<T, page_size>::operator!=(const dbTable<T, page_size>& table) const
{
  return !operator==(table);
}

template <class T, uint32_t page_size>
bool dbTable<T, page_size>::operator==(const dbTable<T, page_size>& rhs) const
{
  const dbTable<T, page_size>& lhs = *this;

  // These basic parameters should be the same...
  assert(lhs.kPageMask == rhs.kPageMask);
  assert(lhs.kPageShift == rhs.kPageShift);

  // empty tables
  if ((lhs.page_cnt_ == 0) && (rhs.page_cnt_ == 0)) {
    return true;
  }

  // Simple rejection test
  if (lhs.page_cnt_ != rhs.page_cnt_) {
    return false;
  }

  // Simple rejection test
  if (lhs.bottom_idx_ != rhs.bottom_idx_) {
    return false;
  }

  // Simple rejection test
  if (lhs.top_idx_ != rhs.top_idx_) {
    return false;
  }

  // Simple rejection test
  if (lhs.alloc_cnt_ != rhs.alloc_cnt_) {
    return false;
  }

  for (uint32_t i = bottom_idx_; i <= top_idx_; ++i) {
    bool lhs_valid_o = lhs.validId(i);
    bool rhs_valid_o = rhs.validId(i);

    if (lhs_valid_o && rhs_valid_o) {
      const T* l = lhs.getPtr(i);
      const T* r = rhs.getPtr(i);

      if (*l != *r) {
        return false;
      }
    } else if (lhs_valid_o) {
      return false;
    } else if (rhs_valid_o) {
      return false;
    }
  }

  return true;
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::collectMemInfo(MemInfo& info)
{
  for (int i = bottom_idx_; i <= top_idx_; ++i) {
    if (validId(i)) {
      getPtr(i)->collectMemInfo(info);
    }
  }
}

}  // namespace odb
