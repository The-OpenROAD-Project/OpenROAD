// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdarg>
#include <stdexcept>

#include "odb.h"

#ifdef __GNUC__
#define ADS_FORMAT_PRINTF(F, A) __attribute__((format(printf, F, A)))
#else
#define ADS_FORMAT_PRINTF(F, A)
#endif

namespace odb {

inline std::runtime_error ZException(const char* fmt,
                                     ...)  // ADS_FORMAT_PRINTF(2, 3)
{
  char buffer[8192];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, 8192, fmt, args);
  va_end(args);
  return std::runtime_error(buffer);
}

// See http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert
// Avoids unused variable warnings.
#ifdef NDEBUG
#define ZASSERT(x)    \
  do {                \
    (void) sizeof(x); \
  } while (0)
#else
#define ZASSERT(x) assert(x)
#endif

#define ZALLOCATED(expr)                         \
  do {                                           \
    if ((expr) == nullptr)                       \
      throw std::runtime_error("Out of memory"); \
  } while (0);

}  // namespace odb
