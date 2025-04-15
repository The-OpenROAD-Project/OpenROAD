// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbObject.h"
#include "dbSet.h"

namespace odb {

class dbNet;

template <>
class dbSetIterator<dbNet>
{
  friend class dbSet<dbNet>;

  dbIterator* _itr;
  uint _cur;
  dbObject* _parent;

  dbSetIterator(dbIterator* itr, uint id, dbObject* parent)
  {
    _itr = itr;
    _cur = id;
    _parent = parent;
  }

 public:
  using value_type = dbNet*;
  using difference_type = std::ptrdiff_t;
  using pointer = dbNet**;
  using reference = dbNet*&;
  using iterator_category = std::input_iterator_tag;

  dbSetIterator()
  {
    _itr = nullptr;
    _cur = 0;
    _parent = nullptr;
  }

  dbSetIterator(const dbSetIterator& it)
  {
    _itr = it._itr;
    _cur = it._cur;
    _parent = it._parent;
  }

  bool operator==(const dbSetIterator<dbNet>& it)
  {
    return (_itr == it._itr) && (_cur == it._cur) && (_parent == it._parent);
  }

  bool operator!=(const dbSetIterator<dbNet>& it)
  {
    return (_itr != it._itr) || (_cur != it._cur) || (_parent != it._parent);
  }

  dbNet* operator*() { return (dbNet*) _itr->getObject(_cur, _parent); }

  dbNet* operator->() { return (dbNet*) _itr->getObject(_cur, _parent); }

  dbSetIterator<dbNet>& operator++()
  {
    _cur = _itr->next(_cur);
    return *this;
  }

  dbSetIterator<dbNet> operator++(int)
  {
    dbSetIterator it(*this);
    _cur = _itr->next(_cur);
    return it;
  }
};

template <>
class dbSet<dbNet>
{
  dbIterator* _itr;
  dbObject* _parent;

 public:
  using iterator = dbSetIterator<dbNet>;

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

  dbSet(const dbSet<dbNet>& c)
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
  iterator begin() { return iterator(_itr, _itr->begin(_parent), _parent); }

  ///
  /// Return an end() iterator.
  ///
  iterator end() { return iterator(_itr, _itr->end(_parent), _parent); }

  ///
  /// Returns the maximum number sequential elements the this set
  /// may iterate.
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

}  // namespace odb
