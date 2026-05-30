// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//
// Implementations of abort/diagnostic helpers required by the shared frontend.
//

#include "slang/ast/Compilation.h"
#include "slang/ast/Expression.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang_frontend.h"

namespace slang_frontend {

[[noreturn]] void unimplemented_(const ast::Symbol& obj,
                                 const char* file,
                                 int line,
                                 const char* condition)
{
  if (condition) {
    log_error("Feature unimplemented at %s:%d: %s\n", file, line, condition);
  } else {
    log_error("Feature unimplemented at %s:%d\n", file, line);
  }
}

[[noreturn]] void unimplemented_(const ast::Expression& obj,
                                 const char* file,
                                 int line,
                                 const char* condition)
{
  auto kind = slang::ast::toString(obj.kind);
  if (condition) {
    log_error("Feature unimplemented at %s:%d: %s (expression kind: %.*s)\n",
              file,
              line,
              condition,
              static_cast<int>(kind.size()),
              kind.data());
  } else {
    log_error("Feature unimplemented at %s:%d (expression kind: %.*s)\n",
              file,
              line,
              static_cast<int>(kind.size()),
              kind.data());
  }
}

[[noreturn]] void unimplemented_(const ast::Statement& obj,
                                 const char* file,
                                 int line,
                                 const char* condition)
{
  (void) obj;
  if (condition) {
    log_error("Feature unimplemented at %s:%d: %s\n", file, line, condition);
  } else {
    log_error("Feature unimplemented at %s:%d\n", file, line);
  }
}

[[noreturn]] void unimplemented_(const ast::TimingControl& obj,
                                 const char* file,
                                 int line,
                                 const char* condition)
{
  (void) obj;
  if (condition) {
    log_error("Feature unimplemented at %s:%d: %s\n", file, line, condition);
  } else {
    log_error("Feature unimplemented at %s:%d\n", file, line);
  }
}

[[noreturn]] void wire_missing_(NetlistContext& netlist,
                                const ast::Symbol& symbol,
                                const char* file,
                                int line)
{
  (void) netlist;
  (void) symbol;
  log_error("Wire missing at %s:%d\n", file, line);
}

// No-op transfer_attrs for the syn backend (AttributeGuard is also a no-op)
void transfer_attrs(NetlistContext&, const ast::Symbol&, AttributeGuard&)
{
}
void transfer_attrs(NetlistContext&, const ast::Statement&, AttributeGuard&)
{
}
void transfer_attrs(NetlistContext&, const ast::Expression&, AttributeGuard&)
{
}

}  // namespace slang_frontend
