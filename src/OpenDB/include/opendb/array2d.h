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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "name.h"
#include "parse.h"
#include "util.h"

template <class T>
class Ath__array2d
{
 private:
  AthArray<T>** _array;
  unsigned int  _bankCnt;
  uint          _tmpBank;
  uint          _tmpCnt;
  uint          _current;
  bool          _doIteratorAll;

 public:
  Ath__array2d(
      unsigned int bCnt)  // for bCnt small and each bank= 10000 max, for quad
  {
    _bankCnt = 4;
    if (bCnt > 0)
      _bankCnt = bCnt;

    _array = new AthArray<T>*[_bankCnt];
    assert(_array);

    unsigned int ii;
    for (ii = 0; ii < bCnt; ii++) {
      _array[ii] = new AthArray<T>(128);
      assert(_array[ii]);
    }
    resetIteratorFlags();
  }
  void resetIteratorFlags()
  {
    _tmpBank       = 0;
    _tmpCnt        = 0;
    _current       = 0;
    _doIteratorAll = false;
  }
  Ath__array2d(int bCnt, bool)  // for max 1024 and 128, for quad
  {
    _bankCnt = 32;
    if (bCnt > 0)
      _bankCnt = bCnt;

    _array = new AthArray<T>*[_bankCnt];
    assert(_array);

    _array[0] = new AthArray<T>(3);
    assert(_array[0]);

    unsigned int halfCnt = _bankCnt / 2 + 1;

    unsigned int ii;
    for (ii = 1; ii < halfCnt; ii++) {
      _array[ii] = new AthArray<T>(64);
      assert(_array[ii]);
    }
    for (ii = halfCnt; ii < _bankCnt; ii++) {
      _array[ii] = new AthArray<T>(4);
      assert(_array[ii]);
    }
    resetIteratorFlags();
  }
  Ath__array2d(int bCnt, uint quarterSize1, uint quarterSize2, uint halfSize2)
  {
    _bankCnt = 32;
    if (bCnt > 0)
      _bankCnt = bCnt;

    _array = new AthArray<T>*[_bankCnt];
    assert(_array);

    _array[0] = new AthArray<T>(4);
    assert(_array[0]);

    unsigned int halfCnt    = _bankCnt / 2 + 1;
    unsigned int quarterCnt = halfCnt / 2 + 1;

    unsigned int ii;
    for (ii = 1; ii < quarterCnt; ii++) {
      _array[ii] = new AthArray<T>(quarterSize1);
      assert(_array[ii]);
    }
    for (ii = quarterCnt; ii < halfCnt; ii++) {
      _array[ii] = new AthArray<T>(quarterSize2);
      assert(_array[ii]);
    }
    for (ii = halfCnt; ii < _bankCnt; ii++) {
      _array[ii] = new AthArray<T>(halfSize2);
      assert(_array[ii]);
    }
    resetIteratorFlags();
  }

  ~Ath__array2d()
  {
    for (unsigned int ii = 0; ii < _bankCnt; ii++)
      delete _array[ii];
    delete _array;
  }
  uint getBankCnt() { return _bankCnt; }
  uint getCnt()
  {
    uint cnt = 0;
    for (unsigned int ii = 0; ii < _bankCnt; ii++)
      cnt += getCnt(ii);

    return cnt;
  }
  unsigned int add(unsigned int ii, T a)
  {
    if (ii >= _bankCnt)
      ii = _bankCnt - 1;

    return _array[ii]->add(a);
  }
  T            get(int ii, int jj) { return _array[ii]->get(jj); }
  unsigned int getCnt(unsigned int ii) { return _array[ii]->getLast(); }
  unsigned int startIterator(unsigned int ii)
  {
    _doIteratorAll = false;
    _tmpBank       = ii;
    _current       = 0;
    if (ii < _bankCnt)
      _tmpCnt = _array[ii]->getLast();
    else
      _tmpCnt = 0;

    return _tmpCnt;
  }
  unsigned int startIteratorAll()
  {
    uint cnt       = startIterator(0);
    _doIteratorAll = true;
    return cnt;
  }
  int getNext(T* a)
  {
    if (_current < _tmpCnt) {
      *a = _array[_tmpBank]->get(_current++);
      return _current;
    } else if (_doIteratorAll) {
      _tmpBank++;
      if (_tmpBank == _bankCnt)
        return 0;

      startIterator(_tmpBank);
      _doIteratorAll = true;

      return getNext(a);
    } else {
      return 0;
    }
  }

  void printCnt(FILE* fp, uint ii, char* msg)
  {
    fprintf(fp, "%6d %s\n", getCnt(ii), msg);
  }
  void printAllCnts(FILE* fp, char* indexMsg, char* msg)
  {
    for (unsigned int ii = 0; ii < _bankCnt; ii++) {
      if (getCnt(ii) <= 0)
        continue;
      fprintf(fp, "%s= %2d has ", indexMsg, ii);
      printCnt(fp, ii, msg);
    }
  }
};


