load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_binary")

# ---------------------------------------------------------------------------
# Code generation scripts
# ---------------------------------------------------------------------------

py_binary(
    name = "diagnostic_gen",
    srcs = ["scripts/diagnostic_gen.py"],
    main = "scripts/diagnostic_gen.py",
)

py_binary(
    name = "syntax_gen",
    srcs = ["scripts/syntax_gen.py"],
    main = "scripts/syntax_gen.py",
)

# ---------------------------------------------------------------------------
# Generate diagnostic headers and DiagCode.cpp
# ---------------------------------------------------------------------------

_DIAG_HEADERS = [
    "slang/diagnostics/AllDiags.h",
    "slang/diagnostics/AnalysisDiags.h",
    "slang/diagnostics/CompilationDiags.h",
    "slang/diagnostics/ConstEvalDiags.h",
    "slang/diagnostics/DeclarationsDiags.h",
    "slang/diagnostics/ExpressionsDiags.h",
    "slang/diagnostics/GeneralDiags.h",
    "slang/diagnostics/LexerDiags.h",
    "slang/diagnostics/LookupDiags.h",
    "slang/diagnostics/MetaDiags.h",
    "slang/diagnostics/NumericDiags.h",
    "slang/diagnostics/ParserDiags.h",
    "slang/diagnostics/PreprocessorDiags.h",
    "slang/diagnostics/StatementsDiags.h",
    "slang/diagnostics/SysFuncsDiags.h",
    "slang/diagnostics/TypesDiags.h",
]

genrule(
    name = "gen_diagnostics",
    srcs = [
        "scripts/diagnostics.txt",
    ],
    outs = _DIAG_HEADERS + ["DiagCode.cpp"],
    cmd = """
        $(execpath :diagnostic_gen) \
            --outDir $(@D) \
            --srcDir $(RULEDIR)/source \
            --incDir $(RULEDIR)/include/slang \
            --diagnostics $(location scripts/diagnostics.txt)
    """,
    tools = [":diagnostic_gen"],
)

# ---------------------------------------------------------------------------
# Generate syntax headers and sources
# ---------------------------------------------------------------------------

_SYNTAX_HEADERS = [
    "slang/syntax/AllSyntax.h",
    "slang/syntax/SyntaxKind.h",
    "slang/syntax/SyntaxFwd.h",
    "slang/syntax/CSTJsonVisitorGen.h",
    "slang/parsing/TokenKind.h",
    "slang/parsing/KnownSystemName.h",
]

_SYNTAX_SRCS = [
    "AllSyntax.cpp",
    "SyntaxClone.cpp",
    "TokenKind.cpp",
    "KnownSystemName.cpp",
]

genrule(
    name = "gen_syntax",
    srcs = [
        "scripts/syntax.txt",
        "scripts/triviakinds.txt",
        "scripts/tokenkinds.txt",
        "scripts/systemnames.txt",
    ],
    outs = _SYNTAX_HEADERS + _SYNTAX_SRCS,
    cmd = """
        $(execpath :syntax_gen) \
            --dir $(@D) \
            --syntax $(location scripts/syntax.txt)
    """,
    tools = [":syntax_gen"],
)

# ---------------------------------------------------------------------------
# Generate VersionInfo.cpp from template
# ---------------------------------------------------------------------------

genrule(
    name = "gen_version_info",
    srcs = ["source/util/VersionInfo.cpp.in"],
    outs = ["VersionInfo.cpp"],
    cmd = """
        sed \
            -e 's/@SLANG_VERSION_MAJOR@/9/g' \
            -e 's/@SLANG_VERSION_MINOR@/1/g' \
            -e 's/@SLANG_VERSION_PATCH@/176/g' \
            -e 's/@SLANG_VERSION_HASH@/03dfa2131/g' \
            $(location source/util/VersionInfo.cpp.in) > $@
    """,
)

# ---------------------------------------------------------------------------
# Generate slang_export.h (normally done by CMake's GenerateExportHeader)
# ---------------------------------------------------------------------------

genrule(
    name = "gen_slang_export",
    outs = ["slang/slang_export.h"],
    cmd = """
        cat > $@ << 'EXPORT_EOF'
#pragma once

#ifdef SLANG_STATIC_DEFINE
#  define SLANG_EXPORT
#  define SLANG_NO_EXPORT
#else
#  ifdef slang_slang_EXPORTS
#    define SLANG_EXPORT __attribute__((visibility("default")))
#  else
#    define SLANG_EXPORT __attribute__((visibility("default")))
#  endif
#  define SLANG_NO_EXPORT __attribute__((visibility("hidden")))
#endif

#ifndef SLANG_DEPRECATED
#  define SLANG_DEPRECATED __attribute__((__deprecated__))
#endif

#ifndef SLANG_DEPRECATED_EXPORT
#  define SLANG_DEPRECATED_EXPORT SLANG_EXPORT SLANG_DEPRECATED
#endif

#ifndef SLANG_DEPRECATED_NO_EXPORT
#  define SLANG_DEPRECATED_NO_EXPORT SLANG_NO_EXPORT SLANG_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef SLANG_NO_DEPRECATED
#    define SLANG_NO_DEPRECATED
#  endif
#endif

#if defined(_MSC_VER) && !defined(__ICL)
  #pragma warning(disable:4251)
  #pragma warning(disable:4275)
#elif defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic ignored "-Wattributes"
#endif
EXPORT_EOF
    """,
)

# ---------------------------------------------------------------------------
# Main slang library
# ---------------------------------------------------------------------------

cc_library(
    name = "slang",
    srcs = [
        # Generated sources
        "DiagCode.cpp",
        "AllSyntax.cpp",
        "SyntaxClone.cpp",
        "TokenKind.cpp",
        "KnownSystemName.cpp",
        "VersionInfo.cpp",
        # analysis
        "source/analysis/AbstractFlowAnalysis.cpp",
        "source/analysis/AnalysisManager.cpp",
        "source/analysis/AnalyzedAssertion.cpp",
        "source/analysis/AnalyzedProcedure.cpp",
        "source/analysis/CaseDecisionDag.cpp",
        "source/analysis/ClockInference.cpp",
        "source/analysis/DataFlowAnalysis.cpp",
        "source/analysis/DFAResults.cpp",
        "source/analysis/DriverTracker.cpp",
        "source/analysis/ValueDriver.cpp",
        # ast (core)
        "source/ast/ASTContext.cpp",
        "source/ast/ASTDiagMap.cpp",
        "source/ast/ASTSerializer.cpp",
        "source/ast/Bitstream.cpp",
        "source/ast/Compilation.cpp",
        "source/ast/Constraints.cpp",
        "source/ast/EvalContext.cpp",
        "source/ast/Expression.cpp",
        "source/ast/FmtHelpers.cpp",
        "source/ast/HierarchicalReference.cpp",
        "source/ast/InstanceCacheKey.cpp",
        "source/ast/LSPUtilities.cpp",
        "source/ast/LValue.cpp",
        "source/ast/Lookup.cpp",
        "source/ast/OpaqueInstancePath.cpp",
        "source/ast/Patterns.cpp",
        "source/ast/SFormat.cpp",
        "source/ast/Scope.cpp",
        "source/ast/ScriptSession.cpp",
        "source/ast/SemanticFacts.cpp",
        "source/ast/Statement.cpp",
        "source/ast/Symbol.cpp",
        "source/ast/SystemSubroutine.cpp",
        "source/ast/TimingControl.cpp",
        "source/ast/TypeProvider.cpp",
        # ast/builtins
        "source/ast/builtins/ArrayMethods.cpp",
        "source/ast/builtins/ConversionFuncs.cpp",
        "source/ast/builtins/CoverageFuncs.cpp",
        "source/ast/builtins/EnumMethods.cpp",
        "source/ast/builtins/GateTypes.cpp",
        "source/ast/builtins/MathFuncs.cpp",
        "source/ast/builtins/MiscSystemFuncs.cpp",
        "source/ast/builtins/NonConstFuncs.cpp",
        "source/ast/builtins/QueryFuncs.cpp",
        "source/ast/builtins/StdPackage.cpp",
        "source/ast/builtins/StringMethods.cpp",
        "source/ast/builtins/SystemTasks.cpp",
        # ast/expressions
        "source/ast/expressions/AssertionExpr.cpp",
        "source/ast/expressions/AssignmentExpressions.cpp",
        "source/ast/expressions/CallExpression.cpp",
        "source/ast/expressions/ConversionExpression.cpp",
        "source/ast/expressions/LiteralExpressions.cpp",
        "source/ast/expressions/MiscExpressions.cpp",
        "source/ast/expressions/Operator.cpp",
        "source/ast/expressions/OperatorExpressions.cpp",
        "source/ast/expressions/SelectExpressions.cpp",
        # ast/statements
        "source/ast/statements/ConditionalStatements.cpp",
        "source/ast/statements/LoopStatements.cpp",
        "source/ast/statements/MiscStatements.cpp",
        # ast/symbols
        "source/ast/symbols/AttributeSymbol.cpp",
        "source/ast/symbols/BlockSymbols.cpp",
        "source/ast/symbols/CheckerSymbols.cpp",
        "source/ast/symbols/ClassSymbols.cpp",
        "source/ast/symbols/CompilationUnitSymbols.cpp",
        "source/ast/symbols/CoverSymbols.cpp",
        "source/ast/symbols/InstanceSymbols.cpp",
        "source/ast/symbols/MemberSymbols.cpp",
        "source/ast/symbols/ParameterBuilder.cpp",
        "source/ast/symbols/ParameterSymbols.cpp",
        "source/ast/symbols/PortSymbols.cpp",
        "source/ast/symbols/SpecifySymbols.cpp",
        "source/ast/symbols/SubroutineSymbols.cpp",
        "source/ast/symbols/SymbolBuilders.cpp",
        "source/ast/symbols/ValueSymbol.cpp",
        "source/ast/symbols/VariableSymbols.cpp",
        # ast/types
        "source/ast/types/AllTypes.cpp",
        "source/ast/types/DeclaredType.cpp",
        "source/ast/types/NetType.cpp",
        "source/ast/types/Type.cpp",
        "source/ast/types/TypePrinter.cpp",
        # diagnostics
        "source/diagnostics/DiagnosticClient.cpp",
        "source/diagnostics/DiagnosticEngine.cpp",
        "source/diagnostics/Diagnostics.cpp",
        "source/diagnostics/JsonDiagnosticClient.cpp",
        "source/diagnostics/TextDiagnosticClient.cpp",
        # driver
        "source/driver/CompatSettings.cpp",
        "source/driver/Driver.cpp",
        "source/driver/SourceLoader.cpp",
        # numeric
        "source/numeric/ConstantValue.cpp",
        "source/numeric/SVInt.cpp",
        "source/numeric/Time.cpp",
        # parsing
        "source/parsing/Lexer.cpp",
        "source/parsing/LexerFacts.cpp",
        "source/parsing/NumberParser.cpp",
        "source/parsing/Parser.cpp",
        "source/parsing/Parser_expressions.cpp",
        "source/parsing/Parser_members.cpp",
        "source/parsing/Parser_statements.cpp",
        "source/parsing/ParserBase.cpp",
        "source/parsing/ParserMetadata.cpp",
        "source/parsing/Preprocessor.cpp",
        "source/parsing/Preprocessor_macros.cpp",
        "source/parsing/Preprocessor_pragmas.cpp",
        "source/parsing/Token.cpp",
        # syntax
        "source/syntax/CSTSerializer.cpp",
        "source/syntax/SyntaxFacts.cpp",
        "source/syntax/SyntaxNode.cpp",
        "source/syntax/SyntaxPrinter.cpp",
        "source/syntax/SyntaxTree.cpp",
        "source/syntax/SyntaxVisitor.cpp",
        # text
        "source/text/CharInfo.cpp",
        "source/text/Glob.cpp",
        "source/text/Json.cpp",
        "source/text/SourceLocation.cpp",
        "source/text/SourceManager.cpp",
        # util
        "source/util/BumpAllocator.cpp",
        "source/util/CommandLine.cpp",
        "source/util/IntervalMap.cpp",
        "source/util/OS.cpp",
        "source/util/SmallVector.cpp",
        "source/util/String.cpp",
        "source/util/TimeTrace.cpp",
        "source/util/Util.cpp",
    ] + glob([
        # Internal headers used by source files
        "source/**/*.h",
    ]),
    hdrs = glob([
        "include/**/*.h",
        "external/**/*.h",
        "external/**/*.hpp",
    ]) + _DIAG_HEADERS + _SYNTAX_HEADERS + [
        "slang/slang_export.h",
    ],
    defines = [
        "SLANG_BOOST_SINGLE_HEADER",
        "SLANG_STATIC_DEFINE",
    ],
    includes = [
        ".",
        "external",
        "include",
        "source",
    ],
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "//conditions:default": [],
    }),
    local_defines = ["SLANG_USE_THREADS"],
    visibility = ["//visibility:public"],
    deps = ["@fmt"],
)
