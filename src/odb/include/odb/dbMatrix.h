// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "dbStream.h"
#include "odb.h"

namespace odb {

template <class T>
class dbMatrix
{
 public:
  dbMatrix() = default;
  dbMatrix(uint n, uint m);

  uint numRows() const { return _n; }
  uint numCols() const { return _m; }
  uint numElems() const;

  void resize(uint n, uint m);
  void clear();

  const T& operator()(uint i, uint j) const;
  T& operator()(uint i, uint j);

  bool operator==(const dbMatrix<T>& rhs) const;
  bool operator!=(const dbMatrix<T>& rhs) const { return !operator==(rhs); }

 private:
  uint _n = 0;
  uint _m = 0;
  std::vector<std::vector<T>> _matrix;
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
  _n = n;
  _m = m;
  resize(n, m);
}

template <class T>
inline void dbMatrix<T>::clear()
{
  _n = _m = 0;
  _matrix.clear();
}

template <class T>
inline uint dbMatrix<T>::numElems() const
{
  return _n * _m;
}

template <class T>
inline void dbMatrix<T>::resize(uint n, uint m)
{
  _n = n;
  _m = m;
  _matrix.resize(n);

  for (uint i = 0; i < _n; ++i) {
    _matrix[i].resize(m);
  }
}

template <class T>
inline const T& dbMatrix<T>::operator()(uint i, uint j) const
{
  assert((i >= 0) && (i < _n) && (j >= 0) && (j < _m));
  return _matrix[i][j];
}

template <class T>
inline T& dbMatrix<T>::operator()(uint i, uint j)
{
  assert((i >= 0) && (i < _n) && (j >= 0) && (j < _m));
  return _matrix[i][j];
}

template <class T>
inline bool dbMatrix<T>::operator==(const dbMatrix<T>& rhs) const
{
  return _matrix == rhs._matrix;
}

}  // namespace odb
