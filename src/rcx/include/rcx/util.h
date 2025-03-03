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
#include <cstdlib>
#include <cstring>

namespace rcx {

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
template <class T>
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
  int _allocKeyFlag;

 public:
  AthList<t_elem>* m_data;
  unsigned int m_prime;

  AthHash(unsigned int size = 100, int store = 1)
  {
    init_list_of_primes();
    m_prime = l_find_largest_prime_below_number(size);
    m_data = new AthList<t_elem>[m_prime];
    _allocKeyFlag = store;
  }

  ~AthHash()
  {
    delete m_listOfPrimes;
    unsigned int i;
    for (i = 0; i < m_prime; i++) {
      typename AthList<t_elem>::iterator iter = m_data[i].start();
      while (!iter.end()) {
        if (_allocKeyFlag > 0) {
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
    if (_allocKeyFlag > 0) {
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
