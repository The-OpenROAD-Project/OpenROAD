// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
class FuncExpr;
class LibertyPort;
class Sequential;
}  // namespace sta

namespace utl {
class Logger;
}

namespace ret {

// Rewrites every synchronous register of the design as a plain D flip-flop
// plus explicit combinational logic built from inverters, 2-input ANDs,
// 2-input ORs and tie cells.  The replacement is derived from the liberty
// `next_state` function of the original cell, so an enable, synchronous
// set/reset, inverted data or inverted output all become ordinary logic in
// front of (or behind) one base register.  Registers with asynchronous
// controls, latches, scan cells and multi-bit cells are left untouched.
//
// This is the normalization step for register retiming: after it, every
// movable register is the same cell with the same (empty) control set.
class SequentialUnmapper
{
 public:
  SequentialUnmapper(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);

  // Locate the library cells used to rebuild registers (base posedge and
  // negedge D flip-flops, inverter, AND2, OR2, tie cells).  Returns false
  // when a required cell is missing; the reason is logged.
  bool prepare();

  // Unmap every register, or only the instances in `candidates` when given.
  void run(const std::unordered_set<odb::dbInst*>* candidates = nullptr);

  odb::dbMaster* getBaseSequentialMaster() const { return seq_master_; }
  odb::dbMTerm* getBaseSequentialQMTerm() const { return seq_q_mterm_; }
  odb::dbMaster* getNegedgeSequentialMaster() const
  {
    return negedge_seq_master_;
  }

 private:
  struct RegPins
  {
    odb::dbITerm* d = nullptr;
    odb::dbITerm* q = nullptr;
    odb::dbITerm* clk = nullptr;
  };

  // Library discovery.  Each picks the smallest-area qualifying cell.
  bool findBaseSequentialCell(bool negedge,
                              odb::dbMaster*& master,
                              odb::dbMTerm*& q_mterm);
  bool findInverterCell();
  bool findTwoInputGate(bool is_and, odb::dbMaster*& master);
  bool findTieCell(bool one);

  // One register.
  void decompose(odb::dbInst* inst, const sta::Sequential* seq);
  bool canSynth(const sta::FuncExpr* expr) const;
  odb::dbNet* synth(const sta::FuncExpr* expr);
  bool isInverted(const sta::FuncExpr* expr) const;
  RegPins getRegPins(odb::dbInst* inst, const sta::Sequential* seq) const;

  // Netlist builders; each returns the output net of the new cell.
  odb::dbNet* newNet(const char* prefix);
  odb::dbInst* newInst(const char* prefix, odb::dbMaster* master);
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

  // Per-register state while decompose() runs.
  std::unordered_map<const sta::LibertyPort*, odb::dbNet*> in_port_net_;
  const sta::LibertyPort* out_port_ = nullptr;
  const sta::LibertyPort* out_inv_port_ = nullptr;
  odb::dbNet* state_net_ = nullptr;
  std::vector<odb::dbInst*> created_;  // cells made for this register

  int unmapped_ = 0;
  int skipped_ = 0;
  int uniquify_ = 0;
};

}  // namespace ret
