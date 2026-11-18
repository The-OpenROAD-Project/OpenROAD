// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

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

  std::vector<Group> collectGroups(const RetimingGraph& graph,
                                   const std::vector<int>& r) const;
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

  // The original registers standing for the j-th register upstream of a
  // gate under retiming r; nullptr marks a position never filled.
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

  std::vector<int> remapLag(const RetimingGraph& graph,
                            const std::unordered_map<odb::dbInst*, int>& lag,
                            bool& ok) const;
  int lagOfNewGate(odb::dbInst* gate,
                   const std::unordered_map<odb::dbInst*, int>& lag,
                   bool& ok) const;

  void applyAfterUnmapping(const RetimingGraph& graph,
                           const std::vector<int>& r,
                           const std::unordered_set<odb::dbInst*>& movers,
                           int max_moves,
                           est::IncrementalParasiticsGuard& guard);
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
  odb::dbNet* groupClockNet(const RetimingGraph& graph,
                            const FaninIndex& fanin,
                            const std::vector<int>& r,
                            const Group& group) const;
  void rebuildGroup(const Group& group,
                    const std::vector<const RegisterClass*>& classes,
                    odb::dbNet* clock_net,
                    ApplyStats& stats);
  void removeRegister(odb::dbInst* inst);
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
  odb::dbMTerm* dff_q_mterm_ = nullptr;
  odb::dbNet* clock_net_ = nullptr;
};

}  // namespace ret
