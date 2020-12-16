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

/*-------------------------------------------------------------
////	AUTHOR: SANJEEV MAHAJAN
---------------------------------------------------------------*/
#include <stdlib.h>
#include "assert.h"
#pragma once

template <class T>
class Darr
{
 public:
  Darr(int n = 0);
  ~Darr(void);
  void insert(T el);
  void remove(int i);
  T    pop();
  T    get(int i);
  void set(int i, T el);
  int  n();
  void dsort(int cmp(const void* a, const void* b));
  void reset(int n = 0);

 private:
  T*  _ar;
  int _n;
  int _tot;
};

template <class T>
Darr<T>::Darr(int n)
{
  _n   = 0;
  _tot = n;
  if (n == 0) {
    _ar = NULL;
    return;
  }
  _ar = new T[_tot];
}
template <class T>
void Darr<T>::insert(T el)
{
  if (_n < _tot) {
    _ar[_n++] = el;
    return;
  }
  _tot    = 2 * _tot + 1;
  T*  ara = new T[_tot];
  int i;
  for (i = 0; i < _n; i++)
    ara[i] = _ar[i];
  ara[_n++] = el;
  if (_ar)
    delete[] _ar;
  _ar = ara;
  return;
}
template <class T>
void Darr<T>::remove(int i)
{
  assert(i < _n);
  assert(i >= 0);
  int j;
  for (j = i + 1; j < _n; j++)
    _ar[j - 1] = _ar[j];
  _n--;
}
template <class T>
T Darr<T>::pop()
{
  assert(_n > 0);
  _n--;
  return _ar[_n];
}

template <class T>
T Darr<T>::get(int i)
{
  assert(i < _n);
  assert(i >= 0);
  return _ar[i];
}
template <class T>
void Darr<T>::set(int i, T el)
{
  assert(i < _n);
  assert(i >= 0);
  _ar[i] = el;
}
template <class T>
void Darr<T>::reset(int n)
{
  if (_n > n)
    _n = n;
  assert(_n <= _tot);
}
template <class T>
int Darr<T>::n(void)
{
  return _n;
}
template <class T>
Darr<T>::~Darr(void)
{
  if (_ar)
    delete[] _ar;
}
template <class T>
void Darr<T>::dsort(int cmp(const void* a, const void* b))
{
  qsort(_ar, _n, sizeof(T), cmp);
}

