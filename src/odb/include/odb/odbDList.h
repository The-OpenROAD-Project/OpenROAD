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
  DListIterator() { _cur = nullptr; }
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
