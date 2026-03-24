// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>

#include "odb/dbIterator.h"

namespace odb {

class dbBlock;
class dbCCSeg;
class dbNet;

template <class T>
class dbSet;

template <class T>
class dbSetIterator
{
 public:
  using value_type = T*;
  using difference_type = std::ptrdiff_t;
  using pointer = T**;
  using reference = T*&;
  using iterator_category = std::input_iterator_tag;

  dbSetIterator() = default;
  dbSetIterator(const dbSetIterator& it) = default;

  bool operator==(const dbSetIterator<T>& it) const;
  bool operator!=(const dbSetIterator<T>& it) const;

  T* operator*() const;
  T* operator->() const;
  dbSetIterator<T>& operator++();
  dbSetIterator<T> operator++(int);

 private:
  dbSetIterator(dbIterator* itr, uint32_t id);

  dbIterator* itr_{nullptr};
  uint32_t cur_{0};

  friend class dbSet<T>;
};

///
/// The dbSet class provides an abstration of database objects collections.
///
/// There are two types of sets; sequential and non-sequential.
///
/// Sequential set iterators have the following property:
///
///     Let (o1,o2,o3,...,oN) be the objects iterated of a set, where 1..N
///     represents the iteration order.
///
///     Then the following condition holds:
///
///        0 < o1->getId() < o2->getId() < o3->getId() < ... < oN->getId()
///
///     Because of object deletion, there may be gaps between the object
///     identifiers in the iterated set:
///
///        Given: ... < oi->getId() < oj->getId() < ...
///        Then: ... < (oi->getId() + 1) <= oj->getId() < ...
///
/// Non-Sequential set iterators do not have any ordering property.
///
template <class T>
class dbSet
{
 public:
  using iterator = dbSetIterator<T>;

  dbSet() = default;

  dbSet(dbObject* parent, dbIterator* itr)
  {
    parent_ = parent;
    itr_ = itr;
  }

  dbSet(const dbSet<T>& c) = default;

  ///
  /// Returns the number of items in this set.
  ///
  uint32_t size() const { return itr_->size(parent_); }

  ///
  /// Return a begin() iterator.
  ///
  iterator begin() const { return iterator(itr_, itr_->begin(parent_)); }

  ///
  /// Return an end() iterator.
  ///
  iterator end() const { return iterator(itr_, itr_->end(parent_)); }

  ///
  /// If this set is sequential, this function will return the database
  /// identifier of the "Nth" iterated element (see the class description
  /// above):
  ///
  ///      returns oN->getId()
  ///
  /// If this set is non-sequential then it returns 0.
  ///
  uint32_t sequential() const { return itr_->sequential(); }

  ///
  /// Returns true if this set is reversible.
  ///
  bool reversible() const { return itr_->reversible(); }

  ///
  /// Returns true if the is iterated in the reverse order that
  /// it was created.
  ///
  bool orderReversed() const { return itr_->orderReversed(); }

  ///
  /// Reverse the order of this set.
  ///
  void reverse() { itr_->reverse(parent_); }

  ///
  /// Returns true if set is empty
  ///
  bool empty() const { return begin() == end(); }

 private:
  dbIterator* itr_{nullptr};
  dbObject* parent_{nullptr};
};

template <class T>
inline dbSetIterator<T>::dbSetIterator(dbIterator* itr, uint32_t id)
{
  itr_ = itr;
  cur_ = id;
}

template <class T>
inline bool dbSetIterator<T>::operator==(const dbSetIterator& it) const
{
  return (itr_ == it.itr_) && (cur_ == it.cur_);
}

template <class T>
inline bool dbSetIterator<T>::operator!=(const dbSetIterator& it) const
{
  return !(*this == it);
}

template <class T>
inline T* dbSetIterator<T>::operator*() const
{
  return (T*) itr_->getObject(cur_);
}

template <class T>
inline T* dbSetIterator<T>::operator->() const
{
  return (T*) itr_->getObject(cur_);
}

template <class T>
inline dbSetIterator<T>& dbSetIterator<T>::operator++()
{
  cur_ = itr_->next(cur_);
  return *this;
}

template <class T>
inline dbSetIterator<T> dbSetIterator<T>::operator++(int)
{
  dbSetIterator it(*this);
  cur_ = itr_->next(cur_);
  return it;
}

// Specialization declarations
template <>
class dbSetIterator<dbBlock>;
template <>
class dbSet<dbBlock>;
template <>
class dbSetIterator<dbCCSeg>;
template <>
class dbSet<dbCCSeg>;
template <>
class dbSetIterator<dbNet>;
template <>
class dbSet<dbNet>;

}  // namespace odb
