// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "odb/PtrSetMap.h"
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
  // Registers named by a path delay, a path group, or a constraint on the
  // cell move too; what names a register that moves is dropped.
  bool move_sdc_registers = false;
};

// Forward register retiming: Retime proposes slides on a delay model read
// from STA and StaCheck keeps what STA confirms.
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

  // The edges whose registers start on one net: rebuilt as one chain.
  struct Group
  {
    odb::dbNet* driver_net = nullptr;
    std::vector<Sink> sinks;
    std::vector<const std::vector<odb::dbInst*>*> chains;
    // In id order: the removal order decides how the nets list the rewired
    // pins.
    odb::PtrSet<odb::dbInst> flops;
    bool changed = false;
    bool dont_touch = false;
  };

  // Flat net to its module net; empty in a flat netlist.
  using ModNets = std::unordered_map<odb::dbNet*, odb::dbModNet*>;

  // By (gate, crossing); a nullptr: an input of the gate has none to give.
  using Origins = std::map<std::pair<int, int>, std::vector<odb::dbInst*>>;

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

  struct Chain
  {
    // Per depth of the rebuilt chain; nullptr: the base cell.
    std::vector<const RegisterClass*> classes;
    odb::dbNet* clock_net = nullptr;
    odb::dbInst* origin = nullptr;  // a register the chain replaces
  };

  odb::dbBlock* getBlock() const;
  odb::dbNet* findClockNet(odb::dbBlock* block) const;
  bool canRetime(odb::dbBlock* block) const;
  void warnNegedgeRegisters(odb::dbBlock* block) const;
  void runRounds(odb::dbBlock* block, est::IncrementalParasiticsGuard& guard);
  void reportUnplaced(odb::dbBlock* block, int unplaced_before) const;

  const RetimingGraph& syncGraph();
  // False when none was unmapped.
  bool unmap(const std::unordered_set<odb::dbInst*>& registers,
             est::IncrementalParasiticsGuard& guard);
  std::vector<odb::dbInst*> proposeSlides(odb::dbBlock* block, int max_moves);
  // Returns the number of slides; negative when they were not applied.
  int applySlides(const std::vector<odb::dbInst*>& slides,
                  int max_moves,
                  est::IncrementalParasiticsGuard& guard);
  Retime runSearch(const RetimingGraph& graph, int max_moves) const;
  int applyAfterUnmapping(const std::vector<int>& r,
                          int slides,
                          const std::unordered_set<odb::dbInst*>& movers,
                          int max_moves,
                          est::IncrementalParasiticsGuard& guard);

  Plan planRebuild(const RetimingGraph& graph, const std::vector<int>& r) const;
  std::vector<Group> changedGroups(const RetimingGraph& graph,
                                   const std::vector<int>& r,
                                   const std::vector<int>& slid) const;
  static void addSink(const RetimingGraph& graph,
                      const Sink& sink,
                      Group& group);
  // Applies the retiming, or returns the registers to unmap first.
  std::unordered_set<odb::dbInst*> applyOrMovers(const RetimingGraph& graph,
                                                 const std::vector<int>& r);
  std::unordered_set<odb::dbInst*> moversToUnmap(const RetimingGraph& graph,
                                                 const Plan& plan) const;
  void addGroupMovers(const RetimingGraph& graph,
                      const Group& group,
                      std::unordered_set<odb::dbInst*>& movers) const;

  // The original registers standing for the j-th register upstream of gate v
  // under retiming r; a nullptr marks a position never filled.
  static const std::vector<odb::dbInst*>& originRegisters(
      const RetimingGraph& graph,
      const std::vector<int>& r,
      int v,
      int j,
      Origins& known);
  static std::vector<odb::dbInst*> originRegistersOnEdge(
      const RetimingGraph& graph,
      const std::vector<int>& r,
      int u,
      int k,
      int j,
      Origins& known);
  static std::vector<odb::dbInst*> faninOrigins(const RetimingGraph& graph,
                                                const std::vector<int>& r,
                                                int gate,
                                                int j,
                                                Origins& known);

  void applyRetiming(const RetimingGraph& graph,
                     const std::vector<int>& r,
                     const Plan& plan);
  static std::vector<odb::dbInst*> chainOrigins(const RetimingGraph& graph,
                                                const std::vector<int>& r,
                                                const Sink& sink,
                                                int p,
                                                Origins& known);
  Chain planChain(const RetimingGraph& graph,
                  const std::vector<int>& r,
                  const Group& group,
                  Origins& known) const;
  void checkRebuildable(const RetimingGraph& graph, const Group& group) const;
  // False when cls cannot be one class with those taken at the depth before.
  static bool takeAtDepth(const RegisterClass& cls,
                          int depth,
                          Chain& chain,
                          std::vector<bool>& plain_at);
  void checkCrossings(const RetimingGraph& graph, const Plan& plan) const;
  void rebuildGroup(const Group& group, const Chain& chain);
  void removeRegister(odb::dbInst* inst);
  ModNets groupModNets(const Group& group, const Chain& chain) const;
  void connectRegister(odb::dbInst* ff,
                       const RegisterClass* cls,
                       odb::dbNet* in,
                       odb::dbNet* out,
                       odb::dbNet* clock_net,
                       const ModNets& mod_nets);
  void tieBaseControls(odb::dbInst* ff, sta::Instance* parent);
  std::vector<odb::dbNet*> buildChain(odb::dbNet* driver_net,
                                      const Chain& chain,
                                      const ModNets& mod_nets);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::unique_ptr<SequentialUnmapper> unmapper_;
  std::unique_ptr<RetimerPlacer> placer_;
  std::unique_ptr<Discards> discards_;

  RetimeOptions options_;
  std::unique_ptr<RetimingGraph> graph_;
  // Of the last trial.
  std::vector<int> trial_sources_;
  int gates_before_ = 0;
  bool trial_undone_ = false;
  odb::dbNet* clock_net_ = nullptr;
  bool report_graph_ = false;
};

}  // namespace ret
