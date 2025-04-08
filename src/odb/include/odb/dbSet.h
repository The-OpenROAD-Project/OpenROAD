// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbIterator.h"

namespace odb {

class dbBlock;
class dbCCSeg;
class dbNet;

template <class T>
class dbSet;

template <class T>
class dbSetIterator
{
  friend class dbSet<T>;

  dbIterator* _itr;
  uint _cur;

  dbSetIterator(dbIterator* itr, uint id);

 public:
  using value_type = T*;
  using difference_type = std::ptrdiff_t;
  using pointer = T**;
  using reference = T*&;
  using iterator_category = std::input_iterator_tag;

  dbSetIterator();
  dbSetIterator(const dbSetIterator& it) = default;

  bool operator==(const dbSetIterator<T>& it);
  bool operator!=(const dbSetIterator<T>& it);

  T* operator*();
  T* operator->();
  dbSetIterator<T>& operator++();
  dbSetIterator<T> operator++(int);
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
  dbIterator* _itr;
  dbObject* _parent;

 public:
  using iterator = dbSetIterator<T>;

  dbSet()
  {
    _itr = nullptr;
    _parent = nullptr;
  }

  dbSet(dbObject* parent, dbIterator* itr)
  {
    _parent = parent;
    _itr = itr;
  }

  dbSet(const dbSet<T>& c)
  {
    _itr = c._itr;
    _parent = c._parent;
  }

  ///
  /// Returns the number of items in this set.
  ///
  uint size() { return _itr->size(_parent); }

  ///
  /// Return a begin() iterator.
  ///
  iterator begin() { return iterator(_itr, _itr->begin(_parent)); }

  ///
  /// Return an end() iterator.
  ///
  iterator end() { return iterator(_itr, _itr->end(_parent)); }

  ///
  /// If this set is sequential, this function will return the database
  /// identifier of the "Nth" iterated element (see the class description
  /// above):
  ///
  ///      returns oN->getId()
  ///
  /// If this set is non-sequential then it returns 0.
  ///
  uint sequential() { return _itr->sequential(); }

  ///
  /// Returns true if this set is reversible.
  ///
  bool reversible() { return _itr->reversible(); }

  ///
  /// Returns true if the is iterated in the reverse order that
  /// it was created.
  ///
  bool orderReversed() { return _itr->orderReversed(); }

  ///
  /// Reverse the order of this set.
  ///
  void reverse() { _itr->reverse(_parent); }

  ///
  /// Returns true if set is empty
  ///
  bool empty() { return begin() == end(); }
};

template <class T>
inline dbSetIterator<T>::dbSetIterator()
{
  _itr = nullptr;
  _cur = 0;
}

template <class T>
inline dbSetIterator<T>::dbSetIterator(dbIterator* itr, uint id)
{
  _itr = itr;
  _cur = id;
}

template <class T>
inline bool dbSetIterator<T>::operator==(const dbSetIterator& it)
{
  return (_itr == it._itr) && (_cur == it._cur);
}

template <class T>
inline bool dbSetIterator<T>::operator!=(const dbSetIterator& it)
{
  return (_itr != it._itr) || (_cur != it._cur);
}

template <class T>
inline T* dbSetIterator<T>::operator*()
{
  return (T*) _itr->getObject(_cur);
}

template <class T>
inline T* dbSetIterator<T>::operator->()
{
  return (T*) _itr->getObject(_cur);
}

template <class T>
inline dbSetIterator<T>& dbSetIterator<T>::operator++()
{
  _cur = _itr->next(_cur);
  return *this;
}

template <class T>
inline dbSetIterator<T> dbSetIterator<T>::operator++(int)
{
  dbSetIterator it(*this);
  _cur = _itr->next(_cur);
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
