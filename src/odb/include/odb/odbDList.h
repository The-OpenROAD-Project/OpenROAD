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
  T* _next{nullptr};
  T* _prev{nullptr};
};

template <class T, DListEntry<T>*(T*)>
class DList;

template <class T, DListEntry<T>* ENTRY(T*)>
class DListIterator
{
 public:
  DListIterator() = default;
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

 private:
  void incr() { _cur = next(_cur); }
  T*& next(T* n) { return ENTRY(n)->_next; }

  T* _cur{nullptr};

  friend class DList<T, ENTRY>;
};

template <class T, DListEntry<T>* ENTRY(T*)>
class DList
{
 public:
  using iterator = DListIterator<T, ENTRY>;

  T* front() { return _head; }
  T* back() { return _tail; }

  void push_front(T* p)
  {
    if (_head == nullptr) {
      _head = p;
      _tail = p;
      next(p) = nullptr;
      prev(p) = nullptr;
    } else {
      prev(_head) = p;
      next(p) = _head;
      prev(p) = nullptr;
      _head = p;
    }
  }

  void push_back(T* p)
  {
    if (_head == nullptr) {
      _head = p;
      _tail = p;
      next(p) = nullptr;
      prev(p) = nullptr;
    } else {
      next(_tail) = p;
      prev(p) = _tail;
      next(p) = nullptr;
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
      } else {
        _head = next(*cur);
        prev(_head) = nullptr;
      }
    } else if (*cur == _tail) {
      _tail = prev(*cur);
      next(_tail) = nullptr;
    } else {
      next(prev(*cur)) = next(*cur);
      prev(next(*cur)) = prev(*cur);
    }

    return iterator(next(*cur));
  }

 private:
  T*& next(T* n) { return ENTRY(n)->_next; }
  T*& prev(T* n) { return ENTRY(n)->_prev; }

  T* _head{nullptr};
  T* _tail{nullptr};
};

}  // namespace odb
