// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dbSwapMasterSanityChecker.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/container/small_vector.hpp"
#include "dbModule.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "utl/Logger.h"

namespace odb {

dbSwapMasterSanityChecker::dbSwapMasterSanityChecker(dbModInst* new_mod_inst,
                                                     dbModule* src_module,
                                                     utl::Logger* logger)
    : new_mod_inst_(new_mod_inst),
      new_master_(new_mod_inst->getMaster()),
      src_module_(src_module),
      parent_(new_mod_inst->getParent()),
      logger_(logger)
{
}

int dbSwapMasterSanityChecker::run()
{
  warn_count_ = 0;
  error_count_ = 0;

  checkStructuralIntegrity();
  if (error_count_ > 0) {
    // Structural integrity failed — remaining checks would dereference
    // invalid pointers (e.g., null parent_ or new_master_).
    logger_->error(utl::ODB,
                   512,
                   "swapMaster structural integrity failed: {} error(s) "
                   "detected, aborting.",
                   error_count_);
  }

  checkPortPinMatching();
  checkHierNetConnectivity();
  checkFlatNetConnectivity();
  checkInstanceHierarchy();
  checkHashTableIntegrity();
  checkNoDanglingObjects();
  checkCombinationalLoops();

  const int total = warn_count_ + error_count_;
  if (total == 0) {
    logger_->info(utl::ODB, 496, "swapMaster sanity check passed (0 failures)");
  } else {
    logger_->info(utl::ODB,
                  497,
                  "swapMaster sanity check completed with {} warning(s) "
                  "and {} error(s)",
                  warn_count_,
                  error_count_);
  }

  if (error_count_ > 0) {
    logger_->error(utl::ODB,
                   499,
                   "swapMaster sanity check: {} error(s) detected, "
                   "aborting. See warnings above for details.",
                   error_count_);
  }

  return total;
}

void dbSwapMasterSanityChecker::warnMsg(const std::string& msg)
{
  ++warn_count_;
  logger_->warn(utl::ODB, 498, "SanityCheck(swapMaster) warning: {}", msg);
}

void dbSwapMasterSanityChecker::errorMsg(const std::string& msg)
{
  ++error_count_;
  logger_->warn(utl::ODB, 508, "SanityCheck(swapMaster) error: {}", msg);
}

std::string dbSwapMasterSanityChecker::masterContext() const
{
  return fmt::format("module '{}' (inst '{}')",
                     new_master_->getName(),
                     new_mod_inst_->getName());
}

// 1. Structural integrity: basic pointer consistency [ERROR]
void dbSwapMasterSanityChecker::checkStructuralIntegrity()
{
  if (parent_ == nullptr) {
    error("new_mod_inst '{}' has null parent", new_mod_inst_->getName());
    return;
  }

  if (new_master_ == nullptr) {
    error("new_mod_inst '{}' has null master", new_mod_inst_->getName());
    return;
  }

  if (new_master_->getModInst() != new_mod_inst_) {
    error("{}: getModInst() != new_mod_inst (reverse pointer mismatch)",
          masterContext());
  }

  dbModInst* found = parent_->findModInst(new_mod_inst_->getName());
  if (found != new_mod_inst_) {
    error("parent->findModInst('{}') does not return new_mod_inst",
          new_mod_inst_->getName());
  }
}

// 2. Port/pin matching: ModITerm <-> ModBTerm bidirectional links
//    Broken links / count mismatch → ERROR, IO type mismatch → WARNING
void dbSwapMasterSanityChecker::checkPortPinMatching()
{
  const std::string ctx = masterContext();

  // Count ModITerms while checking bidirectional links
  int iterm_count = new_mod_inst_->getModITerms().size();
  for (dbModITerm* iterm : new_mod_inst_->getModITerms()) {
    dbModBTerm* child_bterm = iterm->getChildModBTerm();
    if (child_bterm == nullptr) {
      error("{}: ModITerm '{}' has null child ModBTerm", ctx, iterm->getName());
      continue;
    }

    dbModITerm* back_iterm = child_bterm->getParentModITerm();
    if (back_iterm != iterm) {
      error(
          "{}: ModITerm '{}' -> child ModBTerm -> getParentModITerm() "
          "doesn't point back",
          ctx,
          iterm->getName());
    }

    if (std::string(iterm->getName()) != std::string(child_bterm->getName())) {
      error("{}: ModITerm name '{}' != child ModBTerm name '{}'",
            ctx,
            iterm->getName(),
            child_bterm->getName());
    }
  }

  // Count ModBTerms while checking parent ModITerm links
  int bterm_count = 0;
  for (dbModBTerm* bterm : new_master_->getModBTerms()) {
    if (bterm->isBusPort()) {
      continue;  // Bus-level headers don't have ModITerms by design
    }
    ++bterm_count;

    dbModITerm* parent_iterm = bterm->getParentModITerm();
    if (parent_iterm == nullptr) {
      error(
          "{}: ModBTerm '{}' has null parent ModITerm", ctx, bterm->getName());
      continue;
    }
    if (parent_iterm->getParent() != new_mod_inst_) {
      error("{}: ModBTerm '{}' parent ModITerm doesn't belong to new_mod_inst",
            ctx,
            bterm->getName());
    }
  }

  if (iterm_count != bterm_count) {
    error("{}: ModITerm count ({}) != ModBTerm count ({})",
          ctx,
          iterm_count,
          bterm_count);
  }

  // Check IO type consistency between new_master_ and src_module_
  for (dbModBTerm* new_bterm : new_master_->getModBTerms()) {
    dbModBTerm* src_bterm = src_module_->findModBTerm(new_bterm->getName());
    if (src_bterm == nullptr) {
      continue;
    }
    if (new_bterm->getIoType() != src_bterm->getIoType()) {
      warn("{}: ModBTerm '{}' IO type mismatch: new={} src={}",
           ctx,
           new_bterm->getName(),
           new_bterm->getIoType().getString(),
           src_bterm->getIoType().getString());
    }
  }
}

// 3. Hierarchical net connectivity
//    Wrong parent / missing reverse link → ERROR
//    Zero connections / missing internal ModNet → WARNING
void dbSwapMasterSanityChecker::checkHierNetConnectivity()
{
  const std::string ctx = masterContext();

  // Each ModITerm connected to a ModNet: net's parent should be parent_
  for (dbModITerm* iterm : new_mod_inst_->getModITerms()) {
    dbModNet* mod_net = iterm->getModNet();
    if (mod_net == nullptr) {
      continue;
    }

    if (mod_net->getParent() != parent_) {
      error(
          "{}: ModITerm '{}' connected to ModNet '{}' whose parent is not "
          "the parent module",
          ctx,
          iterm->getName(),
          mod_net->getName());
    }

    // Verify reverse link: the ModNet's ModITerms should contain this iterm
    bool found = false;
    for (dbModITerm* net_iterm : mod_net->getModITerms()) {
      if (net_iterm == iterm) {
        found = true;
        break;
      }
    }
    if (found == false) {
      error(
          "{}: ModNet '{}' does not contain ModITerm '{}' in its "
          "ModITerms list",
          ctx,
          mod_net->getName(),
          iterm->getName());
    }
  }

  // All ModNets inside new_master_: parent == new_master_, connectionCount > 0
  for (dbModNet* mod_net : new_master_->getModNets()) {
    if (mod_net->getParent() != new_master_) {
      error("{}: Internal ModNet '{}' parent is not new_master",
            ctx,
            mod_net->getName());
    }
    if (mod_net->connectionCount() == 0) {
      warn("{}: Internal ModNet '{}' has zero connections",
           ctx,
           mod_net->getName());
    }
  }

  // Each ModBTerm in new_master_: should have internal ModNet connection
  for (dbModBTerm* bterm : new_master_->getModBTerms()) {
    if (bterm->isBusPort()) {
      continue;  // Bus-level headers don't carry internal ModNet connections
    }
    if (bterm->getModNet() == nullptr) {
      warn("{}: ModBTerm '{}' has no internal ModNet connection",
           ctx,
           bterm->getName());
    }
  }

  // Call checkSanity on each internal ModNet
  for (dbModNet* mod_net : new_master_->getModNets()) {
    mod_net->checkSanity();
  }
}

// 4. Flat net connectivity
//    Wrong block → ERROR
void dbSwapMasterSanityChecker::checkFlatNetConnectivity()
{
  const std::string ctx = masterContext();

  // For each dbInst inside new_master_: verify flat net consistency
  dbBlock* block = new_master_->getOwner();
  for (dbInst* inst : new_master_->getInsts()) {
    if (inst->getModule() != new_master_) {
      // This is checked in checkInstanceHierarchy, skip here
      continue;
    }
    for (dbITerm* iterm : inst->getITerms()) {
      dbNet* net = iterm->getNet();

      // If ITerm has an internal ModNet but no flat net, the flat net
      // backing the hierarchical connection is missing.
      // Exception: if the ITerm is an output and the ModNet has no input
      // consumers, the signal is a dead-end output (producer with no
      // load) and the flat net absence is benign.
      if (net == nullptr) {
        dbModNet* mod_net = iterm->getModNet();
        if (mod_net != nullptr && mod_net->getParent() == new_master_) {
          // Check if this is a dead-end output (no internal consumers)
          bool has_input_consumer = false;
          if (iterm->isOutputSignal()) {
            for (dbITerm* net_iterm : mod_net->getITerms()) {
              if (net_iterm->isInputSignal()) {
                has_input_consumer = true;
                break;
              }
            }
          } else {
            has_input_consumer = true;  // ITerm itself is a consumer
          }
          if (has_input_consumer) {
            error(
                "{}: ITerm '{}' of inst '{}' has internal ModNet '{}' but "
                "no flat net (missing internal net creation)",
                ctx,
                iterm->getName(),
                inst->getName(),
                mod_net->getName());
          }
        }
        continue;
      }

      if (net->getBlock() != block) {
        error("{}: ITerm '{}' of inst '{}' connected to net in wrong block",
              ctx,
              iterm->getName(),
              inst->getName());
      }
    }
  }
}

// 5. Instance hierarchy: verify instances belong to correct modules [ERROR]
void dbSwapMasterSanityChecker::checkInstanceHierarchy()
{
  const std::string ctx = masterContext();

  // All dbInsts in new_master_: each inst's getModule() == new_master_
  for (dbInst* inst : new_master_->getInsts()) {
    if (inst->getModule() != new_master_) {
      error("{}: dbInst '{}' has wrong module pointer", ctx, inst->getName());
    }
  }

  // All child ModInsts in new_master_: parent == new_master_, master != nullptr
  for (dbModInst* child : new_master_->getModInsts()) {
    if (child->getParent() != new_master_) {
      error("{}: child ModInst '{}' has wrong parent", ctx, child->getName());
    }
    if (child->getMaster() == nullptr) {
      error("{}: child ModInst '{}' has null master", ctx, child->getName());
    }
  }

  // Instance counts should match src_module_
  int new_db_inst_count = new_master_->getDbInstCount();
  int src_db_inst_count = src_module_->getDbInstCount();
  if (new_db_inst_count != src_db_inst_count) {
    error("{}: dbInst count mismatch: new_master={} src_module={}",
          ctx,
          new_db_inst_count,
          src_db_inst_count);
  }

  int new_mod_inst_count = new_master_->getModInstCount();
  int src_mod_inst_count = src_module_->getModInstCount();
  if (new_mod_inst_count != src_mod_inst_count) {
    error("{}: ModInst count mismatch: new_master={} src_module={}",
          ctx,
          new_mod_inst_count,
          src_mod_inst_count);
  }
}

// 6. Hash table integrity: verify internal hash tables match actual sets
// [WARNING]
void dbSwapMasterSanityChecker::checkHashTableIntegrity()
{
  const std::string ctx = masterContext();

  _dbModule* mod_impl = (_dbModule*) new_master_;

  // ModBTerm hash
  int hash_bterm_count = static_cast<int>(mod_impl->modbterm_hash_.size());
  int set_bterm_count = new_master_->getModBTerms().size();
  for (dbModBTerm* bterm : new_master_->getModBTerms()) {
    // Cross-check: findModBTerm should return same object
    dbModBTerm* found = new_master_->findModBTerm(bterm->getName());
    if (found != bterm) {
      warn("{}: findModBTerm('{}') returns different object",
           ctx,
           bterm->getName());
    }
  }
  if (hash_bterm_count != set_bterm_count) {
    warn("{}: modbterm_hash_ size ({}) != getModBTerms size ({})",
         ctx,
         hash_bterm_count,
         set_bterm_count);
  }

  // ModNet hash
  int hash_net_count = static_cast<int>(mod_impl->modnet_hash_.size());
  int set_net_count = new_master_->getModNets().size();
  if (hash_net_count != set_net_count) {
    warn("{}: modnet_hash_ size ({}) != getModNets size ({})",
         ctx,
         hash_net_count,
         set_net_count);
  }

  // ModInst hash
  int hash_modinst_count = static_cast<int>(mod_impl->modinst_hash_.size());
  int set_modinst_count = new_master_->getModInsts().size();
  if (hash_modinst_count != set_modinst_count) {
    warn("{}: modinst_hash_ size ({}) != getModInsts size ({})",
         ctx,
         hash_modinst_count,
         set_modinst_count);
  }

  // dbInst hash
  int hash_dbinst_count = static_cast<int>(mod_impl->dbinst_hash_.size());
  int set_dbinst_count = new_master_->getInsts().size();
  if (hash_dbinst_count != set_dbinst_count) {
    warn("{}: dbinst_hash_ size ({}) != getInsts size ({})",
         ctx,
         hash_dbinst_count,
         set_dbinst_count);
  }
}

// 7. No dangling objects: verify no orphaned ModNets remain [WARNING]
void dbSwapMasterSanityChecker::checkNoDanglingObjects()
{
  // Check ModNets in parent module for zero connections
  // (new_master_ ModNets are already checked in checkHierNetConnectivity)
  for (dbModNet* mod_net : parent_->getModNets()) {
    if (mod_net->connectionCount() == 0) {
      warn("ModNet '{}' in parent module has zero connections",
           mod_net->getName());
    }
  }
}

// 8. Combinational loop detection in the flat netlist within new_master_
// [ERROR]
void dbSwapMasterSanityChecker::checkCombinationalLoops()
{
  const std::string ctx = masterContext();

  // Build instance index map for instances in new_master_
  std::unordered_map<dbInst*, int> inst_to_idx;
  std::vector<dbInst*> insts;
  for (dbInst* inst : new_master_->getInsts()) {
    inst_to_idx[inst] = static_cast<int>(insts.size());
    insts.push_back(inst);
  }

  if (insts.empty()) {
    return;
  }

  const int n = static_cast<int>(insts.size());

  // Build adjacency list: edge from inst A to inst B if A has an output
  // ITerm connected to a net that has an input ITerm on B.
  std::vector<boost::container::small_vector<int, 8>> adj(n);
  for (int i = 0; i < n; i++) {
    for (dbITerm* iterm : insts[i]->getITerms()) {
      if (!iterm->isOutputSignal()) {
        continue;
      }
      dbNet* net = iterm->getNet();
      if (net == nullptr) {
        continue;
      }
      for (dbITerm* sink : net->getITerms()) {
        if (sink == iterm) {
          continue;
        }
        if (!sink->isInputSignal()) {
          continue;
        }
        auto it = inst_to_idx.find(sink->getInst());
        if (it != inst_to_idx.end()) {
          adj[i].push_back(it->second);
        }
      }
    }
  }

  // Iterative DFS-based cycle detection with path tracking
  enum DfsState
  {
    kUnvisited,
    kInPath,
    kProcessed
  };
  std::vector<DfsState> state(n, kUnvisited);
  std::vector<int> path;
  // DFS stack: (node, index into adj[node])
  std::vector<std::pair<int, int>> stack;

  for (int start = 0; start < n; start++) {
    if (state[start] != kUnvisited) {
      continue;
    }

    stack.emplace_back(start, 0);
    state[start] = kInPath;
    path.push_back(start);

    while (!stack.empty()) {
      auto& [u, edge_idx] = stack.back();

      if (edge_idx < static_cast<int>(adj[u].size())) {
        int v = adj[u][edge_idx];
        ++edge_idx;

        if (state[v] == kInPath) {
          // Found a cycle — collect instances in the loop
          std::string loop_path;
          bool in_loop = false;
          for (int idx : path) {
            if (idx == v) {
              in_loop = true;
            }
            if (in_loop) {
              if (loop_path.empty() == false) {
                loop_path += " -> ";
              }
              loop_path += insts[idx]->getName();
            }
          }
          loop_path += " -> ";
          loop_path += insts[v]->getName();
          error("{}: combinational loop detected: {}", ctx, loop_path);
          return;
        }
        if (state[v] == kUnvisited) {
          state[v] = kInPath;
          path.push_back(v);
          stack.emplace_back(v, 0);
        }
      } else {
        // All neighbors processed, backtrack
        path.pop_back();
        state[u] = kProcessed;
        stack.pop_back();
      }
    }
  }
}

}  // namespace odb
