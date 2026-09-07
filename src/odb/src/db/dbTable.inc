// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <sstream>
#include <string>
#include <vector>

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
  assert((p->offset_in_bytes_ & kAllocBit) == 0);
  return (_dbFreeObject*) p;
}

template <class T, uint32_t page_size>
inline T* dbTable<T, page_size>::getPtr(dbId<T> id) const
{
  const uint32_t page = (uint32_t) id >> kPageShift;
  const uint32_t offset = (uint32_t) id & kPageMask;

  assert(((uint32_t) id != 0) && (page < page_cnt_));
  T* p = (T*) &(pages_[page]->objects_[offset * sizeof(T)]);
  assert(p->offset_in_bytes_ & kAllocBit);
  return p;
}

template <class T, uint32_t page_size>
inline bool dbTable<T, page_size>::validId(dbId<T> id) const
{
  const uint32_t page = (uint32_t) id >> kPageShift;
  const uint32_t offset = (uint32_t) id & kPageMask;

  if (((uint32_t) id != 0) && (page < page_cnt_)) {
    T* p = (T*) &(pages_[page]->objects_[offset * sizeof(T)]);
    return (p->offset_in_bytes_ & kAllocBit) == kAllocBit;
  }

  return false;
}

template <class T, uint32_t page_size>
inline uint32_t dbTable<T, page_size>::freeNext(T* t) const
{
  if constexpr (dbSoaTable<T>()) {
    return t->freeNext();
  } else {
    return ((_dbFreeObject*) t)->next_;
  }
}

template <class T, uint32_t page_size>
inline uint32_t dbTable<T, page_size>::freePrev(T* t) const
{
  if constexpr (dbSoaTable<T>()) {
    return t->freePrev();
  } else {
    return ((_dbFreeObject*) t)->prev_;
  }
}

template <class T, uint32_t page_size>
inline void dbTable<T, page_size>::setFreeLinks(T* t,
                                                const uint32_t next,
                                                const uint32_t prev)
{
  if constexpr (dbSoaTable<T>()) {
    t->setFreeNext(next);
    t->setFreePrev(prev);
  } else {
    _dbFreeObject* o = (_dbFreeObject*) t;
    o->next_ = next;
    o->prev_ = prev;
  }
}

template <class T, uint32_t page_size>
inline void dbTable<T, page_size>::pushQ(uint32_t& Q, T* t)
{
  const uint32_t head_id = t->getImpl()->getOID();
  setFreeLinks(t, Q, 0);
  if (Q != 0) {
    T* head = (T*) getFreeObj(Q);
    setFreeLinks(head, freeNext(head), head_id);
  }
  Q = head_id;
}

template <class T, uint32_t page_size>
inline T* dbTable<T, page_size>::popQ(uint32_t& Q)
{
  T* e = (T*) getFreeObj(Q);
  Q = freeNext(e);

  if (Q) {
    T* head = (T*) getFreeObj(Q);
    setFreeLinks(head, freeNext(head), 0);
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
      if (t->offset_in_bytes_ & kAllocBit) {
        t->~T();
      }
    }

    // The columns outlive the slots and are freed with the page.
    if constexpr (dbSoaTable<T>()) {
      delete (typename T::Fields*) page->fields_;
      page->fields_ = nullptr;
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
  // The columns have to exist before anything reaches for a field, and the
  // free list below is threaded through two of them.
  if constexpr (dbSoaTable<T>()) {
    page->fields_ = new typename T::Fields(pageSize());
  }
  pages_[page_id] = page;

  // The objects are put on the list in reverse order, so they can be removed
  // in low-to-high order.
  T* b = (T*) page->objects_;
  T* t = &b[kPageMask];

  for (; t >= b; --t) {
    t->offset_in_bytes_ = (uint32_t) ((char*) t - (char*) b);
    t->oid_ = page->page_addr_ | (uint32_t) (t - b);

    if (page_id == 0 && t == b) {
      continue;  // don't link zero-object
    }
    pushQ(free_list_, t);
  }
}

template <class T, uint32_t page_size>
T* dbTable<T, page_size>::create()
{
  ++alloc_cnt_;

  if (free_list_ == 0) {
    newPage();
  }

  T* t = popQ(free_list_);
  const uint32_t offset = t->offset_in_bytes_;
  const uint32_t saved_oid = t->oid_;
  new (t) T(db_);
  t->offset_in_bytes_ = offset | kAllocBit;
  t->oid_ = saved_oid;
  // A columnar slot holds nothing for the constructor to initialize, so the
  // fields it left behind in the columns are reset here instead.
  if constexpr (dbSoaTable<T>()) {
    t->initFields();
  }

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
      if (s->offset_in_bytes_ & kAllocBit) {
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
    if (s->offset_in_bytes_ & kAllocBit) {
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
      if (s->offset_in_bytes_ & kAllocBit) {
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
    if (s->offset_in_bytes_ & kAllocBit) {
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
  assert(t->offset_in_bytes_ & kAllocBit);

  dbTablePage* page = (dbTablePage*) t->getObjectPage();

  page->alloc_cnt_--;
  const uint32_t offset = t->offset_in_bytes_;
  const uint32_t oid = t->oid_;
  // Release whatever the slot owns. For a columnar table that is in the
  // columns rather than in the slot, so the destructor has nothing to do
  // and clearFields does the work -- and it has to run while the slot can
  // still find its page.
  if constexpr (dbSoaTable<T>()) {
    t->clearFields();
  }
  t->~T();  // call destructor
  t->offset_in_bytes_ = offset & ~kAllocBit;
  t->oid_ = oid;

  const uint32_t id_in_page = t - (T*) page->objects_;
  const uint32_t id = page->page_addr_ + id_in_page;

  // Add to freelist
  pushQ(free_list_, t);

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
    if (p->offset_in_bytes_ & kAllocBit) {
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
    if (t->offset_in_bytes_ & kAllocBit) {
      const char allocated = 1;
      stream << allocated;
      stream << *t;
    } else {
      const char allocated = 0;
      stream << allocated;
      stream << freeNext((T*) t);
      stream << freePrev((T*) t);
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

    const uint32_t obj_oid
        = page->page_addr_ | (uint32_t) (t - (T*) page->objects_);

    if (!allocated) {
      t->offset_in_bytes_ = (uint32_t) ((char*) t - page->objects_);
      t->oid_ = obj_oid;
      uint32_t next;
      uint32_t prev;
      stream >> next;
      stream >> prev;
      setFreeLinks(t, next, prev);
    } else {
      new (t) T(db_);
      uint32_t offset = uint32_t((char*) t - page->objects_) | kAllocBit;
      t->offset_in_bytes_ = offset;  // Set so the stream code can call the
                                     // dbObject methods.
      t->oid_ = obj_oid;
      page->alloc_cnt_++;
      stream >> *t;
      // Check that the streaming code did not overwrite the offset
      assert(t->offset_in_bytes_ == offset);
    }
  }
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::writeBlock(dbOStream& stream,
                                       const uint32_t first_page,
                                       const uint32_t pages) const
{
  const uint32_t slots = pages * page_size;

  // Serialize every allocated slot through the type's own operator<<, which
  // is the point: a field-major writer that needed a hand-written
  // serializer per type would not be worth having. The records land
  // end-to-end in a scratch buffer and we remember where each one starts.
  std::ostringstream scratch(std::ios::binary);
  dbOStream records(stream.getDatabase(), scratch);
  std::vector<uint32_t> lengths;      // per allocated slot, in slot order
  std::vector<uint32_t> starts;       // ditto, into the scratch buffer
  std::vector<uint32_t> free_links;   // next, prev per free slot, in order
  std::vector<char> allocated(slots, 0);

  dbOStream::Position mark = records.pos();
  for (uint32_t i = 0; i < slots; ++i) {
    const dbTablePage* page = pages_[first_page + (i >> kPageShift)];
    const T* t = &((const T*) page->objects_)[i & kPageMask];

    if ((t->offset_in_bytes_ & kAllocBit) == 0) {
      free_links.push_back(freeNext((T*) t));
      free_links.push_back(freePrev((T*) t));
      continue;
    }

    allocated[i] = 1;
    records << *t;
    const dbOStream::Position now = records.pos();
    starts.push_back((uint32_t) mark);
    lengths.push_back((uint32_t) (now - mark));
    mark = now;
  }

  const std::string buffer = scratch.str();

  // Records of one length transpose; records of different lengths do not.
  // After pin access an iterm either carries an access point or does not, so
  // a block holds two lengths and splitting by length is what keeps the
  // layout working -- measured at +14% on the routed case over passing a
  // mixed block through unchanged.
  std::vector<uint32_t> classes(lengths);
  std::sort(classes.begin(), classes.end());
  classes.erase(std::unique(classes.begin(), classes.end()), classes.end());

  // A table whose records are all different lengths -- a net and its wire,
  // say -- has as many classes as records, and then splitting by length
  // buys nothing and costs a pass per class. Past a handful of classes the
  // block goes out exactly as the per-slot writer would have written it,
  // which is also what makes it safe to opt every table in: the ones the
  // layout cannot help simply fall back.
  const bool transposable = classes.size() <= kMaxLengthClasses;
  if (!transposable) {
    classes.clear();
  }

  stream << (uint32_t) classes.size();
  for (const uint32_t length : classes) {
    stream << length;
  }

  // The allocation bitmap, and then one byte of class index per allocated
  // slot. A byte rather than packed bits on purpose: the index column is
  // one of the most compressible things in the block, so the bytes cost
  // almost nothing once the block is compressed, and the reader stays
  // obvious.
  for (uint32_t i = 0; i < slots; i += 8) {
    unsigned char bits = 0;
    for (uint32_t bit = 0; bit < 8 && i + bit < slots; ++bit) {
      if (allocated[i + bit]) {
        bits |= (unsigned char) (1u << bit);
      }
    }
    stream << bits;
  }
  // One class needs no index at all, and the fallback needs none either.
  if (classes.size() > 1) {
    for (const uint32_t length : lengths) {
      const uint32_t index
          = (uint32_t) (std::lower_bound(classes.begin(), classes.end(), length)
                        - classes.begin());
      stream << (unsigned char) index;
    }
  }
  for (const uint32_t link : free_links) {
    stream << link;
  }


  // Field-major at last: byte j of every record of a class, then byte j + 1.
  // Gathered into one column and written with a single call -- a byte at a
  // time through the stream is a function call per byte, which costs more
  // than the whole rest of the write put together.
  if (!transposable) {
    stream.writeBytes(buffer);
    return;
  }

  // Bucket the records by length in one pass. Scanning every record once
  // per class instead is quadratic when a table's records are all different
  // lengths, which is the common case for anything carrying a wire.
  std::vector<std::vector<uint32_t>> members(classes.size());
  for (size_t record = 0; record < lengths.size(); ++record) {
    const size_t klass
        = (size_t) (std::lower_bound(classes.begin(), classes.end(), lengths[record])
                    - classes.begin());
    members[klass].push_back(starts[record]);
  }

  std::vector<char> column;
  for (size_t klass = 0; klass < classes.size(); ++klass) {
    const std::vector<uint32_t>& group = members[klass];
    column.resize(group.size());
    for (uint32_t j = 0; j < classes[klass]; ++j) {
      for (size_t i = 0; i < group.size(); ++i) {
        column[i] = buffer[group[i] + j];
      }
      stream.writeBytes(column);
    }
  }
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::readBlock(dbIStream& stream,
                                      const uint32_t first_page,
                                      const uint32_t pages)
{
  const uint32_t slots = pages * page_size;

  uint32_t class_cnt;
  stream >> class_cnt;
  std::vector<uint32_t> classes(class_cnt);
  for (uint32_t i = 0; i < class_cnt; ++i) {
    stream >> classes[i];
  }

  std::vector<char> allocated(slots, 0);
  uint32_t alloc_cnt = 0;
  for (uint32_t i = 0; i < slots; i += 8) {
    unsigned char bits;
    stream >> bits;
    for (uint32_t bit = 0; bit < 8 && i + bit < slots; ++bit) {
      allocated[i + bit] = (bits >> bit) & 1u;
      alloc_cnt += allocated[i + bit];
    }
  }

  std::vector<uint32_t> lengths(alloc_cnt, 0);
  if (class_cnt == 1) {
    std::fill(lengths.begin(), lengths.end(), classes[0]);
  } else if (class_cnt > 1) {
    for (uint32_t i = 0; i < alloc_cnt; ++i) {
      unsigned char index;
      stream >> index;
      lengths[i] = classes[index];
    }
  }

  std::vector<uint32_t> free_links(2 * (slots - alloc_cnt));
  for (uint32_t& link : free_links) {
    stream >> link;
  }

  // No classes means the writer could not use the layout for this block and
  // wrote the records verbatim, in slot order -- which is the order the
  // per-slot reader wants anyway, so it reads them straight from the file.
  const bool transposed = class_cnt > 0;

  std::string buffer;
  if (transposed) {
    // Un-transpose into the records the per-slot reader expects, so the
    // rest of this is the same code path a legacy read takes.
    std::vector<uint32_t> starts(alloc_cnt);
    uint32_t total = 0;
    for (uint32_t i = 0; i < alloc_cnt; ++i) {
      starts[i] = total;
      total += lengths[i];
    }
    buffer.assign(total, '\0');

    std::vector<std::vector<uint32_t>> members(classes.size());
    for (uint32_t record = 0; record < alloc_cnt; ++record) {
      const size_t klass
          = (size_t) (std::lower_bound(
                          classes.begin(), classes.end(), lengths[record])
                      - classes.begin());
      members[klass].push_back(starts[record]);
    }

    std::vector<char> column;
    for (size_t klass = 0; klass < classes.size(); ++klass) {
      const std::vector<uint32_t>& group = members[klass];
      column.resize(group.size());
      for (uint32_t j = 0; j < classes[klass]; ++j) {
        stream.readBytes(column);
        for (size_t i = 0; i < group.size(); ++i) {
          buffer[group[i] + j] = column[i];
        }
      }
    }
  }

  std::istringstream scratch(buffer, std::ios::binary);
  dbIStream unpacked(stream.getDatabase(), scratch);
  dbIStream& records = transposed ? unpacked : stream;

  uint32_t next_record = 0;
  uint32_t next_link = 0;
  for (uint32_t i = 0; i < slots; ++i) {
    dbTablePage* page = pages_[first_page + (i >> kPageShift)];
    T* t = &((T*) page->objects_)[i & kPageMask];
    const uint32_t slot = i & kPageMask;
    const uint32_t obj_oid = page->page_addr_ | slot;

    if (!allocated[i]) {
      t->offset_in_bytes_ = slot * sizeof(T);
      t->oid_ = obj_oid;
      const uint32_t next = free_links[next_link++];
      const uint32_t prev = free_links[next_link++];
      setFreeLinks(t, next, prev);
      continue;
    }

    new (t) T(db_);
    const uint32_t offset = (uint32_t) (slot * sizeof(T)) | kAllocBit;
    t->offset_in_bytes_ = offset;
    t->oid_ = obj_oid;
    page->alloc_cnt_++;
    records >> *t;
    // The record must have been consumed exactly, or the field-major layout
    // and the per-slot reader disagree about this type.
    assert(t->offset_in_bytes_ == offset);
    ++next_record;
  }
  assert(next_record == alloc_cnt);
}

template <class T, uint32_t page_size>
uint32_t dbTable<T, page_size>::blockPages(const uint32_t first_page) const
{
  // Slots bound the block, and so does its size in bytes. Without the byte
  // bound a table of large records -- a net and its wire -- would buffer
  // thousands of them at once, which costs more memory than the layout
  // saves. Records are sized here by walking the page's allocation counts
  // rather than by serializing anything: the estimate only has to be good
  // enough to pick a boundary, and the boundary is written to the stream.
  uint32_t pages = 0;
  uint64_t bytes = 0;
  while (first_page + pages < page_cnt_ && pages < kPagesPerBlock
         && bytes < kMaxBlockBytes) {
    bytes += (uint64_t) pages_[first_page + pages]->alloc_cnt_ * sizeof(T);
    ++pages;
  }
  return pages ? pages : 1;
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::writeBlocks(dbOStream& stream) const
{
  for (uint32_t page = 0; page < page_cnt_;) {
    const uint32_t pages = blockPages(page);
    stream << pages;
    writeBlock(stream, page, pages);
    page += pages;
  }
}

template <class T, uint32_t page_size>
void dbTable<T, page_size>::readBlocks(dbIStream& stream)
{
  for (uint32_t page = 0; page < page_cnt_;) {
    uint32_t pages;
    stream >> pages;
    readBlock(stream, page, pages);
    page += pages;
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


  if constexpr (dbFieldMajorTable<T>()) {
    table.writeBlocks(stream);
  } else {
    for (uint32_t i = 0; i < table.page_cnt_; ++i) {
      const dbTablePage* page = table.pages_[i];
      table.writePage(stream, page);
    }
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

  // A file written before kSchemaFieldMajorBlocks holds whole records, one
  // per slot, whatever this table opted into since. Reading it is the same
  // per-slot loop it always was -- simple, and deliberately not made fast:
  // the cost falls only on files older than the bump, and correctness there
  // matters far more than speed.
  const bool field_major
      = dbFieldMajorTable<T>() && db->isSchema(kSchemaFieldMajorBlocks);

  uint32_t i;
  for (i = 0; i < table.page_cnt_; ++i) {
    uint32_t size = (table.pageSize() * sizeof(T)) + sizeof(dbObjectPage);
    dbTablePage* page = (dbTablePage*) safe_malloc(size);
    memset(page, 0, size);
    page->page_addr_ = i << table.kPageShift;
    page->table_ = &table;
    // Pages read from a file are built here rather than by newPage(), so
    // their columns have to be created here too.
    if constexpr (dbSoaTable<T>()) {
      page->fields_ = new typename T::Fields(table.pageSize());
    }
    table.pages_[i] = page;
    if (!field_major) {
      table.readPage(stream, page);
    }
  }

  if (field_major) {
    table.readBlocks(stream);
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
