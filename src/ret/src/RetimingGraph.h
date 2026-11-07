// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

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
  int target;  // sink gate
  int weight;  // number of registers on the edge
};

// odb handles for one edge, used by the register move surgery.
struct EdgePhys
{
  odb::dbITerm* driver;             // output pin feeding the sink
  odb::dbITerm* sink;               // input pin of the target gate
  std::vector<odb::dbInst*> flops;  // registers crossed, driver to sink
};

// Leiserson-Saxe retiming graph G = (V, E, w): vertices are combinational
// gates, registers live on edges as weights.
class RetimingGraph
{
 public:
  RetimingGraph(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);

  void build();
  void report() const;

  std::vector<odb::dbInst*> nodes_;          // vertex id -> gate
  std::vector<std::vector<Edge>> adj_;       // out edges of each vertex
  std::vector<std::vector<EdgePhys>> phys_;  // parallel to adj_
  std::vector<bool> pinned_;  // gate may not move: I/O boundary, don't
                              // touch, or fed through a fenced register
  // Registers on a chain whose output reaches a primary output, a control
  // pin or a cell outside the graph: moving any register of the chain would
  // change what those see.
  std::unordered_set<odb::dbInst*> fenced_;
  // Class of every register in the design (see RegisterUtils.h).
  std::unordered_map<odb::dbInst*, RegisterClass> reg_class_;
  std::vector<bool> self_dual_;  // gate function is self-dual
  float clock_period_ = 0.0f;

  // A register that may not move: fenced, marked don't-touch (the instance
  // or a net on it), or of a kind the retimer never moves (see
  // isDontTouchRegister).
  bool isDontTouch(odb::dbInst* reg) const;

 private:
  bool continuesThrough(odb::dbInst* reg, odb::dbITerm* pin) const;
  bool fencesChain(odb::dbNet* net) const;
  void walkFanout(int source,
                  odb::dbInst* current,
                  int weight,
                  std::vector<odb::dbInst*>& flops,
                  std::unordered_set<odb::dbInst*>& visited);
  void markBoundary(odb::dbInst* inst, int id);
  void markDontTouch(odb::dbInst* inst, int id);
  void fenceDontTouch();
  void findClockPeriod();

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::unordered_map<odb::dbInst*, int> index_;  // build-time only
};

}  // namespace ret
