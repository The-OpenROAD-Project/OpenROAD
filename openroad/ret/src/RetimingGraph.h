// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "RegisterUtils.h"
#include "odb/db.h"

namespace sta {
class dbSta;
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

struct EdgePhys
{
  odb::dbITerm* driver;
  odb::dbITerm* sink;
  std::vector<odb::dbInst*> flops;  // driver to sink
};

// Leiserson-Saxe retiming graph: vertices are combinational gates,
// registers are edge weights.
class RetimingGraph
{
 public:
  RetimingGraph(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);

  void build();
  void report() const;

  const std::vector<odb::dbInst*>& nodes() const { return nodes_; }
  const std::vector<std::vector<Edge>>& adj() const { return adj_; }
  // Parallel to adj().
  const std::vector<std::vector<EdgePhys>>& phys() const { return phys_; }
  // The gates the search never crosses.
  const std::vector<bool>& pinned() const { return pinned_; }
  const std::vector<bool>& selfDual() const { return self_dual_; }
  float clockPeriod() const { return clock_period_; }
  const RegisterClass& regClass(odb::dbInst* reg) const;

  bool isDontTouch(odb::dbInst* reg) const;
  odb::dbITerm* dataPin(odb::dbInst* reg) const;
  // The net an edge's registers start on; the driver net when it has none.
  odb::dbNet* chainNet(const EdgePhys& phys) const;

 private:
  // An enable pin travels with its register's class; it is not a data edge.
  bool continuesThrough(odb::dbInst* reg, odb::dbITerm* pin) const;
  // A net read by a port or a control pin: the registers in front of it
  // keep their depth, and the gate that drives them is never crossed.
  bool fencesChain(odb::dbNet* net) const;
  void walkFanout(int source,
                  odb::dbInst* current,
                  int weight,
                  std::vector<odb::dbInst*>& flops,
                  std::unordered_set<odb::dbInst*>& visited);
  void markBoundary(odb::dbInst* inst, int id);
  void markDontTouch(odb::dbInst* inst, int id);
  // Pins a gate with an input no edge ends on (a port, or a register no gate
  // drives): no register can be taken from that input.
  void pinUnmodeledInputs();
  void fenceDontTouch();
  // The registers that start on one net stay in one module: when they, the
  // gate that drives them and the gates that read them are not all in one
  // module, that gate and the gates behind the registers are never crossed.
  void fenceModules();
  void findClockPeriod();

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;

  std::vector<odb::dbInst*> nodes_;
  std::vector<std::vector<Edge>> adj_;
  std::vector<std::vector<EdgePhys>> phys_;
  std::vector<bool> pinned_;
  std::unordered_set<odb::dbInst*> fenced_;  // registers that never move
  std::unordered_map<odb::dbInst*, RegisterClass> reg_class_;
  std::vector<bool> self_dual_;
  float clock_period_ = 0.0f;
  std::unordered_map<odb::dbInst*, int> index_;
};

}  // namespace ret
