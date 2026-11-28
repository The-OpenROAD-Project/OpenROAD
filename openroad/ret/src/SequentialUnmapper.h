// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <optional>
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
// (inverters, 2-input ANDs and ORs, and tie cells) computing the cell's
// liberty `next_state`.
class SequentialUnmapper
{
 public:
  struct PinTie
  {
    odb::dbMTerm* pin;
    bool high;
  };
  using PinTies = std::vector<PinTie>;

  // resizer: nullptr ignores set_dont_use.
  SequentialUnmapper(odb::dbDatabase* db,
                     sta::dbSta* sta,
                     utl::Logger* logger,
                     rsz::Resizer* resizer = nullptr);

  // False, after a warning, when a cell new instances are made of is missing.
  bool prepare();

  // Returns the number of registers unmapped.  candidates: nullptr is every
  // register.  discards: nullptr deletes the replaced registers at once.
  int run(const std::unordered_set<odb::dbInst*>* candidates = nullptr,
          Discards* discards = nullptr);

  odb::dbMaster* getBaseSequentialMaster() const { return base_.master; }
  odb::dbMTerm* getBaseSequentialDMTerm() const { return base_.data; }
  odb::dbMTerm* getBaseSequentialQMTerm() const { return base_.output; }
  const PinTies& getBaseSequentialTies() const { return base_.ties; }
  odb::dbMaster* getNegedgeSequentialMaster() const
  {
    return negedge_base_.master;
  }
  odb::dbMaster* getTieMaster(const bool one) const
  {
    return tieCell(one).master;
  }
  std::vector<odb::dbMaster*> newCellMasters() const;
  // Returns the net the new tie cell drives.
  odb::dbNet* makeTieNet(sta::Instance* parent,
                         const char* cell_name,
                         const char* net_name,
                         bool one,
                         odb::dbInst* like) const;

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

  struct LoadedOutput
  {
    odb::dbITerm* pin = nullptr;
    std::optional<bool> inverted;  // nullopt: does not read the state
  };

  struct OutputMove
  {
    odb::dbNet* old_net = nullptr;
    odb::dbModNet* old_mod_net = nullptr;
    odb::dbNet* new_net = nullptr;
    std::vector<odb::dbITerm*> loads;
    std::vector<odb::dbBTerm*> ports;
  };

  // nullptr when no new instance may be of the cell.  A tie cell follows only
  // the liberty dont_use: flows keep tie cells out of sizing with
  // set_dont_use and still tie nets with them.
  odb::dbMaster* usableMaster(const sta::LibertyCell* cell,
                              bool tie = false) const;
  // No master when the cell is not a base register.
  BaseRegister baseRegister(const sta::LibertyCell* cell, bool negedge) const;
  BaseRegister findBaseRegister(bool negedge) const;
  BaseRegister plainBaseRegister(bool negedge) const;
  // The least area with its tie cells, then the smallest data pin capacitance.
  BaseRegister tiedBaseRegister(bool negedge) const;
  // False when the controls cannot be tied.  area: with the tie cells.
  bool tieControls(const sta::LibertyCell* cell,
                   BaseRegister& base,
                   float& area) const;
  void reportTiedBase() const;
  odb::dbMaster* smallestGate(sta::FuncExpr::Op op, int inputs) const;
  TieCell findTieCell(bool one) const;
  const TieCell& tieCell(const bool one) const { return one ? tie1_ : tie0_; }

  std::vector<Register> collectRegisters(
      const std::unordered_set<odb::dbInst*>* candidates,
      Untouched& untouched) const;
  void reportRun(const Untouched& untouched);

  void decompose(odb::dbInst* inst, const sta::Sequential* seq);
  void skip(odb::dbInst* inst, const char* reason);
  void selectRegister(odb::dbInst* inst, const sta::Sequential* seq);
  std::vector<LoadedOutput> loadedOutputs(odb::dbInst* inst,
                                          const sta::Sequential* seq) const;
  // False, after a warning, when the new cell has no clock pin.
  bool makeBaseRegister(const BaseRegister& base,
                        odb::dbNet* clock_net,
                        const sta::FuncExpr* next_state);
  std::vector<OutputMove> collectOutputMoves(
      odb::dbInst* inst,
      const std::vector<LoadedOutput>& outputs);
  void applyOutputMoves(const std::vector<OutputMove>& moves);
  bool canSynthesize(const sta::FuncExpr* expr) const;
  odb::dbNet* synthesize(const sta::FuncExpr* expr);

  // Reads `inputs` in pin order; returns the output net.
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

  BaseRegister base_;
  BaseRegister negedge_base_;
  odb::dbMaster* inv_master_ = nullptr;
  odb::dbMaster* and2_master_ = nullptr;
  odb::dbMaster* or2_master_ = nullptr;
  TieCell tie1_;
  TieCell tie0_;

  std::unordered_map<const sta::LibertyPort*, odb::dbNet*> input_nets_;
  std::unordered_map<odb::dbNet*, odb::dbModNet*> mod_nets_;
  sta::Instance* parent_ = nullptr;
  odb::dbInst* register_ = nullptr;  // the one being unmapped
  Discards* discards_ = nullptr;
  const sta::LibertyPort* out_port_ = nullptr;
  const sta::LibertyPort* out_inv_port_ = nullptr;
  odb::dbNet* state_net_ = nullptr;

  int unmapped_ = 0;
  int skipped_ = 0;
  bool skip_reported_ = false;  // RET-0009 once per prepare()
};

}  // namespace ret
