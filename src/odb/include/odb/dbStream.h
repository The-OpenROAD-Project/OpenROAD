// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <tuple>
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

  _dbDatabase* getDatabase() { return db_; }

  dbOStream& operator<<(bool c)
  {
    unsigned char b = (c == true ? 1 : 0);
    return *this << b;
  }

  dbOStream& operator<<(char c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(unsigned char c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(int16_t c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(uint16_t c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(int c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(uint64_t c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(unsigned int c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(int8_t c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(float c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(double c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(long double c)
  {
    writeValueAsBytes(c);
    return *this;
  }

  dbOStream& operator<<(const char* c)
  {
    if (c == nullptr) {
      *this << 0;
    } else {
      int l = strlen(c) + 1;
      *this << l;
      f_.write(c, l);
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
    for (auto val : m) {
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
    char* tmp = strdup(s.c_str());
    *this << tmp;
    free((void*) tmp);
    return *this;
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

  Position pos() const { return f_.tellp(); }

  void pushScope(const std::string& name);
  void popScope();

 private:
  struct Scope
  {
    std::string name;
    Position start_pos;
  };

  // By default values are written as their string ("255" vs 0xFF)
  // representations when using the << stream method. In dbOstream we are
  // primarly writing the byte representation which the below accomplishes.
  template <typename T>
  void writeValueAsBytes(T type)
  {
    f_.write(reinterpret_cast<char*>(&type), sizeof(T));
  }

  _dbDatabase* db_;
  std::ostream& f_;
  double lef_area_factor_;
  double lef_dist_factor_;
  std::vector<Scope> scopes_;
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
    c = (b == 1 ? true : false);
    return *this;
  }

  dbIStream& operator>>(char& c)
  {
    f_.read(&c, sizeof(c));
    return *this;
  }

  dbIStream& operator>>(unsigned char& c)
  {
    f_.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(int16_t& c)
  {
    f_.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(uint16_t& c)
  {
    f_.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(int& c)
  {
    f_.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(uint64_t& c)
  {
    f_.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(unsigned int& c)
  {
    f_.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(int8_t& c)
  {
    f_.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(float& c)
  {
    f_.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(double& c)
  {
    f_.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(long double& c)
  {
    f_.read(reinterpret_cast<char*>(&c), sizeof(c));
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
    char* tmp;
    *this >> tmp;
    if (!tmp) {
      s = "";
      return *this;
    }
    s = std::string(tmp);
    free((void*) tmp);
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
