// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "LibcxxAbort.h"

#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "utl/StackTrace.h"

namespace ord {

[[noreturn]] void logLibcxxFailure(utl::Logger& logger,
                                   const std::string& trace,
                                   const std::string& message)
{
  logger.report("Stack trace:\n{}", trace);
  logger.critical(utl::ORD, 2057, "libc++ check failed: {}", message);
}

}  // namespace ord

#if defined(_LIBCPP_VERSION)
// Replaces libc++'s weak default, which a failed hardening check calls under
// the "enforce" assertion semantic. Report it through the logger so it lands
// in the log file; fall back to stderr before the logger exists.
[[noreturn]] void std::__libcpp_verbose_abort(const char* format, ...) noexcept
{
  std::array<char, 1024> buffer{};
  va_list args;
  va_start(args, format);
  std::vsnprintf(buffer.data(), buffer.size(), format, args);
  va_end(args);
  std::string message(buffer.data());
  while (!message.empty() && message.back() == '\n') {
    message.pop_back();
  }

  const std::string trace = utl::stackTrace();

  ord::OpenRoad* app = ord::OpenRoad::openRoad();
  utl::Logger* logger = app ? app->getLogger() : nullptr;
  if (logger) {
    ord::logLibcxxFailure(*logger, trace, message);
  }
  std::cerr << "Stack trace:\n"
            << trace << "libc++ check failed: " << message << '\n';
  std::abort();
}
#endif
