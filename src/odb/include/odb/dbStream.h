// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <map>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "boost/container/flat_map.hpp"
#include "odb/dbObject.h"

namespace odb {

class _dbDatabase;

inline constexpr size_t kTemplateRecursionLimit = 16;

class dbOStream
{
 public:
  using Position = std::ostream::pos_type;

  dbOStream(_dbDatabase* db, std::ostream& f);
  ~dbOStream()
  {
    try {
      flush();
    } catch (...) {
      // Ignore exceptions in destructor
    }
  }

  template <typename T>
    requires(std::is_trivially_copyable_v<T>)
  void writeValueAsBytes(const T& val)
  {
    writeBytes(
        std::span<const char>(reinterpret_cast<const char*>(&val), sizeof(T)));
  }

  void flush()
  {
    if (buffer_pos_ > 0) {
      f_.write(buffer_.data(), static_cast<std::streamsize>(buffer_pos_));
      buffer_pos_ = 0;
    }
  }

  _dbDatabase* getDatabase() { return db_; }

  void writeBytes(std::span<const char> bytes)
  {
    const char* data = bytes.data();
    const size_t len = bytes.size();

    if (len == 0) {
      return;
    }

    // Flush buffer if new data won't fit
    if (buffer_pos_ + len > kBufferSize) {
      flush();
    }

    // If payload exceeds entire buffer size, bypass buffering
    if (len > kBufferSize) {
      f_.write(data, static_cast<std::streamsize>(len));
    } else {
      std::memcpy(buffer_.data() + buffer_pos_, data, len);
      buffer_pos_ += len;
    }
  }

  template <typename T>
    requires(std::is_arithmetic_v<T>
             && !std::is_same_v<std::remove_cvref_t<T>, bool>)
  dbOStream& operator<<(const T& val)
  {
    writeValueAsBytes(val);
    return *this;
  }

  dbOStream& operator<<(bool c)
  {
    const unsigned char b = (c ? 1 : 0);
    return *this << b;
  }

  dbOStream& operator<<(std::string_view s)
  {
    *this << static_cast<uint32_t>(s.size() + 1);
    writeBytes(s);
    writeBytes({"\0", 1});
    return *this;
  }

  dbOStream& operator<<(const char* c)
  {
    if (c == nullptr) {
      *this << 0u;
    } else {
      *this << std::string_view(c);
    }
    return *this;
  }

  template <class T1, class T2>
  dbOStream& operator<<(const std::pair<T1, T2>& p)
  {
    *this << p.first;
    *this << p.second;
    return *this;
  }

  template <size_t I = 0, typename... Ts>
  constexpr dbOStream& operator<<(const std::tuple<Ts...>& tup)
  {
    static_assert(I <= kTemplateRecursionLimit,
                  "OpenROAD disallows of std::tuple larger than 16 "
                  "elements. You should look into alternate solutions");
    if constexpr (I == sizeof...(Ts)) {
      return *this;
    } else {
      *this << std::get<I>(tup);
      return ((*this).operator<< <I + 1>(tup));
    }
  }

  template <class T1, class T2>
  dbOStream& operator<<(const std::map<T1, T2>& m)
  {
    uint32_t sz = m.size();
    *this << sz;
    for (auto const& [key, val] : m) {
      *this << key;
      *this << val;
    }
    return *this;
  }

  template <class T1, class T2>
  dbOStream& operator<<(const boost::container::flat_map<T1, T2>& m)
  {
    uint32_t sz = m.size();
    *this << sz;
    for (auto const& [key, val] : m) {
      *this << key;
      *this << val;
    }
    return *this;
  }

  template <class T1, class T2>
  dbOStream& operator<<(const std::unordered_map<T1, T2>& m)
  {
    uint32_t sz = m.size();
    *this << sz;
    for (auto const& [key, val] : m) {
      *this << key;
      *this << val;
    }
    return *this;
  }

  template <class T1>
  dbOStream& operator<<(const std::vector<T1>& m)
  {
    uint32_t sz = m.size();
    *this << sz;
    for (const auto& val : m) {
      *this << val;
    }
    return *this;
  }

  template <class T, std::size_t SIZE>
  dbOStream& operator<<(const std::array<T, SIZE>& a)
  {
    for (auto& val : a) {
      *this << val;
    }
    return *this;
  }

  dbOStream& operator<<(const std::string& s)
  {
    return *this << std::string_view(s);
  }

  template <uint32_t I = 0, typename... Ts>
  dbOStream& operator<<(const std::variant<Ts...>& v)
  {
    static_assert(I <= kTemplateRecursionLimit,
                  "OpenROAD disallows of std::variants larger than 16 "
                  "elements. You should look into alternate solutions");
    if constexpr (I == sizeof...(Ts)) {
      return *this;
    } else {
      if (I == v.index()) {
        *this << (uint32_t) v.index();
        *this << std::get<I>(v);
      }
      return ((*this).operator<< <I + 1>(v));
    }
  }

  double lefarea(int value) { return ((double) value * lef_area_factor_); }
  double lefdist(int value) { return ((double) value * lef_dist_factor_); }

  Position pos()
  {
    flush();
    return f_.tellp();
  }

  void pushScope(const std::string& name);
  void popScope();

 private:
  struct Scope
  {
    std::string name;
    Position start_pos;
  };

  _dbDatabase* db_;
  std::ostream& f_;
  double lef_area_factor_;
  double lef_dist_factor_;
  std::vector<Scope> scopes_;
  static constexpr size_t kBufferSize = 65536;
  std::array<char, kBufferSize> buffer_;
  size_t buffer_pos_ = 0;
};

// RAII class for scoping ostream operations
class dbOStreamScope
{
 public:
  dbOStreamScope(dbOStream& ostream, const std::string& name)
      : ostream_(ostream)
  {
    ostream_.pushScope(name);
  }

  ~dbOStreamScope() { ostream_.popScope(); }

  dbOStream& ostream_;
};

class dbIStream
{
 public:
  dbIStream(_dbDatabase* db, std::istream& f);

  _dbDatabase* getDatabase() { return db_; }

  dbIStream& operator>>(bool& c)
  {
    unsigned char b;
    *this >> b;
    c = (b == 1);
    return *this;
  }

  template <typename T>
    requires(std::is_arithmetic_v<T>
             && !std::is_same_v<std::remove_cvref_t<T>, bool>)
  dbIStream& operator>>(T& val)
  {
    readValueAsBytes(val);
    return *this;
  }

  dbIStream& operator>>(char*& c)
  {
    int l;
    *this >> l;

    if (l == 0) {
      c = nullptr;
    } else {
      c = (char*) malloc(l);
      f_.read(c, l);
    }

    return *this;
  }

  template <class T1, class T2>
  dbIStream& operator>>(std::pair<T1, T2>& p)
  {
    *this >> p.first;
    *this >> p.second;
    return *this;
  }
  template <class T1, class T2>
  dbIStream& operator>>(std::map<T1, T2>& m)
  {
    uint32_t sz;
    *this >> sz;
    m.clear();
    for (uint32_t i = 0; i < sz; i++) {
      T1 key;
      T2 val;
      *this >> key;
      *this >> val;
      m[key] = std::move(val);
    }
    return *this;
  }
  template <class T1, class T2>
  dbIStream& operator>>(boost::container::flat_map<T1, T2>& m)
  {
    uint32_t sz;
    *this >> sz;
    m.clear();
    for (uint32_t i = 0; i < sz; i++) {
      T1 key;
      T2 val;
      *this >> key;
      *this >> val;
      m[key] = std::move(val);
    }
    return *this;
  }
  template <class T1, class T2>
  dbIStream& operator>>(std::unordered_map<T1, T2>& m)
  {
    uint32_t sz;
    *this >> sz;
    m.clear();
    for (uint32_t i = 0; i < sz; i++) {
      T1 key;
      T2 val;
      *this >> key;
      *this >> val;
      m[key] = std::move(val);
    }
    return *this;
  }

  template <class T1>
  dbIStream& operator>>(std::vector<T1>& m)
  {
    uint32_t sz;
    *this >> sz;
    m.clear();
    m.reserve(sz);
    for (uint32_t i = 0; i < sz; i++) {
      T1 val;
      *this >> val;
      m.push_back(std::move(val));
    }
    return *this;
  }

  template <class T, std::size_t SIZE>
  dbIStream& operator>>(std::array<T, SIZE>& a)
  {
    for (std::size_t i = 0; i < SIZE; i++) {
      *this >> a[i];
    }
    return *this;
  }

  template <size_t I = 0, typename... Ts>
  constexpr dbIStream& operator>>(std::tuple<Ts...>& tup)
  {
    static_assert(I <= kTemplateRecursionLimit,
                  "OpenROAD disallows of std::tuple larger than 16 "
                  "elements. You should look into alternate solutions");
    if constexpr (I == sizeof...(Ts)) {
      return *this;
    } else {
      *this >> std::get<I>(tup);
      return ((*this).operator>> <I + 1>(tup));
    }
  }

  dbIStream& operator>>(std::string& s)
  {
    uint32_t len = 0;
    *this >> len;
    if (len == 0) {
      s.clear();
      return *this;
    }
    s.resize(len);
    f_.read(s.data(), static_cast<std::streamsize>(len));
    s.pop_back();  // Strip trailing '\0'
    return *this;
  }

  template <typename... Ts>
  dbIStream& operator>>(std::variant<Ts...>& v)
  {
    uint32_t index = 0;
    *this >> index;
    return variantHelper(index, v);
  }

  double lefarea(int value) { return ((double) value * lef_area_factor_); }

  double lefdist(int value) { return ((double) value * lef_dist_factor_); }

 private:
  template <typename T>
    requires(std::is_trivially_copyable_v<T>)
  void readValueAsBytes(T& val)
  {
    f_.read(reinterpret_cast<char*>(&val), sizeof(T));
  }

  template <uint32_t I = 0, typename... Ts>
  dbIStream& variantHelper(uint32_t index, std::variant<Ts...>& v)
  {
    static_assert(I <= kTemplateRecursionLimit,
                  "OpenROAD disallows of std::variants larger than 16 "
                  "elements. You should look into alternate solutions");
    if constexpr (I == sizeof...(Ts)) {
      return *this;
    } else {
      if (I == index) {
        std::variant_alternative_t<I, std::variant<Ts...>> val;
        *this >> val;
        v = std::move(val);
      }
      return (*this).variantHelper<I + 1>(index, v);
    }
  }

  std::istream& f_;
  _dbDatabase* db_;
  double lef_area_factor_;
  double lef_dist_factor_;
};

}  // namespace odb
