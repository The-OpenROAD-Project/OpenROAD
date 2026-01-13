// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ConcreteSwapArithModules.hh"

#include <algorithm>
#include <fstream>
#include <vector>

#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Report.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

namespace rsz {

using odb::dbBlock;
using odb::dbInst;
using odb::dbModInst;
using odb::dbModITerm;
using odb::dbModule;
using odb::dbStringProperty;
using sta::Graph;
using sta::Instance;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::PortDirection;
using sta::Slack;
using sta::Vertex;
using sta::VertexSet;
using std::pair;
using std::vector;
using utl::RSZ;

// debugPrint for replace_arith level 1
#define debugRAPrint1(format_str, ...) \
  debugPrint(logger_, RSZ, "replace_arith", 1, format_str, ##__VA_ARGS__)
// debugPrint for replace_arith level 2
#define debugRAPrint2(format_str, ...) \
  debugPrint(logger_, RSZ, "replace_arith", 2, format_str, ##__VA_ARGS__)

ConcreteSwapArithModules::ConcreteSwapArithModules(Resizer* resizer)
    : SwapArithModules(resizer)
{
}

void ConcreteSwapArithModules::init()
{
  if (init_) {
    return;
  }

  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;
  init_ = true;
}

bool ConcreteSwapArithModules::replaceArithModules(const int path_count,
                                                   const std::string& target,
                                                   const float slack_threshold)
{
  init();

  logger_->info(
      RSZ,
      151,
      "Starting arithmetic module replacement to optimize '{}' target",
      target);

  // Identify critical mod instances based on target, path_count and
  // slack_threshold
  std::set<dbModInst*> arithInsts;
  findCriticalInstances(path_count, target, slack_threshold, arithInsts);
  if (arithInsts.empty()) {
    return false;
  }

  // Replace arithmetic instances to improve target QoR
  return doSwapInstances(arithInsts, target);
}

void ConcreteSwapArithModules::findCriticalInstances(
    const int path_count,
    const std::string& target,
    const float slack_threshold,
    std::set<dbModInst*>& insts)
{
  logger_->info(RSZ,
                152,
                "Analyzing {} critical paths with slack < {:.2f}",
                path_count,
                slack_threshold);

  // Find all violating endpoints and slacks
  const VertexSet* endpoints = sta_->endpoints();
  vector<pair<Vertex*, Slack>> violating_ends;
  for (Vertex* end : *endpoints) {
    const Slack end_slack = sta_->vertexSlack(end, max_);
    if (end_slack < slack_threshold) {
      violating_ends.emplace_back(end, end_slack);
    }
  }
  std::ranges::stable_sort(violating_ends,
                           [](const auto& end_slack1, const auto& end_slack2) {
                             return end_slack1.second < end_slack2.second;
                           });

  logger_->info(
      RSZ, 153, "Identified {} violating endpoints", violating_ends.size());

  // Collect arithmetic instances along critical paths to violating endpoints
  int num_endpoints = 0;
  for (const auto& vertex_slack : violating_ends) {
    if (num_endpoints < path_count) {
      debugRAPrint1(
          "Collecting worst path instances for endpoint {} with slack {}",
          vertex_slack.first->name(network_),
          vertex_slack.second);
      const Path* end_path
          = sta_->vertexWorstSlackPath(vertex_slack.first, max_);
      collectArithInstsOnPath(end_path, insts);
      num_endpoints++;
    }
  }
  logger_->info(
      RSZ, 154, "Identified {} critical arithmetic instances", insts.size());

  if (logger_->debugCheck(RSZ, "replace_arith", 1)) {
    logger_->report("{} critical instances are as follows:", insts.size());
    for (dbModInst* mod_inst : insts) {
      dbModule* master = mod_inst->getMaster();
      logger_->report(
          "  Instance {} Module {}", mod_inst->getName(), master->getName());
    }
  }
}

void ConcreteSwapArithModules::collectArithInstsOnPath(
    const Path* path,
    std::set<dbModInst*>& arithInsts)
{
  PathExpanded expanded(path, sta_);
  if (expanded.size() > 1) {
    const int path_length = expanded.size();
    const int start_index = expanded.startIndex();
    for (int i = start_index; i < path_length; i++) {
      const Path* path_i = expanded.path(i);
      Vertex* vertex = path_i->vertex(sta_);
      if (vertex) {
        Pin* pin = vertex->pin();
        if (pin && network_->direction(pin)->isAnyOutput()) {
          debugRAPrint2(
              "Traversing output pin {} at path {}", network_->name(pin), i);
          const Instance* inst = network_->instance(pin);
          dbModInst* db_mod_inst;
          if (isArithInstance(inst, db_mod_inst)) {
            arithInsts.insert(db_mod_inst);
          }
        }
      }
    }
  }
}

bool ConcreteSwapArithModules::isArithInstance(const Instance* inst,
                                               dbModInst*& mod_inst)
{
  if (inst == nullptr) {
    return false;
  }

  init();
  mod_inst = nullptr;

  const Instance* curr_inst = inst;
  int hier_depth = 0;
  const int MAX_HIER_DEPTH = 100;

  do {
    debugRAPrint2("Traversing instance {}", network_->name(curr_inst));
    dbInst* db_inst = nullptr;
    dbModInst* curr_mod_inst = nullptr;
    db_network_->staToDb(curr_inst, db_inst, curr_mod_inst);
    if (curr_mod_inst) {
      debugRAPrint2("  Instance {} has mod inst", network_->name(curr_inst));
      if (hasArithOperatorProperty(
              static_cast<const dbModInst*>(curr_mod_inst))) {
        mod_inst = curr_mod_inst;
        return true;
      }
      debugRAPrint2("  Mod inst has no arith operator");
    }
    curr_inst = network_->parent(curr_inst);
    hier_depth++;
  } while (curr_inst != nullptr && hier_depth < MAX_HIER_DEPTH);

  if (hier_depth >= MAX_HIER_DEPTH) {
    logger_->warn(
        RSZ,
        161,
        "Arithmetic instance search terminated after hierarchy depth of {}",
        MAX_HIER_DEPTH);
  }

  return false;
}

bool ConcreteSwapArithModules::hasArithOperatorProperty(
    const dbModInst* mod_inst)
{
  if (!mod_inst) {
    return false;
  }

  dbStringProperty* prop = dbStringProperty::find(
      const_cast<dbModInst*>(mod_inst), "implements_operator");
  if (prop) {
    debugRAPrint2(
        "Found arith instance {} [{}]", mod_inst->getName(), prop->getValue());
    return true;
  }

  dbModule* module = mod_inst->getMaster();
  if (module) {
    const char* name = module->getName();
    debugRAPrint2("  Mod inst has master {}", name);
    if (strncmp(name, "ALU_", 4) == 0 || strncmp(name, "MACC_", 5) == 0) {
      debugRAPrint2("Found arith instance {} [{}]", mod_inst->getName(), name);
      return true;
    }
  }

  return false;
}

bool ConcreteSwapArithModules::doSwapInstances(std::set<dbModInst*>& insts,
                                               const std::string& target)
{
  int swapped_count = 0;
  std::set<dbModInst*> swappedInsts;

  // We DO NOT suppress callbacks. Instead, we manage graph levelization
  // to prevent STA-2300 (Level Regression) during incremental updates.

  for (dbModInst* inst : insts) {
    // Skip if instance is no longer in its parent
    if (!inst->getParent()) {
      continue;
    }

    dbModule* old_master = inst->getMaster();
    if (!old_master) {
      continue;
    }

    std::string old_name(old_master->getName());
    std::string new_name;
    produceNewModuleName(old_name, new_name, target);

    if (new_name != old_name) {
      dbBlock* block = old_master->getOwner();
      dbModule* new_master = block->findModule(new_name.c_str());
      if (!new_master) {
        continue;
      }

      // Pre-emptive clear of levels to avoid STA-2300 when callbacks trigger
      // timing update
      sta_->search()->levelsChangedBefore();

      // DEBUG: Check graph connections BEFORE swap (only for the first
      // instance)
      static bool debug_printed = false;
      if (!debug_printed) {
        sta_->dumpGraphConnections(inst->getName(),
                                   "sta_graph_before.log");
        debug_printed = true;
      }

      // swapMaster triggers callbacks (inDbModITermPostConnect etc)
      // which will update the STA graph automatically.
      dbModInst* new_inst = inst->swapMaster(new_master);

      if (new_inst) {
        swapped_count++;
        swappedInsts.insert(new_inst);
      }
    }
  }

  if (!swappedInsts.empty()) {
    // Ensure graph is consistent and levelized after all changes
    debugRAPrint1("Forcing full re-levelization");
    sta_->graphDelayCalc()->delaysInvalid();
    sta_->search()->levelsChangedBefore();
    sta_->ensureLevelized();

    // DEBUG: Check graph connections AFTER swap and levelization
    sta_->dumpGraphConnections((*swappedInsts.begin())->getName(),
                               "sta_graph_after.log");
  }

  insts.clear();
  insts.insert(swappedInsts.begin(), swappedInsts.end());

  logger_->info(RSZ,
                160,
                "{} arithmetic instances have swapped to improve '{}' target",
                swapped_count,
                target);
  logger_->metric("design__instance__count__swapped_arithmetic_operator",
                  swapped_count);
  return (swapped_count > 0);
}

void ConcreteSwapArithModules::produceNewModuleName(const std::string& old_name,
                                                    std::string& new_name,
                                                    const std::string& target)
{
  new_name = old_name;  // Default
  if (old_name.empty()) {
    return;
  }

  if (target == "setup") {
    const std::string ALU_TARGET("KOGGE_STONE");
    const std::string MACC_TARGET("BASE");
    size_t pos;
    if (old_name.starts_with("ALU_")) {
      // Swap ALU to KOGGE_STONE for best timing
      const std::vector<std::string> alu_types
          = {"HAN_CARLSON", "BRENT_KUNG", "SKLANSKY"};
      for (const std::string& alu_type : alu_types) {
        pos = old_name.find(alu_type);
        if (pos != std::string::npos) {
          new_name = old_name.substr(0, pos) + ALU_TARGET;
          return;
        }
      }
    } else if (old_name.starts_with("MACC_")) {
      // Swap multiplier to Han-Carlson BASE for best timing
      pos = old_name.find("BOOTH");
      if (pos != std::string::npos) {
        new_name = old_name.substr(0, pos) + MACC_TARGET;
        return;
      }
    }
  } else {
    logger_->error(RSZ, 155, "Target {} is not supported now", target);
  }
}

void ConcreteSwapArithModules::rebuildInternalGraph(dbModInst* mod_inst)
{
  Graph* graph = sta_->graph();

  // 1. ModInst itself (Boundary Pins)
  // Manually iterate because pinIterator might rely on stale dbNetwork state
  // or callbacks
  for (dbModITerm* iterm : mod_inst->getModITerms()) {
    Pin* pin = db_network_->dbToSta(iterm);
    // Force dbNetwork to recognize the new pin connections
    db_network_->connectPinAfter(pin);
    graph->makePinVertices(pin);
  }

  dbModule* master = mod_inst->getMaster();

  // 2. Leaf instances
  for (dbInst* inst : master->getInsts()) {
    Instance* sta_inst = db_network_->dbToSta(inst);
    sta_->makeInstanceAfter(sta_inst);
  }

  // 3. Sub-modules (Recursion)
  for (dbModInst* child : master->getModInsts()) {
    rebuildInternalGraph(child);
  }
}

}  // namespace rsz
