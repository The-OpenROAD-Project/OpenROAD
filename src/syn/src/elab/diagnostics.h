// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//
// Routing of slang-elab diagnostics to utl::Logger.
//
// Only utl::Logger is forward declared here: utl/Logger.h transitively brings
// spdlog's bundled fmt, slang headers bring a different fmt version, and
// mixing the two in one TU causes inline-namespace collisions on fmt::vNN.
// This header therefore stays includable from both sides, and the definitions
// live in error.cc, the one elab TU that pulls utl/Logger.h.
//
#pragma once

#include <string_view>

namespace utl {
class Logger;
}

namespace slang {
class SourceManager;
}

namespace slang_frontend {

// Wraps `logger->error(utl::SYN, code, "{}", message)`.
[[noreturn]] void reportError(utl::Logger* logger,
                              int code,
                              std::string_view message);

// The logger of the elaboration in progress. Diagnostics raised from the
// shared frontend's hooks (unimplemented_(), wire_missing_(), the
// "unsupported without Yosys" paths) have no backend or netlist to reach a
// logger through, so elaborateImpl() parks it here for the duration of the
// run. Non-null only while an ElabDiagnosticScope is alive.
utl::Logger* elabLogger();

// The source manager of the elaboration in progress, for resolving a slang
// SourceLocation to file:line:column when reporting on an AST node. Parked
// alongside the logger, and non-null under the same conditions.
const slang::SourceManager* elabSourceManager();

// Makes `logger` and `source_manager` the elabLogger() and
// elabSourceManager() for the enclosing scope.
class ElabDiagnosticScope
{
 public:
  ElabDiagnosticScope(utl::Logger* logger,
                      const slang::SourceManager* source_manager);
  ~ElabDiagnosticScope();
  ElabDiagnosticScope(const ElabDiagnosticScope&) = delete;
  ElabDiagnosticScope& operator=(const ElabDiagnosticScope&) = delete;

 private:
  utl::Logger* previous_logger_;
  const slang::SourceManager* previous_source_manager_;
};

}  // namespace slang_frontend
