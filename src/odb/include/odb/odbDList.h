// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

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
  DListIterator() { _cur = nullptr; }
  DListIterator(T* cur) { _cur = cur; }
  DListIterator(const DListIterator& i) { _cur = i._cur; }
  DListIterator& operator=(const DListIterator& i)
  {
    if (this == &i) {
      return *this;
    }

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
 private:
  T* _head;
  T* _tail;

  T*& NEXT(T* n) { return ENTRY(n)->_next; }

  T*& PREV(T* n) { return ENTRY(n)->_prev; }

 public:
  using iterator = DListIterator<T, ENTRY>;

  DList()
  {
    _head = nullptr;
    _tail = nullptr;
  }

  T* front() { return _head; }
  T* back() { return _tail; }

  void push_front(T* p)
  {
    if (_head == nullptr) {
      _head = p;
      _tail = p;
      NEXT(p) = nullptr;
      PREV(p) = nullptr;
    } else {
      PREV(_head) = p;
      NEXT(p) = _head;
      PREV(p) = nullptr;
      _head = p;
    }
  }

  void push_back(T* p)
  {
    if (_head == nullptr) {
      _head = p;
      _tail = p;
      NEXT(p) = nullptr;
      PREV(p) = nullptr;
    } else {
      NEXT(_tail) = p;
      PREV(p) = _tail;
      NEXT(p) = nullptr;
      _tail = p;
    }
  }

  void clear() { _head = _tail = nullptr; }
  bool empty() const { return _head == nullptr; }
  iterator begin() { return iterator(_head); }
  iterator end() { return iterator(nullptr); }

  iterator remove(T* p) { return remove(iterator(p)); }

  DListIterator<T, ENTRY> remove(iterator cur)
  {
    if (*cur == _head) {
      if (*cur == _tail) {
        _head = nullptr;
        _tail = nullptr;
      }

      else {
        _head = NEXT(*cur);
        PREV(_head) = nullptr;
      }
    }

    else if (*cur == _tail) {
      _tail = PREV(*cur);
      NEXT(_tail) = nullptr;
    }

    else {
      NEXT(PREV(*cur)) = NEXT(*cur);
      PREV(NEXT(*cur)) = PREV(*cur);
    }

    return iterator(NEXT(*cur));
  }
};

}  // namespace odb
