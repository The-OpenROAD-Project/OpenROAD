// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstdio>
#include <string>
#include <utility>

namespace web {

// Escape a string for safe inclusion in a JSON string value.
// Handles: \\ \" \n \r \t and control characters below 0x20.
inline std::string json_escape(const std::string& s)
{
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8];
          std::snprintf(
              buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          out += buf;
        } else {
          out += c;
        }
        break;
    }
  }
  return out;
}

// Lightweight streaming JSON builder.
//
// Usage:
//   JsonBuilder builder;
//   builder.beginObject();
//   builder.field("name", some_string);
//   builder.field("count", 42);
//   builder.field("active", true);
//   builder.beginArray("items");
//     builder.beginObject();
//     builder.field("id", 1);
//     builder.endObject();
//   builder.endArray();
//   builder.endObject();
//   std::string json = builder.str();
//
class JsonBuilder
{
 public:
  JsonBuilder() { buf_.reserve(256); }

  // -- Containers --

  void beginObject()
  {
    maybeComma();
    buf_ += '{';
    pushContext();
  }

  void beginObject(const char* key)
  {
    writeKey(key);
    buf_ += '{';
    pushContext();
  }
  void beginObject(const std::string& key) { beginObject(key.c_str()); }

  void endObject()
  {
    buf_ += '}';
    popContext();
  }

  void beginArray()
  {
    maybeComma();
    buf_ += '[';
    pushContext();
  }

  void beginArray(const char* key)
  {
    writeKey(key);
    buf_ += '[';
    pushContext();
  }
  void beginArray(const std::string& key) { beginArray(key.c_str()); }

  void endArray()
  {
    buf_ += ']';
    popContext();
  }

  // -- Named fields (inside objects) --

  void field(const char* key, const std::string& val)
  {
    writeKey(key);
    writeString(val);
  }
  void field(const std::string& key, const std::string& val)
  {
    field(key.c_str(), val);
  }

  void field(const char* key, const char* val)
  {
    writeKey(key);
    writeString(val);
  }

  void field(const char* key, int val)
  {
    writeKey(key);
    buf_ += std::to_string(val);
  }

  void field(const char* key, float val)
  {
    writeKey(key);
    writeFloat(val);
  }

  void field(const char* key, double val)
  {
    writeKey(key);
    writeDouble(val);
  }

  void field(const char* key, bool val)
  {
    writeKey(key);
    buf_ += val ? "true" : "false";
  }

  void field(const std::string& key, int val) { field(key.c_str(), val); }
  void field(const std::string& key, float val) { field(key.c_str(), val); }
  void field(const std::string& key, double val) { field(key.c_str(), val); }
  void field(const std::string& key, bool val) { field(key.c_str(), val); }

  // -- Unnamed values (inside arrays) --

  void value(const std::string& val)
  {
    maybeComma();
    writeString(val);
  }

  void value(const char* val)
  {
    maybeComma();
    writeString(val);
  }

  void value(int val)
  {
    maybeComma();
    buf_ += std::to_string(val);
  }

  void value(float val)
  {
    maybeComma();
    writeFloat(val);
  }

  void value(double val)
  {
    maybeComma();
    writeDouble(val);
  }

  void value(bool val)
  {
    maybeComma();
    buf_ += val ? "true" : "false";
  }

  // -- Output --

  const std::string& str() const { return buf_; }
  std::string&& take() { return std::move(buf_); }

 private:
  static constexpr int kMaxDepth = 16;
  bool need_comma_[kMaxDepth];
  int depth_ = 0;

  std::string buf_;

  void pushContext()
  {
    assert(depth_ < kMaxDepth);
    need_comma_[depth_] = false;
    depth_++;
  }

  void popContext()
  {
    depth_--;
    if (depth_ > 0) {
      need_comma_[depth_ - 1] = true;
    }
  }

  void maybeComma()
  {
    if (depth_ > 0 && need_comma_[depth_ - 1]) {
      buf_ += ", ";
    }
    if (depth_ > 0) {
      need_comma_[depth_ - 1] = true;
    }
  }

  void writeKey(const char* key)
  {
    maybeComma();
    buf_ += '"';
    buf_ += json_escape(key);
    buf_ += "\": ";
  }

  void writeString(const std::string& s)
  {
    buf_ += '"';
    buf_ += json_escape(s);
    buf_ += '"';
  }

  void writeString(const char* s)
  {
    buf_ += '"';
    buf_ += json_escape(s);
    buf_ += '"';
  }

  void writeFloat(float val)
  {
    char buf[32];
    int n = std::snprintf(buf, sizeof(buf), "%g", static_cast<double>(val));
    buf_.append(buf, n);
  }

  void writeDouble(double val)
  {
    char buf[32];
    int n = std::snprintf(buf, sizeof(buf), "%g", val);
    buf_.append(buf, n);
  }
};

}  // namespace web
