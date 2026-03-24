// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace utl {

// Various helpers for parsing the keys & flags from TCL commands.

// Set field to value if name found in flags
inline void checkFlag(const std::map<std::string, std::string>& flags,
                      const char* name,
                      bool& field,
                      const bool value = true)
{
  if (auto it = flags.find(name); it != flags.end()) {
    field = value;
  }
}

// Call func(value) if name found in flags
inline void checkFlag(const std::map<std::string, std::string>& flags,
                      const char* name,
                      const std::function<void(bool)>& func,
                      const bool value = true)
{
  if (auto it = flags.find(name); it != flags.end()) {
    func(value);
  }
}

// Provides string to T conversion.  Specialize for any values actually used.
template <typename T>
void convert(const std::string& str, T& field) = delete;

// String to int
template <>
inline void convert(const std::string& str, int& field)
{
  field = std::stoi(str);
}

// String to float
template <>
inline void convert(const std::string& str, float& field)
{
  field = std::stof(str);
}

// String to vector<T>
template <typename T>
inline void convert(const std::string& str, std::vector<T>& field)
{
  field.clear();
  std::istringstream stream(str);
  T val;
  while (stream >> val) {
    field.push_back(val);
  }
}

// String to tuple
template <typename... Ts>
inline void convert(const std::string& str, std::tuple<Ts...>&& tup)
{
  std::istringstream stream(str);

  std::apply([&](auto&... elems) { ((stream >> elems), ...); }, tup);
}

// Set field to the value converted from string to the result type
template <typename T>
inline void checkKey(const std::map<std::string, std::string>& keys,
                     const char* name,
                     T&& field)
{
  if (auto it = keys.find(name); it != keys.end()) {
    convert(it->second, std::forward<T>(field));
  }
}

}  // namespace utl
