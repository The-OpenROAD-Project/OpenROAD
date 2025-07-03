// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <cstdlib>
#include <cstring>
#include <new>

namespace odb {

template <typename T>
inline unsigned int flagsToUInt(T* obj)
{
  // Safe type punning
  static_assert(sizeof(obj->_flags) == 4, "flags size != 4");
  unsigned int i;
  std::memcpy(&i, &obj->_flags, sizeof(obj->_flags));
  return i;
}

inline char* safe_strdup(const char* s)
{
  char* p = strdup(s);
  if (p == nullptr) {
    throw std::bad_alloc();
  }
  return p;
}

inline void* safe_malloc(const size_t size)
{
  void* p = malloc(size);
  if (p == nullptr) {
    throw std::bad_alloc();
  }
  return p;
}

}  // namespace odb
