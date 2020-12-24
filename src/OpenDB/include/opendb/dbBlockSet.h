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

#include "dbObject.h"

namespace odb {

class dbBlock;

template <>
class dbSetIterator<dbBlock>
{
  friend class dbSet<dbBlock>;

  dbIterator* _itr;
  uint        _cur;
  dbObject*   _parent;

  dbSetIterator(dbIterator* itr, uint id, dbObject* parent)
  {
    _itr    = itr;
    _cur    = id;
    _parent = parent;
  }

 public:
  dbSetIterator()
  {
    _itr    = NULL;
    _cur    = 0;
    _parent = NULL;
  }

  dbSetIterator(const dbSetIterator& it)
  {
    _itr    = it._itr;
    _cur    = it._cur;
    _parent = it._parent;
  }

  bool operator==(const dbSetIterator<dbBlock>& it)
  {
    return (_itr == it._itr) && (_cur == it._cur) && (_parent == it._parent);
  }

  bool operator!=(const dbSetIterator<dbBlock>& it)
  {
    return (_itr != it._itr) || (_cur != it._cur) || (_parent != it._parent);
  }

  dbBlock* operator*() { return (dbBlock*) _itr->getObject(_cur, _parent); }

  dbBlock* operator->() { return (dbBlock*) _itr->getObject(_cur, _parent); }

  dbSetIterator<dbBlock>& operator++()
  {
    _cur = _itr->next(_cur);
    return *this;
  }

  dbSetIterator<dbBlock> operator++(int)
  {
    dbSetIterator it(*this);
    _cur = _itr->next(_cur);
    return it;
  }
};

template <>
class dbSet<dbBlock>
{
  dbIterator* _itr;
  dbObject*   _parent;

 public:
  typedef dbSetIterator<dbBlock> iterator;

  dbSet()
  {
    _itr    = NULL;
    _parent = NULL;
  }

  dbSet(dbObject* parent, dbIterator* itr)
  {
    _parent = parent;
    _itr    = itr;
  }

  dbSet(const dbSet<dbBlock>& c)
  {
    _itr    = c._itr;
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
};

}  // namespace odb


