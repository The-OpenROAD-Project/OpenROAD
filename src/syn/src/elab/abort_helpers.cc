// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//
// Implementations of abort/diagnostic helpers required by the shared frontend.
//

#include <cctype>
#include <string>
#include <string_view>

#include "diagnostics.h"
#include "slang/ast/Compilation.h"
#include "slang/ast/Expression.h"
#include "slang/ast/Statement.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/TimingControl.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/syntax/AllSyntax.h"
#include "slang/syntax/SyntaxPrinter.h"
#include "slang/text/SourceLocation.h"
#include "slang/text/SourceManager.h"
#include "slang_frontend.h"

namespace slang_frontend {

namespace {

std::string unimplementedMessage(const char* file,
                                 int line,
                                 const char* condition)
{
  std::string message = "Feature unimplemented at " + std::string(file) + ":"
                        + std::to_string(line);
  if (condition) {
    message += ": ";
    message += condition;
  }
  return message;
}

// "file:line:column" for `location`, or an empty string when the elaboration
// has no source manager or the location is compiler-generated rather than
// something the user wrote.
std::string sourceLocation(slang::SourceLocation location)
{
  const slang::SourceManager* source_manager = elabSourceManager();
  if (!source_manager || !source_manager->isFileLoc(location)) {
    return {};
  }
  return std::string(source_manager->getFileName(location)) + ":"
         + std::to_string(source_manager->getLineNumber(location)) + ":"
         + std::to_string(source_manager->getColumnNumber(location));
}

// The HDL text a syntax node was built from, flattened onto one line. slang
// hands back the source verbatim, so an expression spanning several lines
// arrives with its newlines and indentation; runs of whitespace collapse to a
// single space and the result is capped to keep the message readable.
// Comments are dropped: flattening a trailing // comment onto one line would
// make everything after it look commented out.
std::string sourceText(const slang::syntax::SyntaxNode& syntax)
{
  slang::syntax::SyntaxPrinter printer;
  printer.setIncludeComments(false).setIncludeDirectives(false).print(syntax);
  const std::string text = printer.str();

  std::string flattened;
  flattened.reserve(text.size());
  bool pending_space = false;
  for (const char c : text) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      pending_space = true;
      continue;
    }
    if (pending_space && !flattened.empty()) {
      flattened += ' ';
    }
    pending_space = false;
    flattened += c;
  }

  constexpr size_t max_length = 120;
  if (flattened.size() > max_length) {
    flattened.resize(max_length);
    flattened += "...";
  }
  return flattened;
}

// Appends what identifies an AST node to the user: its kind, where it sits in
// their HDL, and the text it was built from. Each part is dropped when the
// node cannot supply it -- a node the elaborator synthesised has no syntax,
// and a compiler-generated location maps to no file.
void appendNodeDetail(std::string& message,
                      std::string_view node_kind,
                      std::string_view kind_detail,
                      slang::SourceLocation location,
                      const slang::syntax::SyntaxNode* syntax)
{
  message += " (";
  message.append(node_kind.data(), node_kind.size());
  message += " kind: ";
  message.append(kind_detail.data(), kind_detail.size());
  message += ")";

  const std::string location_text = sourceLocation(location);
  if (!location_text.empty()) {
    message += " at " + location_text;
  }
  if (syntax) {
    message += ": " + sourceText(*syntax);
  }
}

}  // namespace

[[noreturn]] void unimplemented_(const ast::Symbol& obj,
                                 const char* file,
                                 int line,
                                 const char* condition)
{
  std::string message = unimplementedMessage(file, line, condition);

  // A symbol carries a name, which is what the user recognises it by. Many
  // kinds (procedural blocks, generate scopes) are unnamed.
  std::string kind_detail(slang::ast::toString(obj.kind));
  if (!obj.name.empty()) {
    kind_detail += " '" + std::string(obj.name) + "'";
  }
  appendNodeDetail(
      message, "symbol", kind_detail, obj.location, obj.getSyntax());

  reportError(elabLogger(), 73, message);
}

[[noreturn]] void unimplemented_(const ast::Expression& obj,
                                 const char* file,
                                 int line,
                                 const char* condition)
{
  std::string message = unimplementedMessage(file, line, condition);
  appendNodeDetail(message,
                   "expression",
                   slang::ast::toString(obj.kind),
                   obj.sourceRange.start(),
                   obj.syntax);

  reportError(elabLogger(), 74, message);
}

[[noreturn]] void unimplemented_(const ast::Statement& obj,
                                 const char* file,
                                 int line,
                                 const char* condition)
{
  std::string message = unimplementedMessage(file, line, condition);
  appendNodeDetail(message,
                   "statement",
                   slang::ast::toString(obj.kind),
                   obj.sourceRange.start(),
                   obj.syntax);

  reportError(elabLogger(), 75, message);
}

[[noreturn]] void unimplemented_(const ast::TimingControl& obj,
                                 const char* file,
                                 int line,
                                 const char* condition)
{
  std::string message = unimplementedMessage(file, line, condition);
  appendNodeDetail(message,
                   "timing control",
                   slang::ast::toString(obj.kind),
                   obj.sourceRange.start(),
                   obj.syntax);

  reportError(elabLogger(), 76, message);
}

[[noreturn]] void wire_missing_(NetlistContext& netlist,
                                const ast::Symbol& symbol,
                                const char* file,
                                int line)
{
  (void) netlist;
  (void) symbol;
  reportError(
      elabLogger(),
      77,
      "Wire missing at " + std::string(file) + ":" + std::to_string(line));
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
