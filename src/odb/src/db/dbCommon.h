// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include <cstring>

namespace odb {

template <typename T>
inline uint flagsToUInt(T* obj)
{
  // Safe type punning
  static_assert(sizeof(obj->_flags) == 4, "flags size != 4");
  uint i;
  std::memcpy(&i, &obj->_flags, sizeof(obj->_flags));
  return i;
}

}  // namespace odb
