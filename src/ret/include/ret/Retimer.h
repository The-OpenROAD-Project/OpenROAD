// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
}
namespace utl {
class Logger;
}

namespace ret {

class RetimingGraph;
class SequentialUnmapper;

// Register retiming on a placed netlist.
//
// retime() runs a forward retiming search on the delay model built from STA
// (see Retime) and applies the result with minimal perturbation: only the
// register groups whose edge weights changed are rebuilt, every other
// register and every combinational cell keeps its placement.  Registers that
// move and are not the base cell are first unmapped to the base cell plus
// logic (SequentialUnmapper), the retiming lags are remapped onto the new
// gates, and the search is re-run only if that remap is ambiguous.
//
// Asynchronous set/reset registers, latches and scan cells never move.
class Retimer
{
 public:
  Retimer(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);
  ~Retimer();

  // unmap_sequential_cells
  bool unmapSequentialCells();

  // retime: max_moves <= 0 is unlimited; stop_at_period stops the search as
  // soon as the model meets the clock period.
  void retime(int max_moves, bool stop_at_period);

 private:
  odb::dbBlock* getBlock();
  odb::dbNet* findClockNet(odb::dbBlock* block);

  // Registers that move under `r` and are not already the base cell.
  std::unordered_set<odb::dbInst*> moversToUnmap(
      const RetimingGraph& graph,
      const std::vector<int>& r) const;

  // Carry lags across an unmap: a gate created by the unmapper takes the lag
  // of the original gate it feeds; ok becomes false when that is ambiguous.
  std::vector<int> remapLag(const RetimingGraph& graph,
                            const std::unordered_map<odb::dbInst*, int>& lag,
                            bool& ok) const;
  int lagOfNewGate(odb::dbInst* gate,
                   const std::unordered_map<odb::dbInst*, int>& lag,
                   bool& ok) const;

  void applyRetiming(RetimingGraph& graph, const std::vector<int>& r);
  void removeRegister(odb::dbInst* inst);
  std::vector<odb::dbNet*> buildChain(odb::dbBlock* block,
                                      odb::dbNet* driver_net,
                                      int depth,
                                      const odb::Point* location);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::unique_ptr<SequentialUnmapper> unmapper_;

  odb::dbMaster* dff_master_ = nullptr;
  odb::dbMTerm* dff_q_mterm_ = nullptr;
  odb::dbNet* clock_net_ = nullptr;
  int uniquify_ = 0;
};

}  // namespace ret
