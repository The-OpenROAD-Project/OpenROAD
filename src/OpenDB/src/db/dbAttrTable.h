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

#include "ZException.h"
#include "dbDiff.h"
#include "dbStream.h"
#include "odb.h"

namespace odb {

class dbDiff;

//
// dbAttrTable - Stores the property list for each table.
//
template <typename T>
class dbAttrTable
{
 public:
  static const uint PAGE_SIZE;
  static const uint PAGE_SHIFT;
  unsigned int      _page_cnt;
  T**               _pages;

  dbAttrTable()
  {
    _pages    = NULL;
    _page_cnt = 0;
  }

  dbAttrTable(const dbAttrTable<T>& V)
  {
    _pages    = NULL;
    _page_cnt = 0;
  }

  ~dbAttrTable() { clear(); }

  T getAttr(uint id) const
  {
    // Pages are not created util the prop-list is being set.
    // This approach allows objects to test for properties without populating
    // pages. (which would populate the table).
    unsigned int page = (id & ~(PAGE_SIZE - 1)) >> PAGE_SHIFT;

    if (page >= _page_cnt)  // Page not present...
      return T();

    if (_pages[page] == NULL)  // Page not present
      return T();

    unsigned int offset = id & (PAGE_SIZE - 1);
    return _pages[page][offset];
  }

  void setAttr(uint id, T attr)
  {
    unsigned int page   = (id & ~(PAGE_SIZE - 1)) >> PAGE_SHIFT;
    T*           pg     = getPage(page);
    unsigned int offset = id & (PAGE_SIZE - 1);
    pg[offset]          = attr;
  }

  void clear()
  {
    if (_pages) {
      unsigned int i;

      for (i = 0; i < _page_cnt; ++i)
        delete[] _pages[i];

      delete[] _pages;
    }

    _pages    = NULL;
    _page_cnt = 0;
  }

  bool operator==(const dbAttrTable<T>& rhs) const;
  bool operator!=(const dbAttrTable<T>& rhs) const { return !operator==(rhs); }
  void differences(dbDiff&               diff,
                   const char*           field,
                   const dbAttrTable<T>& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  T* getPage(uint page)
  {
    if (page >= _page_cnt)
      resizePageTable(page);

    if (_pages[page] == NULL) {
      _pages[page] = new T[PAGE_SIZE];
      ZALLOCATED(_pages[page]);

      uint i;

      for (i = 0; i < PAGE_SIZE; ++i)
        _pages[page][i] = 0U;
    }

    return _pages[page];
  }

  void resizePageTable(uint page)
  {
    T**          old_pages    = _pages;
    unsigned int old_page_cnt = _page_cnt;

    if (_page_cnt == 0)
      _page_cnt = 1;

    while (page >= _page_cnt)
      _page_cnt *= 2;

    _pages = new T*[_page_cnt];
    ZALLOCATED(_pages);

    unsigned int i;

    for (i = 0; i < old_page_cnt; ++i)
      _pages[i] = old_pages[i];

    for (; i < _page_cnt; ++i)
      _pages[i] = NULL;

    delete[] old_pages;
  }
};

template <typename T>
const uint dbAttrTable<T>::PAGE_SIZE = 32;
template <typename T>
const uint dbAttrTable<T>::PAGE_SHIFT = 5;

template <typename T>
inline bool dbAttrTable<T>::operator==(const dbAttrTable<T>& rhs) const
{
  if (_page_cnt != rhs._page_cnt)
    return false;

  uint i;
  uint n = _page_cnt * PAGE_SIZE;

  for (i = 0; i < n; ++i)
    if (getAttr(i) != rhs.getAttr(i))
      return false;

  return true;
}

template <typename T>
inline void dbAttrTable<T>::differences(dbDiff&               diff,
                                        const char*           field,
                                        const dbAttrTable<T>& rhs) const
{
  uint sz1 = _page_cnt * PAGE_SIZE;
  uint sz2 = rhs._page_cnt * PAGE_SIZE;
  uint i   = 0;

  for (; i < sz1 && i < sz2; ++i) {
    T o1 = getAttr(i);
    T o2 = rhs.getAttr(i);

    if (o1 != o2) {
      diff.report("< %s[%d] = ", field, i);
      diff << o1;
      diff << "\n";
      diff.report("> %s[%d] = ", field, i);
      diff << o2;
      diff << "\n";
    }
  }

  if (i < sz1) {
    for (; i < sz1; ++i) {
      T o1 = getAttr(i);
      diff.report("< %s[%d] = ", field, i);
      diff << o1;
      diff << "\n";
    }
  }

  if (i < sz2) {
    for (; i < sz2; ++i) {
      T o2 = rhs.getAttr(i);
      diff.report("> %s[%d] = ", field, i);
      diff << o2;
      diff << "\n";
    }
  }
}

template <typename T>
inline void dbAttrTable<T>::out(dbDiff&     diff,
                                char        side,
                                const char* field) const
{
  uint sz1 = _page_cnt * PAGE_SIZE;
  uint i   = 0;

  for (; i < sz1; ++i) {
    T o1 = getAttr(i);
    diff.report("%c %s[%d] = ", side, field, i);
    diff << o1;
    diff << "\n";
  }
}

template <typename T>
inline dbOStream& operator<<(dbOStream& stream, const dbAttrTable<T>& t)
{
  stream << t._page_cnt;

  uint i;
  for (i = 0; i < t._page_cnt; ++i) {
    if (t._pages[i] == NULL)
      stream << 0U;
    else {
      stream << (i + 1);

      uint j;

      for (j = 0; j < dbAttrTable<T>::PAGE_SIZE; ++j)
        stream << t._pages[i][j];
    }
  }

  return stream;
}

template <typename T>
inline dbIStream& operator>>(dbIStream& stream, dbAttrTable<T>& t)
{
  t.clear();

  stream >> t._page_cnt;

  if (t._page_cnt == 0)
    return stream;

  t._pages = new T*[t._page_cnt];
  ZALLOCATED(t._pages);

  uint i;

  for (i = 0; i < t._page_cnt; ++i) {
    uint p;
    stream >> p;

    if (p == 0U)
      t._pages[i] = NULL;
    else {
      t._pages[i] = new T[dbAttrTable<T>::PAGE_SIZE];
      ZALLOCATED(t._pages[i]);
      uint j;

      for (j = 0; j < dbAttrTable<T>::PAGE_SIZE; j++)
        stream >> t._pages[i][j];
    }
  }

  return stream;
}

}  // namespace odb
