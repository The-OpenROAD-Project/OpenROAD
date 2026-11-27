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
}
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

// An edge seen from one end: the gate at the other end and the edge's index
// in the list it is kept in.
struct EdgeRef
{
  int gate;
  int index;
  auto operator<=>(const EdgeRef&) const = default;
};

struct EdgePhys
{
  odb::dbITerm* driver;
  odb::dbITerm* sink;
  std::vector<odb::dbInst*> flops;     // driver to sink
  std::vector<odb::dbITerm*> entries;  // the pin each register is entered by
};

// Leiserson-Saxe retiming graph: vertices are combinational gates,
// registers are edge weights.
class RetimingGraph
{
 public:
  RetimingGraph(odb::dbDatabase* db,
                sta::dbSta* sta,
                rsz::Resizer* resizer,
                utl::Logger* logger);

  // move_named_registers: RetimeOptions.  new_cells: the cells a retiming
  // can create.
  void build(bool move_named_registers, std::vector<odb::dbMaster*> new_cells);
  // sources: the gates whose fanout registers changed.  gates_changed: gates
  // were appended or dropped.  Returns the gates whose edges were read again.
  std::vector<int> update(std::vector<int> sources, bool gates_changed);
  // Keeps the first `gates` gates.
  void truncate(int gates);
  void report() const;

  const std::vector<odb::dbInst*>& nodes() const { return nodes_; }
  int size() const { return static_cast<int>(nodes_.size()); }
  // -1: not a gate of the graph.
  int indexOf(odb::dbInst* gate) const;
  // The gates with one of these registers on an edge.
  std::vector<int> sourcesOf(
      const std::unordered_set<odb::dbInst*>& registers) const;
  const std::vector<std::vector<Edge>>& adj() const { return adj_; }
  // Parallel to adj().
  const std::vector<std::vector<EdgePhys>>& phys() const { return phys_; }
  // Per gate: its incoming edges as (source, index into adj()[source]),
  // in that order.
  const std::vector<std::vector<EdgeRef>>& fanin() const { return fanin_; }
  // The gates the search never crosses.
  const std::vector<bool>& pinned() const { return pinned_; }
  // The signal inputs of the gate that no edge ends on (a port, or a
  // register no gate drives).
  std::vector<odb::dbITerm*> unmodeledInputs(int v) const;
  float clockPeriod() const { return clock_period_; }
  const RegisterClass& regClass(odb::dbInst* reg) const;
  bool oneClass(const std::vector<odb::dbInst*>& registers) const;
  // Registers cross a gate together as one class, an inverting class only
  // when the gate is self-dual.
  bool moveKeepsClass(const std::vector<odb::dbInst*>& registers, int v) const;

  bool isDontTouch(odb::dbInst* reg) const;
  // Every supply pin of a new cell finds its net on the register: the pin
  // of its name, or the one net of the pins of its kind.
  bool suppliesFit(odb::dbInst* reg) const;
  // The net an edge's registers start on; the driver net when it has none.
  odb::dbNet* chainNet(const EdgePhys& phys) const;

 private:
  // An enable pin travels with its register's class; it is not a data edge.
  // Nor does a chain run on through a negative-edge register or a scan
  // cell: neither ever moves.
  bool continuesThrough(odb::dbInst* reg, odb::dbITerm* pin) const;
  // A signal input on a net with a second driver: STA's delay calculation
  // of such a net fails when the net gets a new load.
  bool readsSharedNet(odb::dbInst* reg) const;
  // The pin a bypass puts the register's loads on is a constant for STA (a
  // tie cell, case analysis).  The gates behind would read the constant,
  // and what STA then disables does not come back with an undo.
  bool hasConstantData(odb::dbInst* reg) const;
  // A net read by a port, a control pin, an inout pin, or a don't-touch gate,
  // or driven by two cells: the registers in front of it keep their depth,
  // and the gate that drives them is never crossed.
  bool fencesChain(odb::dbNet* net) const;
  // The chain's registers keep their depth and the source is never crossed.
  void fenceChain(int source, const EdgePhys& chain);
  // Adds the registers that chains from `source` run through.
  void findReached(odb::dbInst* source,
                   std::unordered_set<odb::dbInst*>& reached) const;
  // The register is entered by two nets, both from the gate or the registers
  // of `reached`: two chains of one walk meet in it.
  bool reconverges(odb::dbInst* reg,
                   const std::unordered_set<odb::dbInst*>& reached) const;
  // reached: the source gate and the registers its chains run through.
  void walkFanout(int source,
                  odb::dbInst* current,
                  int weight,
                  EdgePhys& chain,
                  const std::unordered_set<odb::dbInst*>& reached);
  void walkFrom(int source);
  void unlinkEdges(int source);
  void linkEdges(int source);
  // Not a register, not a tie cell that only holds control pins of
  // registers (such a cell has no edge, and it goes when its register does),
  // and not a cell with nothing connected, as one a trial discarded.
  bool isGate(odb::dbInst* inst) const;
  void addGates();
  // A gate with an input tied to a supply or left open is not self-dual,
  // whatever its cell.
  bool selfDual(odb::dbInst* gate);
  // The gate behind a net, through the registers in between; -1: none.
  int sourceOfNet(odb::dbNet* net) const;
  // The pins registers do not decide.
  void pinGates();
  // The pins registers decide.
  void fence();
  // The cells the SDC names: in an exception, by a constraint on a pin or
  // on a driven net, by a disabled timing edge, as the reference of a port
  // delay, and the registers with a constraint on the cell.  A move would
  // change which paths the entry covers, or STA would drop it with the
  // cell.  move_named_registers leaves out the registers that only a
  // constraint on the cell, or the -from or -to list of a path delay or a
  // path group, names as cells.
  void findSdcCells(odb::dbBlock* block, bool move_named_registers);
  void markBoundary(odb::dbInst* inst, int id);
  void markDontTouch(odb::dbInst* inst, int id);
  // Pins a gate with an unmodeled input: no register can be taken from it.
  void pinUnmodeledInputs();
  void fenceDontTouch();
  // The registers that start on one net stay in one module: when they, the
  // gate that drives them, and the gates that read them are not all in one
  // module, that gate and the gates behind the registers are never crossed.
  void fenceModules();
  void findClockPeriod();

  // A longer run of registers with no gate in between is left alone: each
  // register is a level of the walk, and each edge holds a copy of its chain.
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
  std::vector<bool> fixed_pinned_;           // by pinGates
  std::vector<bool> walk_fence_;             // a fencing net behind the gate
  std::unordered_set<odb::dbInst*> fenced_;  // registers that never move
  std::vector<std::vector<odb::dbInst*>> fenced_from_;  // per gate
  std::unordered_set<odb::dbInst*> sdc_cells_;
  std::unordered_set<odb::dbNet*> shared_nets_;  // with a second driver
  std::vector<odb::dbMaster*> new_cells_;
  // Per register cell: it has every supply pin of the new cells.
  mutable std::unordered_map<odb::dbMaster*, bool> has_new_supply_pins_;
  // Filled as registers are met.
  mutable std::unordered_map<odb::dbInst*, RegisterClass> reg_class_;
  std::vector<bool> self_dual_;
  std::unordered_map<odb::dbMaster*, bool> self_dual_master_;
  float clock_period_ = 0.0f;
  std::unordered_map<odb::dbInst*, int> index_;
};

}  // namespace ret
