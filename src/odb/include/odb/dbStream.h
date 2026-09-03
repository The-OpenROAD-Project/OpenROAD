// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
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

template <typename T>
concept MapContainer = requires(T m, const T cm) {
  typename T::key_type;
  typename T::mapped_type;
  cm.size();
  m.clear();
  cm.begin();
  cm.end();
  // Ensure the container supports emplace_hint with an iterator
  m.emplace_hint(m.end(),
                 std::declval<typename T::key_type>(),
                 std::declval<typename T::mapped_type>());
};

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

  template <typename... Ts>
    requires(... && std::is_trivially_copyable_v<Ts>)
  void writeValues(const Ts&... vals)
  {
    constexpr size_t kTotal = (0 + ... + sizeof(Ts));
    static_assert(kTotal <= kBufferSize);
    if constexpr (kTotal > 0) {
      if (buffer_pos_ + kTotal > kBufferSize) {
        flush();
      }
      char* p = buffer_.data() + buffer_pos_;
      ((std::memcpy(p, std::addressof(vals), sizeof(Ts)), p += sizeof(Ts)),
       ...);
      buffer_pos_ += kTotal;
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

  template <typename... Ts>
  constexpr dbOStream& operator<<(const std::tuple<Ts...>& tup)
  {
    std::apply([this](const auto&... args) { ((*this << args), ...); }, tup);
    return *this;
  }

  template <MapContainer Map>
  dbOStream& operator<<(const Map& m)
  {
    const uint32_t sz = m.size();
    *this << sz;
    for (const auto& [key, val] : m) {
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

#if defined(__cpp_exceptions) && __cpp_exceptions
class ScopedExceptionToggle
{
 public:
  explicit ScopedExceptionToggle(std::istream& is)
      : is_(is), old_exceptions_(is.exceptions()), restored_(false)
  {
    std::ios_base::iostate mask
        = std::ios_base::failbit | std::ios_base::eofbit;
    toggled_ = (old_exceptions_ & mask) != 0;
    if (toggled_) {
      is_.exceptions(old_exceptions_ & ~mask);
    }
  }

  // Explicitly restore exceptions so they can throw safely outside the
  // destructor
  void restore()
  {
    if (toggled_ && !restored_) {
      restored_ = true;
      is_.exceptions(old_exceptions_);
    }
  }

  ~ScopedExceptionToggle()
  {
    if (toggled_ && !restored_) {
      try {
        is_.exceptions(old_exceptions_);
      } catch (...) {
        // Fallback: Ignore exceptions in destructor during stack unwinding
        // to prevent std::terminate
      }
    }
  }

 private:
  std::istream& is_;
  std::ios_base::iostate old_exceptions_;
  bool toggled_;
  bool restored_;
};
#endif

class dbIStream
{
 public:
  dbIStream(_dbDatabase* db, std::istream& f);

  _dbDatabase* getDatabase() { return db_; }

  void read_bytes(std::span<char> bytes)
  {
    char* data = bytes.data();
    size_t len = bytes.size();

    // 1. Consume what is already in the buffer
    if (buffer_pos_ < buffer_size_) {
      size_t chunk = std::min(len, buffer_size_ - buffer_pos_);
      std::memcpy(data, buffer_.data() + buffer_pos_, chunk);
      data += chunk;
      len -= chunk;
      buffer_pos_ += chunk;
    }

    if (len == 0) {
      return;
    }

    // 2. Read remaining payload
    if (len >= kBufferSize) {
      // Unbuffered fast-path
      if (eof_reached_) {
        f_.setstate(std::ios_base::eofbit | std::ios_base::failbit);
        return;
      }
#if defined(__cpp_exceptions) && __cpp_exceptions
      ScopedExceptionToggle toggle(f_);
#endif
      f_.read(data, static_cast<std::streamsize>(len));
      if (f_.eof()) {
        eof_reached_ = true;
      }
#if defined(__cpp_exceptions) && __cpp_exceptions
      toggle.restore();
#endif
    } else {
      // Buffered path
#if defined(__cpp_exceptions) && __cpp_exceptions
      ScopedExceptionToggle toggle(f_);
#endif
      if (!refill_buffer()) {
        f_.setstate(std::ios_base::eofbit | std::ios_base::failbit);
#if defined(__cpp_exceptions) && __cpp_exceptions
        toggle.restore();
#endif
        return;
      }

      // Clear failbit/eofbit if we refilled enough to satisfy the current
      // request
      if (buffer_size_ >= len) {
        std::ios_base::iostate mask
            = std::ios_base::failbit | std::ios_base::eofbit;
        f_.clear(f_.rdstate() & ~mask);
      }

      size_t chunk = std::min(len, buffer_size_);
      std::memcpy(data, buffer_.data(), chunk);
      buffer_pos_ = chunk;
#if defined(__cpp_exceptions) && __cpp_exceptions
      toggle.restore();
#endif
    }
  }

  template <typename... Ts>
    requires(... && std::is_trivially_copyable_v<Ts>)
  void readValues(Ts&... vals)
  {
    constexpr size_t kTotal = (0 + ... + sizeof(Ts));
    static_assert(kTotal <= kBufferSize);
    if constexpr (kTotal > 0) {
      char temp[kTotal];
      read_bytes(std::span<char>(temp, kTotal));
      const char* p = temp;
      ((std::memcpy(std::addressof(vals), p, sizeof(Ts)), p += sizeof(Ts)),
       ...);
    }
  }

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
      read_bytes(std::span<char>(c, l));
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
  template <MapContainer Map>
  dbIStream& operator>>(Map& m)
  {
    uint32_t sz = 0;
    *this >> sz;
    m.clear();
    using Key = typename Map::key_type;
    using Value = typename Map::mapped_type;
    for (uint32_t i = 0; i < sz; i++) {
      Key key;
      Value val;
      *this >> key;
      *this >> val;
      m.emplace_hint(m.end(), std::move(key), std::move(val));
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

  template <typename... Ts>
  constexpr dbIStream& operator>>(std::tuple<Ts...>& tup)
  {
    std::apply([this](auto&... args) { ((*this >> args), ...); }, tup);
    return *this;
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
    read_bytes(std::span<char>(s.data(), len));
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
    read_bytes(std::span<char>(reinterpret_cast<char*>(&val), sizeof(T)));
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

  // Refills the internal buffer. Returns false if EOF is reached and no data
  // was read.
  bool refill_buffer()
  {
    if (eof_reached_) {
      return false;
    }
    f_.read(buffer_.data(), kBufferSize);
    buffer_size_ = f_.gcount();
    buffer_pos_ = 0;
    if (f_.eof()) {
      eof_reached_ = true;
    }
    return buffer_size_ > 0;
  }

  std::istream& f_;
  _dbDatabase* db_;
  double lef_area_factor_;
  double lef_dist_factor_;
  static constexpr size_t kBufferSize = 65536;
  std::array<char, kBufferSize> buffer_;
  size_t buffer_pos_ = 0;
  size_t buffer_size_ = 0;
  bool eof_reached_ = false;
};

}  // namespace odb
