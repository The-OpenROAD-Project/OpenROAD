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
#include "dbTable.h"
#include "dbStream.h"
#include "dbDatabase.h"

//#define ADS_DB_CHECK_STREAM

namespace odb {

template <class T>
inline void dbTable<T>::pushQ(uint& Q, _dbFreeObject* e)
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
inline _dbFreeObject* dbTable<T>::popQ(uint& Q)
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
inline void dbTable<T>::unlinkQ(uint& Q, _dbFreeObject* e)
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
void dbTable<T>::clear()
{
  uint i;
  for (i = 0; i < _page_cnt; ++i) {
    dbTablePage* page = _pages[i];
    const T*     t    = (T*) page->_objects;
    const T*     e    = &t[page_size()];

    for (; t < e; t++) {
      if (t->_oid & DB_ALLOC_BIT)
        t->~T();
    }

    free((void*) page);
  }

  if (_pages)
    delete[] _pages;

  _bottom_idx    = 0;
  _top_idx       = 0;
  _page_cnt      = 0;
  _page_tbl_size = 0;
  _alloc_cnt     = 0;
  _free_list     = 0;
  _pages         = NULL;
}

template <class T>
dbTable<T>::dbTable(_dbDatabase* db,
                    dbObject*    owner,
                    dbObjectTable* (dbObject::*m)(dbObjectType),
                    dbObjectType type,
                    uint         page_size,
                    uint         page_shift)
    : dbObjectTable(db, owner, m, type, sizeof(T))
{
  _page_mask     = page_size - 1;
  _page_shift    = page_shift;
  _bottom_idx    = 0;
  _top_idx       = 0;
  _page_cnt      = 0;
  _page_tbl_size = 0;
  _alloc_cnt     = 0;
  _free_list     = 0;
  _pages         = NULL;
}

template <class T>
dbTable<T>::dbTable(_dbDatabase* db, dbObject* owner, const dbTable<T>& t)
    : dbObjectTable(db, owner, t._getObjectTable, t._type, sizeof(T)),
      _page_mask(t._page_mask),
      _page_shift(t._page_shift),
      _top_idx(t._top_idx),
      _bottom_idx(t._bottom_idx),
      _page_cnt(t._page_cnt),
      _page_tbl_size(t._page_tbl_size),
      _alloc_cnt(t._alloc_cnt),
      _free_list(t._free_list),
      _pages(NULL)
{
  copy_pages(t);
}

template <class T>
dbTable<T>::~dbTable()
{
  clear();
}

template <class T>
void dbTable<T>::resizePageTbl()
{
  uint          i;
  dbTablePage** old_tbl      = _pages;
  uint          old_tbl_size = _page_tbl_size;
  _page_tbl_size *= 2;

  _pages = new dbTablePage*[_page_tbl_size];
  ZALLOCATED(_pages);

  for (i = 0; i < old_tbl_size; ++i)
    _pages[i] = old_tbl[i];

  for (; i < _page_tbl_size; ++i)
    _pages[i] = NULL;

  delete[] old_tbl;
}

template <class T>
void dbTable<T>::newPage()
{
  uint         size = page_size() * sizeof(T) + sizeof(dbObjectPage);
  dbTablePage* page = (dbTablePage*) malloc(size);
  ZALLOCATED(page);
  memset(page, 0, size);

  uint page_id = _page_cnt;

  if (_page_tbl_size == 0) {
    _pages = new dbTablePage*[1];
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
T* dbTable<T>::create()
{
  ++_alloc_cnt;

  if (_free_list == 0)
    newPage();

  _dbFreeObject* o = popQ(_free_list);
  new (o) T(_db);
  o->_oid |= DB_ALLOC_BIT;
  T* t = (T*) o;

  dbTablePage* page = (dbTablePage*) t->getObjectPage();
  page->_alloccnt++;

  uint id = t->getOID();

  if (id > _top_idx)
    _top_idx = id;

  if ((_bottom_idx == 0) || (id < _bottom_idx))
    _bottom_idx = id;

  return t;
}

template <class T>
T* dbTable<T>::duplicate(T* c)
{
  ++_alloc_cnt;

  if (_free_list == 0)
    newPage();

  _dbFreeObject* o = popQ(_free_list);
  o->_oid |= DB_ALLOC_BIT;
  new (o) T(_db, *c);
  T* t = (T*) o;

  dbTablePage* page = (dbTablePage*) t->getObjectPage();
  page->_alloccnt++;

  uint id = t->getOID();

  if (id > _top_idx)
    _top_idx = id;

  if ((_bottom_idx == 0) || (id < _bottom_idx))
    _bottom_idx = id;

  return t;
}

#define ADS_DB_TABLE_BOTTOM_SEARCH_FAILED 0
#define ADS_DB_TABLE_TOP_SEARCH_FAILED 0

template <class T>
inline void dbTable<T>::findBottom()
{
  if (_alloc_cnt == 0) {
    _bottom_idx = 0;
    return;
  }

  uint         page_id = _bottom_idx >> _page_shift;
  dbTablePage* page    = _pages[page_id];

  // if page is still valid, find the next allocated object
  if (page->valid_page()) {
    uint offset = _bottom_idx & _page_mask;
    T*   b      = (T*) page->_objects;
    T*   s      = &b[offset + 1];
    T*   e      = &b[page_size()];
    for (; s < e; s++) {
      if (s->_oid & DB_ALLOC_BIT) {
        offset      = s - b;
        _bottom_idx = (page_id << _page_shift) + offset;
        return;
      }
    }

    // if s == e, then something is corrupted...
    ZASSERT(ADS_DB_TABLE_BOTTOM_SEARCH_FAILED);
  }

  for (++page_id;; ++page_id) {
    assert(page_id < _page_cnt);
    page = _pages[page_id];

    if (page->valid_page())
      break;
  }

  T* b = (T*) page->_objects;
  T* s = b;
  T* e = &s[page_size()];

  for (; s < e; s++) {
    if (s->_oid & DB_ALLOC_BIT) {
      uint offset = s - b;
      _bottom_idx = (page_id << _page_shift) + offset;
      return;
    }
  }

  // if s == e, then something is corrupted...
  ZASSERT(ADS_DB_TABLE_BOTTOM_SEARCH_FAILED);
}

template <class T>
inline void dbTable<T>::findTop()
{
  if (_alloc_cnt == 0) {
    _top_idx = 0;
    return;
  }

  uint         page_id = _top_idx >> _page_shift;
  dbTablePage* page    = _pages[page_id];

  // if page is still valid, find the next allocated object
  if (page->valid_page()) {
    uint offset = _top_idx & _page_mask;
    T*   b      = (T*) page->_objects;
    T*   s      = &b[offset - 1];

    for (; s >= b; s--) {
      if (s->_oid & DB_ALLOC_BIT) {
        offset   = s - b;
        _top_idx = (page_id << _page_shift) + offset;
        return;
      }
    }

    // if s < e, then something is corrupted...
    ZASSERT(ADS_DB_TABLE_TOP_SEARCH_FAILED);
  }

  for (--page_id;; --page_id) {
    assert(page_id >= 0);
    page = _pages[page_id];

    if (page->valid_page())
      break;
  }

  T* b = (T*) page->_objects;
  T* s = &b[_page_mask];

  for (; s >= b; s--) {
    if (s->_oid & DB_ALLOC_BIT) {
      uint offset = s - b;
      _top_idx    = (page_id << _page_shift) + offset;
      return;
    }
  }

  // if s < e, then something is corrupted...
  ZASSERT(ADS_DB_TABLE_TOP_SEARCH_FAILED);
}

template <class T>
void dbTable<T>::destroy(T* t)
{
  --_alloc_cnt;

  ZASSERT(t->getOID() != 0);
  ZASSERT(t->getTable() == this);
  ZASSERT(t->_oid & DB_ALLOC_BIT);

  dbTablePage* page = (dbTablePage*) t->getObjectPage();

  page->_alloccnt--;
  t->~T();  // call destructor
  t->_oid &= ~DB_ALLOC_BIT;

  uint offset = t - (T*) page->_objects;
  uint id     = page->_page_addr + offset;

  // Add to freelist
  _dbFreeObject* o = (_dbFreeObject*) t;
  pushQ(_free_list, o);

  if (id == _bottom_idx)
    findBottom();

  if (id == _top_idx)
    findTop();
}

template <class T>
bool dbTable<T>::reversible()
{
  return false;
}

template <class T>
bool dbTable<T>::orderReversed()
{
  return false;
}

template <class T>
void dbTable<T>::reverse(dbObject* /* unused: parent */)
{
}

template <class T>
uint dbTable<T>::sequential()
{
  return _top_idx;
}

template <class T>
uint dbTable<T>::size(dbObject* /* unused: parent */)
{
  return size();
}

template <class T>
uint dbTable<T>::begin(dbObject* /* unused: parent */)
{
  return _bottom_idx;
}

template <class T>
uint dbTable<T>::end(dbObject* /* unused: parent */)
{
  return 0;
}

template <class T>
uint dbTable<T>::next(uint id, ...)
{
  ZASSERT(id != 0);
  ++id;

  if (id > _top_idx)
    return 0;

  uint         page_id = id >> _page_shift;
  dbTablePage* page    = _pages[page_id];
  uint         offset  = id & _page_mask;

next_obj:
  T* p = (T*) &(page->_objects[offset * sizeof(T)]);
  T* e = (T*) &(page->_objects[page_size() * sizeof(T)]);

  for (; p < e; ++p) {
    if (p->_oid & DB_ALLOC_BIT) {
      offset = p - (T*) page->_objects;
      uint n = (page_id << _page_shift) + offset;
      ZASSERT(n <= _top_idx);
      return n;
    }
  }

  // search for next non empty page
  for (++page_id; page_id < _page_cnt; ++page_id) {
    page = _pages[page_id];

    if (page->valid_page()) {
      offset = 0;
      goto next_obj;
    }
  }

  return 0;
}

template <class T>
dbObject* dbTable<T>::getObject(uint id, ...)
{
  return getPtr(id);
}

template <class T>
void dbTable<T>::writePage(dbOStream& stream, const dbTablePage* page) const
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
void dbTable<T>::readPage(dbIStream& stream, dbTablePage* page)
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
      uint oid = uint((char*) t - page->_objects) | DB_ALLOC_BIT;
      t->_oid  = oid;  // Set the oid so the stream code can call the dbObject
                       // methods.
      page->_alloccnt++;
      stream >> *t;
      assert(
          t->_oid
          == oid);  // Check that the streaming code did not overwrite the oid
    }
  }
}

template <class T>
void dbTable<T>::copy_pages(const dbTable<T>& t)
{
  _pages = new dbTablePage*[_page_tbl_size];
  ZALLOCATED(_pages);

  uint i;

  for (i = 0; i < _page_tbl_size; ++i)
    _pages[i] = NULL;

  for (i = 0; i < _page_cnt; ++i) {
    dbTablePage* page = t._pages[i];
    copy_page(i, page);
  }
}

template <class T>
void dbTable<T>::copy_page(uint page_id, dbTablePage* page)
{
  uint         size = page_size() * sizeof(T) + sizeof(dbObjectPage);
  dbTablePage* p    = (dbTablePage*) malloc(size);
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
dbOStream& operator<<(dbOStream& stream, const dbTable<T>& table)
{
#ifdef ADS_DB_CHECK_STREAM
  stream.markStream();
#endif
  stream << table._page_mask;
  stream << table._page_shift;
  stream << table._top_idx;
  stream << table._bottom_idx;
  stream << table._page_cnt;
  stream << table._page_tbl_size;
  stream << table._alloc_cnt;
  stream << table._free_list;

  uint i;
  for (i = 0; i < table._page_cnt; ++i) {
    const dbTablePage* page = table._pages[i];
    table.writePage(stream, page);
  }

  stream << table._prop_list;

#ifdef ADS_DB_CHECK_STREAM
  stream.markStream();
#endif

  return stream;
}

template <class T>
dbIStream& operator>>(dbIStream& stream, dbTable<T>& table)
{
#ifdef ADS_DB_CHECK_STREAM
  stream.checkStream();
#endif
  table.clear();
  stream >> table._page_mask;
  stream >> table._page_shift;
  stream >> table._top_idx;
  stream >> table._bottom_idx;
  stream >> table._page_cnt;
  stream >> table._page_tbl_size;
  stream >> table._alloc_cnt;
  stream >> table._free_list;

  if (table._page_tbl_size == 0)
    table._pages = NULL;
  else {
    table._pages = new dbTablePage*[table._page_tbl_size];
    ZALLOCATED(table._pages);
  }

  uint i;
  for (i = 0; i < table._page_cnt; ++i) {
    uint         size = table.page_size() * sizeof(T) + sizeof(dbObjectPage);
    dbTablePage* page = (dbTablePage*) malloc(size);
    ZALLOCATED(page);
    memset(page, 0, size);
    page->_page_addr = i << table._page_shift;
    page->_table     = &table;
    table._pages[i]  = page;
    table.readPage(stream, page);
  }

  for (; i < table._page_tbl_size; ++i)
    table._pages[i] = NULL;

  stream >> table._prop_list;

#ifdef ADS_DB_CHECK_STREAM
  stream.checkStream();
#endif
  return stream;
}

template <class T>
bool dbTable<T>::operator!=(const dbTable<T>& table) const
{
  return !operator==(table);
}

template <class T>
bool dbTable<T>::operator==(const dbTable<T>& rhs) const
{
  const dbTable<T>& lhs = *this;

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
  if (lhs._bottom_idx != rhs._bottom_idx)
    return false;

  // Simple rejection test
  if (lhs._top_idx != rhs._top_idx)
    return false;

  // Simple rejection test
  if (lhs._alloc_cnt != rhs._alloc_cnt)
    return false;

  uint i;

  for (i = _bottom_idx; i <= _top_idx; ++i) {
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
void dbTable<T>::differences(dbDiff& diff, const dbTable<T>& rhs) const
{
  const dbTable<T>& lhs = *this;

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

  if (i < lhs_max) {
    for (; i < lhs_max; ++i) {
      bool lhs_valid_o = lhs.validId(i);

      if (lhs_valid_o) {
        T* l = lhs.getPtr(i);
        l->out(diff, dbDiff::LEFT, NULL);
      } else {
        diff.report("< %s [%u] FREE\n", name, i);
      }
    }
  } else if (i < rhs_max) {
    for (; i < rhs_max; ++i) {
      bool rhs_valid_o = rhs.validId(i);

      if (rhs_valid_o) {
        T* r = rhs.getPtr(i);
        r->out(diff, dbDiff::RIGHT, NULL);
      } else {
        diff.report("> %s [%u] FREE\n", name, i);
      }
    }
  }
}

template <class T>
void dbTable<T>::out(dbDiff& diff, char side) const
{
  uint i;

  for (i = _bottom_idx; i <= _top_idx; ++i) {
    if (validId(i)) {
      T* o = getPtr(i);
      o->out(diff, side, NULL);
    }
  }
}

template <class T>
void dbTable<T>::getObjects(std::vector<T*>& objects)
{
  objects.clear();
  objects.reserve(size());

  uint i;

  for (i = _bottom_idx; i <= _top_idx; ++i) {
    if (validId(i))
      objects.push_back(getPtr(i));
  }
}

}  // namespace odb
