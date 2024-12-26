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

#include <array>
#include <cstdint>
#include <cstring>
#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ZException.h"
#include "dbObject.h"
#include "odb.h"

namespace odb {

class _dbDatabase;

inline constexpr size_t kTemplateRecursionLimit = 16;

class dbOStream
{
  using Position = std::ostream::pos_type;
  struct Scope
  {
    std::string name;
    Position start_pos;
  };

  _dbDatabase* _db;
  std::ostream& _f;
  double _lef_area_factor;
  double _lef_dist_factor;
  std::vector<Scope> _scopes;

  // By default values are written as their string ("255" vs 0xFF)
  // representations when using the << stream method. In dbOstream we are
  // primarly writing the byte representation which the below accomplishes.
  template <typename T>
  void writeValueAsBytes(T type)
  {
    _f.write(reinterpret_cast<char*>(&type), sizeof(T));
  }

 public:
  dbOStream(_dbDatabase* db, std::ostream& f);

  _dbDatabase* getDatabase() { return _db; }

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
      _f.write(c, l);
    }

    return *this;
  }

  dbOStream& operator<<(dbObjectType c)
  {
    writeValueAsBytes(c);
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
    uint sz = m.size();
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
    uint sz = m.size();
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
    uint sz = m.size();
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

  double lefarea(int value) { return ((double) value * _lef_area_factor); }
  double lefdist(int value) { return ((double) value * _lef_dist_factor); }

  Position pos() const { return _f.tellp(); }

  void pushScope(const std::string& name);
  void popScope();
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
  std::istream& _f;
  _dbDatabase* _db;
  double _lef_area_factor;
  double _lef_dist_factor;

 public:
  dbIStream(_dbDatabase* db, std::istream& f);

  _dbDatabase* getDatabase() { return _db; }

  dbIStream& operator>>(bool& c)
  {
    unsigned char b;
    *this >> b;
    c = (b == 1 ? true : false);
    return *this;
  }

  dbIStream& operator>>(char& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(unsigned char& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(int16_t& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(uint16_t& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(int& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(uint64_t& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(unsigned int& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(int8_t& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(float& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(double& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
    return *this;
  }

  dbIStream& operator>>(long double& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
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
      _f.read(reinterpret_cast<char*>(c), l);
    }

    return *this;
  }

  dbIStream& operator>>(dbObjectType& c)
  {
    _f.read(reinterpret_cast<char*>(&c), sizeof(c));
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
    uint sz;
    *this >> sz;
    for (uint i = 0; i < sz; i++) {
      T1 key;
      T2 val;
      *this >> key;
      *this >> val;
      m[key] = val;
    }
    return *this;
  }
  template <class T1, class T2>
  dbIStream& operator>>(std::unordered_map<T1, T2>& m)
  {
    uint sz;
    *this >> sz;
    for (uint i = 0; i < sz; i++) {
      T1 key;
      T2 val;
      *this >> key;
      *this >> val;
      m[key] = val;
    }
    return *this;
  }

  template <class T1>
  dbIStream& operator>>(std::vector<T1>& m)
  {
    uint sz;
    *this >> sz;
    m.reserve(sz);
    for (uint i = 0; i < sz; i++) {
      T1 val;
      *this >> val;
      m.push_back(val);
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
      return ((*this).operator>><I + 1>(tup));
    }
  }

  dbIStream& operator>>(std::string& s)
  {
    char* tmp;
    *this >> tmp;
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

  double lefarea(int value) { return ((double) value * _lef_area_factor); }

  double lefdist(int value) { return ((double) value * _lef_dist_factor); }

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
        v = val;
      }
      return (*this).variantHelper<I + 1>(index, v);
    }
  }
};

}  // namespace odb
