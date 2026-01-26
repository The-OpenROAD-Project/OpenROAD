// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstddef>
#include <vector>

#include "boost/multi_array.hpp"
#include "odb/dbStream.h"

namespace odb {

template <class T>
class dbMatrix
{
 public:
  dbMatrix() = default;
  dbMatrix(int n, int m);

  int numRows() const { return matrix_.shape()[0]; }
  int numCols() const { return matrix_.shape()[1]; }
  int numElems() const;

  void resize(int n, int m);
  void clear();

  const T& operator()(int i, int j) const;
  T& operator()(int i, int j);

  dbMatrix<T>& operator=(const dbMatrix<T>& rhs);

  bool operator==(const dbMatrix<T>& rhs) const;
  bool operator!=(const dbMatrix<T>& rhs) const { return !operator==(rhs); }

 private:
  boost::multi_array<T, 2> matrix_;
};

template <class T>
inline dbOStream& operator<<(dbOStream& stream, const dbMatrix<T>& mat)
{
  stream << mat.numRows();
  stream << mat.numCols();

  for (int i = 0; i < mat.numRows(); ++i) {
    for (int j = 0; j < mat.numCols(); ++j) {
      stream << mat(i, j);
    }
  }

  return stream;
}

template <class T>
inline dbIStream& operator>>(dbIStream& stream, dbMatrix<T>& mat)
{
  int n;
  int m;

  stream >> n;
  stream >> m;
  mat.resize(n, m);

  for (int i = 0; i < mat.numRows(); ++i) {
    for (int j = 0; j < mat.numCols(); ++j) {
      stream >> mat(i, j);
    }
  }

  return stream;
}

template <class T>
inline dbMatrix<T>::dbMatrix(int n, int m)
{
  resize(n, m);
}

template <class T>
inline void dbMatrix<T>::clear()
{
  matrix_.resize(boost::extents[0][0]);
}

template <class T>
inline int dbMatrix<T>::numElems() const
{
  return matrix_.num_elements();
}

template <class T>
inline void dbMatrix<T>::resize(int n, int m)
{
  matrix_.resize(boost::extents[n][m]);
}

template <class T>
inline const T& dbMatrix<T>::operator()(int i, int j) const
{
  return matrix_[i][j];
}

template <class T>
inline T& dbMatrix<T>::operator()(int i, int j)
{
  return matrix_[i][j];
}

template <class T>
inline bool dbMatrix<T>::operator==(const dbMatrix<T>& rhs) const
{
  return matrix_ == rhs.matrix_;
}

template <class T>
inline dbMatrix<T>& dbMatrix<T>::operator=(const dbMatrix<T>& rhs)
{
  if (this == &rhs) {
    return *this;
  }

  const auto lhs_shape = matrix_.shape();
  const auto rhs_shape = rhs.matrix_.shape();

  if (lhs_shape[0] != rhs_shape[0] || lhs_shape[1] != rhs_shape[1]) {
    std::vector<size_t> new_extents{rhs_shape[0], rhs_shape[1]};
    matrix_.resize(new_extents);
  }
  matrix_ = rhs.matrix_;

  return *this;
}

}  // namespace odb
