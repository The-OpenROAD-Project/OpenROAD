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

#include <cstring>
#include <new>
#include <vector>

#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/ZException.h"
#include "odb/dbStream.h"

namespace odb {

//
// Get the object of this id
// This method is the same as getPtr() but is is
// use to get objects on the free-list.
//
template <class T>
inline _dbFreeObject* dbTable<T>::getFreeObj(dbId<T> id)
{
  const uint page = (uint) id >> _page_shift;
  const uint offset = (uint) id & _page_mask;

  assert(((uint) id != 0) && (page < _page_cnt));
  T* p = (T*) &(_pages[page]->_objects[offset * sizeof(T)]);
  assert((p->_oid & DB_ALLOC_BIT) == 0);
  return (_dbFreeObject*) p;
}

template <class T>
inline T* dbTable<T>::getPtr(dbId<T> id) const
{
  const uint page = (uint) id >> _page_shift;
  const uint offset = (uint) id & _page_mask;

  assert(((uint) id != 0) && (page < _page_cnt));
  T* p = (T*) &(_pages[page]->_objects[offset * sizeof(T)]);
  assert(p->_oid & DB_ALLOC_BIT);
  return p;
}

template <class T>
inline bool dbTable<T>::validId(dbId<T> id) const
{
  const uint page = (uint) id >> _page_shift;
  const uint offset = (uint) id & _page_mask;

  if (((uint) id != 0) && (page < _page_cnt)) {
    T* p = (T*) &(_pages[page]->_objects[offset * sizeof(T)]);
    return (p->_oid & DB_ALLOC_BIT) == DB_ALLOC_BIT;
  }

  return false;
}

template <class T>
inline void dbTable<T>::pushQ(uint& Q, _dbFreeObject* e)
{
  e->_prev = 0;
  e->_next = Q;
  const uint head_id = e->getImpl()->getOID();
  if (Q != 0) {
    getFreeObj(Q)->_prev = head_id;
  }
  Q = head_id;
}

template <class T>
inline _dbFreeObject* dbTable<T>::popQ(uint& Q)
{
  _dbFreeObject* e = getFreeObj(Q);
  Q = e->_next;

  if (Q) {
    _dbFreeObject* head = getFreeObj(Q);
    head->_prev = 0;
  }

  return e;
}

template <class T>
void dbTable<T>::clear()
{
  for (uint i = 0; i < _page_cnt; ++i) {
    dbTablePage* page = _pages[i];
    const T* t = (T*) page->_objects;
    const T* e = &t[pageSize()];

    for (; t < e; t++) {
      if (t->_oid & DB_ALLOC_BIT) {
        t->~T();
      }
    }

    free(page);
  }

  delete[] _pages;

  _bottom_idx = 0;
  _top_idx = 0;
  _page_cnt = 0;
  _page_tbl_size = 0;
  _alloc_cnt = 0;
  _free_list = 0;
  _pages = nullptr;
}

template <class T>
dbTable<T>::dbTable(_dbDatabase* db,
                    dbObject* owner,
                    dbObjectTable* (dbObject::*m)(dbObjectType),
                    const dbObjectType type,
                    const uint page_size,
                    const uint page_shift)
    : dbObjectTable(db, owner, m, type, sizeof(T))
{
  _page_mask = page_size - 1;
  _page_shift = page_shift;
  _bottom_idx = 0;
  _top_idx = 0;
  _page_cnt = 0;
  _page_tbl_size = 0;
  _alloc_cnt = 0;
  _free_list = 0;
  _pages = nullptr;
}

template <class T>
dbTable<T>::~dbTable()
{
  clear();
}

template <class T>
void dbTable<T>::resizePageTbl()
{
  dbTablePage** old_tbl = _pages;
  const uint old_tbl_size = _page_tbl_size;
  _page_tbl_size *= 2;

  _pages = new dbTablePage*[_page_tbl_size];

  uint i;
  for (i = 0; i < old_tbl_size; ++i) {
    _pages[i] = old_tbl[i];
  }

  for (; i < _page_tbl_size; ++i) {
    _pages[i] = nullptr;
  }

  delete[] old_tbl;
}

template <class T>
void dbTable<T>::newPage()
{
  const uint size = pageSize() * sizeof(T) + sizeof(dbObjectPage);
  dbTablePage* page = (dbTablePage*) malloc(size);
  ZALLOCATED(page);
  memset(page, 0, size);

  const uint page_id = _page_cnt;

  if (_page_tbl_size == 0) {
    _pages = new dbTablePage*[1];
    _page_tbl_size = 1;
  } else if (_page_tbl_size == _page_cnt) {
    resizePageTbl();
  }

  ++_page_cnt;
  page->_table = this;
  page->_page_addr = page_id << _page_shift;
  page->_alloccnt = 0;
  _pages[page_id] = page;

  // The objects are put on the list in reverse order, so they can be removed
  // in low-to-high order.
  if (page_id == 0) {
    T* b = (T*) page->_objects;
    T* t = &b[_page_mask];

    for (; t >= b; --t) {
      _dbFreeObject* o = (_dbFreeObject*) t;
      o->_oid = (uint) ((char*) t - (char*) b);

      if (t != b) {  // don't link zero-object
        pushQ(_free_list, o);
      }
    }
  } else {
    T* b = (T*) page->_objects;
    T* t = &b[_page_mask];

    for (; t >= b; --t) {
      _dbFreeObject* o = (_dbFreeObject*) t;
      o->_oid = (uint) ((char*) t - (char*) b);
      pushQ(_free_list, o);
    }
  }
}

template <class T>
T* dbTable<T>::create()
{
  ++_alloc_cnt;

  if (_free_list == 0) {
    newPage();
  }

  _dbFreeObject* o = popQ(_free_list);
  const uint oid = o->_oid;
  new (o) T(_db);
  T* t = (T*) o;
  t->_oid = oid | DB_ALLOC_BIT;

  dbTablePage* page = (dbTablePage*) t->getObjectPage();
  page->_alloccnt++;

  const uint id = t->getOID();

  if (id > _top_idx) {
    _top_idx = id;
  }

  if ((_bottom_idx == 0) || (id < _bottom_idx)) {
    _bottom_idx = id;
  }

  return t;
}

#define ADS_DB_TABLE_BOTTOM_SEARCH_FAILED 0
#define ADS_DB_TABLE_TOP_SEARCH_FAILED 0

// find the new bottom_idx...
template <class T>
inline void dbTable<T>::findBottom()
{
  if (_alloc_cnt == 0) {
    _bottom_idx = 0;
    return;
  }

  uint page_id = _bottom_idx >> _page_shift;
  dbTablePage* page = _pages[page_id];

  // if page is still valid, find the next allocated object
  if (page->valid_page()) {
    uint offset = _bottom_idx & _page_mask;
    T* b = (T*) page->_objects;
    T* s = &b[offset + 1];
    T* e = &b[pageSize()];
    for (; s < e; s++) {
      if (s->_oid & DB_ALLOC_BIT) {
        offset = s - b;
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

    if (page->valid_page()) {
      break;
    }
  }

  T* b = (T*) page->_objects;
  T* s = b;
  T* e = &s[pageSize()];

  for (; s < e; s++) {
    if (s->_oid & DB_ALLOC_BIT) {
      const uint offset = s - b;
      _bottom_idx = (page_id << _page_shift) + offset;
      return;
    }
  }

  // if s == e, then something is corrupted...
  ZASSERT(ADS_DB_TABLE_BOTTOM_SEARCH_FAILED);
}

// find the new top_idx...
template <class T>
inline void dbTable<T>::findTop()
{
  if (_alloc_cnt == 0) {
    _top_idx = 0;
    return;
  }

  uint page_id = _top_idx >> _page_shift;
  dbTablePage* page = _pages[page_id];

  // if page is still valid, find the next allocated object
  if (page->valid_page()) {
    uint offset = _top_idx & _page_mask;
    T* b = (T*) page->_objects;
    T* s = &b[offset - 1];

    for (; s >= b; s--) {
      if (s->_oid & DB_ALLOC_BIT) {
        offset = s - b;
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

    if (page->valid_page()) {
      break;
    }
  }

  T* b = (T*) page->_objects;
  T* s = &b[_page_mask];

  for (; s >= b; s--) {
    if (s->_oid & DB_ALLOC_BIT) {
      const uint offset = s - b;
      _top_idx = (page_id << _page_shift) + offset;
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
  _dbFreeObject* o = (_dbFreeObject*) t;

  page->_alloccnt--;
  const uint oid = t->_oid;
  t->~T();  // call destructor
  o->_oid = oid & ~DB_ALLOC_BIT;

  const uint offset = t - (T*) page->_objects;
  const uint id = page->_page_addr + offset;

  // Add to freelist
  pushQ(_free_list, o);

  if (id == _bottom_idx) {
    findBottom();
  }

  if (id == _top_idx) {
    findTop();
  }
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

  if (id > _top_idx) {
    return 0;
  }

  uint page_id = id >> _page_shift;
  dbTablePage* page = _pages[page_id];
  uint offset = id & _page_mask;

next_obj:
  T* p = (T*) &(page->_objects[offset * sizeof(T)]);
  T* e = (T*) &(page->_objects[pageSize() * sizeof(T)]);

  for (; p < e; ++p) {
    if (p->_oid & DB_ALLOC_BIT) {
      offset = p - (T*) page->_objects;
      const uint n = (page_id << _page_shift) + offset;
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
  const T* e = &t[pageSize()];

  for (; t < e; t++) {
    if (t->_oid & DB_ALLOC_BIT) {
      const char allocated = 1;
      stream << allocated;
      stream << *t;
    } else {
      const char allocated = 0;
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
  T* t = (T*) page->_objects;
  T* e = &t[pageSize()];
  page->_alloccnt = 0;

  for (; t < e; t++) {
    char allocated;
    stream >> allocated;

    if (!allocated) {
      t->_oid = (uint) ((char*) t - page->_objects);
      _dbFreeObject* o = (_dbFreeObject*) t;
      stream >> o->_next;
      stream >> o->_prev;
    } else {
      new (t) T(_db);
      uint oid = uint((char*) t - page->_objects) | DB_ALLOC_BIT;
      t->_oid = oid;  // Set the oid so the stream code can call the dbObject
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
dbOStream& operator<<(dbOStream& stream, const NamedTable<T>& named_table)
{
  dbOStreamScope scope(
      stream,
      fmt::format("{}({})", named_table.name, named_table.table->size()));
  stream << *named_table.table;
  return stream;
}

template <class T>
dbOStream& operator<<(dbOStream& stream, const dbTable<T>& table)
{
  stream << table._page_mask;
  stream << table._page_shift;
  stream << table._top_idx;
  stream << table._bottom_idx;
  stream << table._page_cnt;
  stream << table._page_tbl_size;
  stream << table._alloc_cnt;
  stream << table._free_list;

  for (uint i = 0; i < table._page_cnt; ++i) {
    const dbTablePage* page = table._pages[i];
    table.writePage(stream, page);
  }

  stream << table._prop_list;

  return stream;
}

template <class T>
dbIStream& operator>>(dbIStream& stream, dbTable<T>& table)
{
  table.clear();
  stream >> table._page_mask;
  stream >> table._page_shift;
  stream >> table._top_idx;
  stream >> table._bottom_idx;
  stream >> table._page_cnt;
  stream >> table._page_tbl_size;
  stream >> table._alloc_cnt;
  stream >> table._free_list;

  if (table._page_tbl_size == 0) {
    table._pages = nullptr;
  } else {
    table._pages = new dbTablePage*[table._page_tbl_size];
  }

  uint i;
  for (i = 0; i < table._page_cnt; ++i) {
    uint size = table.pageSize() * sizeof(T) + sizeof(dbObjectPage);
    dbTablePage* page = (dbTablePage*) malloc(size);
    ZALLOCATED(page);
    memset(page, 0, size);
    page->_page_addr = i << table._page_shift;
    page->_table = &table;
    table._pages[i] = page;
    table.readPage(stream, page);
  }

  for (; i < table._page_tbl_size; ++i) {
    table._pages[i] = nullptr;
  }

  stream >> table._prop_list;

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
  if ((lhs._page_cnt == 0) && (rhs._page_cnt == 0)) {
    return true;
  }

  // Simple rejection test
  if (lhs._page_cnt != rhs._page_cnt) {
    return false;
  }

  // Simple rejection test
  if (lhs._bottom_idx != rhs._bottom_idx) {
    return false;
  }

  // Simple rejection test
  if (lhs._top_idx != rhs._top_idx) {
    return false;
  }

  // Simple rejection test
  if (lhs._alloc_cnt != rhs._alloc_cnt) {
    return false;
  }

  for (uint i = _bottom_idx; i <= _top_idx; ++i) {
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
void dbTable<T>::collectMemInfo(MemInfo& info)
{
  for (int i = _bottom_idx; i <= _top_idx; ++i) {
    if (validId(i)) {
      getPtr(i)->collectMemInfo(info);
    }
  }
}

}  // namespace odb
