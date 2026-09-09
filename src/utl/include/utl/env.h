// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cctype>
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
  if (parsed_chars != std::strlen(value)) {
    throw std::runtime_error(std::string("Environment variable ") + name
                             + " must be an integer.");
  }
  return parsed;
}

// Like readEnvarInt, but rejects negative values.
inline int readEnvarNonNegativeInt(const char* name, const int default_value)
{
  const int value = readEnvarInt(name, default_value);
  if (value < 0) {
    throw std::runtime_error(std::string("Environment variable ") + name
                             + " must be a non-negative integer.");
  }
  return value;
}

inline float readEnvarFloat(const char* name, const float default_value)
{
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return default_value;
  }

  size_t parsed_chars = 0;
  const float parsed = std::stof(value, &parsed_chars);
  if (parsed_chars != std::strlen(value)) {
    throw std::runtime_error(std::string("Environment variable ") + name
                             + " must be a floating-point number.");
  }
  return parsed;
}

inline double readEnvarDouble(const char* name, const double default_value)
{
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return default_value;
  }

  size_t parsed_chars = 0;
  const double parsed = std::stod(value, &parsed_chars);
  if (parsed_chars != std::strlen(value)) {
    throw std::runtime_error(std::string("Environment variable ") + name
                             + " must be a floating-point number.");
  }
  return parsed;
}

// Accepts 0/1, true/false, on/off, yes/no (case-insensitive).
inline bool readEnvarBool(const char* name, const bool default_value)
{
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return default_value;
  }

  std::string text(value);
  std::ranges::transform(
      text, text.begin(), [](unsigned char c) { return std::tolower(c); });
  if (text == "1" || text == "true" || text == "on" || text == "yes") {
    return true;
  }
  if (text == "0" || text == "false" || text == "off" || text == "no") {
    return false;
  }
  throw std::runtime_error(
      std::string("Environment variable ") + name
      + " must be a boolean (0/1, true/false, on/off, yes/no).");
}

}  // namespace utl
