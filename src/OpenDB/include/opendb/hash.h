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

/*---------------------------------------------------------
//////		AUTHOR: SANJEEV MAHAJAN
------------------------------------------------------------*/
#pragma once

#include <string.h>
#include "darr.h"
typedef unsigned long int ub4;
typedef unsigned char     ub1;
#define FACTOR 1

#ifdef ATHENA_64BIT
#define HASH_LO_WORD(n) (0x00000000FFFFFFFFULL & ((unsigned long long) n));
#define HASH_HI_WORD(n) (((unsigned long long) n) >> 32);
#else
#define HASH_LO_WORD(n) (0x0000FFFFUL & ((unsigned long) n));
#define HASH_HI_WORD(n) (((unsigned long) n) >> 16);
#endif

#define hs(n) (1 << (n))
#define hm(n) (hs(n) - 1)
#define mix(a, b, c) \
  {                  \
    a -= b;          \
    a -= c;          \
    a ^= (c >> 13);  \
    b -= c;          \
    b -= a;          \
    b ^= (a << 8);   \
    c -= a;          \
    c -= b;          \
    c ^= (b >> 13);  \
    a -= b;          \
    a -= c;          \
    a ^= (c >> 12);  \
    b -= c;          \
    b -= a;          \
    b ^= (a << 16);  \
    c -= a;          \
    c -= b;          \
    c ^= (b >> 5);   \
    a -= b;          \
    a -= c;          \
    a ^= (c >> 3);   \
    b -= c;          \
    b -= a;          \
    b ^= (a << 10);  \
    c -= a;          \
    c -= b;          \
    c ^= (b >> 15);  \
  }

template <class Key, class Val>
class Keyval
{
 public:
  Key key;
  Val val;
};
// DON'T USE HashG! Use HashP for pairs of keys of size 4 bytes
// each and Hash for single keys with at most 8 bytes.
// Use HashN for name hashing.
template <class Key, class Val>
class HashG
{
 public:
  HashG();
  virtual ~HashG();
  void insert(Key key, Val val);
  int  find(Key key, Val& val);
  int  remove(Key key, Val& val);
  int  find_and_modify(Key key, Val& val, Val f(Val val));
  int  modify(Key key, Val val);
  void clear(void);
  int  size(void);
  int  n(void);
  void print();
  void begin();
  bool next(Key& key, Val& val);
  int  max_size();

 protected:
  int _lsz;

 private:
  Darr<Keyval<Key, Val>*>* _table;
  int                      _n;
  int                      _b;
  int                      _c;
  int                      _m;
  // dummy function
  virtual ub4 _map(Key /* unused: key*/) { return 1; }
  // dummy function
  virtual int equal(Key /* unused: key1*/, Key /* unused: key2*/) { return 1; }
};

template <class Key, class Val>
HashG<Key, Val>::HashG(void)
{
  _n     = 0;
  _lsz   = 0;
  _b     = 0;
  _c     = -1;
  _m     = 0;
  _table = new Darr<Keyval<Key, Val>*>[(1 << _lsz)];
}
template <class Key, class Val>
void HashG<Key, Val>::begin(void)
{
  _b = 0;
  _c = -1;
}
template <class Key, class Val>
int HashG<Key, Val>::max_size()
{
  return _m;
}
template <class Key, class Val>
bool HashG<Key, Val>::next(Key& key, Val& val)
{
  Keyval<Key, Val> keyval;
  if ((_b == (1 << _lsz) - 1) && (_c == _table[_b].n() - 1))
    return false;
  if (_c < _table[_b].n() - 1) {
    _c++;
    keyval = *(_table[_b].get(_c));
    key    = keyval.key;
    val    = keyval.val;
    return true;
  }
  _b++;
  _c = 0;
  while (_b < (1 << _lsz) && _table[_b].n() == 0)
    _b++;
  if (_b >= (1 << _lsz))
    return false;
  keyval = *(_table[_b].get(0));
  key    = keyval.key;
  val    = keyval.val;
  return true;
}
template <class Key, class Val>
HashG<Key, Val>::~HashG(void)
{
  int i;
  if (!_table)
    return;
  for (i = 0; i < (1 << _lsz); i++) {
    int j;
    for (j = 0; j < _table[i].n(); j++)
      delete _table[i].get(j);
  }
  if (_table)
    delete[] _table;
}
template <class Key, class Val>
void HashG<Key, Val>::clear(void)
{
  int i;
  if (!_table)
    return;
  for (i = 0; i < (1 << _lsz); i++) {
    int j;
    for (j = 0; j < _table[i].n(); j++) {
      delete _table[i].get(j);
    }
    if (_table[i].n() > 0)
      _table[i].reset();
  }
  _n = 0;
}
template <class Key, class Val>
void HashG<Key, Val>::insert(Key key, Val val)
{
  Keyval<Key, Val>* kv = new Keyval<Key, Val>;
  kv->key              = key;
  kv->val              = val;
  if (_n++ < FACTOR * (1 << _lsz)) {
    ub4 map = _map(key);
    _table[map].insert(kv);
    int mm = _table[map].n();
    if (mm > _m)
      _m = mm;
    return;
  }
  int i;
  _lsz += 1;
  Darr<Keyval<Key, Val>*>* tab = new Darr<Keyval<Key, Val>*>[(1 << _lsz)];
  for (i = 0; i < (1 << (_lsz - 1)); i++) {
    int j;
    for (j = 0; j < _table[i].n(); j++) {
      Keyval<Key, Val>* kv1 = _table[i].get(j);
      ub4               map = _map(kv1->key);
      tab[map].insert(kv1);
    }
  }
  delete[] _table;
  ub4 map = _map(key);
  tab[map].insert(kv);
  _m = 0;
  for (i = 0; i < (1 << (_lsz - 1)); i++) {
    if (tab[i].n() > _m)
      _m = tab[i].n();
  }
  _table = tab;
}
template <class Key, class Val>
int HashG<Key, Val>::find(Key key, Val& val)
{
  ub4 map = _map(key);
  int i;
  for (i = 0; i < _table[map].n(); i++) {
    Keyval<Key, Val>* kv = _table[map].get(i);
    if (equal(kv->key, key)) {
      val = kv->val;
      return 1;
    }
  }
  val = 0;
  return 0;
}
template <class Key, class Val>
int HashG<Key, Val>::remove(Key key, Val& val)
{
  ub4 map = _map(key);
  int i;
  for (i = 0; i < _table[map].n(); i++) {
    Keyval<Key, Val>* kv = _table[map].get(i);
    if (equal(kv->key, key)) {
      val = kv->val;
      _table[map].remove(i);
      return 1;
    }
  }
  val = 0;
  return 0;
}
template <class Key, class Val>
int HashG<Key, Val>::find_and_modify(Key key, Val& val, Val f(Val val))
{
  ub4 map = _map(key);
  int i;
  for (i = 0; i < _table[map].n(); i++) {
    Keyval<Key, Val>* kv = _table[map].get(i);
    if (equal(kv->key, key)) {
      (kv->val) = f(kv->val);
      val       = kv->val;
      return 1;
    }
  }
  val = 0;
  return 0;
}
template <class Key, class Val>
int HashG<Key, Val>::modify(Key key, Val val)
{
  ub4 map = _map(key);
  int i;
  for (i = 0; i < _table[map].n(); i++) {
    Keyval<Key, Val>* kv = _table[map].get(i);
    if (equal(kv->key, key)) {
      (kv->val) = val;
      return 1;
    }
  }
  return 0;
}

template <class Key, class Val>
int HashG<Key, Val>::size(void)
{
  return _lsz;
}
template <class Key, class Val>
int HashG<Key, Val>::n(void)
{
  return _n;
}
template <class Key, class Val>
void HashG<Key, Val>::print()
{
  int i;
  for (i = 0; i < (1 << _lsz); i++)
    printf("num elem is %d\n", _table[i].n());
}
template <class Key, class Val>
class HashP : public HashG<Keyval<Key, Key>, Val>
{
 private:
  typedef HashG<Keyval<Key, Key>, Val> base;
  ub4                                  _map(Keyval<Key, Key> key)
  {
    ub4 a, b, c;
    ub4 ikey1 = (ub4)(key.key);
    ub4 ikey2 = (ub4)(key.val);
    a = b = 0x7fffffff;
    c     = 0;
    a += ikey1;
    b += ikey2;
    mix(a, b, c);
    c = (c & (ub4)(hm(base::_lsz)));
    return c;
  }
  int equal(Keyval<Key, Key> key1, Keyval<Key, Key> key2)
  {
    if (key1.key != key2.key)
      return 0;
    if (key1.val != key2.val)
      return 0;
    return 1;
  }
};
template <class Key, class Val>
class Hash : public HashG<Key, Val>
{
 private:
  typedef HashG<Key, Val> base;

  ub4 _map(Key key)
  {
    ub4 a, b, c;

    // Per sanjiv, break key into hi/lo words...
    ub4 ikey1 = (ub4) HASH_LO_WORD(key);
    ub4 ikey2 = (ub4) HASH_HI_WORD(key);
    // golden ratio
    a = b = 0x7fffffff;
    c     = 0;
    a += ikey1;
    b += ikey2;
    mix(a, b, c);
    c = (c & hm(base::_lsz));
    return c;
  }
  int equal(Key key1, Key key2) { return (key1 == key2); }
};
template <class Val>
class HashN : public HashG<char*, Val>
{
 private:
  typedef HashG<char*, Val> base;
  ub4                       _map(char* key)
  {
    ub4 a, b, c, len, leng;
    a = b = 0x7fffffff;
    c     = 0;
    leng = len = strlen(key);
    char* k    = key;
    while (len >= 12) {
      a += k[0] + ((ub4) k[1] << 8) + ((ub4) k[2] << 16) + ((ub4) k[3] << 24);
      b += k[4] + ((ub4) k[5] << 8) + ((ub4) k[6] << 16) + ((ub4) k[7] << 24);
      c += k[8] + ((ub4) k[9] << 8) + ((ub4) k[10] << 16) + ((ub4) k[11] << 24);
      mix(a, b, c);
      k += 12;
      len -= 12;
    }
    c += leng;
    switch (len) {
      case 11:
        c += ((ub4) k[10] << 24);
      case 10:
        c += ((ub4) k[9] << 16);
      case 9:
        c += ((ub4) k[8] << 8);
      case 8:
        b += ((ub4) k[7] << 24);
      case 7:
        b += ((ub4) k[6] << 16);
      case 6:
        b += ((ub4) k[5] << 8);
      case 5:
        b += (ub4) k[4];
      case 4:
        a += ((ub4) k[3] << 24);
      case 3:
        a += ((ub4) k[2] << 16);
      case 2:
        a += ((ub4) k[1] << 8);
      case 1:
        a += (ub4) k[0];
    }
    mix(a, b, c);
    c = (c & hm(base::_lsz));
    return c;
  }
  int equal(char* key1, char* key2) { return !strcmp(key1, key2); }
};

