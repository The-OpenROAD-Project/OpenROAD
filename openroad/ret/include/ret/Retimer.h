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
class Instance;
}  // namespace sta
namespace est {
class EstimateParasitics;
class IncrementalParasiticsGuard;
}  // namespace est
namespace rsz {
class Resizer;
}
namespace utl {
class Logger;
}

namespace ret {

class Retime;
class RetimerPlacer;
class RetimingGraph;
class SequentialUnmapper;
struct RegisterClass;

// Forward register retiming: Retime proposes moves on a delay model read
// from STA, and StaCheck keeps what STA confirms.  Only the register groups
// whose edge weights changed are rebuilt; a register that cannot keep its
// class (RegisterUtils.h) is first unmapped (SequentialUnmapper).
class Retimer
{
 public:
  Retimer(odb::dbDatabase* db,
          sta::dbSta* sta,
          est::EstimateParasitics* estimate_parasitics,
          rsz::Resizer* resizer,
          utl::Logger* logger);
  ~Retimer();

  void retime(int max_moves);  // <= 0: unlimited

 private:
  // Fanin edges of every gate as (source gate, index into graph.adj()[source]).
  using FaninIndex = std::vector<std::vector<std::pair<int, int>>>;

  struct Sink
  {
    odb::dbITerm* pin = nullptr;
    int wr = 0;  // registers on the edge under the retiming
    int u = 0;
    int k = 0;
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

  // Flat net to its module net in the group's module; empty when flat.
  using ModNets = std::unordered_map<odb::dbNet*, odb::dbModNet*>;

  // Each register once, in the order first reached; each gate walked once.
  struct Origins
  {
    std::vector<odb::dbInst*> regs;
    std::unordered_set<odb::dbInst*> seen;
    std::unordered_set<int> visited;
    void add(odb::dbInst* reg);
  };

  odb::dbBlock* getBlock() const;
  odb::dbNet* findClockNet(odb::dbBlock* block) const;
  void checkRegisters(odb::dbBlock* block) const;

  // Returns the number of slides; negative when they were not applied.
  int searchAndApply(odb::dbBlock* block,
                     int max_moves,
                     est::IncrementalParasiticsGuard& guard);
  Retime newSearch(const RetimingGraph& graph) const;

  // Registers that start on one net at the same depth hold the same value
  // only when they are one class, so each class of first register is a group
  // of its own.
  std::vector<Group> collectGroups(const RetimingGraph& graph,
                                   const std::vector<int>& r) const;

  std::unordered_set<odb::dbInst*> moversToUnmap(
      const RetimingGraph& graph,
      const std::vector<int>& r) const;
  void addGroupMovers(const RetimingGraph& graph,
                      const Group& group,
                      std::unordered_set<odb::dbInst*>& movers) const;

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
                                    Origins& out);
  static void originRegisters(const RetimingGraph& graph,
                              const FaninIndex& fanin,
                              const std::vector<int>& r,
                              int v,
                              int j,
                              Origins& out);

  // Returns the slides of the search whose retiming it applied; negative
  // when a register could not be unmapped and nothing was applied.
  int applyAfterUnmapping(const RetimingGraph& graph,
                          const Retime& search,
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
                    odb::dbNet* clock_net);
  void removeRegister(odb::dbInst* inst);
  // Looked up before the old registers go: they may hold the module's only
  // pin on one of the nets.
  ModNets groupModNets(const Group& group,
                       const std::vector<const RegisterClass*>& classes,
                       odb::dbNet* clock_net) const;
  void connectRegister(odb::dbInst* ff,
                       const RegisterClass* cls,
                       odb::dbNet* in,
                       odb::dbNet* out,
                       odb::dbNet* clock_net,
                       const ModNets& mod_nets);
  void tieBaseControls(odb::dbInst* ff,
                       sta::Instance* parent,
                       odb::dbModInst* parent_mod_inst);
  // The new registers are left unplaced; the STA check places them.
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
  std::unique_ptr<RetimerPlacer> placer_;

  odb::dbNet* clock_net_ = nullptr;
  bool report_graph_ = false;  // by the first search of a retime call
  int largest_search_ = 0;     // the most slides a search of the trial made
};

}  // namespace ret
