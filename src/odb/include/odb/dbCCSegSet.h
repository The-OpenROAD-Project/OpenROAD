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

class dbCCSeg;

template <>
class dbSetIterator<dbCCSeg>
{
 public:
  using value_type = dbCCSeg*;
  using difference_type = std::ptrdiff_t;
  using pointer = dbCCSeg**;
  using reference = dbCCSeg*&;
  using iterator_category = std::input_iterator_tag;

  dbSetIterator()
  {
    itr_ = nullptr;
    cur_ = 0;
    pid_ = 0;
  }

  dbSetIterator(const dbSetIterator& it)
  {
    itr_ = it.itr_;
    cur_ = it.cur_;
    pid_ = it.pid_;
  }

  bool operator==(const dbSetIterator<dbCCSeg>& it)
  {
    return (itr_ == it.itr_) && (cur_ == it.cur_) && (pid_ == it.pid_);
  }

  bool operator!=(const dbSetIterator<dbCCSeg>& it)
  {
    return (itr_ != it.itr_) || (cur_ != it.cur_) || (pid_ != it.pid_);
  }

  dbCCSeg* operator*() { return (dbCCSeg*) itr_->getObject(cur_); }

  dbCCSeg* operator->() { return (dbCCSeg*) itr_->getObject(cur_); }

  dbSetIterator<dbCCSeg>& operator++()
  {
    cur_ = itr_->next(cur_, pid_);
    return *this;
  }

  dbSetIterator<dbCCSeg> operator++(int)
  {
    dbSetIterator it(*this);
    cur_ = itr_->next(cur_, pid_);
    return it;
  }

 private:
  friend class dbSet<dbCCSeg>;

  dbSetIterator(dbIterator* itr, uint32_t id, uint32_t pid)
  {
    itr_ = itr;
    cur_ = id;
    pid_ = pid;
  }

  dbIterator* itr_;
  uint32_t cur_;
  uint32_t pid_;
};

template <>
class dbSet<dbCCSeg>
{
 public:
  using iterator = dbSetIterator<dbCCSeg>;

  dbSet()
  {
    itr_ = nullptr;
    parent_ = nullptr;
    pid_ = 0;
  }

  dbSet(dbObject* parent, dbIterator* itr)
  {
    parent_ = parent;
    itr_ = itr;
    pid_ = parent->getId();
  }

  dbSet(const dbSet<dbCCSeg>& c)
  {
    itr_ = c.itr_;
    parent_ = c.parent_;
    pid_ = c.pid_;
  }

  ///
  /// Returns the number of items in this set.
  ///
  uint32_t size() { return itr_->size(parent_); }

  ///
  /// Return a begin() iterator.
  ///
  iterator begin() { return iterator(itr_, itr_->begin(parent_), pid_); }

  ///
  /// Return an end() iterator.
  ///
  iterator end() { return iterator(itr_, itr_->end(parent_), pid_); }

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
  uint32_t pid_;
};

}  // namespace odb
