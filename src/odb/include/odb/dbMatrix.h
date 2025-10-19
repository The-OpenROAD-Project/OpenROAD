// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstddef>
#include <vector>

#include "boost/multi_array.hpp"
#include "odb/dbStream.h"
#include "odb/odb.h"

namespace odb {

template <class T>
class dbMatrix
{
 public:
  dbMatrix() = default;
  dbMatrix(uint n, uint m);

  uint numRows() const { return _matrix.shape()[0]; }
  uint numCols() const { return _matrix.shape()[1]; }
  uint numElems() const;

  void resize(uint n, uint m);
  void clear();

  const T& operator()(uint i, uint j) const;
  T& operator()(uint i, uint j);

  dbMatrix<T>& operator=(const dbMatrix<T>& rhs);

  bool operator==(const dbMatrix<T>& rhs) const;
  bool operator!=(const dbMatrix<T>& rhs) const { return !operator==(rhs); }

 private:
  boost::multi_array<T, 2> _matrix;
};

template <class T>
inline dbOStream& operator<<(dbOStream& stream, const dbMatrix<T>& mat)
{
  stream << mat.numRows();
  stream << mat.numCols();

  for (uint i = 0; i < mat.numRows(); ++i) {
    for (uint j = 0; j < mat.numCols(); ++j) {
      stream << mat(i, j);
    }
  }

  return stream;
}

template <class T>
inline dbIStream& operator>>(dbIStream& stream, dbMatrix<T>& mat)
{
  uint n;
  uint m;

  stream >> n;
  stream >> m;
  mat.resize(n, m);

  for (uint i = 0; i < mat.numRows(); ++i) {
    for (uint j = 0; j < mat.numCols(); ++j) {
      stream >> mat(i, j);
    }
  }

  return stream;
}

template <class T>
inline dbMatrix<T>::dbMatrix(uint n, uint m)
{
  resize(n, m);
}

template <class T>
inline void dbMatrix<T>::clear()
{
  _matrix.resize(boost::extents[0][0]);
}

template <class T>
inline uint dbMatrix<T>::numElems() const
{
  return _matrix.num_elements();
}

template <class T>
inline void dbMatrix<T>::resize(uint n, uint m)
{
  _matrix.resize(boost::extents[n][m]);
}

template <class T>
inline const T& dbMatrix<T>::operator()(uint i, uint j) const
{
  return _matrix[i][j];
}

template <class T>
inline T& dbMatrix<T>::operator()(uint i, uint j)
{
  return _matrix[i][j];
}

template <class T>
inline bool dbMatrix<T>::operator==(const dbMatrix<T>& rhs) const
{
  return _matrix == rhs._matrix;
}

template <class T>
inline dbMatrix<T>& dbMatrix<T>::operator=(const dbMatrix<T>& rhs)
{
  if (this == &rhs) {
    return *this;
  }

  const auto lhs_shape = _matrix.shape();
  const auto rhs_shape = rhs._matrix.shape();

  if (lhs_shape[0] != rhs_shape[0] || lhs_shape[1] != rhs_shape[1]) {
    std::vector<size_t> new_extents{rhs_shape[0], rhs_shape[1]};
    _matrix.resize(new_extents);
  }
  _matrix = rhs._matrix;

  return *this;
}

}  // namespace odb
