// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <compare>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "RegisterUtils.h"
#include "odb/db.h"

namespace sta {
class dbSta;
class ExceptionPath;
}  // namespace sta
namespace rsz {
class Resizer;
}
namespace utl {
class Logger;
}

namespace ret {

struct Edge
{
  int target;
  int weight;  // registers on the edge
};

// The registers on edge e out of gate u under retiming r.
inline int registersOn(const std::vector<int>& r, const int u, const Edge& e)
{
  return e.weight + r[e.target] - r[u];
}

// An edge seen from one end: the gate at the other end and the edge's index.
struct EdgeRef
{
  int gate;
  int index;
  auto operator<=>(const EdgeRef&) const = default;
};

struct EdgePhys
{
  odb::dbITerm* driver;  // of the last register, else of the source gate
  odb::dbITerm* sink;
  std::vector<odb::dbInst*> flops;     // source to sink
  std::vector<odb::dbITerm*> entries;  // the pin each register is entered by
};

// The net an edge's registers start on; the driver net when it has none.
odb::dbNet* chainNet(const EdgePhys& phys);

// Leiserson-Saxe retiming graph: vertices are combinational gates,
// registers are edge weights.
class RetimingGraph
{
 public:
  RetimingGraph(odb::dbDatabase* db,
                sta::dbSta* sta,
                rsz::Resizer* resizer,
                utl::Logger* logger);

  // new_cells: the cells a retiming can create.
  void build(bool move_named_registers, std::vector<odb::dbMaster*> new_cells);
  // sources: the gates whose fanout registers changed.  Returns the gates
  // whose edges were read again.
  std::vector<int> update(std::vector<int> sources, bool gates_changed);
  // update() with gates_changed must follow.
  void truncate(int gates);
  void report() const;

  const std::vector<odb::dbInst*>& nodes() const { return nodes_; }
  int size() const { return static_cast<int>(nodes_.size()); }
  // -1: not a gate of the graph.
  int indexOf(odb::dbInst* gate) const;
  std::vector<int> sourcesOf(
      const std::unordered_set<odb::dbInst*>& registers) const;
  const std::vector<std::vector<Edge>>& adj() const { return adj_; }
  // Parallel to adj().
  const std::vector<std::vector<EdgePhys>>& phys() const { return phys_; }
  // Per gate: its incoming edges as (source, index into adj()[source]).
  const std::vector<std::vector<EdgeRef>>& fanin() const { return fanin_; }
  // The gates the search never crosses.
  const std::vector<bool>& pinned() const { return pinned_; }
  // The signal inputs of the gate that no edge ends on.
  std::vector<odb::dbITerm*> unmodeledInputs(int v) const;
  float clockPeriod() const { return clock_period_; }
  const RegisterClass& regClass(odb::dbInst* reg) const;
  bool oneClass(const std::vector<odb::dbInst*>& registers) const;
  bool moveKeepsClass(const std::vector<odb::dbInst*>& registers, int v) const;

  bool neverMoves(odb::dbInst* reg) const;

 private:
  void findClockPeriod();
  void findSharedNets(odb::dbBlock* block);
  void findSdcCells(odb::dbBlock* block, bool move_named_registers);
  void addExceptionCells(const sta::ExceptionPath* exception,
                         bool move_named_registers);
  void addGates();
  bool isGate(odb::dbInst* inst) const;
  bool selfDual(odb::dbInst* gate);

  void walkFrom(int source);
  void clearEdges(int source);
  void findReached(odb::dbInst* source,
                   std::unordered_set<odb::dbInst*>& reached) const;
  void walkFanout(int source,
                  odb::dbInst* current,
                  EdgePhys& chain,
                  const std::unordered_set<odb::dbInst*>& reached);
  void walkNet(int source,
               odb::dbITerm* driver,
               EdgePhys& chain,
               const std::unordered_set<odb::dbInst*>& reached);
  void walkThrough(int source,
                   odb::dbITerm* entry,
                   EdgePhys& chain,
                   const std::unordered_set<odb::dbInst*>& reached);
  void addEdge(int source,
               odb::dbITerm* driver,
               odb::dbITerm* sink,
               const EdgePhys& chain);
  void fenceChain(int source, const EdgePhys& chain);
  bool isFencingNet(odb::dbNet* net) const;
  bool continuesThrough(odb::dbInst* reg, odb::dbITerm* pin) const;
  bool reconverges(odb::dbInst* reg,
                   const std::unordered_set<odb::dbInst*>& reached) const;
  void unlinkEdges(int source);
  void linkEdges(int source);

  void pinGates();
  void pinAtPorts(int v);
  void pinDontTouch(int v);
  void pinUnmodeledInputs();

  void fence();
  void fenceDontTouch();
  std::unordered_set<odb::dbNet*> fixedChainNets() const;
  void fenceModules();
  std::unordered_set<odb::dbNet*> moduleCrossingNets() const;

  void addNewGates(std::vector<int>& sources);
  int sourceOfNet(odb::dbNet* net) const;

  // STA's delay calculation fails when a net with two drivers gets a load.
  bool readsSharedNet(odb::dbInst* reg) const;
  // What STA disables for a constant does not come back with an undo.
  bool hasConstantData(odb::dbInst* reg) const;
  bool suppliesFit(odb::dbInst* reg) const;

  // A longer run of registers with no gate in between is left alone.
  static constexpr size_t kMaxChainLength = 256;

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  utl::Logger* logger_ = nullptr;

  std::vector<odb::dbInst*> nodes_;
  std::vector<std::vector<Edge>> adj_;
  std::vector<std::vector<EdgePhys>> phys_;
  std::vector<std::vector<EdgeRef>> fanin_;
  std::vector<bool> pinned_;
  std::vector<bool> fixed_pinned_;           // the result of pinGates
  std::vector<bool> walk_fence_;             // a fencing net behind the gate
  std::unordered_set<odb::dbInst*> fenced_;  // registers on fenced chains
  std::vector<std::vector<odb::dbInst*>> fenced_from_;  // per gate
  std::unordered_set<odb::dbInst*> sdc_cells_;
  std::unordered_set<odb::dbNet*> shared_nets_;  // with a second driver
  std::vector<odb::dbMaster*> new_cells_;
  mutable std::unordered_map<odb::dbMaster*, bool> has_new_supply_pins_;
  mutable std::unordered_map<odb::dbInst*, RegisterClass> reg_class_;
  mutable NetDrivers drivers_;
  std::vector<bool> self_dual_;
  std::unordered_map<odb::dbMaster*, bool> self_dual_master_;
  float clock_period_ = 0.0f;
  std::unordered_map<odb::dbInst*, int> index_;
};

}  // namespace ret
