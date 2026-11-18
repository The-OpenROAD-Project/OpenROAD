// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
class FuncExpr;
class Instance;
class LibertyPort;
class Sequential;
}  // namespace sta

namespace utl {
class Logger;
}

namespace ret {

// Rewrites synchronous registers as a plain D flip-flop plus logic
// (inverters, 2-input ANDs and ORs, tie cells) computing the cell's liberty
// `next_state`.  Registers with asynchronous controls, latches, scan cells
// and multi-bit cells are left untouched.
//
// In a hierarchical netlist the cells that replace a register are created
// in the register's module.
class SequentialUnmapper
{
 public:
  SequentialUnmapper(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);

  bool prepare();

  void run(const std::unordered_set<odb::dbInst*>* candidates = nullptr);

  odb::dbMaster* getBaseSequentialMaster() const { return seq_master_; }
  odb::dbMTerm* getBaseSequentialQMTerm() const { return seq_q_mterm_; }
  odb::dbMaster* getNegedgeSequentialMaster() const
  {
    return negedge_seq_master_;
  }

 private:
  using Register = std::pair<odb::dbInst*, const sta::Sequential*>;

  struct RegPins
  {
    odb::dbITerm* d = nullptr;
    odb::dbITerm* q = nullptr;
    odb::dbITerm* clk = nullptr;
  };

  struct Untouched
  {
    int async = 0;
    int latches = 0;
    int scan = 0;
    int other = 0;
  };

  struct OutputMove
  {
    odb::dbNet* old_net = nullptr;
    odb::dbModNet* old_mod_net = nullptr;
    odb::dbNet* new_net = nullptr;
    std::vector<odb::dbITerm*> loads;
    std::vector<odb::dbBTerm*> ports;
  };

  bool findBaseSequentialCell(bool negedge,
                              odb::dbMaster*& master,
                              odb::dbMTerm*& q_mterm);
  bool findInverterCell();
  bool findTwoInputGate(bool is_and, odb::dbMaster*& master);
  bool findTieCell(bool one);

  std::vector<Register> collectRegisters(
      const std::unordered_set<odb::dbInst*>* candidates,
      Untouched& untouched) const;
  void reportRun(const std::map<std::string, int>& per_cell,
                 const Untouched& untouched) const;

  void decompose(odb::dbInst* inst, const sta::Sequential* seq);
  odb::dbNet* collectRegisterNets(odb::dbInst* inst,
                                  std::vector<odb::dbITerm*>& out_pins);
  std::vector<OutputMove> collectOutputMoves(
      odb::dbInst* inst,
      const std::vector<odb::dbITerm*>& out_pins);
  void applyOutputMoves(const std::vector<OutputMove>& moves);
  void traceDecompose(const std::string& inst_name,
                      const std::string& inst_master,
                      odb::dbInst* flop);
  bool canSynth(const sta::FuncExpr* expr) const;
  odb::dbNet* synth(const sta::FuncExpr* expr);
  bool isInverted(const sta::FuncExpr* func) const;
  RegPins getRegPins(odb::dbInst* inst, const sta::Sequential* seq) const;

  void setParent(odb::dbInst* inst);
  void connectInput(odb::dbITerm* pin, odb::dbNet* net);
  odb::dbNet* newNet(const char* base_name);
  odb::dbInst* newInst(const char* base_name, odb::dbMaster* master);
  odb::dbNet* makeInverter(odb::dbNet* input);
  odb::dbNet* makeTwoInputGate(bool is_and, odb::dbNet* a, odb::dbNet* b);
  odb::dbNet* makeTie(bool one);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  odb::dbBlock* block_ = nullptr;

  odb::dbMaster* seq_master_ = nullptr;
  odb::dbMTerm* seq_q_mterm_ = nullptr;
  odb::dbMaster* negedge_seq_master_ = nullptr;
  odb::dbMTerm* negedge_seq_q_mterm_ = nullptr;
  odb::dbMaster* inv_master_ = nullptr;
  odb::dbMaster* and2_master_ = nullptr;
  odb::dbMaster* or2_master_ = nullptr;
  odb::dbMaster* tie1_master_ = nullptr;
  odb::dbMTerm* tie1_out_mterm_ = nullptr;
  odb::dbMaster* tie0_master_ = nullptr;
  odb::dbMTerm* tie0_out_mterm_ = nullptr;

  std::unordered_map<const sta::LibertyPort*, odb::dbNet*> in_port_net_;
  // Module net of each net the register reads; empty in a flat netlist.
  std::unordered_map<odb::dbNet*, odb::dbModNet*> mod_net_;
  sta::Instance* parent_ = nullptr;
  odb::dbModInst* parent_mod_inst_ = nullptr;
  const sta::LibertyPort* out_port_ = nullptr;
  const sta::LibertyPort* out_inv_port_ = nullptr;
  odb::dbNet* state_net_ = nullptr;
  std::vector<odb::dbInst*> created_;

  int unmapped_ = 0;
  int skipped_ = 0;
};

}  // namespace ret
