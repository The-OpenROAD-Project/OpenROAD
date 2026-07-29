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
#include "slang/ast/Compilation.h"
#include "slang/ast/SemanticFacts.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/diagnostics/DiagnosticEngine.h"
#include "slang/diagnostics/Diagnostics.h"
#include "slang/driver/Driver.h"
#include "slang_frontend.h"
#include "syn/ir/Graph.h"

namespace utl {
class Logger;
}

namespace syn {

using namespace slang_frontend;  // NOLINT(google-build-using-namespace)

// Non-static so error.cc can call it from elaborateText. Not declared in
// driver.h because the source-buffer parameter is only useful to the test
// helper.
std::unique_ptr<Graph> elaborateImpl(utl::Logger* logger,
                                     const std::vector<std::string>& args,
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
    return nullptr;
  }

  fixup_options(settings, driver);
  if (!driver.processOptions()) {
    return nullptr;
  }
  catch_forbidden_options(driver);

  if (!driver.parseAllSources()) {
    return nullptr;
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
    return nullptr;
  }

  if (settings.ast_compilation_only.value_or(false)) {
    (void) driver.reportDiagnostics(/*quiet=*/false);
    return nullptr;
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
  backend->graph_->setLogger(logger);
  NetlistContext netlist(
      std::move(backend), settings, *compilation, *tops.front());
  populate_netlist(hqueue, netlist);

  {
    // Report per-netlist diagnostics.
    slang::Diagnostics diags;
    diags.append_range(netlist.issued_diagnostics);
    diags.sort(driver.sourceManager);
    for (int j = 0; j < diags.size(); j++) {
      if (j > 0 && diags[j] == diags[j - 1]) {
        continue;
      }
      driver.diagEngine.issue(diags[j]);
    }
  }

  if (!driver.reportDiagnostics(/*quiet=*/false)) {
    return nullptr;
  }

  // Extract the graph from the top netlist's backend.
  auto* builder = static_cast<BackendGraphBuilder*>(netlist.backend.get());
  builder->graph().setName(std::string(instance->name));
  return std::move(builder->graph_);
}

std::unique_ptr<Graph> elaborate(utl::Logger* logger,
                                 const std::vector<std::string>& args,
                                 sta::dbSta* sta)
{
  return elaborateImpl(logger, args, sta, {});
}

}  // namespace syn
