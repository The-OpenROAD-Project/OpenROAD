// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

namespace utl {

inline int readEnvarInt(const char* name, const int default_value)
{
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return default_value;
  }

  size_t parsed_chars = 0;
  const int parsed = std::stoi(value, &parsed_chars, 10);
  if (parsed_chars != std::strlen(value) || parsed <= 0) {
    throw std::runtime_error(std::string("Environment variable ") + name
                             + " must be a positive integer.");
  }
  return parsed;
}

}  // namespace utl
