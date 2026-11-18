// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
class Instance;
}  // namespace sta
namespace est {
class EstimateParasitics;
class IncrementalParasiticsGuard;
}  // namespace est
namespace utl {
class Logger;
}

namespace ret {

class RetimingGraph;
class SequentialUnmapper;
struct RegisterClass;

// Forward register retiming on the STA delay model (see Retime), applied
// with minimal perturbation: only the register groups whose edge weights
// changed are rebuilt.  Registers move by class (see RegisterUtils.h); a
// register that cannot keep its class is first unmapped to the base cell
// plus logic (SequentialUnmapper).  In a hierarchical netlist a register
// stays inside its module.
class Retimer
{
 public:
  Retimer(odb::dbDatabase* db,
          sta::dbSta* sta,
          est::EstimateParasitics* estimate_parasitics,
          utl::Logger* logger);
  ~Retimer();

  // Errors out on designs it cannot retime.
  void retime(int max_moves);  // <= 0: unlimited

 private:
  // Fanin edges of every gate as (source gate, index into adj_[source]).
  using FaninIndex = std::vector<std::vector<std::pair<int, int>>>;

  struct Sink
  {
    odb::dbITerm* pin;
    int wr;  // registers on the edge under the retiming
    int u;
    int k;
  };

  // Edges whose register chains start on one driver net with one class of
  // first register; rebuilt as one shared chain when any edge changed weight.
  struct Group
  {
    odb::dbNet* driver_net = nullptr;
    std::vector<Sink> sinks;
    std::vector<const std::vector<odb::dbInst*>*> chains;
    std::unordered_set<odb::dbInst*> flops;
    bool changed = false;
    bool dont_touch = false;
  };

  struct ApplyStats
  {
    int removed = 0;
    int inserted = 0;
    int inserted_classed = 0;
    int groups_changed = 0;
  };

  // The module net of a flat net inside the module a group is rebuilt in;
  // empty in a flat netlist.
  using ModNets = std::unordered_map<odb::dbNet*, odb::dbModNet*>;

  odb::dbBlock* getBlock();
  odb::dbNet* findClockNet(odb::dbBlock* block);
  void checkRegisters(odb::dbBlock* block);

  // Registers that start on one net at the same depth hold the same value
  // only when they are one class, so each class of first register is a group
  // of its own.
  std::vector<Group> collectGroups(const RetimingGraph& graph,
                                   const std::vector<int>& r) const;
  // Which of a net's class groups a chain belongs to: the one whose first
  // register is the same class as this chain's.  A chain without a register
  // goes with the first group.
  static int classBranch(const RetimingGraph& graph,
                         const std::vector<odb::dbInst*>& flops,
                         std::vector<odb::dbInst*>& heads);

  std::unordered_set<odb::dbInst*> moversToUnmap(
      const RetimingGraph& graph,
      const std::vector<int>& r) const;
  void addGroupMovers(const RetimingGraph& graph,
                      const Group& group,
                      std::unordered_set<odb::dbInst*>& movers) const;
  static void addClassedMovers(const RetimingGraph& graph,
                               const std::vector<odb::dbInst*>& regs,
                               std::unordered_set<odb::dbInst*>& movers);

  static FaninIndex faninIndex(const RetimingGraph& graph);
  // The original registers standing for the j-th register upstream of a
  // gate under retiming r; nullptr marks a position never filled.  An edge
  // that held registers only loses them from the sink end, so the j-th
  // register upstream of the sink is the j-th from the end of its chain.
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

  std::vector<int> remapLag(const RetimingGraph& graph,
                            const std::unordered_map<std::string, int>& lag,
                            bool& ok) const;
  int lagOfNewGate(odb::dbInst* gate,
                   const std::unordered_map<std::string, int>& lag,
                   bool& ok) const;

  void applyAfterUnmapping(const RetimingGraph& graph,
                           const std::vector<int>& r,
                           const std::unordered_set<odb::dbInst*>& movers,
                           int max_moves,
                           est::IncrementalParasiticsGuard& guard);
  // Every check runs before the first edit, so a retiming that cannot be
  // rebuilt leaves the netlist untouched.  So does every lookup of an
  // original register: rebuilding one group deletes registers another group
  // came from.
  void applyRetiming(const RetimingGraph& graph, const std::vector<int>& r);
  // The original registers standing for the p-th register of a sink's
  // rebuilt chain, counted from the driver net.
  static std::vector<odb::dbInst*> chainOrigins(const RetimingGraph& graph,
                                                const FaninIndex& fanin,
                                                const std::vector<int>& r,
                                                const Sink& sink,
                                                int p);
  // nullptr for the base cell.
  std::vector<const RegisterClass*> chainClasses(const RetimingGraph& graph,
                                                 const FaninIndex& fanin,
                                                 const std::vector<int>& r,
                                                 const Group& group) const;
  void checkMoves(const RetimingGraph& graph,
                  const FaninIndex& fanin,
                  const std::vector<int>& r) const;
  // The registers a group is rebuilt from share one clock net: the graph
  // pins the gates that registers on different clock nets could reach.
  odb::dbNet* groupClockNet(const RetimingGraph& graph,
                            const FaninIndex& fanin,
                            const std::vector<int>& r,
                            const Group& group) const;
  void rebuildGroup(const Group& group,
                    const std::vector<const RegisterClass*>& classes,
                    odb::dbNet* clock_net,
                    ApplyStats& stats);
  void removeRegister(odb::dbInst* inst);
  // The module nets of the nets a group's new registers connect to, looked
  // up before the old registers go: they may hold the module's only pin on
  // one.
  ModNets groupModNets(const Group& group,
                       const std::vector<const RegisterClass*>& classes,
                       odb::dbNet* clock_net) const;
  static void connectPin(odb::dbITerm* pin,
                         odb::dbNet* net,
                         const ModNets& mod_nets);
  void connectRegister(odb::dbInst* ff,
                       const RegisterClass* cls,
                       odb::dbNet* in,
                       odb::dbNet* out,
                       odb::dbNet* clock_net,
                       const ModNets& mod_nets);
  // A base cell with asynchronous controls gets a tie cell on each of them,
  // holding it inactive.
  void tieBaseControls(odb::dbInst* ff,
                       sta::Instance* parent,
                       odb::dbModInst* parent_mod_inst);
  // The new registers are left unplaced for global placement.
  std::vector<odb::dbNet*> buildChain(
      odb::dbNet* driver_net,
      odb::dbNet* clock_net,
      const std::vector<const RegisterClass*>& classes,
      const ModNets& mod_nets);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::unique_ptr<SequentialUnmapper> unmapper_;

  odb::dbMaster* dff_master_ = nullptr;
  odb::dbMTerm* dff_d_mterm_ = nullptr;
  odb::dbMTerm* dff_q_mterm_ = nullptr;
  odb::dbNet* clock_net_ = nullptr;
};

}  // namespace ret
