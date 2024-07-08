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

#include <cassert>
#include <cstdio>
#include <cstdlib>

namespace odb {

template <class T>
class Ath__array1D
{
 public:
  explicit Ath__array1D(int chunk = 0)
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
  ~Ath__array1D()
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
  T& get(int i)
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
  int getSize() { return size_; }
  void resetIterator(int v = 0) { iter_cnt_ = v; }
  void resetCnt(int v = 0)
  {
    current_ = v;
    iter_cnt_ = v;
  }
  int getCnt() { return current_; }
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

}  // namespace odb
