// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//
// Routing of slang-elab diagnostics to utl::Logger.
//
// This TU pulls utl/Logger.h (and transitively spdlog's bundled fmt) and so
// must not include any slang header: slang brings a different fmt version,
// and mixing the two in one TU causes inline-namespace collisions on fmt::vNN.
//

#include "diagnostics.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include "log_stubs.h"
#include "utl/Logger.h"

namespace slang_frontend {

namespace {

// Icky, see povik/sv-elab#383
utl::Logger* elab_logger = nullptr;
const slang::SourceManager* elab_source_manager = nullptr;

}  // namespace

void reportError(utl::Logger* logger, int code, std::string_view message)
{
  logger->error(utl::SYN, code, "{}", message);
}

utl::Logger* elabLogger()
{
  return elab_logger;
}

const slang::SourceManager* elabSourceManager()
{
  return elab_source_manager;
}

ElabDiagnosticScope::ElabDiagnosticScope(
    utl::Logger* logger,
    const slang::SourceManager* source_manager)
    : previous_logger_(elab_logger),
      previous_source_manager_(elab_source_manager)
{
  elab_logger = logger;
  elab_source_manager = source_manager;
}

ElabDiagnosticScope::~ElabDiagnosticScope()
{
  elab_logger = previous_logger_;
  elab_source_manager = previous_source_manager_;
}

}  // namespace slang_frontend

// The Yosys logging entry points the vendored frontend calls, declared in
// log_stubs.h and implemented here so they reach the user through utl::Logger
// rather than raw stderr. Keeping them here is what lets
// third-party/slang-elab/ stay untouched.
namespace Yosys {

namespace {

// One code per shim, not per message: the vendored call sites pass a format
// string, so there is nothing here to hang a distinct message ID off.
constexpr int kFrontendError = 79;
constexpr int kFrontendWarning = 80;

// Renders a printf-style message and trims the trailing newline the Yosys
// convention includes; utl::Logger supplies its own.
std::string formatMessage(const char* fmt, va_list ap)
{
  va_list measure;
  va_copy(measure, ap);
  const int length = std::vsnprintf(nullptr, 0, fmt, measure);
  va_end(measure);
  if (length <= 0) {
    return {};
  }

  std::string message(length, '\0');
  std::vsnprintf(message.data(), length + 1, fmt, ap);
  while (!message.empty()
         && (message.back() == '\n' || message.back() == '\r')) {
    message.pop_back();
  }
  return message;
}

}  // namespace

void log(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  const std::string message = formatMessage(fmt, ap);
  va_end(ap);

  utl::Logger* logger = slang_frontend::elabLogger();
  if (logger) {
    logger->reportLiteral(message);
  } else {
    std::fprintf(stderr, "%s\n", message.c_str());
  }
}

void log_warning(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  const std::string message = formatMessage(fmt, ap);
  va_end(ap);

  utl::Logger* logger = slang_frontend::elabLogger();
  if (logger) {
    logger->warn(utl::SYN, kFrontendWarning, "{}", message);
  } else {
    std::fprintf(stderr, "Warning: %s\n", message.c_str());
  }
}

[[noreturn]] void log_error(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  const std::string message = formatMessage(fmt, ap);
  va_end(ap);

  utl::Logger* logger = slang_frontend::elabLogger();
  if (logger) {
    slang_frontend::reportError(logger, kFrontendError, message);
  }

  // Reached only outside an elaboration, where there is no logger to throw
  // through; still has to honour [[noreturn]].
  std::fprintf(stderr, "%s\n", message.c_str());
  std::abort();
}

}  // namespace Yosys
