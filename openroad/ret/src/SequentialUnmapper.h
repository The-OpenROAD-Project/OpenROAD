// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "odb/db.h"
#include "sta/FuncExpr.hh"

namespace sta {
class dbSta;
class Instance;
class LibertyCell;
class LibertyPort;
class Sequential;
}  // namespace sta

namespace rsz {
class Resizer;
}

namespace utl {
class Logger;
}

namespace ret {

class Discards;

// Rewrites synchronous registers as a plain D flip-flop plus logic
// (inverters, 2-input ANDs and ORs, tie cells) computing the cell's liberty
// `next_state`.  Registers with an asynchronous control that is not tied
// inactive, latches, scan cells, multi-bit cells, and cells that cannot be
// replaced (isReplaceable) are left untouched.
// Without a plain D flip-flop the base cell has asynchronous controls, tied
// inactive on each new instance.  New cells go into the register's module.
class SequentialUnmapper
{
 public:
  // Pins held at a constant, as (pin, tied high).
  struct PinTie
  {
    odb::dbMTerm* pin;
    bool high;
  };
  using PinTies = std::vector<PinTie>;

  // resizer: its set_dont_use cells are not used; nullptr: only the liberty
  // dont_use attribute counts.
  SequentialUnmapper(odb::dbDatabase* db,
                     sta::dbSta* sta,
                     utl::Logger* logger,
                     rsz::Resizer* resizer = nullptr);

  bool prepare();

  // Returns the number of registers unmapped.  discards: where the registers
  // it replaces go; nullptr: they are deleted before it returns.
  int run(const std::unordered_set<odb::dbInst*>* candidates = nullptr,
          Discards* discards = nullptr);

  odb::dbMaster* getBaseSequentialMaster() const { return seq_.master; }
  odb::dbMTerm* getBaseSequentialDMTerm() const { return seq_.data; }
  odb::dbMTerm* getBaseSequentialQMTerm() const { return seq_.output; }
  // Empty on a plain flip-flop.
  const PinTies& getBaseSequentialTies() const { return seq_.ties; }
  odb::dbMaster* getNegedgeSequentialMaster() const
  {
    return negedge_seq_.master;
  }
  odb::dbMaster* getTieMaster(const bool one) const
  {
    return tieCell(one).master;
  }
  odb::dbMTerm* getTieOutputMTerm(const bool one) const
  {
    return tieCell(one).output;
  }
  // The cells a new instance can be, of those prepare() found.
  std::vector<odb::dbMaster*> newCellMasters() const;

 private:
  struct Register
  {
    odb::dbInst* inst;
    const sta::Sequential* seq;
  };

  struct BaseRegister
  {
    odb::dbMaster* master = nullptr;
    odb::dbMTerm* data = nullptr;
    odb::dbMTerm* output = nullptr;
    PinTies ties;
  };

  struct TieCell
  {
    odb::dbMaster* master = nullptr;
    odb::dbMTerm* output = nullptr;
    float area = 0.0f;
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

  // A tie cell follows only the liberty dont_use: flows ban tie cells with
  // set_dont_use to keep them out of sizing and still tie nets with them
  // (ORFS on gt2n, repair_tie_fanout).  A cell whose LEF pins are not its
  // liberty pins (hasLibertyPins) is not usable either.
  bool usableCell(const sta::LibertyCell* cell, bool tie = false) const;
  // The cell as a base register: a D flip-flop of this clock edge with a
  // state output and no scan pin, whatever its asynchronous controls.  No
  // master, as from every finder below, when it is not one.
  BaseRegister baseRegister(const sta::LibertyCell* cell, bool negedge) const;
  BaseRegister findBaseRegister(bool negedge) const;
  BaseRegister plainBaseRegister(bool negedge) const;
  // The register that costs the least area with tie cells holding its
  // asynchronous controls inactive, then the one with the smallest data pin
  // capacitance.
  BaseRegister tiedBaseRegister(bool negedge) const;
  // Adds the ties that hold the cell's asynchronous controls inactive;
  // `area`: the cell with those tie cells.  False when it cannot be tied.
  bool tieControls(const sta::LibertyCell* cell,
                   BaseRegister& base,
                   float& area) const;
  void reportTiedBase() const;
  // The smallest cell that is `op` of this many input pins.
  odb::dbMaster* smallestGate(sta::FuncExpr::Op op, int inputs) const;
  // The smallest cell without inputs whose output always drives the
  // constant and has a pin in LEF.
  TieCell findTieCell(bool one) const;
  const TieCell& tieCell(const bool one) const { return one ? tie1_ : tie0_; }

  std::vector<Register> collectRegisters(
      const std::unordered_set<odb::dbInst*>* candidates,
      Untouched& untouched) const;
  void reportRun(const Untouched& untouched);

  void decompose(odb::dbInst* inst, const sta::Sequential* seq);
  void skip(odb::dbInst* inst, const char* reason);
  // Records the nets of the input pins; returns the loaded output pins.
  std::vector<odb::dbITerm*> collectRegisterNets(odb::dbInst* inst);
  bool outputsReadState(const sta::Sequential* seq,
                        const std::vector<odb::dbITerm*>& out_pins) const;
  std::vector<OutputMove> collectOutputMoves(
      odb::dbInst* inst,
      const sta::Sequential* seq,
      const std::vector<odb::dbITerm*>& out_pins);
  void applyOutputMoves(const std::vector<OutputMove>& moves);
  bool canSynth(const sta::FuncExpr* expr) const;
  odb::dbNet* synth(const sta::FuncExpr* expr);

  void setParent(odb::dbInst* inst);
  // A new gate reading `inputs` in pin order; returns its output net.
  odb::dbNet* makeGate(const char* base_name,
                       odb::dbMaster* master,
                       const std::vector<odb::dbNet*>& inputs);
  odb::dbNet* makeInverter(odb::dbNet* input);
  odb::dbNet* makeAnd(odb::dbNet* a, odb::dbNet* b);
  odb::dbNet* makeOr(odb::dbNet* a, odb::dbNet* b);
  odb::dbNet* makeTie(bool one);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  odb::dbBlock* block_ = nullptr;

  BaseRegister seq_;
  BaseRegister negedge_seq_;
  odb::dbMaster* inv_master_ = nullptr;
  odb::dbMaster* and2_master_ = nullptr;
  odb::dbMaster* or2_master_ = nullptr;
  TieCell tie1_;
  TieCell tie0_;

  std::unordered_map<const sta::LibertyPort*, odb::dbNet*> in_port_net_;
  // Module net of each net the register reads; empty in a flat netlist.
  std::unordered_map<odb::dbNet*, odb::dbModNet*> mod_net_;
  sta::Instance* parent_ = nullptr;
  odb::dbInst* register_ = nullptr;  // the one being unmapped
  Discards* discards_ = nullptr;     // of the run in progress
  const sta::LibertyPort* out_port_ = nullptr;
  const sta::LibertyPort* out_inv_port_ = nullptr;
  odb::dbNet* state_net_ = nullptr;

  int unmapped_ = 0;
  int skipped_ = 0;
  bool skip_reported_ = false;  // RET-0009 once per prepare()
};

}  // namespace ret
