// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstdint>

#include "odb/dbStream.h"

namespace odb {

template <class T, const uint32_t P, const uint32_t S>
class dbPagedVector;

//
// Vector - Creates a vector of type T. However, the vector is created
// as a series of pages of type T. The size of the page is specified
// by the template params.
//
// page_size = number of objects per page (MUST BE POWER OF TWO)
// page_shift = log2(page_size)
//
template <class T,
          const uint32_t page_size = 128,
          const uint32_t page_shift = 7>
class dbPagedVector
{
 public:
  dbPagedVector();
  dbPagedVector(const dbPagedVector<T, page_size, page_shift>& V);
  ~dbPagedVector();

  void push_back(const T& item);

  uint32_t push_back(int cnt, const T& item)
  {
    uint32_t id = next_idx_;
    uint32_t i;
    for (i = 0; i < cnt; ++i) {
      push_back(item);
    }
    return id;
  }

  unsigned int size() const { return next_idx_; }
  unsigned int getIdx(uint32_t chunkSize, const T& ival);
  void freeIdx(uint32_t idx);
  void clear();

  T& operator[](unsigned int id)
  {
    assert(id < next_idx_);
    unsigned int page = (id & ~(page_size - 1)) >> page_shift;
    unsigned int offset = id & (page_size - 1);
    return pages_[page][offset];
  }
  const T& operator[](unsigned int id) const
  {
    assert(id < next_idx_);
    unsigned int page = (id & ~(page_size - 1)) >> page_shift;
    unsigned int offset = id & (page_size - 1);
    return pages_[page][offset];
  }

  bool operator==(const dbPagedVector<T, page_size, page_shift>& rhs) const;
  bool operator!=(const dbPagedVector<T, page_size, page_shift>& rhs) const
  {
    return !operator==(rhs);
  }

 private:
  void resizePageTbl();
  void newPage();

  T** pages_;
  unsigned int page_cnt_;
  unsigned int page_tbl_size_;
  unsigned int next_idx_;

  int freed_idx_head_;
  int freed_idx_tail_;
};

template <class T, const uint32_t P, const uint32_t S>
unsigned int dbPagedVector<T, P, S>::getIdx(uint32_t chunkSize, const T& ival)
{
  uint32_t idx;
  if (freed_idx_head_ == -1) {
    idx = next_idx_;
    next_idx_ += chunkSize;
    unsigned int page = ((next_idx_ - 1) & ~(P - 1)) >> S;
    if (page == page_cnt_) {
      newPage();
    }
  } else {
    idx = (uint32_t) freed_idx_head_;
    if (idx == (uint32_t) freed_idx_tail_) {
      freed_idx_head_ = -1;
    } else {
      unsigned int page = (idx & ~(P - 1)) >> S;
      unsigned int offset = idx & (P - 1);
      uint32_t* fidxp = (uint32_t*) (&pages_[page][offset]);
      freed_idx_head_ = (int) *fidxp;
    }
  }
  for (uint32_t dd = 0; dd < chunkSize; dd++) {
    uint32_t id = idx + dd;
    assert(id < next_idx_);
    unsigned int page = (id & ~(P - 1)) >> S;
    unsigned int offset = id & (P - 1);
    pages_[page][offset] = ival;
  }
  return idx;
}

template <class T, const uint32_t P, const uint32_t S>
void dbPagedVector<T, P, S>::freeIdx(uint32_t idx)
{
  if (freed_idx_head_ == -1) {
    freed_idx_head_ = idx;
    freed_idx_tail_ = idx;
  } else {
    unsigned int page = (freed_idx_tail_ & ~(P - 1)) >> S;
    unsigned int offset = freed_idx_tail_ & (P - 1);
    uint32_t* fidxp = (uint32_t*) (&pages_[page][offset]);
    *fidxp = idx;
    freed_idx_tail_ = idx;
  }
}

template <class T, const uint32_t P, const uint32_t S>
dbPagedVector<T, P, S>::dbPagedVector()
{
  pages_ = nullptr;
  page_cnt_ = 0;
  page_tbl_size_ = 0;
  next_idx_ = 0;
  freed_idx_head_ = -1;
}

template <class T, const uint32_t P, const uint32_t S>
dbPagedVector<T, P, S>::dbPagedVector(const dbPagedVector<T, P, S>& V)
{
  pages_ = nullptr;
  page_cnt_ = 0;
  page_tbl_size_ = 0;
  next_idx_ = 0;
  freed_idx_head_ = -1;
  uint32_t sz = V.size();

  uint32_t i;
  for (i = 0; i < sz; ++i) {
    push_back(V[i]);
  }
}

template <class T, const uint32_t P, const uint32_t S>
void dbPagedVector<T, P, S>::clear()
{
  if (pages_) {
    unsigned int i;

    for (i = 0; i < page_cnt_; ++i) {
      delete[] pages_[i];
    }

    delete[] pages_;
  }

  pages_ = nullptr;
  page_cnt_ = 0;
  page_tbl_size_ = 0;
  next_idx_ = 0;
  freed_idx_head_ = -1;
}

template <class T, const uint32_t P, const uint32_t S>
dbPagedVector<T, P, S>::~dbPagedVector()
{
  clear();
}

template <class T, const uint32_t P, const uint32_t S>
void dbPagedVector<T, P, S>::resizePageTbl()
{
  T** old_tbl = pages_;
  unsigned int old_tbl_size = page_tbl_size_;

  if (page_tbl_size_ == 1) {
    ++page_tbl_size_;
  } else {
    page_tbl_size_ += (unsigned int) ((float) page_tbl_size_ * (0.5));
  }

  pages_ = new T*[page_tbl_size_];

  unsigned int i;

  for (i = 0; i < old_tbl_size; ++i) {
    pages_[i] = old_tbl[i];
  }

  for (; i < page_tbl_size_; ++i) {
    pages_[i] = nullptr;
  }

  delete[] old_tbl;
}

template <class T, const uint32_t P, const uint32_t S>
void dbPagedVector<T, P, S>::newPage()
{
  T* page = new T[P];

  if (page_tbl_size_ == 0) {
    pages_ = new T*[1];
    page_tbl_size_ = 1;
  } else if (page_tbl_size_ == page_cnt_) {
    resizePageTbl();
  }

  pages_[page_cnt_] = page;
  ++page_cnt_;
}

template <class T, const uint32_t P, const uint32_t S>
void dbPagedVector<T, P, S>::push_back(const T& item)
{
  unsigned int page = (next_idx_ & ~(P - 1)) >> S;

  if (page == page_cnt_) {
    newPage();
  }

  unsigned int offset = next_idx_ & (P - 1);
  ++next_idx_;

  T* objects = pages_[page];
  objects[offset] = item;
}

template <class T, const uint32_t P, const uint32_t S>
inline bool dbPagedVector<T, P, S>::operator==(
    const dbPagedVector<T, P, S>& rhs) const
{
  uint32_t sz = size();

  if (sz != rhs.size()) {
    return false;
  }

  uint32_t i;
  for (i = 0; i < sz; ++i) {
    const T& l = (*this)[i];
    const T& r = rhs[i];

    if (l != r) {
      return false;
    }
  }

  return true;
}

template <class T, const uint32_t P, const uint32_t S>
inline dbOStream& operator<<(dbOStream& stream, const dbPagedVector<T, P, S>& v)
{
  uint32_t sz = v.size();
  stream << sz;

  uint32_t i;
  for (i = 0; i < sz; ++i) {
    const T& t = v[i];
    stream << t;
  }

  return stream;
}

template <class T, const uint32_t P, const uint32_t S>
inline dbIStream& operator>>(dbIStream& stream, dbPagedVector<T, P, S>& v)
{
  v.clear();

  uint32_t sz;
  stream >> sz;
  T t;
  uint32_t i;

  for (i = 0; i < sz; ++i) {
    stream >> t;
    v.push_back(t);
  }

  return stream;
}

}  // namespace odb
