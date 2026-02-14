// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "rcx/array1.h"

namespace rcx {

// A class that implements an array that can grow efficiently
template <class T>
class AthArray
{
 public:
  AthArray(int alloc_size = 128)
  {
    alloc_size = std::max(alloc_size, 2);
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
    free_table_ = new Array1D<T*>(freeAllocSize);
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
  Array1D<T*>* free_table_;
};

// Simple list
template <class T>
class AthList
{
 private:
  struct t_elem
  {
    T m_data;
    t_elem* m_next;
  };
  t_elem* m_start;

 public:
  AthList() { m_start = nullptr; };
  ~AthList()
  {
    t_elem* i;
    i = m_start;
    while (i) {
      t_elem* next = i->m_next;
      delete i;
      i = next;
    }
  };
  void push(T a)
  {
    t_elem* new_t_elem = new t_elem;
    new_t_elem->m_next = m_start;
    new_t_elem->m_data = a;
    m_start = new_t_elem;
  };

  struct iterator
  {
    AthList<T>* actual_class;
    t_elem* ptr_to_elem;

    iterator next()
    {
      ptr_to_elem = ptr_to_elem->m_next;
      return *this;
    }

    T getVal() { return ptr_to_elem->m_data; }

    bool end()
    {
      if (ptr_to_elem) {
        return false;
      }
      return true;
    }
  };
  iterator start()
  {
    iterator ret_val;
    ret_val.actual_class = this;
    ret_val.ptr_to_elem = m_start;
    return ret_val;
  }
};

// A simple hash implementation
template <class T, bool kStoreKey = true>
class AthHash
{
  unsigned int* m_listOfPrimes;

  void init_list_of_primes()
  {
    m_listOfPrimes = new unsigned int[16];
    m_listOfPrimes[0] = 49978783;
    m_listOfPrimes[1] = 18409199;
    m_listOfPrimes[2] = 1299827;
    m_listOfPrimes[3] = 1176221;
    m_listOfPrimes[4] = 981493;
    m_listOfPrimes[5] = 779377;
    m_listOfPrimes[6] = 530279;
    m_listOfPrimes[7] = 143567;
    m_listOfPrimes[8] = 30389;
    m_listOfPrimes[9] = 6869;
    m_listOfPrimes[10] = 1049;
    m_listOfPrimes[11] = 149;
    m_listOfPrimes[12] = 11;
    m_listOfPrimes[13] = 3;
    m_listOfPrimes[14] = 2;
    m_listOfPrimes[15] = 1;
  }

  // unsigned int m_prime;

  unsigned int l_find_largest_prime_below_number(unsigned int number)
  {
    assert(number > 3);
    int i = 0;
    while (m_listOfPrimes[i] > number) {
      i++;
    }
    return m_listOfPrimes[i];
  }

  unsigned int hashFunction(const char* key, unsigned int, unsigned int prime)
  {
    unsigned int hash = 0;
    int c;

    while ((c = static_cast<unsigned char>(*key++)) != '\0') {
      hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash % prime;
  }

  struct t_elem
  {
    const char* key;
    T data;
  };

 public:
  AthList<t_elem>* m_data;
  unsigned int m_prime;

  AthHash(unsigned int size = 100)
  {
    init_list_of_primes();
    m_prime = l_find_largest_prime_below_number(size);
    m_data = new AthList<t_elem>[m_prime];
  }

  ~AthHash()
  {
    delete[] m_listOfPrimes;
    unsigned int i;
    for (i = 0; i < m_prime; i++) {
      typename AthList<t_elem>::iterator iter = m_data[i].start();
      while (!iter.end()) {
        if (kStoreKey) {
          free((void*) iter.getVal().key);
        }
        iter.next();
      }
    }
    delete[] m_data;
  }

  void add(const char* key, T data)
  {
    unsigned int hash_val = hashFunction(key, strlen(key), m_prime);
    t_elem new_t_elem;
    new_t_elem.data = data;
    if (kStoreKey) {
      new_t_elem.key = strdup(key);
    } else {
      new_t_elem.key = key;
    }
    m_data[hash_val].push(new_t_elem);
  }

  // Get a stored value. Returns success of failure depending
  // if the value actually is stored or not
  bool get(const char* key, T& data)
  {
    unsigned int hash_val = hashFunction(key, strlen(key), m_prime);
    typename AthList<t_elem>::iterator iter = m_data[hash_val].start();
    while (!iter.end()) {
      if (strcmp(key, iter.getVal().key) == 0) {
        data = iter.getVal().data;
        return true;
      }
      iter.next();
    }
    return false;
  }
  struct iterator
  {
    unsigned int m_first_level_idx;
    typename AthList<t_elem>::iterator m_list_iterator;
    AthHash<T>* m_ptr_to_hash;

    iterator next()
    {
      if (m_list_iterator.end()) {
        m_first_level_idx++;
        m_list_iterator = m_ptr_to_hash->m_data[m_first_level_idx].start();
      } else {
        m_list_iterator.next();
      }
      if (m_list_iterator.end()) {
        assert(m_first_level_idx < m_ptr_to_hash->m_prime);
        return next();
      }
      return *this;
    }

    bool end()
    {
      if (m_first_level_idx >= m_ptr_to_hash->m_prime) {
        return true;
      }
      if (m_first_level_idx < m_ptr_to_hash->m_prime - 1) {
        return false;
      }
      return m_list_iterator.end();
    }

    T getVal() { return m_list_iterator.getVal().data; }

    char* getKey() { return m_list_iterator.getVal().key; }
  };

  iterator start()
  {
    iterator tmp_iter;
    unsigned int i;
    for (i = 0; i < m_prime; i++) {
      tmp_iter.m_list_iterator = m_data[i].start();
      if (!tmp_iter.m_list_iterator.end()) {
        break;
      }
    }
    tmp_iter.m_first_level_idx = i;
    tmp_iter.m_ptr_to_hash = this;
    return tmp_iter;
  }
};

}  // namespace rcx
