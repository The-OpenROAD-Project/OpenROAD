// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "odb/dbStream.h"

namespace odb {

template <class T>
class dbVector : public std::vector<T>
{
 public:
  using iterator = typename std::vector<T>::iterator;
  using const_iterator = typename std::vector<T>::const_iterator;
  using _base = std::vector<T>;

  dbVector<T>& operator=(const std::vector<T>& v)
  {
    if (this != &v) {
      *(std::vector<T>*) this = v;
    }

    return *this;
  }

  dbVector() = default;

  dbVector(const dbVector<T>& v) : std::vector<T>(v) {}

  iterator begin() { return _base::begin(); }
  iterator end() { return _base::end(); }
  const_iterator begin() const { return _base::begin(); }
  const_iterator end() const { return _base::end(); }
};

template <class T>
class dbVector<T*> : public std::vector<T*>
{
 public:
  using iterator = typename std::vector<T*>::iterator;
  using const_iterator = typename std::vector<T*>::const_iterator;
  using _base = std::vector<T*>;

  dbVector<T*>& operator=(const std::vector<T*>& v)
  {
    if (this != &v) {
      *(std::vector<T>*) this = v;
    }

    return *this;
  }

  dbVector() = default;

  dbVector(const dbVector<T*>& v) : std::vector<T*>(v) {}

  iterator begin() { return _base::begin(); }
  iterator end() { return _base::end(); }
  const_iterator begin() const { return _base::begin(); }
  const_iterator end() const { return _base::end(); }

  bool operator==(const dbVector<T*>& rhs) const
  {
    if (_base::size() != rhs.size()) {
      return false;
    }

    typename dbVector<T*>::const_iterator i1 = begin();
    typename dbVector<T*>::const_iterator i2 = rhs.begin();

    for (; i1 != end(); ++i1, ++i2) {
      if (**i1 != **i2) {
        return false;
      }
    }

    return true;
  }

  bool operator!=(const dbVector<T*>& rhs) const
  {
    return !this->operator==(rhs);
  }
};

template <class T>
inline dbOStream& operator<<(dbOStream& stream, const dbVector<T>& v)
{
  unsigned int sz = v.size();
  stream << sz;

  typename dbVector<T>::const_iterator itr;

  for (itr = v.begin(); itr != v.end(); ++itr) {
    const T& value = *itr;
    stream << value;
  }

  return stream;
}

template <class T>
inline dbIStream& operator>>(dbIStream& stream, dbVector<T>& v)
{
  v.clear();
  unsigned int sz;
  stream >> sz;
  v.reserve(sz);

  T t{};
  unsigned int i;
  for (i = 0; i < sz; ++i) {
    stream >> t;
    v.push_back(t);
  }

  return stream;
}

}  // namespace odb
