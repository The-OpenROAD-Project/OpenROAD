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

#include <new>

#include "odb.h"

#ifdef ODB_PURIFY
#define ODB_PURIFY_ALLOCATOR
#endif

namespace odb {

///
/// Simple memory pool.
///
/// This is a replacement for malloc and free when
/// you need to maintain a large and dynamic pool of objects.
///
/// NOTE: The memory used by this pool is only released when the pool is
///       destroyed.
///
/// There are two usage alternatives when using C++ classes.
///
/// 1) You can apply a specific constructor by doing the folowing:
///
///      class Foo {};
///
///      Allocator<Foo> alloc;
///      Foo * p = new(alloc.malloc()) Foo;
///      ...
///      p->~Foo();
///      alloc.free(p);
///
/// 2) Or you can use the create() and destroy() methods.
///    The create() method will call the default constructor.
///    The destroy() method will call the destructor. The example above can
///    be recoded as:
///
///      Allocator<Foo> alloc;
///      Foo * p = alloc.create();
///      alloc.destroy(p);
///
/// DO NOT CALL DELETE ON a POINTER RETURNED FROM THE ALLOCATOR
///
///      delete p;    // ILLEGAL
///
template <class T>
class Allocator
{
  struct chunk
  {
    chunk* _next;
  };

  struct block
  {
    void*  _chunk;
    block* _next;
  };

  block* _block_list;
  int    _block_size;
  uint   _size;
  uint   _vm_size;
  chunk* _free_list;
  void   new_block();

 public:
  // construct an allocator, block-size = number of object reserved per block
  // allocation
  Allocator(int block_size = 128);

  // destroy the allocator, all memory is free'ed...
  ~Allocator();

  // clear the pool, release all memory and resets the pool to it's initial
  // state.
  void clear();

  // malloc a new object
  T* malloc();

  // free an object
  void free(T*);

  // Create a new object and call the default constructor
  T* create();

  // Destroy an object and call the destructor
  void destroy(T*);

  uint size() const { return _size; }

  uint vm_size() const;
};

template <class T>
inline Allocator<T>::Allocator(int block_size)
{
  _block_size = block_size;
  _free_list  = NULL;
  _block_list = NULL;
  _size       = 0;
  _vm_size    = 0;
}

template <class T>
inline Allocator<T>::~Allocator()
{
  clear();
}

template <class T>
inline uint Allocator<T>::vm_size() const
{
  return _vm_size;
}

template <class T>
inline void Allocator<T>::clear()
{
  block* b    = _block_list;
  block* next = NULL;

  for (; b; b = next) {
    next = b->_next;
    ::free((void*) b->_chunk);
    ::free((void*) b);
  }

  _free_list  = NULL;
  _block_list = NULL;
}

template <class T>
inline T* Allocator<T>::malloc()
{
#ifdef ODB_PURIFY_ALLOCATOR
  _size += sizeof(T);
  return (T*) ::malloc(sizeof(T));
#else
  if (_free_list == NULL)
    new_block();

  chunk* c   = _free_list;
  _free_list = c->_next;
  return (T*) c;
#endif
}

template <class T>
inline T* Allocator<T>::create()
{
  T* t = Allocator<T>::malloc();
  new (t) T;
  return t;
}

template <class T>
inline void Allocator<T>::free(T* t)
{
#ifdef ODB_PURIFY_ALLOCATOR
  _size -= sizeof(T);
  ::free((void*) t);
#else

  chunk* c   = (chunk*) t;
  c->_next   = _free_list;
  _free_list = c;
#endif
}

template <class T>
inline void Allocator<T>::destroy(T* t)
{
  t->~T();
  Allocator<T>::free(t);
}

template <class T>
inline void Allocator<T>::new_block()
{
  int obj_size = sizeof(T) < sizeof(chunk) ? sizeof(chunk) : sizeof(T);
  _size += sizeof(block);
  _size += _block_size * obj_size;

  block* b = (block*) ::malloc(sizeof(block));
  assert(b);
  b->_chunk = ::malloc(obj_size * _block_size);
  assert(b->_chunk);
  _vm_size += sizeof(block);
  _vm_size += obj_size * _block_size;

  char* base = (char*) b->_chunk;
  char* end  = base + obj_size * _block_size;
  char* c;

  for (c = end - obj_size; c >= base; c -= obj_size) {
    chunk* ck  = (chunk*) c;
    ck->_next  = _free_list;
    _free_list = ck;
  }

  b->_next    = _block_list;
  _block_list = b;
}

}  // namespace odb


