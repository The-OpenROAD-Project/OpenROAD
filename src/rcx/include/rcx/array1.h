// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>

namespace rcx {

template <class T>
class Array1D
{
 public:
  explicit Array1D(int chunk = 0)
  {
    chunk_ = chunk;
    if (chunk_ <= 0) {
      chunk_ = 1024;
    }

    current_ = 0;
    if (chunk > 0) {
      size_ = chunk_;
      array_ = (T*) realloc(nullptr, size_ * sizeof(T));
    } else {
      size_ = 0;
      array_ = nullptr;
    }
    iter_cnt_ = 0;
  }
  ~Array1D()
  {
    if (array_ != nullptr) {
      ::free(array_);
    }
  }
  int add(T t)
  {
    if (current_ >= size_) {
      size_ += chunk_;
      array_ = (T*) realloc(array_, size_ * sizeof(T));
    }
    int n = current_;
    array_[current_++] = t;

    return n;
  }
  int resize(int maxSize)
  {
    if (maxSize < size_) {
      return size_;
    }

    size_ = (maxSize / chunk_ + 1) * chunk_;
    array_ = (T*) realloc(array_, size_ * sizeof(T));

    if (array_ == nullptr) {
      fprintf(stderr, "Cannot allocate array of size %d\n", size_);
      assert(0);
    }
    return size_;
  }
  T& get(const int i)
  {
    assert((i >= 0) && (i < current_));

    return array_[i];
  }
  const T& get(const int i) const
  {
    assert((i >= 0) && (i < current_));

    return array_[i];
  }
  T& geti(int i)
  {
    if (i >= size_) {
      resize(i + 1);
    }

    return array_[i];
  }
  T& getLast()
  {
    assert(current_ - 1 >= 0);

    return array_[current_ - 1];
  }
  void clear(T t)
  {
    for (int ii = 0; ii < size_; ii++) {
      array_[ii] = t;
    }
  }
  int findIndex(T t)
  {
    for (int ii = 0; ii < current_; ii++) {
      if (array_[ii] == t) {
        return ii;
      }
    }

    return -1;
  }
  int findNextBiggestIndex(T t, int start = 0)
  {
    for (int ii = start; ii < current_; ii++) {
      if (t == array_[ii]) {
        return ii;
      }
      if (t < array_[ii]) {
        return ii > 0 ? ii - 1 : 0;
      }
    }
    return current_;
  }

  bool notEmpty() { return current_ > 0; }
  bool getNext(T& a)
  {
    if (iter_cnt_ < current_) {
      a = array_[iter_cnt_++];
      return true;
    }
    return false;
  }
  T& pop()
  {
    assert(current_ > 0);

    current_--;

    return array_[current_];
  }
  int getSize() const { return size_; }
  void resetIterator(int v = 0) { iter_cnt_ = v; }
  void resetCnt(int v = 0)
  {
    current_ = v;
    iter_cnt_ = v;
  }
  int getCnt() const { return current_; }
  void set(int ii, T t)
  {
    if (ii >= size_) {
      resize(ii + 1);
    }

    array_[ii] = t;
  }

 private:
  T* array_;
  int size_;
  int chunk_;
  int current_;
  int iter_cnt_;
};

}  // namespace rcx
