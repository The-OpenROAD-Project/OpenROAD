// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
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

class Discards;
class Retime;
class RetimerPlacer;
class RetimingGraph;
class SequentialUnmapper;
struct RegisterClass;

struct RetimeOptions
{
  int max_moves = 0;      // slides over the whole run; 0: unlimited
  double margin = 0.002;  // the gain a slide must make, x the clock period
};

// Forward register retiming: Retime proposes slides on a delay model read
// from STA and StaCheck keeps what STA confirms.  Only the register groups
// whose counts changed are rebuilt; a register that cannot keep its class
// is first unmapped.
class Retimer
{
 public:
  Retimer(odb::dbDatabase* db,
          sta::dbSta* sta,
          est::EstimateParasitics* estimate_parasitics,
          rsz::Resizer* resizer,
          utl::Logger* logger);
  ~Retimer();

  void retime(const RetimeOptions& options);

 private:
  struct Sink
  {
    odb::dbITerm* pin = nullptr;
    int registers = 0;  // on the edge under the retiming
    int source = 0;
    int edge = 0;  // index in the source's list
  };

  // The edges whose registers start on one net with one class of first
  // register: rebuilt as one shared chain.
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

  struct Origins
  {
    std::vector<odb::dbInst*> registers;
    std::unordered_set<odb::dbInst*> seen;
    std::unordered_set<int> visited;
    void add(odb::dbInst* reg);
  };

  // The original registers that become one register behind a slid gate.
  // Not filled: an input of the gate has none to give.
  struct Crossing
  {
    int gate = 0;
    std::vector<odb::dbInst*> registers;
    bool filled = false;
  };

  struct Plan
  {
    std::vector<Group> groups;
    std::vector<Crossing> crossings;
  };

  // A class per depth of the rebuilt chain; nullptr: the base cell.
  struct Chain
  {
    std::vector<const RegisterClass*> classes;
    odb::dbNet* clock_net = nullptr;
  };

  odb::dbBlock* getBlock() const;
  odb::dbNet* findClockNet(odb::dbBlock* block) const;
  void checkRegisters(odb::dbBlock* block) const;

  // Built once, then updated around what the last trial changed.
  const RetimingGraph& graph();
  // False when none was unmapped.
  bool unmap(const std::unordered_set<odb::dbInst*>& registers,
             est::IncrementalParasiticsGuard& guard);
  // A trial deletes none of the gates it started with, so the pointers
  // survive its undo.
  std::vector<odb::dbInst*> proposeSlides(odb::dbBlock* block, int max_moves);
  // Returns the number of slides; negative when they were not applied.
  int applySlides(const std::vector<odb::dbInst*>& slides,
                  int max_moves,
                  est::IncrementalParasiticsGuard& guard);
  Retime runSearch(const RetimingGraph& graph, int max_moves) const;

  Plan planRebuild(const RetimingGraph& graph, const std::vector<int>& r) const;
  // Registers that start on one net at one depth hold the same value only
  // when they are one class, so each class of first register is a group.
  std::vector<Group> changedGroups(const RetimingGraph& graph,
                                   const std::vector<int>& r,
                                   const std::vector<int>& slid) const;
  // Applies the retiming, or returns the registers to unmap first.
  std::unordered_set<odb::dbInst*> applyOrMovers(const RetimingGraph& graph,
                                                 const std::vector<int>& r);
  std::unordered_set<odb::dbInst*> moversToUnmap(const RetimingGraph& graph,
                                                 const Plan& plan) const;
  void addGroupMovers(const RetimingGraph& graph,
                      const Group& group,
                      std::unordered_set<odb::dbInst*>& movers) const;

  // The original registers standing for the j-th register upstream of a
  // gate under retiming r; nullptr marks a position never filled.  An edge
  // that held registers only loses them from the sink end, so the j-th
  // register upstream of the sink is the j-th from the end of its chain.
  static void originRegistersOnEdge(const RetimingGraph& graph,
                                    const std::vector<int>& r,
                                    int u,
                                    int k,
                                    int j,
                                    Origins& out);
  static void originRegisters(const RetimingGraph& graph,
                              const std::vector<int>& r,
                              int v,
                              int j,
                              Origins& out);

  int applyAfterUnmapping(const std::vector<int>& r,
                          int slides,
                          const std::unordered_set<odb::dbInst*>& movers,
                          int max_moves,
                          est::IncrementalParasiticsGuard& guard);
  // Every check and every lookup of an original register runs before the
  // first edit: rebuilding one group discards registers another came from.
  void applyRetiming(const RetimingGraph& graph,
                     const std::vector<int>& r,
                     const Plan& plan);
  // The original registers standing for the p-th register of a sink's
  // rebuilt chain, counted from the driver net.
  static std::vector<odb::dbInst*> chainOrigins(const RetimingGraph& graph,
                                                const std::vector<int>& r,
                                                const Sink& sink,
                                                int p);
  // The registers a group is rebuilt from share one clock net: the graph
  // pins the gates that registers on different clock nets could reach.
  Chain planChain(const RetimingGraph& graph,
                  const std::vector<int>& r,
                  const Group& group) const;
  void checkCrossings(const RetimingGraph& graph, const Plan& plan) const;
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
  void tieBaseControls(odb::dbInst* ff, sta::Instance* parent);
  // The new registers are left unplaced; the STA check places them.
  std::vector<odb::dbNet*> buildChain(
      odb::dbNet* driver_net,
      odb::dbNet* clock_net,
      const std::vector<const RegisterClass*>& classes,
      const ModNets& mod_nets);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::unique_ptr<SequentialUnmapper> unmapper_;
  std::unique_ptr<RetimerPlacer> placer_;
  std::unique_ptr<Discards> discards_;  // of the retime call in progress

  RetimeOptions options_;
  std::unique_ptr<RetimingGraph> graph_;
  // Of the last trial.
  std::vector<int> trial_sources_;
  int gates_before_ = 0;
  bool trial_undone_ = false;
  odb::dbNet* clock_net_ = nullptr;
  bool report_graph_ = false;  // by the first search of a retime call
};

}  // namespace ret
