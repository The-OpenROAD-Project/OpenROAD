// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//
// Import Liberty cell definitions as blackbox modules into a slang
// Compilation so that standard cell instantiations are recognized
// during elaboration.
//
// The approach mirrors slang-elab's own blackboxes.cc:
// We construct slang syntax trees for each Liberty cell, with port
// declarations matching the Liberty port names, widths, and directions.
//

#include "blackboxes.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "db_sta/dbSta.hh"
#include "slang/ast/Compilation.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/numeric/SVInt.h"
#include "slang/parsing/Token.h"
#include "slang/parsing/TokenKind.h"
#include "slang/syntax/AllSyntax.h"
#include "slang/syntax/SyntaxKind.h"
#include "slang/syntax/SyntaxNode.h"
#include "slang/syntax/SyntaxTree.h"
#include "slang/text/SourceManager.h"
#include "slang/util/BumpAllocator.h"
#include "slang/util/SmallVector.h"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"

namespace syn {

// NOLINTBEGIN(google-build-using-namespace)
using namespace slang;
using namespace slang::ast;
using namespace slang::syntax;
using namespace slang::parsing;
// NOLINTEND(google-build-using-namespace)

void importBlackboxesFromLiberty(SourceManager& mgr,
                                 Compilation& target,
                                 sta::dbSta* sta)
{
  if (!sta) {
    return;
  }

  sta::Network* network = sta->network();
  if (!network) {
    return;
  }

  BumpAllocator alloc;

  auto token = [&](TokenKind kind,
                   std::string text = "",
                   bool space = false,
                   bool eol = false) {
    char* ptr = (char*) alloc.allocate(text.size(), 1);
    // Buffer is intentionally not null-terminated; consumed via
    // std::string_view{ptr, text.size()} below, sized exactly to text.size().
    // NOLINTNEXTLINE(bugprone-not-null-terminated-result)
    memcpy(ptr, text.data(), text.size());

    SmallVector<Trivia, 2> trivia;
    if (space) {
      trivia.push_back(Trivia(TriviaKind::Whitespace, " "sv));
    }
    if (eol) {
      trivia.push_back(Trivia(TriviaKind::EndOfLine, "\n"sv));
    }

    return Token(target,
                 kind,
                 trivia.copy(target),
                 std::string_view{ptr, text.size()},
                 SourceLocation::NoLocation);
  };

  auto integer_literal = [&](int value) {
    std::string text = std::to_string(value);
    char* ptr = (char*) alloc.allocate(text.size(), 1);
    // Buffer is intentionally not null-terminated; consumed via
    // std::string_view{ptr, text.size()} below, sized exactly to text.size().
    // NOLINTNEXTLINE(bugprone-not-null-terminated-result)
    memcpy(ptr, text.data(), text.size());

    return Token(target,
                 TokenKind::IntegerLiteral,
                 {},
                 std::string_view{ptr, text.size()},
                 SourceLocation::NoLocation,
                 SVInt(value));
  };

  SmallVector<MemberSyntax*, 16> decls;

  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      network->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();

    sta::LibertyCellIterator cell_iter(lib);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      std::string cell_name = cell->name();

      // Skip if already defined in the compilation
      if (target.tryGetDefinition(cell_name, target.getRootNoFinalize())
              .definition) {
        continue;
      }

      SmallVector<TokenOrSyntax, 16> port_list;

      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        sta::LibertyPort* port = port_iter.next();

        // Skip power/ground pins
        if (port->isPwrGnd()) {
          continue;
        }

        // Determine direction
        TokenKind direction;
        sta::PortDirection* dir = port->direction();
        if (dir->isInput()) {
          direction = TokenKind::InputKeyword;
        } else if (dir->isOutput()) {
          direction = TokenKind::OutputKeyword;
        } else if (dir->isBidirect()) {
          direction = TokenKind::InOutKeyword;
        } else {
          continue;  // skip internal/unknown directions
        }

        // Determine width
        int width = 1;
        if (port->isBus()) {
          width = std::abs(port->toIndex() - port->fromIndex()) + 1;
        }

        // Build dimension syntax [width-1:0] if width > 1
        SmallVector<VariableDimensionSyntax*, 2> dims;
        if (width > 1) {
          dims.push_back(alloc.emplace<VariableDimensionSyntax>(
              token(TokenKind::OpenBracket, "", true),
              alloc.emplace<RangeDimensionSpecifierSyntax>(
                  *alloc.emplace<RangeSelectSyntax>(
                      SyntaxKind::SimpleRangeSelect,
                      *alloc.emplace<LiteralExpressionSyntax>(
                          SyntaxKind::IntegerLiteralExpression,
                          integer_literal(width - 1)),
                      Token(target,
                            TokenKind::Colon,
                            {},
                            "",
                            SourceLocation::NoLocation),
                      *alloc.emplace<LiteralExpressionSyntax>(
                          SyntaxKind::IntegerLiteralExpression,
                          integer_literal(0)))),
              Token(target,
                    TokenKind::CloseBracket,
                    {},
                    "",
                    SourceLocation::NoLocation)));
        }

        std::string port_name = port->name();
        port_list.push_back(alloc.emplace<ImplicitAnsiPortSyntax>(
            *alloc.emplace<SyntaxList<AttributeInstanceSyntax>>(nullptr),
            *alloc.emplace<VariablePortHeaderSyntax>(
                Token(),
                token(direction, "", false, true),
                Token(),
                *alloc.emplace<ImplicitTypeSyntax>(
                    Token(),
                    *alloc.emplace<SyntaxList<VariableDimensionSyntax>>(
                        dims.copy(target)),
                    Token())),
            *alloc.emplace<DeclaratorSyntax>(
                token(TokenKind::Identifier, port_name, true),
                *alloc.emplace<SyntaxList<VariableDimensionSyntax>>(nullptr),
                nullptr)));
        port_list.push_back(token(TokenKind::Comma));
      }

      if (!port_list.empty()) {
        port_list.pop_back();  // remove trailing comma
      }

      auto header = alloc.emplace<ModuleHeaderSyntax>(
          SyntaxKind::ModuleHeader,
          token(TokenKind::ModuleKeyword, "", false, true),
          Token(),
          token(TokenKind::Identifier, cell_name, true),
          *alloc.emplace<SyntaxList<PackageImportDeclarationSyntax>>(nullptr),
          nullptr,  // parameters
          alloc.emplace<AnsiPortListSyntax>(
              token(TokenKind::OpenParenthesis),
              *alloc.emplace<SeparatedSyntaxList<MemberSyntax>>(
                  port_list.copy(target)),
              token(TokenKind::CloseParenthesis)),
          token(TokenKind::Semicolon));

      // Add (* blackbox *) attribute
      SmallVector<TokenOrSyntax, 2> attrs_spec;
      SmallVector<AttributeInstanceSyntax*, 2> attrs;
      attrs_spec.push_back(alloc.emplace<AttributeSpecSyntax>(
          token(TokenKind::Identifier, "blackbox", true), nullptr));
      attrs.push_back(alloc.emplace<AttributeInstanceSyntax>(
          token(TokenKind::OpenParenthesis),
          token(TokenKind::Star),
          *alloc.emplace<SeparatedSyntaxList<AttributeSpecSyntax>>(
              attrs_spec.copy(target)),
          token(TokenKind::Star, "", true),
          token(TokenKind::CloseParenthesis)));

      auto syntax = alloc.emplace<ModuleDeclarationSyntax>(
          SyntaxKind::ModuleDeclaration,
          *alloc.emplace<SyntaxList<AttributeInstanceSyntax>>(
              attrs.copy(target)),
          *header,
          *alloc.emplace<SyntaxList<MemberSyntax>>(nullptr),
          token(TokenKind::EndModuleKeyword, "", false, true),
          nullptr);

      decls.push_back(syntax);
    }
  }

  if (decls.empty()) {
    return;
  }

  auto unit_syntax = alloc.emplace<CompilationUnitSyntax>(
      *target.emplace<SyntaxList<MemberSyntax>>(decls.copy(target)),
      token(TokenKind::EndOfFile, "", false, false));
  auto tree = std::make_shared<SyntaxTree>(
      unit_syntax, mgr, std::move(alloc), &target.getDefaultLibrary(), nullptr);
  tree->isLibraryUnit = true;
  target.addSyntaxTree(tree);
}

}  // namespace syn
