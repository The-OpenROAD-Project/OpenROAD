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

#include <vector>

#include "odb/dbStream.h"
#include "odb/odb.h"

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
