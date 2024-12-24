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
#include <cstring>
#include <string>
#include <vector>

#include "array1.h"

namespace utl {
class Logger;
}

namespace odb {
class dbBlock;
class dbBox;
class dbNet;

// A class that implements an array that can grow efficiently
template <class T>
class AthArray
{
 public:
  AthArray(int alloc_size = 128)
  {
    if (alloc_size < 2) {
      alloc_size = 2;
    }
    m_alloc_size_ = alloc_size;
    m_num_mallocated_first_level_ = alloc_size;
    m_ptr_ = (T**) malloc(sizeof(T*) * m_num_mallocated_first_level_);
    for (int i = 0; i < m_num_mallocated_first_level_; i++) {
      m_ptr_[i] = nullptr;
    }
    m_num_allocated_elem_ = 0;
    m_num_mallocated_elem_ = 0;
  }

  ~AthArray()
  {
    for (int i = 0; i < m_num_mallocated_first_level_; i++) {
      if (m_ptr_[i]) {
        free(m_ptr_[i]);
      }
    }
    free(m_ptr_);
  }

  void add()
  {
    int first_level_idx, second_level_idx;
    allocNext(&first_level_idx, &second_level_idx);
    m_num_allocated_elem_++;
  }

  int add(T elem)
  {
    int first_level_idx, second_level_idx;
    allocNext(&first_level_idx, &second_level_idx);

    m_ptr_[first_level_idx][second_level_idx] = elem;
    int n = m_num_allocated_elem_++;
    return n;
  }

  T& operator[](const int idx)
  {
    assert(idx < m_num_allocated_elem_);
    int first_level_idx, second_level_idx;
    find_indexes(idx, first_level_idx, second_level_idx);
    return m_ptr_[first_level_idx][second_level_idx];
  }

  T get(const int idx)
  {
    assert(idx < m_num_allocated_elem_);
    int first_level_idx, second_level_idx;
    find_indexes(idx, first_level_idx, second_level_idx);
    return m_ptr_[first_level_idx][second_level_idx];
  }
  int getLast() { return m_num_allocated_elem_; }

 private:
  void allocNext(int* ii, int* jj)
  {
    int first_level_idx, second_level_idx;
    find_indexes(m_num_allocated_elem_, first_level_idx, second_level_idx);
    // reallocate first level if it is too small
    if (m_num_mallocated_first_level_ * m_alloc_size_
        <= m_num_allocated_elem_) {
      int orig_first_level_size = m_num_mallocated_first_level_;
      m_num_mallocated_first_level_++;
      m_ptr_
          = (T**) realloc(m_ptr_, sizeof(T*) * m_num_mallocated_first_level_);
      assert(m_ptr_);
      for (int i = orig_first_level_size; i < m_num_mallocated_first_level_;
           i++) {
        m_ptr_[i] = nullptr;
      }
    }
    // Allocate more elements if needed
    if (m_ptr_[first_level_idx] == nullptr) {
      size_t size = sizeof(T);
      m_ptr_[first_level_idx] = (T*) malloc(size * m_alloc_size_);
      m_num_mallocated_elem_ = m_num_mallocated_first_level_ * m_alloc_size_;
    }
    *ii = first_level_idx;
    *jj = second_level_idx;
  }

  void find_indexes(int num, int& first_level, int& second_level)
  {
    first_level = num / m_alloc_size_;
    second_level = num % m_alloc_size_;
  }

  int m_alloc_size_;
  T** m_ptr_;
  int m_num_allocated_elem_;
  int m_num_mallocated_elem_;
  int m_num_mallocated_first_level_;
};

// A simple pool allocation function
//
// Note: T must be default-constructible, as `new` is used to construct T when
// memory debugging is enabled.
template <class T>
class AthPool
{
 public:
  AthPool(const int freeAllocSize)
  {
    m_heap_ = new AthArray<T>(4096);
    free_table_ = new Ath__array1D<T*>(freeAllocSize);
  }

  ~AthPool()
  {
    delete m_heap_;
    delete free_table_;
  }

  T* alloc(int* freeTableFlag = nullptr, int* id = nullptr)
  {
    T* a = nullptr;

    if (free_table_->notEmpty()) {
      a = free_table_->pop();

      if (freeTableFlag != nullptr) {
        *freeTableFlag = 1;
      }
    } else {
      m_heap_->add();
      int n = m_heap_->getLast() - 1;
      a = &(*m_heap_)[n];

      if (id != nullptr) {
        *id = n;
      }

      if (freeTableFlag != nullptr) {
        *freeTableFlag = 0;
      }
    }
    return a;
  }

  void free(T* a)
  {
    assert(a);  // should not free nullptr
    if (free_table_ != nullptr) {
      free_table_->add(a);
    }
  }

  T* get(const int ii)
  {
    T* a = &(*m_heap_)[ii];
    return a;
  }

  int getCnt() { return m_heap_->getLast(); }

 private:
  AthArray<T>* m_heap_;
  Ath__array1D<T*>* free_table_;
};

int makeSiteLoc(int x, double site_width, bool at_left_from_macro, int offset);

void cutRows(dbBlock* block,
             int min_row_width,
             const std::vector<dbBox*>& blockages,
             int halo_x,
             int halo_y,
             utl::Logger* logger);

// Generates a string with the macro placement in mpl2 input format for
// individual macro placement
std::string generateMacroPlacementString(dbBlock* block);

class WireLengthEvaluator
{
 public:
  WireLengthEvaluator(dbBlock* block) : block_(block) {}
  int64_t hpwl() const;

 private:
  int64_t hpwl(dbNet* net) const;

  dbBlock* block_;
};

}  // namespace odb
