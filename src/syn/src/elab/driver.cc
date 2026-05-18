// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//

#include "driver.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "backend_builder.h"
#include "blackboxes.h"
#include "log_stubs.h"
#include "slang/ast/Compilation.h"
#include "slang/ast/SemanticFacts.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/diagnostics/DiagnosticEngine.h"
#include "slang/diagnostics/Diagnostics.h"
#include "slang/driver/Driver.h"
#include "slang_frontend.h"
#include "syn/ir/Graph.h"

namespace syn {

using namespace slang_frontend;  // NOLINT(google-build-using-namespace)

static std::optional<Graph> elaborate1(const std::vector<std::string>& args,
                                       sta::dbSta* sta,
                                       std::optional<std::string> buffer)
{
  slang::driver::Driver driver;
  driver.addStandardArgs();

  SynthesisSettings settings;
  settings.addOptions(driver.cmdLine);

  if (buffer) {
    // Feed the source text directly into slang's source manager.
    auto buffer1 = driver.sourceManager.assignText("<inline>",
                                                   std::string_view{*buffer});
    driver.sourceLoader.addBuffer(buffer1);
  }

  // Build argv for slang's command-line parser.
  // argv[0] is the program name (required by slang).
  std::vector<const char*> c_args;
  c_args.reserve(args.size() + 1);
  c_args.push_back("syn_elaborate");
  for (auto& a : args) {
    c_args.push_back(a.c_str());
  }

  if (!driver.parseCommandLine(static_cast<int>(c_args.size()),
                               const_cast<char**>(c_args.data()))) {
    return std::nullopt;
  }

  fixup_options(settings, driver);
  if (!driver.processOptions()) {
    return std::nullopt;
  }
  catch_forbidden_options(driver);

  if (!driver.parseAllSources()) {
    return std::nullopt;
  }

  auto compilation = driver.createCompilation();

  // Import Liberty cell definitions as blackbox modules.
  if (sta) {
    importBlackboxesFromLiberty(driver.sourceManager, *compilation, sta);
  }

  // Report compilation diagnostics.
  driver.reportCompilation(*compilation, /*quiet=*/false);

  if (driver.diagEngine.getNumErrors()) {
    (void) driver.reportDiagnostics(/*quiet=*/false);
    return std::nullopt;
  }

  if (settings.ast_compilation_only.value_or(false)) {
    (void) driver.reportDiagnostics(/*quiet=*/false);
    return std::nullopt;
  }

  // We require exactly one top-level instance.
  std::vector<const slang::ast::InstanceSymbol*> tops;
  for (auto* instance : compilation->getRoot().topInstances) {
    if (instance->getDefinition().definitionKind
        == slang::ast::DefinitionKind::Program) {
      continue;
    }
    tops.push_back(instance);
  }

  if (tops.size() != 1) {
    log_error("Expected exactly one top-level module, got %d\n",
              (int) tops.size());
  }

  auto* instance = tops[0];
  HierarchyQueue hqueue;
  auto backend = std::make_unique<BackendGraphBuilder>();
  NetlistContext netlist(
      std::move(backend), settings, *compilation, *tops.front());
  populate_netlist(hqueue, netlist);

  {
    // Report per-netlist diagnostics.
    slang::Diagnostics diags;
    diags.append_range(netlist.issued_diagnostics);
    diags.sort(driver.sourceManager);
    for (int j = 0; j < (int) diags.size(); j++) {
      if (j > 0 && diags[j] == diags[j - 1]) {
        continue;
      }
      driver.diagEngine.issue(diags[j]);
    }
  }

  if (!driver.reportDiagnostics(/*quiet=*/false)) {
    return std::nullopt;
  }

  // Extract the graph from the top netlist's backend.
  auto* builder = static_cast<BackendGraphBuilder*>(netlist.backend.get());
  builder->graph().setName(std::string(instance->name));
  return std::move(builder->graph_);
}

std::optional<Graph> elaborate(const std::vector<std::string>& args,
                               sta::dbSta* sta)
{
  return elaborate1(args, sta, {});
}

std::optional<Graph> elaborateText(const std::string& source,
                                   const std::vector<std::string>& args)
{
  return elaborate1(args, nullptr, source);
}

}  // namespace syn
