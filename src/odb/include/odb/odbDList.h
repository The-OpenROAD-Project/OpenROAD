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

#include "odb.h"

namespace odb {

///
/// This class implements an instrusive double-linked list.
///
/// See sample test program below.
///

template <class T>
class DListEntry
{
 public:
  DListEntry() : _next(nullptr), _prev(nullptr) {}
  T* _next;
  T* _prev;
};

template <class T, DListEntry<T>*(T*)>
class DList;

template <class T, DListEntry<T>* ENTRY(T*)>
class DListIterator
{
  T* _cur;

  void incr() { _cur = NEXT(_cur); }
  T*& NEXT(T* n) { return ENTRY(n)->_next; }

 public:
  DListIterator() { _cur = NULL; }
  DListIterator(T* cur) { _cur = cur; }
  DListIterator(const DListIterator& i) { _cur = i._cur; }
  DListIterator& operator=(const DListIterator& i)
  {
    _cur = i._cur;
    return *this;
  }

  bool operator==(const DListIterator& i) const { return _cur == i._cur; }
  bool operator!=(const DListIterator& i) const { return _cur != i._cur; }
  T* operator*() { return _cur; }
  DListIterator<T, ENTRY>& operator++()
  {
    incr();
    return *this;
  }
  DListIterator<T, ENTRY> operator++(int)
  {
    DListIterator<T, ENTRY> i = *this;
    incr();
    return i;
  }

  friend class DList<T, ENTRY>;
};

template <class T, DListEntry<T>* ENTRY(T*)>
class DList
{
  /*
    public:
      typedef DListIterator<T,ENTRY> iterator;
  */
 private:
  T* _head;
  T* _tail;

  bool lessthan(const T& p1, const T& p2) { return *p1 < *p2; }

  T*& NEXT(T* n) { return ENTRY(n)->_next; }

  T*& PREV(T* n) { return ENTRY(n)->_prev; }

 public:
  typedef DListIterator<T, ENTRY> iterator;

  DList()
  {
    _head = NULL;
    _tail = NULL;
  }

  T* front() { return _head; }
  T* back() { return _tail; }

  void push_front(T* p)
  {
    if (_head == NULL) {
      _head = p;
      _tail = p;
      NEXT(p) = NULL;
      PREV(p) = NULL;
    } else {
      PREV(_head) = p;
      NEXT(p) = _head;
      PREV(p) = NULL;
      _head = p;
    }
  }

  void push_back(T* p)
  {
    if (_head == NULL) {
      _head = p;
      _tail = p;
      NEXT(p) = NULL;
      PREV(p) = NULL;
    } else {
      NEXT(_tail) = p;
      PREV(p) = _tail;
      NEXT(p) = NULL;
      _tail = p;
    }
  }

  void clear() { _head = _tail = NULL; }
  bool empty() const { return _head == NULL; }
  iterator begin() { return iterator(_head); }
  iterator end() { return iterator(NULL); }

  void swap(DList& l)
  {
    T* head = l._head;
    l._head = _head;
    _head = head;
    T* tail = l._tail;
    l._tail = _tail;
    _tail = tail;
  }

  iterator remove(T* p) { return remove(iterator(p)); }

  void move(typename DList<T, ENTRY>::iterator itr1,
            typename DList<T, ENTRY>::iterator itr2)
  {
    if (*itr1 == _head) {
      if (_head == NULL) {
        _head = *itr2;
        _tail = *itr2;
        NEXT(*itr2) = NULL;
        PREV(*itr2) = NULL;
      } else {
        PREV(_head) = *itr2;
        NEXT(*itr2) = _head;
        PREV(*itr2) = NULL;
        _head = *itr2;
      }
    } else {
      NEXT(PREV(*itr1)) = *itr2;
      PREV(*itr2) = PREV(*itr1);
      NEXT(*itr2) = *itr1;
      PREV(*itr1) = *itr2;
    }
  }

  DListIterator<T, ENTRY> remove(iterator cur)
  {
    if (*cur == _head) {
      if (*cur == _tail) {
        _head = NULL;
        _tail = NULL;
      }

      else {
        _head = NEXT(*cur);
        PREV(_head) = NULL;
      }
    }

    else if (*cur == _tail) {
      _tail = PREV(*cur);
      NEXT(_tail) = NULL;
    }

    else {
      NEXT(PREV(*cur)) = NEXT(*cur);
      PREV(NEXT(*cur)) = PREV(*cur);
    }

    return iterator(NEXT(*cur));
  }

  void reverse()
  {
    if (_tail == _head)
      return;

    T* c = _head;
    T* n = NEXT(c);

    while (n) {
      T* tmp = NEXT(n);
      PREV(c) = n;
      NEXT(n) = c;
      c = n;
      n = tmp;
    }

    T* tmp = _head;
    _head = _tail;
    _tail = tmp;

    PREV(_head) = NULL;
    NEXT(_tail) = NULL;
  }

  int size()
  {
    T* c;
    int i = 0;
    for (c = _head; c != NULL; c = NEXT(c))
      ++i;
    return i;
  }

  void merge(DList<T, ENTRY>& l)
  {
    iterator first1 = begin();
    iterator last1 = end();
    iterator first2 = l.begin();
    iterator last2 = l.end();

    while (first1 != last1 && first2 != last2) {
      if (lessthan(**first2, **first1)) {
        iterator next = first2;
        ++next;
        move(first1, first2);
        first2 = next;
      } else {
        ++first1;
      }
    }

    if (first2 != last2) {
      if (_head == NULL) {
        _head = *first2;
        _tail = l._tail;
      } else {
        NEXT(_tail) = *first2;
        PREV(*first2) = _tail;
        _tail = l._tail;
      }
    }

    l._head = NULL;
    l._tail = NULL;
  }

  template <class CMP>
  void merge(DList<T, ENTRY>& l, CMP cmp)
  {
    iterator first1 = begin();
    iterator last1 = end();
    iterator first2 = l.begin();
    iterator last2 = l.end();

    while (first1 != last1 && first2 != last2) {
      if (cmp(**first2, **first1)) {
        iterator next = first2;
        ++next;
        move(first1, first2);
        first2 = next;
      } else {
        ++first1;
      }
    }

    if (first2 != last2) {
      if (_head == NULL) {
        _head = *first2;
        _tail = l._tail;
      } else {
        NEXT(_tail) = *first2;
        PREV(*first2) = _tail;
        _tail = l._tail;
      }
    }

    l._head = NULL;
    l._tail = NULL;
  }

  void sort()
  {
    if ((_head != NULL) && (NEXT(_head) != NULL)) {
      DList<T, ENTRY> carry;
      DList<T, ENTRY> counter[64];
      int fill = 0;

      while (!empty()) {
        T* head = NEXT(_head);
        carry.move(carry.begin(), begin());
        _head = head;

        int i = 0;
        while (i < fill && !counter[i].empty()) {
          counter[i].merge(carry);
          carry.swap(counter[i++]);
        }
        carry.swap(counter[i]);
        if (i == fill)
          ++fill;
      }

      for (int i = 1; i < fill; ++i)
        counter[i].merge(counter[i - 1]);

      swap(counter[fill - 1]);
    }
  }

  template <class CMP>
  void sort(CMP cmp)
  {
    if ((_head != NULL) && (NEXT(_head) != NULL)) {
      DList<T, ENTRY> carry;
      DList<T, ENTRY> counter[64];
      int fill = 0;

      while (!empty()) {
        T* head = NEXT(_head);
        carry.move(carry.begin(), begin());
        _head = head;

        int i = 0;
        while (i < fill && !counter[i].empty()) {
          counter[i].merge(carry, cmp);
          carry.swap(counter[i++]);
        }
        carry.swap(counter[i]);
        if (i == fill)
          ++fill;
      }

      for (int i = 1; i < fill; ++i)
        counter[i].merge(counter[i - 1], cmp);

      swap(counter[fill - 1]);
    }
  }
};

}  // namespace odb

#if 0
#include <assert.h>
#include <stdio.h>

#include <vector>

using namespace odb;

struct elem
{
    int a;
    DListEntry<elem> entry;
    elem * next;

    elem(int n )
    {
        a = n;
        next = NULL;
    }

    bool operator<( const elem & e ) { return a < e.a; }
    
    static DListEntry<elem> * getEntry( elem * e ) 
    {
        return &e->entry;
    }
};

struct cmp
{
    bool operator()( const elem & e1, const elem & e2 )
    {
        return e1.a < e2.a;
    }
};

#define M 100000

main()
{
    int i;

    std::vector<elem *> elems;


    for( i = 0; i < M; ++i )
    {
        elem * e = new elem(i);
        elems.push_back(e);
    }
    
    for( i = 0; i < M; ++i )
    {
        int r1 = random() % M;
        int r2 = random() % M;
        elem * e = elems[r1];
        elems[r1] = elems[r2];
        elems[r2] = e;
    }
    
    DList<elem, elem::getEntry> l1;

    for( i = 0; i < M; ++i )
    {
        elem * e = elems[i];
        l1.push_back(e);
    }
    
    l1.sort(cmp());
    
    DList<elem, elem::getEntry>::iterator itr;

    for( i = 0, itr = l1.begin(); itr != l1.end(); ++itr, i++ )
    {
        elem * e = *itr;
        printf("l1: %d\n", e->a );
        assert( e->a == i );
    }

    l1.reverse();

    for( i = 1, itr = l1.begin(); itr != l1.end(); ++itr, i++ )
    {
        elem * e = *itr;
        printf("l1: %d\n", e->a );
        assert( e->a == (M-i) );
    }
    
}
#endif
