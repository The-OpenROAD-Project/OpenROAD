// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstdlib>
#include <string>

namespace utl {

inline unsigned readEnvarUint(const char* name, const unsigned default_value)
{
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return default_value;
  }

  try {
    const unsigned parsed = static_cast<unsigned>(std::stoul(value));
    return parsed > 0 ? parsed : default_value;
  } catch (...) {
    return default_value;
  }
}

}  // namespace utl
