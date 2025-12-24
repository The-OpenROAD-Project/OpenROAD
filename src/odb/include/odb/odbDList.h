// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace odb {

///
/// This class implements an instrusive double-linked list.
///
/// See sample test program below.
///

template <class T>
struct DListEntry
{
  T* next{nullptr};
  T* prev{nullptr};
};

template <class T, DListEntry<T>*(T*)>
class DList;

template <class T, DListEntry<T>* ENTRY(T*)>
class DListIterator
{
 public:
  DListIterator() = default;
  DListIterator(T* cur) { cur_ = cur; }
  DListIterator(const DListIterator& i) { cur_ = i.cur_; }
  DListIterator& operator=(const DListIterator& i)
  {
    if (this == &i) {
      return *this;
    }

    cur_ = i.cur_;
    return *this;
  }

  bool operator==(const DListIterator& i) const { return cur_ == i.cur_; }
  bool operator!=(const DListIterator& i) const { return cur_ != i.cur_; }
  T* operator*() { return cur_; }
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
  void incr() { cur_ = next(cur_); }
  T*& next(T* n) { return ENTRY(n)->next; }

  T* cur_{nullptr};

  friend class DList<T, ENTRY>;
};

template <class T, DListEntry<T>* ENTRY(T*)>
class DList
{
 public:
  using iterator = DListIterator<T, ENTRY>;

  T* front() { return head_; }
  T* back() { return tail_; }

  void push_front(T* p)
  {
    if (head_ == nullptr) {
      head_ = p;
      tail_ = p;
      next(p) = nullptr;
      prev(p) = nullptr;
    } else {
      prev(head_) = p;
      next(p) = head_;
      prev(p) = nullptr;
      head_ = p;
    }
  }

  void push_back(T* p)
  {
    if (head_ == nullptr) {
      head_ = p;
      tail_ = p;
      next(p) = nullptr;
      prev(p) = nullptr;
    } else {
      next(tail_) = p;
      prev(p) = tail_;
      next(p) = nullptr;
      tail_ = p;
    }
  }

  void clear() { head_ = tail_ = nullptr; }
  bool empty() const { return head_ == nullptr; }
  iterator begin() { return iterator(head_); }
  iterator end() { return iterator(nullptr); }

  iterator remove(T* p) { return remove(iterator(p)); }

  DListIterator<T, ENTRY> remove(iterator cur)
  {
    if (*cur == head_) {
      if (*cur == tail_) {
        head_ = nullptr;
        tail_ = nullptr;
      } else {
        head_ = next(*cur);
        prev(head_) = nullptr;
      }
    } else if (*cur == tail_) {
      tail_ = prev(*cur);
      next(tail_) = nullptr;
    } else {
      next(prev(*cur)) = next(*cur);
      prev(next(*cur)) = prev(*cur);
    }

    return iterator(next(*cur));
  }

 private:
  T*& next(T* n) { return ENTRY(n)->next; }
  T*& prev(T* n) { return ENTRY(n)->prev; }

  T* head_{nullptr};
  T* tail_{nullptr};
};

}  // namespace odb
