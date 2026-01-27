// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cassert>
#include <cstdint>
#include <map>
#include <vector>

namespace odb {

template <class T, class D>
inline dbMap<T, D>::dbMap(const dbSet<T>& set)
{
  set_ = set;

  // Use a vector if this set represents a sequential iterator
  if (set_.sequential()) {
    _vector = new std::vector<D>(set_.sequential() + 1);
    map_ = nullptr;
  }

  // Use a map if this set represents random iterator
  else {
    _vector = nullptr;
    map_ = new std::map<T*, D>;
    typename dbSet<T>::iterator itr;

    for (itr = set_.begin(); itr != set_.end(); ++itr) {
      T* object = *itr;
      (*map_)[object] = D();
    }
  }
}

template <class T, class D>
inline dbMap<T, D>::~dbMap()
{
  delete map_;
  delete _vector;
}

template <class T, class D>
inline const D& dbMap<T, D>::operator[](T* object) const
{
  if (map_) {
    typename std::map<T*, D>::const_iterator itr = map_->find(object);
    assert(itr != map_->end());
    return (*itr).second;
  }

  uint32_t idx = object->getId();
  assert(idx && (idx < _vector->size()));
  return (*_vector)[idx];
}

template <class T, class D>
inline D& dbMap<T, D>::operator[](T* object)
{
  if (map_) {
    return map_->at(object);
  }

  uint32_t idx = object->getId();
  assert(idx && (idx < _vector->size()));
  return (*_vector)[idx];
}

}  // namespace odb
