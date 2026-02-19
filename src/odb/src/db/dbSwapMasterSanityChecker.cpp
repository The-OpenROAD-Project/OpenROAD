// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dbSwapMasterSanityChecker.h"

#include <string>
#include <unordered_map>
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

void dbSwapMasterSanityChecker::warn(const std::string& msg)
{
  ++warn_count_;
  logger_->warn(utl::ODB, 498, "SanityCheck(swapMaster) warning: {}", msg);
}

void dbSwapMasterSanityChecker::error(const std::string& msg)
{
  ++error_count_;
  logger_->warn(utl::ODB, 508, "SanityCheck(swapMaster) error: {}", msg);
}

std::string dbSwapMasterSanityChecker::masterContext() const
{
  return std::string("module '") + new_master_->getName() + "' (inst '"
         + new_mod_inst_->getName() + "')";
}

// 1. Structural integrity: basic pointer consistency [ERROR]
void dbSwapMasterSanityChecker::checkStructuralIntegrity()
{
  if (parent_ == nullptr) {
    error("new_mod_inst '" + std::string(new_mod_inst_->getName())
          + "' has null parent");
    return;
  }

  if (new_master_ == nullptr) {
    error("new_mod_inst '" + std::string(new_mod_inst_->getName())
          + "' has null master");
    return;
  }

  if (new_master_->getModInst() != new_mod_inst_) {
    error(masterContext()
          + ": getModInst() != new_mod_inst (reverse pointer mismatch)");
  }

  dbModInst* found = parent_->findModInst(new_mod_inst_->getName());
  if (found != new_mod_inst_) {
    error(std::string("parent->findModInst('") + new_mod_inst_->getName()
          + "') does not return new_mod_inst");
  }
}

// 2. Port/pin matching: ModITerm <-> ModBTerm bidirectional links
//    Broken links / count mismatch → ERROR, IO type mismatch → WARNING
void dbSwapMasterSanityChecker::checkPortPinMatching()
{
  const std::string ctx = masterContext();

  // Count ModITerms while checking bidirectional links
  int iterm_count = 0;
  for (dbModITerm* iterm : new_mod_inst_->getModITerms()) {
    ++iterm_count;

    dbModBTerm* child_bterm = iterm->getChildModBTerm();
    if (child_bterm == nullptr) {
      error(ctx + ": ModITerm '" + iterm->getName()
            + "' has null child ModBTerm");
      continue;
    }

    dbModITerm* back_iterm = child_bterm->getParentModITerm();
    if (back_iterm != iterm) {
      error(ctx + ": ModITerm '" + iterm->getName()
            + "' -> child ModBTerm -> getParentModITerm() doesn't point back");
    }

    if (std::string(iterm->getName()) != std::string(child_bterm->getName())) {
      error(ctx + ": ModITerm name '" + iterm->getName()
            + "' != child ModBTerm name '" + child_bterm->getName() + "'");
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
      error(ctx + ": ModBTerm '" + bterm->getName()
            + "' has null parent ModITerm");
      continue;
    }
    if (parent_iterm->getParent() != new_mod_inst_) {
      error(ctx + ": ModBTerm '" + bterm->getName()
            + "' parent ModITerm doesn't belong to new_mod_inst");
    }
  }

  if (iterm_count != bterm_count) {
    error(ctx + ": ModITerm count (" + std::to_string(iterm_count)
          + ") != ModBTerm count (" + std::to_string(bterm_count) + ")");
  }

  // Check IO type consistency between new_master_ and src_module_
  for (dbModBTerm* new_bterm : new_master_->getModBTerms()) {
    dbModBTerm* src_bterm = src_module_->findModBTerm(new_bterm->getName());
    if (src_bterm == nullptr) {
      continue;
    }
    if (new_bterm->getIoType() != src_bterm->getIoType()) {
      warn(ctx + ": ModBTerm '" + new_bterm->getName()
           + "' IO type mismatch: new=" + new_bterm->getIoType().getString()
           + " src=" + src_bterm->getIoType().getString());
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
      error(ctx + ": ModITerm '" + iterm->getName() + "' connected to ModNet '"
            + mod_net->getName() + "' whose parent is not the parent module");
    }

    // Verify reverse link: the ModNet's ModITerms should contain this iterm
    bool found = false;
    for (dbModITerm* net_iterm : mod_net->getModITerms()) {
      if (net_iterm == iterm) {
        found = true;
        break;
      }
    }
    if (!found) {
      error(ctx + ": ModNet '" + mod_net->getName()
            + "' does not contain ModITerm '" + iterm->getName()
            + "' in its ModITerms list");
    }
  }

  // All ModNets inside new_master_: parent == new_master_, connectionCount > 0
  for (dbModNet* mod_net : new_master_->getModNets()) {
    if (mod_net->getParent() != new_master_) {
      error(ctx + ": Internal ModNet '" + mod_net->getName()
            + "' parent is not new_master");
    }
    if (mod_net->connectionCount() == 0) {
      warn(ctx + ": Internal ModNet '" + mod_net->getName()
           + "' has zero connections");
    }
  }

  // Each ModBTerm in new_master_: should have internal ModNet connection
  for (dbModBTerm* bterm : new_master_->getModBTerms()) {
    if (bterm->isBusPort()) {
      continue;  // Bus-level headers don't carry internal ModNet connections
    }
    if (bterm->getModNet() == nullptr) {
      warn(ctx + ": ModBTerm '" + bterm->getName()
           + "' has no internal ModNet connection");
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
            error(ctx + ": ITerm '" + iterm->getName() + "' of inst '"
                  + inst->getName() + "' has internal ModNet '"
                  + mod_net->getName()
                  + "' but no flat net (missing internal net creation)");
          }
        }
        continue;
      }

      if (net->getBlock() != block) {
        error(ctx + ": ITerm '" + iterm->getName() + "' of inst '"
              + inst->getName() + "' connected to net in wrong block");
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
      error(ctx + ": dbInst '" + inst->getName()
            + "' has wrong module pointer");
    }
  }

  // All child ModInsts in new_master_: parent == new_master_, master != nullptr
  for (dbModInst* child : new_master_->getModInsts()) {
    if (child->getParent() != new_master_) {
      error(ctx + ": child ModInst '" + child->getName()
            + "' has wrong parent");
    }
    if (child->getMaster() == nullptr) {
      error(ctx + ": child ModInst '" + child->getName() + "' has null master");
    }
  }

  // Instance counts should match src_module_
  int new_db_inst_count = new_master_->getDbInstCount();
  int src_db_inst_count = src_module_->getDbInstCount();
  if (new_db_inst_count != src_db_inst_count) {
    error(ctx + ": dbInst count mismatch: new_master="
          + std::to_string(new_db_inst_count)
          + " src_module=" + std::to_string(src_db_inst_count));
  }

  int new_mod_inst_count = new_master_->getModInstCount();
  int src_mod_inst_count = src_module_->getModInstCount();
  if (new_mod_inst_count != src_mod_inst_count) {
    error(ctx + ": ModInst count mismatch: new_master="
          + std::to_string(new_mod_inst_count)
          + " src_module=" + std::to_string(src_mod_inst_count));
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
  int set_bterm_count = 0;
  for (dbModBTerm* bterm : new_master_->getModBTerms()) {
    ++set_bterm_count;
    // Cross-check: findModBTerm should return same object
    dbModBTerm* found = new_master_->findModBTerm(bterm->getName());
    if (found != bterm) {
      warn(ctx + ": findModBTerm('" + bterm->getName()
           + "') returns different object");
    }
  }
  if (hash_bterm_count != set_bterm_count) {
    warn(ctx + ": modbterm_hash_ size (" + std::to_string(hash_bterm_count)
         + ") != getModBTerms size (" + std::to_string(set_bterm_count) + ")");
  }

  // ModNet hash
  int hash_net_count = static_cast<int>(mod_impl->modnet_hash_.size());
  int set_net_count = 0;
  for ([[maybe_unused]] dbModNet* net : new_master_->getModNets()) {
    ++set_net_count;
  }
  if (hash_net_count != set_net_count) {
    warn(ctx + ": modnet_hash_ size (" + std::to_string(hash_net_count)
         + ") != getModNets size (" + std::to_string(set_net_count) + ")");
  }

  // ModInst hash
  int hash_modinst_count = static_cast<int>(mod_impl->modinst_hash_.size());
  int set_modinst_count = 0;
  for ([[maybe_unused]] dbModInst* mi : new_master_->getModInsts()) {
    ++set_modinst_count;
  }
  if (hash_modinst_count != set_modinst_count) {
    warn(ctx + ": modinst_hash_ size (" + std::to_string(hash_modinst_count)
         + ") != getModInsts size (" + std::to_string(set_modinst_count) + ")");
  }

  // dbInst hash
  int hash_dbinst_count = static_cast<int>(mod_impl->dbinst_hash_.size());
  int set_dbinst_count = 0;
  for ([[maybe_unused]] dbInst* inst : new_master_->getInsts()) {
    ++set_dbinst_count;
  }
  if (hash_dbinst_count != set_dbinst_count) {
    warn(ctx + ": dbinst_hash_ size (" + std::to_string(hash_dbinst_count)
         + ") != getInsts size (" + std::to_string(set_dbinst_count) + ")");
  }
}

// 7. No dangling objects: verify no orphaned ModNets remain [WARNING]
void dbSwapMasterSanityChecker::checkNoDanglingObjects()
{
  // Check ModNets in parent module for zero connections
  // (new_master_ ModNets are already checked in checkHierNetConnectivity)
  for (dbModNet* mod_net : parent_->getModNets()) {
    if (mod_net->connectionCount() == 0) {
      warn(std::string("ModNet '") + mod_net->getName()
           + "' in parent module has zero connections");
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
  // 0=unvisited, 1=in current DFS path, 2=fully processed
  std::vector<int> state(n, 0);
  std::vector<int> path;
  // DFS stack: (node, index into adj[node])
  std::vector<std::pair<int, int>> stack;

  for (int start = 0; start < n; start++) {
    if (state[start] != 0) {
      continue;
    }

    stack.emplace_back(start, 0);
    state[start] = 1;
    path.push_back(start);

    while (!stack.empty()) {
      auto& [u, edge_idx] = stack.back();

      if (edge_idx < static_cast<int>(adj[u].size())) {
        int v = adj[u][edge_idx];
        ++edge_idx;

        if (state[v] == 1) {
          // Found a cycle — collect instances in the loop
          std::string msg;
          msg += ctx;
          msg += ": combinational loop detected: ";
          bool in_loop = false;
          for (int idx : path) {
            if (idx == v) {
              in_loop = true;
            }
            if (in_loop) {
              if (msg.back() != ' ') {
                msg += " -> ";
              }
              msg += insts[idx]->getName();
            }
          }
          msg += " -> ";
          msg += insts[v]->getName();
          error(msg);
          return;
        }
        if (state[v] == 0) {
          state[v] = 1;
          path.push_back(v);
          stack.emplace_back(v, 0);
        }
      } else {
        // All neighbors processed, backtrack
        path.pop_back();
        state[u] = 2;
        stack.pop_back();
      }
    }
  }
}

}  // namespace odb
