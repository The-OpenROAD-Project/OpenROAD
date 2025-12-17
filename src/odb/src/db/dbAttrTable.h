// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

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
  unsigned int page_cnt_ = 0;
  T** pages_ = nullptr;

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

  if (page >= page_cnt_) {  // Page not present...
    return T();
  }

  if (pages_[page] == nullptr) {  // Page not present
    return T();
  }

  const unsigned int offset = id & (page_size - 1);
  return pages_[page][offset];
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
  if (pages_) {
    for (int i = 0; i < page_cnt_; ++i) {
      delete[] pages_[i];
    }

    delete[] pages_;
  }

  pages_ = nullptr;
  page_cnt_ = 0;
}

template <typename T>
inline T* dbAttrTable<T>::getPage(const uint page)
{
  if (page >= page_cnt_) {
    resizePageTable(page);
  }

  if (pages_[page] == nullptr) {
    pages_[page] = new T[page_size];

    for (int i = 0; i < page_size; ++i) {
      pages_[page][i] = 0U;
    }
  }

  return pages_[page];
}

template <typename T>
inline void dbAttrTable<T>::resizePageTable(const uint page)
{
  T** old_pages = pages_;
  unsigned int old_page_cnt = page_cnt_;

  if (page_cnt_ == 0) {
    page_cnt_ = 1;
  }

  while (page >= page_cnt_) {
    page_cnt_ *= 2;
  }

  pages_ = new T*[page_cnt_];

  unsigned int i;

  for (i = 0; i < old_page_cnt; ++i) {
    pages_[i] = old_pages[i];
  }

  for (; i < page_cnt_; ++i) {
    pages_[i] = nullptr;
  }

  delete[] old_pages;
}

template <typename T>
inline bool dbAttrTable<T>::operator==(const dbAttrTable<T>& rhs) const
{
  if (page_cnt_ != rhs.page_cnt_) {
    return false;
  }

  const uint n = page_cnt_ * page_size;

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
  stream << t.page_cnt_;

  for (int i = 0; i < t.page_cnt_; ++i) {
    if (t.pages_[i] == nullptr) {
      stream << 0U;
    } else {
      stream << (i + 1);

      uint j;

      for (j = 0; j < dbAttrTable<T>::page_size; ++j) {
        stream << t.pages_[i][j];
      }
    }
  }

  return stream;
}

template <typename T>
inline dbIStream& operator>>(dbIStream& stream, dbAttrTable<T>& t)
{
  t.clear();

  stream >> t.page_cnt_;

  if (t.page_cnt_ == 0) {
    return stream;
  }

  t.pages_ = new T*[t.page_cnt_];

  for (int i = 0; i < t.page_cnt_; ++i) {
    uint p;
    stream >> p;

    if (p == 0U) {
      t.pages_[i] = nullptr;
    } else {
      t.pages_[i] = new T[dbAttrTable<T>::page_size];
      uint j;

      for (j = 0; j < dbAttrTable<T>::page_size; j++) {
        stream >> t.pages_[i][j];
      }
    }
  }

  return stream;
}

}  // namespace odb
