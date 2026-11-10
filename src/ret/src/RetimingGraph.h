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

  std::vector<odb::dbInst*> nodes_;
  std::vector<std::vector<Edge>> adj_;
  std::vector<std::vector<EdgePhys>> phys_;  // parallel to adj_
  std::vector<bool> pinned_;                 // gates the search never crosses
  std::unordered_set<odb::dbInst*> fenced_;  // registers that never move
  std::unordered_map<odb::dbInst*, RegisterClass> reg_class_;
  std::vector<bool> self_dual_;
  float clock_period_ = 0.0f;

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
  std::unordered_map<odb::dbInst*, int> index_;
};

}  // namespace ret
