// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Standalone stubs for Yosys log/assert functions.
//
// When building slang-elab without Yosys (SLANG_NO_YOSYS), the vendored
// frontend in third-party/slang-elab/ still references log_assert, log_abort,
// stringf, etc. This header provides minimal implementations so that code
// compiles and behaves sanely without pulling in any Yosys headers.
//
// The diagnostic entry points -- log, log_warning and log_error -- are only
// declared here; diagnostics.cc defines them and routes them to utl::Logger,
// so a message raised by the vendored frontend reaches the user the same way
// as any other OpenROAD diagnostic. They cannot be inline: utl/Logger.h
// transitively brings spdlog's bundled fmt, slang brings a different version,
// and mixing the two in one TU collides on fmt::vNN.
//
// Only the vendored code needs any of this. OpenROAD's own elab sources call
// assert() and utl::Logger directly.

#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace Yosys {

// Defined in diagnostics.cc, on top of utl::Logger.
[[gnu::format(printf, 1, 2)]] void log(const char* fmt, ...);
[[gnu::format(printf, 1, 2)]] void log_warning(const char* fmt, ...);
[[gnu::format(printf, 1, 2)]] [[noreturn]] void log_error(const char* fmt, ...);

// Nothing in this build calls log_flush or ys_debug, but the vendored
// frontend names them in unguarded using-declarations, so they have to exist.
// utl::Logger does its own flushing.
inline void log_flush()
{
}

inline int ys_debug(int = 0)
{
  return 0;
}

#ifndef log_debug
[[gnu::format(printf, 1, 2)]]
inline void log_debug(const char*, ...)
{
}
#endif

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
