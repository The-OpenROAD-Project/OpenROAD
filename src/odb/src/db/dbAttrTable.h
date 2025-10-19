// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/ZException.h"
#include "odb/dbStream.h"
#include "odb/odb.h"

namespace odb {

//
// dbAttrTable - Stores the property list for each table.
//
template <typename T>
class dbAttrTable
{
 public:
  dbAttrTable() = default;
  dbAttrTable(const dbAttrTable<T>& V) = delete;

  ~dbAttrTable() { clear(); }

  T getAttr(uint id) const;
  void setAttr(uint id, const T& attr);

  T* getPage(uint page);
  void resizePageTable(uint page);

  void clear();

  bool operator==(const dbAttrTable<T>& rhs) const;
  bool operator!=(const dbAttrTable<T>& rhs) const { return !(this == rhs); }

 private:
  unsigned int _page_cnt = 0;
  T** _pages = nullptr;

  static constexpr uint page_size = 32;
  static constexpr uint page_shift = 5;

  template <class U>
  friend dbOStream& operator<<(dbOStream& stream, const dbAttrTable<U>& t);

  template <class U>
  friend dbIStream& operator>>(dbIStream& stream, dbAttrTable<U>& t);
};

template <typename T>
inline T dbAttrTable<T>::getAttr(const uint id) const
{
  // Pages are not created util the prop-list is being set.
  // This approach allows objects to test for properties without populating
  // pages. (which would populate the table).
  const unsigned int page = (id & ~(page_size - 1)) >> page_shift;

  if (page >= _page_cnt) {  // Page not present...
    return T();
  }

  if (_pages[page] == nullptr) {  // Page not present
    return T();
  }

  const unsigned int offset = id & (page_size - 1);
  return _pages[page][offset];
}

template <typename T>
inline void dbAttrTable<T>::setAttr(const uint id, const T& attr)
{
  const unsigned int page = (id & ~(page_size - 1)) >> page_shift;
  T* pg = getPage(page);
  const unsigned int offset = id & (page_size - 1);
  pg[offset] = attr;
}

template <typename T>
inline void dbAttrTable<T>::clear()
{
  if (_pages) {
    for (int i = 0; i < _page_cnt; ++i) {
      delete[] _pages[i];
    }

    delete[] _pages;
  }

  _pages = nullptr;
  _page_cnt = 0;
}

template <typename T>
inline T* dbAttrTable<T>::getPage(const uint page)
{
  if (page >= _page_cnt) {
    resizePageTable(page);
  }

  if (_pages[page] == nullptr) {
    _pages[page] = new T[page_size];

    for (int i = 0; i < page_size; ++i) {
      _pages[page][i] = 0U;
    }
  }

  return _pages[page];
}

template <typename T>
inline void dbAttrTable<T>::resizePageTable(const uint page)
{
  T** old_pages = _pages;
  unsigned int old_page_cnt = _page_cnt;

  if (_page_cnt == 0) {
    _page_cnt = 1;
  }

  while (page >= _page_cnt) {
    _page_cnt *= 2;
  }

  _pages = new T*[_page_cnt];

  unsigned int i;

  for (i = 0; i < old_page_cnt; ++i) {
    _pages[i] = old_pages[i];
  }

  for (; i < _page_cnt; ++i) {
    _pages[i] = nullptr;
  }

  delete[] old_pages;
}

template <typename T>
inline bool dbAttrTable<T>::operator==(const dbAttrTable<T>& rhs) const
{
  if (_page_cnt != rhs._page_cnt) {
    return false;
  }

  const uint n = _page_cnt * page_size;

  for (int i = 0; i < n; ++i) {
    if (getAttr(i) != rhs.getAttr(i)) {
      return false;
    }
  }

  return true;
}

template <typename T>
inline dbOStream& operator<<(dbOStream& stream, const dbAttrTable<T>& t)
{
  stream << t._page_cnt;

  for (int i = 0; i < t._page_cnt; ++i) {
    if (t._pages[i] == nullptr) {
      stream << 0U;
    } else {
      stream << (i + 1);

      uint j;

      for (j = 0; j < dbAttrTable<T>::page_size; ++j) {
        stream << t._pages[i][j];
      }
    }
  }

  return stream;
}

template <typename T>
inline dbIStream& operator>>(dbIStream& stream, dbAttrTable<T>& t)
{
  t.clear();

  stream >> t._page_cnt;

  if (t._page_cnt == 0) {
    return stream;
  }

  t._pages = new T*[t._page_cnt];

  for (int i = 0; i < t._page_cnt; ++i) {
    uint p;
    stream >> p;

    if (p == 0U) {
      t._pages[i] = nullptr;
    } else {
      t._pages[i] = new T[dbAttrTable<T>::page_size];
      uint j;

      for (j = 0; j < dbAttrTable<T>::page_size; j++) {
        stream >> t._pages[i][j];
      }
    }
  }

  return stream;
}

}  // namespace odb
