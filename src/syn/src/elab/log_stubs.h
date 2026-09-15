// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Standalone stubs for Yosys log/assert functions.
//
// When building slang-elab without Yosys (SLANG_NO_YOSYS), the shared
// frontend code still references log_assert, log_error, log_warning, etc.
// This header provides minimal implementations so the code compiles and
// behaves sanely without pulling in any Yosys headers.

#pragma once

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace Yosys {

[[gnu::format(printf, 1, 2)]]
inline void log(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

inline void log_flush()
{
  fflush(stderr);
}

[[gnu::format(printf, 1, 2)]] [[noreturn]]
inline void log_error(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  std::abort();
}

[[gnu::format(printf, 1, 2)]]
inline void log_warning(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "Warning: ");
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

#ifndef log_debug
[[gnu::format(printf, 1, 2)]]
inline void log_debug(const char*, ...)
{
}
#endif

inline int ys_debug(int = 0)
{
  return 0;
}

inline int ceil_log2(int x)
{
  if (x <= 0) {
    return 0;
  }
  int result = 0;
  x--;
  while (x > 0) {
    x >>= 1;
    result++;
  }
  return result;
}

[[noreturn]] inline void log_abort()
{
  std::abort();
}

// log_assert: use a macro so we get file/line info
#ifndef log_assert
#define log_assert(_cond_)                           \
  do {                                               \
    if (!(_cond_)) {                                 \
      std::fprintf(stderr,                           \
                   "Assertion failed: %s [%s:%d]\n", \
                   #_cond_,                          \
                   __FILE__,                         \
                   __LINE__);                        \
      std::abort();                                  \
    }                                                \
  } while (0)
#endif

[[gnu::format(printf, 1, 2)]]
inline std::string stringf(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  va_list ap2;
  va_copy(ap2, ap);
  int n = vsnprintf(nullptr, 0, fmt, ap);
  va_end(ap);
  std::string result(n, '\0');
  vsnprintf(result.data(), n + 1, fmt, ap2);
  va_end(ap2);
  return result;
}

}  // namespace Yosys
