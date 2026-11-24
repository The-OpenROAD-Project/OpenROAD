// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
class FuncExpr;
class Instance;
class LibertyCell;
class LibertyPort;
class Sequential;
}  // namespace sta

namespace utl {
class Logger;
}

namespace ret {

// Rewrites synchronous registers as a plain D flip-flop plus logic
// (inverters, 2-input ANDs and ORs, tie cells) computing the cell's liberty
// `next_state`.  Registers with an asynchronous control that is not tied
// inactive, latches, scan cells and multi-bit cells are left untouched.
// Without a plain D flip-flop the base cell has asynchronous controls, tied
// inactive on each new instance.  New cells go into the register's module.
class SequentialUnmapper
{
 public:
  // Pins held at a constant, as (pin, tied high).
  using PinTies = std::vector<std::pair<odb::dbMTerm*, bool>>;

  SequentialUnmapper(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);

  bool prepare();

  void run(const std::unordered_set<odb::dbInst*>* candidates = nullptr);

  odb::dbMaster* getBaseSequentialMaster() const { return seq_.master; }
  odb::dbMTerm* getBaseSequentialDMTerm() const { return seq_.d; }
  odb::dbMTerm* getBaseSequentialQMTerm() const { return seq_.q; }
  // Empty on a plain flip-flop.
  const PinTies& getBaseSequentialTies() const { return seq_.ties; }
  odb::dbMaster* getNegedgeSequentialMaster() const
  {
    return negedge_seq_.master;
  }
  odb::dbMaster* getTieMaster(bool one) const
  {
    return one ? tie1_master_ : tie0_master_;
  }
  odb::dbMTerm* getTieOutputMTerm(bool one) const
  {
    return one ? tie1_out_mterm_ : tie0_out_mterm_;
  }

 private:
  using Register = std::pair<odb::dbInst*, const sta::Sequential*>;

  struct BaseRegister
  {
    odb::dbMaster* master = nullptr;
    odb::dbMTerm* d = nullptr;
    odb::dbMTerm* q = nullptr;
    PinTies ties;
  };

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

  bool findBaseSequentialCell(bool negedge, BaseRegister& base);
  // Without a plain flip-flop, the base cell is the tied register that costs
  // the least area, then the one with the smallest data pin capacitance.
  bool findTiedBaseSequentialCell(bool negedge, BaseRegister& base);
  // A register cell that is a plain D flip-flop once tie cells hold its
  // asynchronous controls inactive; `area` includes those tie cells.
  bool tiedBaseRegister(sta::LibertyCell* cell,
                        bool negedge,
                        BaseRegister& base,
                        float& area) const;
  BaseRegister makeBaseRegister(const sta::LibertyCell* cell,
                                const sta::LibertyPort* data,
                                const sta::LibertyPort* output) const;
  void reportTiedBase() const;
  bool findInverterCell();
  bool findTwoInputGate(bool is_and, odb::dbMaster*& master);
  bool findTieCell(bool one);

  std::vector<Register> collectRegisters(
      const std::unordered_set<odb::dbInst*>* candidates,
      Untouched& untouched) const;
  void reportRun(const Untouched& untouched) const;

  void decompose(odb::dbInst* inst, const sta::Sequential* seq);
  odb::dbNet* collectRegisterNets(odb::dbInst* inst,
                                  std::vector<odb::dbITerm*>& out_pins);
  std::vector<OutputMove> collectOutputMoves(
      odb::dbInst* inst,
      const sta::Sequential* seq,
      const std::vector<odb::dbITerm*>& out_pins);
  void applyOutputMoves(const std::vector<OutputMove>& moves);
  bool canSynth(const sta::FuncExpr* expr) const;
  odb::dbNet* synth(const sta::FuncExpr* expr);
  RegPins getRegPins(odb::dbInst* inst, const sta::Sequential* seq) const;

  void setParent(odb::dbInst* inst);
  odb::dbNet* newNet(const char* base_name);
  odb::dbInst* newInst(const char* base_name, odb::dbMaster* master);
  odb::dbNet* makeInverter(odb::dbNet* input);
  odb::dbNet* makeTwoInputGate(bool is_and, odb::dbNet* a, odb::dbNet* b);
  odb::dbNet* makeTie(bool one);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  odb::dbBlock* block_ = nullptr;

  BaseRegister seq_;
  BaseRegister negedge_seq_;
  odb::dbMaster* inv_master_ = nullptr;
  odb::dbMaster* and2_master_ = nullptr;
  odb::dbMaster* or2_master_ = nullptr;
  odb::dbMaster* tie1_master_ = nullptr;
  odb::dbMTerm* tie1_out_mterm_ = nullptr;
  float tie1_area_ = 0.0f;
  odb::dbMaster* tie0_master_ = nullptr;
  odb::dbMTerm* tie0_out_mterm_ = nullptr;
  float tie0_area_ = 0.0f;

  std::unordered_map<const sta::LibertyPort*, odb::dbNet*> in_port_net_;
  // Module net of each net the register reads; empty in a flat netlist.
  std::unordered_map<odb::dbNet*, odb::dbModNet*> mod_net_;
  sta::Instance* parent_ = nullptr;
  odb::dbModInst* parent_mod_inst_ = nullptr;
  const sta::LibertyPort* out_port_ = nullptr;
  const sta::LibertyPort* out_inv_port_ = nullptr;
  odb::dbNet* state_net_ = nullptr;

  int unmapped_ = 0;
  int skipped_ = 0;
};

}  // namespace ret
