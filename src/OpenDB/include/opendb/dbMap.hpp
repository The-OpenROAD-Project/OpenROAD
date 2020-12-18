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

#include "ZException.h"

namespace odb {

template <class T, class D>
inline dbMap<T, D>::dbMap(const dbSet<T>& set)
{
  _set = set;

  // Use a vector if this set represents a sequential iterator
  if (_set.sequential()) {
    _vector = new std::vector<D>(_set.sequential() + 1);
    _map    = NULL;
  }

  // Use a map if this set represents random iterator
  else {
    _vector = NULL;
    _map    = new std::map<T*, D>;
    typename dbSet<T>::iterator itr;

    for (itr = _set.begin(); itr != _set.end(); ++itr) {
      T* object       = *itr;
      (*_map)[object] = D();
    }
  }
}

template <class T, class D>
inline dbMap<T, D>::~dbMap()
{
  if (_map)
    delete _map;

  if (_vector)
    delete _vector;
}

template <class T, class D>
inline const D& dbMap<T, D>::operator[](T* object) const
{
  if (_map) {
    typename std::map<T*, D>::const_iterator itr = _map->find(object);
    ZASSERT(itr != _map->end());
    return (*itr).second;
  }

  uint idx = object->getId();
  ZASSERT(idx && (idx < _vector->size()));
  return (*_vector)[idx];
}

template <class T, class D>
inline D& dbMap<T, D>::operator[](T* object)
{
  if (_map) {
    typename std::map<T*, D>::iterator itr = _map->find(object);
    ZASSERT(itr != _map->end());
    return (*itr).second;
  }

  uint idx = object->getId();
  ZASSERT(idx && (idx < _vector->size()));
  return (*_vector)[idx];
}

}  // namespace odb


