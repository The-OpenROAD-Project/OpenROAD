// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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
struct RegisterClass;

// Register retiming on a placed netlist.
//
// retime() runs a forward retiming search on the delay model built from STA
// (see Retime) and applies the result with minimal perturbation: only the
// register groups whose edge weights changed are rebuilt, every other
// register and every combinational cell keeps its placement.
//
// Registers move by class (see RegisterUtils.h).  A plain register is
// rebuilt as the base cell.  An enable register whose move crosses a gate
// together with registers of its own class is rebuilt as the same cell on
// the same enable nets, so the enable logic never becomes data logic.  When
// the registers entering a gate are of different classes, the enable
// registers among them are first unmapped to a plain register plus logic
// (SequentialUnmapper), as is any register of a shape the retimer cannot
// rebuild; the retiming lags are remapped onto the new gates, and the
// search is re-run when that remap is ambiguous.
//
// Asynchronous set/reset registers, latches and scan cells never move.
class Retimer
{
 public:
  Retimer(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);
  ~Retimer();

  // retime: max_moves <= 0 is unlimited.  Errors out on designs it cannot
  // retime yet: missing base cells, several clocks, negative-edge registers.
  void retime(int max_moves);

 private:
  // Fanin edges of every gate as (source gate, index into adj_[source]).
  using FaninIndex = std::vector<std::vector<std::pair<int, int>>>;

  // A sink of a register group: the pin an edge ends at, the edge's new
  // register count under the retiming, and the edge (u, k) itself.
  struct Sink
  {
    odb::dbITerm* pin;
    int wr;
    int u;
    int k;
  };

  // The edges whose register chains start on one driver net.  The group is
  // rebuilt as one shared chain when any of its edges changed weight.
  struct Group
  {
    odb::dbNet* driver_net = nullptr;
    odb::dbITerm* driver = nullptr;  // the pin feeding a sink: a gate output
                                     // pin, or the last register's output
    std::vector<Sink> sinks;
    std::vector<const std::vector<odb::dbInst*>*> chains;  // original edges
    std::unordered_set<odb::dbInst*> flops;
    bool changed = false;
    bool dont_touch = false;
  };

  // Counts for the final report of applyRetiming.
  struct ApplyStats
  {
    int removed = 0;
    int inserted = 0;
    int inserted_classed = 0;
    int reregistered_ports = 0;
    int groups_changed = 0;
    int groups_skipped = 0;
  };

  odb::dbBlock* getBlock();
  odb::dbNet* findClockNet(odb::dbBlock* block);
  void checkRegisters(odb::dbBlock* block);

  // Groups in driver-net id order, so a rebuild names its new cells the same
  // way on every run.
  std::vector<Group> collectGroups(const RetimingGraph& graph,
                                   const std::vector<int>& r) const;

  // Registers that must be unmapped before `r` can be applied: plain
  // registers of another cell than the base cell, registers of a shape the
  // retimer cannot rebuild, and enable registers that a move would merge
  // with registers of a different class.
  std::unordered_set<odb::dbInst*> moversToUnmap(
      const RetimingGraph& graph,
      const std::vector<int>& r) const;

  // The original registers that stand for the j-th register upstream of a
  // gate under retiming r: along one fanin edge (u, k), or along every fanin
  // edge of gate v.  nullptr marks a position the retiming never filled.
  static FaninIndex faninIndex(const RetimingGraph& graph);
  static void originRegistersOnEdge(const RetimingGraph& graph,
                                    const FaninIndex& fanin,
                                    const std::vector<int>& r,
                                    int u,
                                    int k,
                                    int j,
                                    std::vector<odb::dbInst*>& out);
  static void originRegisters(const RetimingGraph& graph,
                              const FaninIndex& fanin,
                              const std::vector<int>& r,
                              int v,
                              int j,
                              std::vector<odb::dbInst*>& out);

  // Carry lags across an unmap: a gate created by the unmapper takes the lag
  // of the original gate it feeds; ok becomes false when that is ambiguous.
  std::vector<int> remapLag(const RetimingGraph& graph,
                            const std::unordered_map<odb::dbInst*, int>& lag,
                            bool& ok) const;
  int lagOfNewGate(odb::dbInst* gate,
                   const std::unordered_map<odb::dbInst*, int>& lag,
                   bool& ok) const;

  void applyAfterUnmapping(const RetimingGraph& graph,
                           const std::vector<int>& r,
                           const std::unordered_set<odb::dbInst*>& movers,
                           int max_moves);
  void applyRetiming(const RetimingGraph& graph, const std::vector<int>& r);
  // Class of every position of the chain that replaces `group`, counted
  // from the driver: nullptr for the base cell.  Errors out when the
  // registers at one position are not one class.
  std::vector<const RegisterClass*> chainClasses(const RetimingGraph& graph,
                                                 const FaninIndex& fanin,
                                                 const std::vector<int>& r,
                                                 const Group& group) const;
  // Errors out when a move merges registers of different classes or takes
  // an inverting register across a gate that is not self-dual.
  void checkMoves(const RetimingGraph& graph,
                  const FaninIndex& fanin,
                  const std::vector<int>& r) const;
  void rebuildGroup(const Group& group,
                    const std::vector<const RegisterClass*>& classes,
                    odb::dbNet* clock_net,
                    ApplyStats& stats);
  void removeRegister(odb::dbInst* inst);
  // classes[k] is the class of the (k+1)-th register of the chain, nullptr
  // for a plain register (the base cell).
  std::vector<odb::dbNet*> buildChain(
      odb::dbNet* driver_net,
      odb::dbNet* clock_net,
      const std::vector<const RegisterClass*>& classes,
      const odb::Point* location);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::unique_ptr<SequentialUnmapper> unmapper_;

  odb::dbMaster* dff_master_ = nullptr;
  odb::dbMTerm* dff_q_mterm_ = nullptr;
  odb::dbNet* clock_net_ = nullptr;  // first register's clock net
  int uniquify_ = 0;
};

}  // namespace ret
