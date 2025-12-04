// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

#include "dbArrayTable.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"
#include "odb/dbStream.h"

namespace odb {
template <class T>
inline void dbArrayTable<T>::pushQ(uint& Q, _dbFreeObject* e)
{
  if (Q == 0) {
    e->_prev = 0;
    e->_next = 0;
    Q = e->getImpl()->getOID();
  } else {
    e->_prev = 0;
    e->_next = Q;
    _dbFreeObject* head = (_dbFreeObject*) getFreeObj(Q);
    head->_prev = e->getImpl()->getOID();
    Q = e->getImpl()->getOID();
  }
}

template <class T>
inline _dbFreeObject* dbArrayTable<T>::popQ(uint& Q)
{
  _dbFreeObject* e = (_dbFreeObject*) getFreeObj(Q);
  Q = e->_next;

  if (Q) {
    _dbFreeObject* head = (_dbFreeObject*) getFreeObj(Q);
    head->_prev = 0;
  }

  return e;
}

template <class T>
inline void dbArrayTable<T>::unlinkQ(uint& Q, _dbFreeObject* e)
{
  uint oid = e->getImpl()->getOID();

  if (oid == Q) {
    Q = e->_next;

    if (Q) {
      _dbFreeObject* head = (_dbFreeObject*) getFreeObj(Q);
      head->_prev = 0;
    }
  } else {
    if (e->_next) {
      _dbFreeObject* next = (_dbFreeObject*) getFreeObj(e->_next);
      next->_prev = e->_prev;
    }

    if (e->_prev) {
      _dbFreeObject* prev = (_dbFreeObject*) getFreeObj(e->_prev);
      prev->_next = e->_next;
    }
  }
}

template <class T>
void dbArrayTable<T>::clear()
{
  uint i;
  for (i = 0; i < page_cnt_; ++i) {
    dbArrayTablePage* page = pages_[i];
    const T* t = (T*) page->objects_;
    const T* e = &t[page_size()];

    for (; t < e; t++) {
      if (t->_oid & DB_ALLOC_BIT) {
        t->~T();
      }
    }

    free((void*) page);
  }

  {
    delete[] pages_;
  }

  page_cnt_ = 0;
  page_tbl_size_ = 0;
  alloc_cnt_ = 0;
  free_list_ = 0;
  pages_ = nullptr;
}

template <class T>
dbArrayTable<T>::dbArrayTable(_dbDatabase* db,
                              dbObject* owner,
                              GetObjTbl_t m,
                              dbObjectType type,
                              uint array_size,
                              uint page_size,
                              uint page_shift)
    : dbObjectTable(db, owner, m, type, sizeof(T))
{
  page_mask_ = page_size - 1;
  page_shift_ = page_shift;
  page_cnt_ = 0;
  page_tbl_size_ = 0;
  alloc_cnt_ = 0;
  objects_per_alloc_ = array_size;
  free_list_ = 0;
  pages_ = nullptr;
}

template <class T>
dbArrayTable<T>::dbArrayTable(_dbDatabase* db,
                              dbObject* owner,
                              const dbArrayTable<T>& t)
    : dbObjectTable(db, owner, t.getObjectTable_, t.type_, sizeof(T)),
      page_mask_(t.page_mask_),
      page_shift_(t.page_shift_),
      page_cnt_(t.page_cnt_),
      page_tbl_size_(t.page_tbl_size_),
      alloc_cnt_(t.alloc_cnt_),
      objects_per_alloc_(t.objects_per_alloc_),
      free_list_(t.free_list_),
      pages_(nullptr)
{
  copy_pages(t);
}

template <class T>
dbArrayTable<T>::~dbArrayTable()
{
  clear();
}

template <class T>
void dbArrayTable<T>::resizePageTbl()
{
  uint i;
  dbArrayTablePage** old_tbl = pages_;
  uint old_tbl_size = page_tbl_size_;
  page_tbl_size_ *= 2;

  pages_ = new dbArrayTablePage*[page_tbl_size_];

  for (i = 0; i < old_tbl_size; ++i) {
    pages_[i] = old_tbl[i];
  }

  for (; i < page_tbl_size_; ++i) {
    pages_[i] = nullptr;
  }

  delete[] old_tbl;
}

template <class T>
void dbArrayTable<T>::newPage()
{
  uint size = page_size() * sizeof(T) + sizeof(dbObjectPage);
  dbArrayTablePage* page = (dbArrayTablePage*) safe_malloc(size);
  memset(page, 0, size);

  uint page_id = page_cnt_;

  if (page_tbl_size_ == 0) {
    pages_ = new dbArrayTablePage*[1];
    page_tbl_size_ = 1;
  } else if (page_tbl_size_ == page_cnt_) {
    resizePageTbl();
  }

  ++page_cnt_;
  page->_table = this;
  page->_page_addr = page_id << page_shift_;
  page->_alloccnt = 0;
  pages_[page_id] = page;

  // The objects are put on the list in reverse order, so they can be removed
  // in low-to-high order.
  if (page_id == 0) {
    T* b = (T*) page->objects_;
    T* t = &b[page_mask_];

    for (; t >= b; --t) {
      _dbFreeObject* o = (_dbFreeObject*) t;
      o->oid_ = (uint) ((char*) t - (char*) b);

      if (t != b) {  // don't link zero-object
        pushQ(free_list_, o);
      }
    }
  } else {
    T* b = (T*) page->objects_;
    T* t = &b[page_mask_];

    for (; t >= b; --t) {
      _dbFreeObject* o = (_dbFreeObject*) t;
      o->oid_ = (uint) ((char*) t - (char*) b);
      pushQ(free_list_, o);
    }
  }
}

template <class T>
T* dbArrayTable<T>::create()
{
  ++alloc_cnt_;

  if (free_list_ == 0) {
    newPage();
  }

  _dbFreeObject* o = popQ(free_list_);
  const uint oid = o->oid_;
  new (o) T(db_);
  T* t = (T*) o;
  t->oid_ = oid | DB_ALLOC_BIT;

  dbArrayTablePage* page = (dbArrayTablePage*) t->getObjectPage();
  page->_alloccnt++;

  // uint id = t->getOID();
  return t;
}

template <class T>
dbId<T> dbArrayTable<T>::createArray()
{
  T* s = create();
  T* e = s;

  uint i;

  for (i = 1; i < objects_per_alloc_; i++) {
    T* c = create();
    assert((e->getOID() + 1) == c->getOID());
    e = c;
  }

  uint sid = s->getOID();
  return sid;
}

template <class T>
void dbArrayTable<T>::destroyArray(dbId<T> id)
{
  uint i = id + objects_per_alloc_ - 1;

  // Destroy in reverse order to ensure the object are pushed onto the
  // freelist in the correct order.
  for (; i >= id; --i) {
    destroy(getPtr(i));
  }
}

template <class T>
void dbArrayTable<T>::destroy(T* t)
{
  --alloc_cnt_;

  assert(t->getOID() != 0);
  assert(t->getTable() == this);
  assert(t->oid_ & DB_ALLOC_BIT);

  dbArrayTablePage* page = (dbArrayTablePage*) t->getObjectPage();
  _dbFreeObject* o = (_dbFreeObject*) t;

  page->_alloccnt--;
  const uint oid = t->oid_;
  t->~T();  // call destructor
  o->oid_ = oid & ~DB_ALLOC_BIT;

  // Add to freelist
  pushQ(free_list_, o);
}

template <class T>
void dbArrayTable<T>::writePage(dbOStream& stream,
                                const dbArrayTablePage* page) const
{
  const T* t = (T*) page->objects_;
  const T* e = &t[page_size()];

  for (; t < e; t++) {
    if (t->oid_ & DB_ALLOC_BIT) {
      char allocated = 1;
      stream << allocated;
      stream << *t;
    } else {
      char allocated = 0;
      stream << allocated;
      _dbFreeObject* o = (_dbFreeObject*) t;
      stream << o->_next;
      stream << o->_prev;
    }
  }
}

template <class T>
void dbArrayTable<T>::readPage(dbIStream& stream, dbArrayTablePage* page)
{
  T* t = (T*) page->objects_;
  T* e = &t[page_size()];
  page->_alloccnt = 0;

  for (; t < e; t++) {
    char allocated;
    stream >> allocated;

    if (!allocated) {
      t->oid_ = (uint) ((char*) t - page->objects_);
      _dbFreeObject* o = (_dbFreeObject*) t;
      stream >> o->_next;
      stream >> o->_prev;
    } else {
      new (t) T(db_);
      stream >> *t;
      t->oid_ = (uint) ((char*) t - page->objects_);
      t->oid_ |= DB_ALLOC_BIT;
      page->_alloccnt++;
    }
  }
}

//
// Copy the pages and the free_page list.
//
template <class T>
void dbArrayTable<T>::copy_pages(const dbArrayTable<T>& t)
{
  pages_ = new dbArrayTablePage*[page_tbl_size_];

  uint i;

  for (i = 0; i < page_tbl_size_; ++i) {
    pages_[i] = nullptr;
  }

  for (i = 0; i < page_cnt_; ++i) {
    dbArrayTablePage* page = t.pages_[i];
    copy_page(i, page);
  }
}

template <class T>
void dbArrayTable<T>::copy_page(uint page_id, dbArrayTablePage* page)
{
  uint size = page_size() * sizeof(T) + sizeof(dbObjectPage);
  dbArrayTablePage* p = (dbArrayTablePage*) safe_malloc(size);
  memset(p, 0, size);
  p->_table = this;
  p->_page_addr = page_id << page_shift_;
  p->_alloccnt = page->_alloccnt;
  pages_[page_id] = p;

  const T* t = (T*) page->objects_;
  const T* e = &t[page_size()];
  T* o = (T*) p->objects_;

  for (; t < e; t++, o++) {
    if (t->oid_ & DB_ALLOC_BIT) {
      new (o) T(db_, *t);
      o->oid_ = t->oid_;
    } else {
      *((_dbFreeObject*) o) = *((_dbFreeObject*) t);
    }
  }
}

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbArrayTable<T>& table)
{
  stream << table.page_mask_;
  stream << table.page_shift_;
  stream << table.page_cnt_;
  stream << table.page_tbl_size_;
  stream << table.alloc_cnt_;
  stream << table.objects_per_alloc_;
  stream << table.free_list_;

  uint i;
  for (i = 0; i < table.page_cnt_; ++i) {
    const dbArrayTablePage* page = table.pages_[i];
    table.writePage(stream, page);
  }

  return stream;
}

template <class T>
dbIStream& operator>>(dbIStream& stream, dbArrayTable<T>& table)
{
  table.clear();
  stream >> table.page_mask_;
  stream >> table.page_shift_;
  stream >> table.page_cnt_;
  stream >> table.page_tbl_size_;
  stream >> table.alloc_cnt_;
  stream >> table.objects_per_alloc_;
  stream >> table.free_list_;

  if (table.page_tbl_size_ == 0) {
    table.pages_ = nullptr;
  } else {
    table.pages_ = new dbArrayTablePage*[table.page_tbl_size_];
  }

  uint i;
  for (i = 0; i < table.page_cnt_; ++i) {
    uint size = table.page_size() * sizeof(T) + sizeof(dbObjectPage);
    dbArrayTablePage* page = (dbArrayTablePage*) safe_malloc(size);
    memset(page, 0, size);
    page->_page_addr = i << table.page_shift_;
    page->_table = &table;
    table.pages_[i] = page;
    table.readPage(stream, page);
  }

  for (; i < table.page_tbl_size_; ++i) {
    table.pages_[i] = nullptr;
  }

  return stream;
}

template <class T>
bool dbArrayTable<T>::operator!=(const dbArrayTable<T>& table) const
{
  return !operator==(table);
}

template <class T>
bool dbArrayTable<T>::operator==(const dbArrayTable<T>& rhs) const
{
  const dbArrayTable<T>& lhs = *this;

  // These basic parameters should be the same...
  assert(lhs.page_mask_ == rhs.page_mask_);
  assert(lhs.page_shift_ == rhs.page_shift_);

  // empty tables
  if ((lhs.page_cnt_ == 0) && (rhs.page_cnt_ == 0)) {
    return true;
  }

  // Simple rejection test
  if (lhs.page_cnt_ != rhs.page_cnt_) {
    return false;
  }

  // Simple rejection test
  if (lhs.alloc_cnt_ != rhs.alloc_cnt_) {
    return false;
  }

  uint i;

  for (i = 1; i < alloc_cnt_; ++i) {
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

template <class T>
void dbArrayTable<T>::getObjects(std::vector<T*>& objects)
{
  objects.clear();
  objects.reserve(size());

  uint i;

  for (i = 1; i < alloc_cnt_; ++i) {
    if (validId(i)) {
      objects.push_back(getPtr(i));
    }
  }
}

}  // namespace odb
