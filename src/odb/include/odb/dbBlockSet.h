// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>

#include "odb/dbIterator.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"

namespace odb {

class dbBlock;

template <>
class dbSetIterator<dbBlock>
{
 public:
  using value_type = dbBlock*;
  using difference_type = std::ptrdiff_t;
  using pointer = dbBlock**;
  using reference = dbBlock*&;
  using iterator_category = std::input_iterator_tag;

  dbSetIterator()
  {
    itr_ = nullptr;
    cur_ = 0;
    parent_ = nullptr;
  }

  dbSetIterator(const dbSetIterator& it)
  {
    itr_ = it.itr_;
    cur_ = it.cur_;
    parent_ = it.parent_;
  }

  bool operator==(const dbSetIterator<dbBlock>& it)
  {
    return (itr_ == it.itr_) && (cur_ == it.cur_) && (parent_ == it.parent_);
  }

  bool operator!=(const dbSetIterator<dbBlock>& it)
  {
    return (itr_ != it.itr_) || (cur_ != it.cur_) || (parent_ != it.parent_);
  }

  dbBlock* operator*() { return (dbBlock*) itr_->getObject(cur_, parent_); }

  dbBlock* operator->() { return (dbBlock*) itr_->getObject(cur_, parent_); }

  dbSetIterator<dbBlock>& operator++()
  {
    cur_ = itr_->next(cur_);
    return *this;
  }

  dbSetIterator<dbBlock> operator++(int)
  {
    dbSetIterator it(*this);
    cur_ = itr_->next(cur_);
    return it;
  }

 private:
  friend class dbSet<dbBlock>;

  dbSetIterator(dbIterator* itr, uint32_t id, dbObject* parent)
  {
    itr_ = itr;
    cur_ = id;
    parent_ = parent;
  }

  dbIterator* itr_;
  uint32_t cur_;
  dbObject* parent_;
};

template <>
class dbSet<dbBlock>
{
 public:
  using iterator = dbSetIterator<dbBlock>;

  dbSet()
  {
    itr_ = nullptr;
    parent_ = nullptr;
  }

  dbSet(dbObject* parent, dbIterator* itr)
  {
    parent_ = parent;
    itr_ = itr;
  }

  dbSet(const dbSet<dbBlock>& c)
  {
    itr_ = c.itr_;
    parent_ = c.parent_;
  }

  ///
  /// Returns the number of items in this set.
  ///
  uint32_t size() { return itr_->size(parent_); }

  ///
  /// Return a begin() iterator.
  ///
  iterator begin() { return iterator(itr_, itr_->begin(parent_), parent_); }

  ///
  /// Return an end() iterator.
  ///
  iterator end() { return iterator(itr_, itr_->end(parent_), parent_); }

  ///
  /// Returns true if set is empty
  ///
  bool empty() { return begin() == end(); }

  ///
  /// Returns the maximum number sequential elements the this set
  /// may iterate.
  ///
  uint32_t sequential() { return itr_->sequential(); }

  ///
  /// Returns true if this set is reversible.
  ///
  bool reversible() { return itr_->reversible(); }

  ///
  /// Returns true if the is iterated in the reverse order that
  /// it was created.
  ///
  bool orderReversed() { return itr_->orderReversed(); }

  ///
  /// Reverse the order of this set.
  ///
  void reverse() { itr_->reverse(parent_); }

 private:
  dbIterator* itr_;
  dbObject* parent_;
};

}  // namespace odb
