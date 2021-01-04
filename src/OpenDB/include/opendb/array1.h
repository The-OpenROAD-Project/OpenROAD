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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

template <class T>
class Ath__array1D
{
 private:
  T*  _array;
  int _size;
  int _chunk;
  int _current;
  int _iterCnt;

 public:
  Ath__array1D(int chunk = 0)
  {
    _chunk = chunk;
    if (_chunk <= 0)
      _chunk = 1024;

    _current = 0;
    if (chunk > 0) {
      _size  = _chunk;
      _array = (T*) realloc(NULL, _size * sizeof(T));
    } else {
      _size  = 0;
      _array = NULL;
    }
    _iterCnt = 0;
  }
  ~Ath__array1D()
  {
    if (_array != NULL)
      ::free(_array);
  }
  int add(T t)
  {
    if (_current >= _size) {
      _size += _chunk;
      _array = (T*) realloc(_array, _size * sizeof(T));
    }
    int n              = _current;
    _array[_current++] = t;

    return n;
  }
  int reSize(int maxSize)
  {
    if (maxSize < _size)
      return _size;

    _size  = (maxSize / _chunk + 1) * _chunk;
    _array = (T*) realloc(_array, _size * sizeof(T));

    if (_array == NULL) {
      fprintf(stderr, "Cannot allocate array of size %d\n", _size);
      assert(0);
    }
    return _size;
  }
  T* getTable() { return _array; }
  T& get(int i)
  {
    assert((i >= 0) && (i < _current));

    return _array[i];
  }
  T& geti(int i)
  {
    if (i >= _size)
      reSize(i + 1);

    return _array[i];
  }
  T& getLast()
  {
    assert(_current - 1 >= 0);

    return _array[_current - 1];
  }
  void clear(T t)
  {
    for (int ii = 0; ii < _size; ii++)
      _array[ii] = t;
  }
  int findIndex(T t)
  {
    for (int ii = 0; ii < _current; ii++) {
      if (_array[ii] == t) {
        return ii;
      }
    }

    return -1;
  }
  int findNextBiggestIndex(T t, int start = 0)
  {
    for (int ii = start; ii < _current; ii++) {
      if (t == _array[ii])
        return ii;
      if (t < _array[ii])
        return ii > 0 ? ii - 1 : 0;
    }
    return _current;
  }

  bool notEmpty()
  {
    if (_current > 0)
      return true;
    else
      return false;
  }
  bool getNext(T& a)
  {
    if (_iterCnt < _current) {
      a = _array[_iterCnt++];
      return true;
    } else
      return false;
  }
  T& pop()
  {
    assert(_current > 0);

    _current--;

    return _array[_current];
  }
  unsigned int getSize(void) { return _size; }
  void         resetIterator(unsigned int v = 0) { _iterCnt = v; }
  void         resetCnt(unsigned int v = 0)
  {
    _current = v;
    _iterCnt = v;
  }
  unsigned int getCnt(void) { return _current; }
  void         set(int ii, T t)
  {
    if (ii >= _size)
      reSize(ii + 1);

    _array[ii] = t;
  }
};


