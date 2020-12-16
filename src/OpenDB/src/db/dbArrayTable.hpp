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

#include <string.h>
#include <new>

#include "dbDiff.h"
#include "ZException.h"
#include "dbArrayTable.h"
#include "dbStream.h"

namespace odb {
template <class T>
inline void dbArrayTable<T>::pushQ(uint& Q, _dbFreeObject* e)
{
  if (Q == 0) {
    e->_prev = 0;
    e->_next = 0;
    Q        = e->getImpl()->getOID();
  } else {
    e->_prev            = 0;
    e->_next            = Q;
    _dbFreeObject* head = (_dbFreeObject*) getFreeObj(Q);
    head->_prev         = e->getImpl()->getOID();
    Q                   = e->getImpl()->getOID();
  }
}

template <class T>
inline _dbFreeObject* dbArrayTable<T>::popQ(uint& Q)
{
  _dbFreeObject* e = (_dbFreeObject*) getFreeObj(Q);
  Q                = e->_next;

  if (Q) {
    _dbFreeObject* head = (_dbFreeObject*) getFreeObj(Q);
    head->_prev         = 0;
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
      head->_prev         = 0;
    }
  } else {
    if (e->_next) {
      _dbFreeObject* next = (_dbFreeObject*) getFreeObj(e->_next);
      next->_prev         = e->_prev;
    }

    if (e->_prev) {
      _dbFreeObject* prev = (_dbFreeObject*) getFreeObj(e->_prev);
      prev->_next         = e->_next;
    }
  }
}

template <class T>
void dbArrayTable<T>::clear()
{
  uint i;
  for (i = 0; i < _page_cnt; ++i) {
    dbArrayTablePage* page = _pages[i];
    const T*          t    = (T*) page->_objects;
    const T*          e    = &t[page_size()];

    for (; t < e; t++) {
      if (t->_oid & DB_ALLOC_BIT)
        t->~T();
    }

    free((void*) page);
  }

  if (_pages)
    delete[] _pages;

  _page_cnt      = 0;
  _page_tbl_size = 0;
  _alloc_cnt     = 0;
  _free_list     = 0;
  _pages         = NULL;
}

template <class T>
dbArrayTable<T>::dbArrayTable(_dbDatabase* db,
                              dbObject*    owner,
                              GetObjTbl_t  m,
                              dbObjectType type,
                              uint         array_size,
                              uint         page_size,
                              uint         page_shift)
    : dbObjectTable(db, owner, m, type, sizeof(T))
{
  _page_mask         = page_size - 1;
  _page_shift        = page_shift;
  _page_cnt          = 0;
  _page_tbl_size     = 0;
  _alloc_cnt         = 0;
  _objects_per_alloc = array_size;
  _free_list         = 0;
  _pages             = NULL;
}

template <class T>
dbArrayTable<T>::dbArrayTable(_dbDatabase*           db,
                              dbObject*              owner,
                              const dbArrayTable<T>& t)
    : dbObjectTable(db, owner, t._getObjectTable, t._type, sizeof(T)),
      _page_mask(t._page_mask),
      _page_shift(t._page_shift),
      _page_cnt(t._page_cnt),
      _page_tbl_size(t._page_tbl_size),
      _alloc_cnt(t._alloc_cnt),
      _objects_per_alloc(t._objects_per_alloc),
      _free_list(t._free_list),
      _pages(NULL)
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
  uint               i;
  dbArrayTablePage** old_tbl      = _pages;
  uint               old_tbl_size = _page_tbl_size;
  _page_tbl_size *= 2;

  _pages = new dbArrayTablePage*[_page_tbl_size];
  ZALLOCATED(_pages);

  for (i = 0; i < old_tbl_size; ++i)
    _pages[i] = old_tbl[i];

  for (; i < _page_tbl_size; ++i)
    _pages[i] = NULL;

  delete[] old_tbl;
}

template <class T>
void dbArrayTable<T>::newPage()
{
  uint              size = page_size() * sizeof(T) + sizeof(dbObjectPage);
  dbArrayTablePage* page = (dbArrayTablePage*) malloc(size);
  ZALLOCATED(page);
  memset(page, 0, size);

  uint page_id = _page_cnt;

  if (_page_tbl_size == 0) {
    _pages = new dbArrayTablePage*[1];
    ZALLOCATED(_pages);
    _page_tbl_size = 1;
  } else if (_page_tbl_size == _page_cnt) {
    resizePageTbl();
  }

  ++_page_cnt;
  page->_table     = this;
  page->_page_addr = page_id << _page_shift;
  page->_alloccnt  = 0;
  _pages[page_id]  = page;

  // The objects are put on the list in reverse order, so they can be removed
  // in low-to-high order.
  if (page_id == 0) {
    T* b = (T*) page->_objects;
    T* t = &b[_page_mask];

    for (; t >= b; --t) {
      _dbFreeObject* o = (_dbFreeObject*) t;
      o->_oid          = (uint)((char*) t - (char*) b);

      if (t != b)  // don't link zero-object
        pushQ(_free_list, o);
    }
  } else {
    T* b = (T*) page->_objects;
    T* t = &b[_page_mask];

    for (; t >= b; --t) {
      _dbFreeObject* o = (_dbFreeObject*) t;
      o->_oid          = (uint)((char*) t - (char*) b);
      pushQ(_free_list, o);
    }
  }
}

template <class T>
T* dbArrayTable<T>::create()
{
  ++_alloc_cnt;

  if (_free_list == 0)
    newPage();

  _dbFreeObject* o = popQ(_free_list);
  o->_oid |= DB_ALLOC_BIT;
  new (o) T(_db);
  T* t = (T*) o;

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

  for (i = 1; i < _objects_per_alloc; i++) {
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
  uint i = id + _objects_per_alloc - 1;

  // Destroy in reverse order to ensure the object are pushed onto the
  // freelist in the correct order.
  for (; i >= id; --i)
    destroy(getPtr(i));
}

template <class T>
void dbArrayTable<T>::destroy(T* t)
{
  --_alloc_cnt;

  ZASSERT(t->getOID() != 0);
  ZASSERT(t->getTable() == this);
  ZASSERT(t->_oid & DB_ALLOC_BIT);

  dbArrayTablePage* page = (dbArrayTablePage*) t->getObjectPage();

  page->_alloccnt--;
  t->~T();  // call destructor
  t->_oid &= ~DB_ALLOC_BIT;

  // Add to freelist
  _dbFreeObject* o = (_dbFreeObject*) t;
  pushQ(_free_list, o);
}

template <class T>
void dbArrayTable<T>::writePage(dbOStream&              stream,
                                const dbArrayTablePage* page) const
{
  const T* t = (T*) page->_objects;
  const T* e = &t[page_size()];

  for (; t < e; t++) {
    if (t->_oid & DB_ALLOC_BIT) {
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
  T* t            = (T*) page->_objects;
  T* e            = &t[page_size()];
  page->_alloccnt = 0;

  for (; t < e; t++) {
    char allocated;
    stream >> allocated;

    if (!allocated) {
      t->_oid          = (uint)((char*) t - page->_objects);
      _dbFreeObject* o = (_dbFreeObject*) t;
      stream >> o->_next;
      stream >> o->_prev;
    } else {
      new (t) T(_db);
      stream >> *t;
      t->_oid = (uint)((char*) t - page->_objects);
      t->_oid |= DB_ALLOC_BIT;
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
  _pages = new dbArrayTablePage*[_page_tbl_size];
  ZALLOCATED(_pages);

  uint i;

  for (i = 0; i < _page_tbl_size; ++i)
    _pages[i] = NULL;

  for (i = 0; i < _page_cnt; ++i) {
    dbArrayTablePage* page = t._pages[i];
    copy_page(i, page);
  }
}

template <class T>
void dbArrayTable<T>::copy_page(uint page_id, dbArrayTablePage* page)
{
  uint              size = page_size() * sizeof(T) + sizeof(dbObjectPage);
  dbArrayTablePage* p    = (dbArrayTablePage*) malloc(size);
  ZALLOCATED(p);
  memset(p, 0, size);
  p->_table       = this;
  p->_page_addr   = page_id << _page_shift;
  p->_alloccnt    = page->_alloccnt;
  _pages[page_id] = p;

  const T* t = (T*) page->_objects;
  const T* e = &t[page_size()];
  T*       o = (T*) p->_objects;

  for (; t < e; t++, o++) {
    if (t->_oid & DB_ALLOC_BIT) {
      o->_oid = t->_oid;
      new (o) T(_db, *t);
    } else {
      *((_dbFreeObject*) o) = *((_dbFreeObject*) t);
    }
  }
}

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbArrayTable<T>& table)
{
  stream << table._page_mask;
  stream << table._page_shift;
  stream << table._page_cnt;
  stream << table._page_tbl_size;
  stream << table._alloc_cnt;
  stream << table._objects_per_alloc;
  stream << table._free_list;

  uint i;
  for (i = 0; i < table._page_cnt; ++i) {
    const dbArrayTablePage* page = table._pages[i];
    table.writePage(stream, page);
  }

  return stream;
}

template <class T>
dbIStream& operator>>(dbIStream& stream, dbArrayTable<T>& table)
{
  table.clear();
  stream >> table._page_mask;
  stream >> table._page_shift;
  stream >> table._page_cnt;
  stream >> table._page_tbl_size;
  stream >> table._alloc_cnt;
  stream >> table._objects_per_alloc;
  stream >> table._free_list;

  if (table._page_tbl_size == 0)
    table._pages = NULL;
  else {
    table._pages = new dbArrayTablePage*[table._page_tbl_size];
    ZALLOCATED(table._pages);
  }

  uint i;
  for (i = 0; i < table._page_cnt; ++i) {
    uint size = table.page_size() * sizeof(T) + sizeof(dbObjectPage);
    dbArrayTablePage* page = (dbArrayTablePage*) malloc(size);
    ZALLOCATED(page);
    memset(page, 0, size);
    page->_page_addr = i << table._page_shift;
    page->_table     = &table;
    table._pages[i]  = page;
    table.readPage(stream, page);
  }

  for (; i < table._page_tbl_size; ++i)
    table._pages[i] = NULL;

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
  assert(lhs._page_mask == rhs._page_mask);
  assert(lhs._page_shift == rhs._page_shift);

  // empty tables
  if ((lhs._page_cnt == 0) && (rhs._page_cnt == 0))
    return true;

  // Simple rejection test
  if (lhs._page_cnt != rhs._page_cnt)
    return false;

  // Simple rejection test
  if (lhs._alloc_cnt != rhs._alloc_cnt)
    return false;

  uint i;

  for (i = 1; i < _alloc_cnt; ++i) {
    bool lhs_valid_o = lhs.validId(i);
    bool rhs_valid_o = rhs.validId(i);

    if (lhs_valid_o && rhs_valid_o) {
      const T* l = lhs.getPtr(i);
      const T* r = rhs.getPtr(i);

      if (*l != *r)
        return false;
    } else if (lhs_valid_o) {
      return false;
    } else if (rhs_valid_o) {
      return false;
    }
  }

  return true;
}

template <class T>
void dbArrayTable<T>::differences(dbDiff&                diff,
                                  const dbArrayTable<T>& rhs) const
{
  const dbArrayTable<T>& lhs = *this;

  // These basic parameters should be the same...
  assert(lhs._page_mask == rhs._page_mask);
  assert(lhs._page_shift == rhs._page_shift);

  uint page_sz = 1U << lhs._page_shift;
  uint lhs_max = lhs._page_cnt * page_sz;
  uint rhs_max = rhs._page_cnt * page_sz;

  uint        i;
  const char* name = dbObject::getObjName(_type);

  for (i = 1; (i < lhs_max) && (i < rhs_max); ++i) {
    bool lhs_valid_o = lhs.validId(i);
    bool rhs_valid_o = rhs.validId(i);

    if (lhs_valid_o && rhs_valid_o) {
      T* l = lhs.getPtr(i);
      T* r = rhs.getPtr(i);
      l->differences(diff, NULL, *r);
    } else if (lhs_valid_o) {
      T* l = lhs.getPtr(i);
      l->out(diff, dbDiff::LEFT, NULL);
      diff.report("> %s [%u] FREE\n", name, i);
    } else if (rhs_valid_o) {
      T* r = rhs.getPtr(i);
      diff.report("< %s [%u] FREE\n", name, i);
      r->out(diff, dbDiff::RIGHT, NULL);
    }
  }
}

template <class T>
void dbArrayTable<T>::out(dbDiff& diff, char side) const
{
  uint i;

  for (i = 1; i < _alloc_cnt; ++i) {
    if (validId(i)) {
      T* o = getPtr(i);
      o->out(diff, side, NULL);
    }
  }
}

template <class T>
void dbArrayTable<T>::getObjects(std::vector<T*>& objects)
{
  objects.clear();
  objects.reserve(size());

  uint i;

  for (i = 1; i < _alloc_cnt; ++i) {
    if (validId(i))
      objects.push_back(getPtr(i));
  }
}

}  // namespace odb
